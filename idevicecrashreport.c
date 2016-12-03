/*
 * idevicecrashreport.c
 * Simple utility to move crash reports from a device to a local directory.
 *
 * Copyright (c) 2014 Martin Szulecki. All Rights Reserved.
 * Copyright (c) 2014 Nikias Bassen. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "idevicecrashreport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include "utils.h"

#include <libimobiledevice/afc.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/libimobiledevice.h>
#include <plist/plist.h>

#ifdef WIN32
#include <windows.h>
#define S_IFLNK S_IFREG
#define S_IFSOCK S_IFREG
#endif

const char* target_directory = NULL;
static int extract_raw_crash_reports = 0;
static int keep_crash_reports = 0;

static int file_exists(const char* path)
{
	struct stat tst;
#ifdef WIN32
	return (stat(path, &tst) == 0);
#else
	return (lstat(path, &tst) == 0);
#endif
}

static int extract_raw_crash_report(const char* filename) {
	int res = 0;
	plist_t report = NULL;
	char* raw = NULL;
	char* raw_filename = strdup(filename);

	/* create filename with '.crash' extension */
	char* p = strrchr(raw_filename, '.');
	if ((p == NULL) || (strcmp(p, ".plist") != 0)) {
		free(raw_filename);
		return res;
	}
	strcpy(p, ".crash");

	/* read plist crash report */
	if (plist_read_from_filename(&report, filename)) {
		plist_t description_node = plist_dict_get_item(report, "description");
		if (description_node && plist_get_node_type(description_node) == PLIST_STRING) {
			plist_get_string_val(description_node, &raw);

			if (raw != NULL) {
				/* write file */
				buffer_write_to_filename(raw_filename, raw, strlen(raw));
				free(raw);
				res = 1;
			}
		}
	}

	if (report)
		plist_free(report);

	if (raw_filename)
		free(raw_filename);

	return res;
}

static int afc_client_copy_and_remove_crash_reports(afc_client_t afc, const char* device_directory, const char* host_directory)
{
    printf("dir %s\n", device_directory);
    
	afc_error_t afc_error;
	int k;
	int res = -1;
	int crash_report_count = 0;
	uint64_t handle;
	char source_filename[512];
	char target_filename[512];

	if (!afc)
		return res;

	char** list = NULL;
	afc_error = afc_read_directory(afc, device_directory, &list);
	if (afc_error != AFC_E_SUCCESS) {
		fprintf(stderr, "ERROR: Could not read device directory '%s'\n", device_directory);
		return res;
	}

	/* ensure we have a trailing slash */
	strcpy(source_filename, device_directory);
	if (source_filename[strlen(source_filename)-1] != '/') {
		strcat(source_filename, "/");
	}
	int device_directory_length = strlen(source_filename);

	/* ensure we have a trailing slash */
	strcpy(target_filename, host_directory);
	if (target_filename[strlen(target_filename)-1] != '/') {
		strcat(target_filename, "/");
	}
	int host_directory_length = strlen(target_filename);

	/* loop over file entries */
	for (k = 0; list[k]; k++) {
        printf("%s \n", list[k]);
        
		if (!strcmp(list[k], ".") || !strcmp(list[k], "..")) {
			continue;
		}

		char **fileinfo = NULL;
		struct stat stbuf;
		stbuf.st_size = 0;

		/* assemble absolute source filename */
		strcpy(((char*)source_filename) + device_directory_length, list[k]);

		/* assemble absolute target filename */
		char* p = strrchr(list[k], '.');
		if (p != NULL && !strncmp(p, ".synced", 7)) {
			/* make sure to strip ".synced" extension as seen on iOS 5 */
			int newlen = strlen(list[k]) - 7;
			strncpy(((char*)target_filename) + host_directory_length, list[k], newlen);
			target_filename[host_directory_length + newlen] = '\0';
		} else {
			strcpy(((char*)target_filename) + host_directory_length, list[k]);
		}

		/* get file information */
		afc_get_file_info(afc, source_filename, &fileinfo);
		if (!fileinfo) {
			printf("Failed to read information for '%s'. Skipping...\n", source_filename);
			continue;
		}

		/* parse file information */
		int i;
		for (i = 0; fileinfo[i]; i+=2) {
			if (!strcmp(fileinfo[i], "st_size")) {
				stbuf.st_size = atoll(fileinfo[i+1]);
			} else if (!strcmp(fileinfo[i], "st_ifmt")) {
				if (!strcmp(fileinfo[i+1], "S_IFREG")) {
					stbuf.st_mode = S_IFREG;
				} else if (!strcmp(fileinfo[i+1], "S_IFDIR")) {
					stbuf.st_mode = S_IFDIR;
				} else if (!strcmp(fileinfo[i+1], "S_IFLNK")) {
					stbuf.st_mode = S_IFLNK;
				} else if (!strcmp(fileinfo[i+1], "S_IFBLK")) {
					stbuf.st_mode = S_IFBLK;
				} else if (!strcmp(fileinfo[i+1], "S_IFCHR")) {
					stbuf.st_mode = S_IFCHR;
				} else if (!strcmp(fileinfo[i+1], "S_IFIFO")) {
					stbuf.st_mode = S_IFIFO;
				} else if (!strcmp(fileinfo[i+1], "S_IFSOCK")) {
					stbuf.st_mode = S_IFSOCK;
				}
			} else if (!strcmp(fileinfo[i], "st_nlink")) {
				stbuf.st_nlink = atoi(fileinfo[i+1]);
			} else if (!strcmp(fileinfo[i], "st_mtime")) {
				stbuf.st_mtime = (time_t)(atoll(fileinfo[i+1]) / 1000000000);
			} else if (!strcmp(fileinfo[i], "LinkTarget")) {
				/* report latest crash report filename */
				printf("Link: %s\n", (char*)target_filename + strlen(target_directory));

				/* remove any previous symlink */
				if (file_exists(target_filename)) {
					remove(target_filename);
				}

#ifndef WIN32
				/* use relative filename */
				char* b = strrchr(fileinfo[i+1], '/');
				if (b == NULL) {
					b = fileinfo[i+1];
				} else {
					b++;
				}

				/* create a symlink pointing to latest log */
				if (symlink(b, target_filename) < 0) {
					fprintf(stderr, "Can't create symlink to %s\n", b);
				}
#endif

				if (!keep_crash_reports)
					afc_remove_path(afc, source_filename);

				res = 0;
			}
		}

		/* free file information */
		afc_dictionary_free(fileinfo);

		/* recurse into child directories */
		if (S_ISDIR(stbuf.st_mode)) {
#ifdef WIN32
			mkdir(target_filename);
#else
			mkdir(target_filename, 0755);
#endif
			res = afc_client_copy_and_remove_crash_reports(afc, source_filename, target_filename);

			/* remove directory from device */
			if (!keep_crash_reports)
				afc_remove_path(afc, source_filename);
		} else if (S_ISREG(stbuf.st_mode)) {
            
            // 只看蘑菇街(PRODUCT_NAME)的crash
            if (!strstr(list[k], "MGJ") && !strstr(list[k], "Mogujie")) {
                printf("skip\n");
                continue;
            }
            
			/* copy file to host */
			afc_error = afc_file_open(afc, source_filename, AFC_FOPEN_RDONLY, &handle);
			if(afc_error != AFC_E_SUCCESS) {
				if (afc_error == AFC_E_OBJECT_NOT_FOUND) {
					continue;
				}
				fprintf(stderr, "Unable to open device file '%s' (%d). Skipping...\n", source_filename, afc_error);
				continue;
			}

			FILE* output = fopen(target_filename, "wb");
			if(output == NULL) {
				fprintf(stderr, "Unable to open local file '%s'. Skipping...\n", target_filename);
				afc_file_close(afc, handle);
				continue;
			}

		//	printf("%s: %s\n", (keep_crash_reports ? "Copy": "Move") , (char*)target_filename + strlen(target_directory));

			uint32_t bytes_read = 0;
			uint32_t bytes_total = 0;
			unsigned char data[0x1000];

			afc_error = afc_file_read(afc, handle, (char*)data, 0x1000, &bytes_read);
			while(afc_error == AFC_E_SUCCESS && bytes_read > 0) {
				fwrite(data, 1, bytes_read, output);
				bytes_total += bytes_read;
				afc_error = afc_file_read(afc, handle, (char*)data, 0x1000, &bytes_read);
			}
			afc_file_close(afc, handle);
			fclose(output);

			if ((uint32_t)stbuf.st_size != bytes_total) {
				fprintf(stderr, "File size mismatch. Skipping...\n");
				continue;
			}

			/* remove file from device */
			if (!keep_crash_reports) {
				afc_remove_path(afc, source_filename);
			}

			/* extract raw crash information into separate '.crash' file */
			if (extract_raw_crash_reports) {
				extract_raw_crash_report(target_filename);
			}

			crash_report_count++;

			res = 0;
		}
	}
	afc_dictionary_free(list);

	/* no reports, no error */
	if (crash_report_count == 0)
		res = 0;

	return res;
}

static void print_usage(int argc, char **argv)
{
	char *name = NULL;

	name = strrchr(argv[0], '/');
	printf("Usage: %s [OPTIONS] DIRECTORY\n", (name ? name + 1: argv[0]));
	printf("Move crash reports from device to a local DIRECTORY.\n\n");
	printf("  -e, --extract\t\textract raw crash report into separate '.crash' file\n");
	printf("  -k, --keep\t\tcopy but do not remove crash reports from device\n");
	printf("  -d, --debug\t\tenable communication debugging\n");
	printf("  -u, --udid UDID\ttarget specific device by its 40-digit device UDID\n");
	printf("  -h, --help\t\tprints usage information\n");
	printf("\n");
//	printf("Homepage: <" PACKAGE_URL ">\n");
}

int _main(int argc, char* argv[]) {
	idevice_t device = NULL;
	lockdownd_client_t lockdownd = NULL;
	afc_client_t afc = NULL;

	idevice_error_t device_error = IDEVICE_E_SUCCESS;
	lockdownd_error_t lockdownd_error = LOCKDOWN_E_SUCCESS;
	afc_error_t afc_error = AFC_E_SUCCESS;

	int i;
	const char* udid = NULL;

	/* parse cmdline args */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
			idevice_set_debug_level(1);
			continue;
		}
		else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--udid")) {
			i++;
			if (!argv[i] || (strlen(argv[i]) != 40)) {
				print_usage(argc, argv);
				return 0;
			}
			udid = argv[i];
			continue;
		}
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(argc, argv);
			return 0;
		}
		else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--extract")) {
			extract_raw_crash_reports = 1;
			continue;
		}
		else if (!strcmp(argv[i], "-k") || !strcmp(argv[i], "--keep")) {
			keep_crash_reports = 1;
			continue;
		}
		else if (target_directory == NULL) {
			target_directory = argv[i];
			continue;
		}
		else {
			print_usage(argc, argv);
			return 0;
		}
	}

	/* ensure a target directory was supplied */
	if (!target_directory) {
		print_usage(argc, argv);
		return 0;
	}

	/* check if target directory exists */
	if (!file_exists(target_directory)) {
		fprintf(stderr, "ERROR: Directory '%s' does not exist.\n", target_directory);
		print_usage(argc, argv);
		return 0;
	}

	device_error = idevice_new(&device, udid);
	if (device_error != IDEVICE_E_SUCCESS) {
		if (udid) {
			printf("No device found with udid %s, is it plugged in?\n", udid);
		} else {
			printf("No device found, is it plugged in?\n");
		}
		return -1;
	}

	lockdownd_error = lockdownd_client_new_with_handshake(device, &lockdownd, "idevicecrashreport");
	if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
		fprintf(stderr, "ERROR: Could not connect to lockdownd, error code %d\n", lockdownd_error);
		idevice_free(device);
		return -1;
	}
	
/// com.apple.crashreportmover

	/* start crash log mover service */
	lockdownd_service_descriptor_t service = NULL;
	lockdownd_error = lockdownd_start_service(lockdownd, "com.apple.crashreportmover", &service);
	if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
		lockdownd_client_free(lockdownd);
		idevice_free(device);
		return -1;
	}

	/* trigger move operation on device */
	idevice_connection_t connection = NULL;
	device_error = idevice_connect(device, service->port, &connection);
	if(device_error != IDEVICE_E_SUCCESS) {
		lockdownd_client_free(lockdownd);
		idevice_free(device);
		return -1;
	}

	/* read "ping" message which indicates the crash logs have been moved to a safe harbor */
	char *ping = malloc(4);
	memset(ping, '\0', 4);
	int attempts = 0;
	while ((strncmp(ping, "ping", 4) != 0) && (attempts < 10)) {
		uint32_t bytes = 0;
		device_error = idevice_connection_receive_timeout(connection, ping, 4, &bytes, 2000);
		if ((bytes == 0) && (device_error == IDEVICE_E_SUCCESS)) {
			attempts++;
			continue;
		} else if (device_error < 0) {
			fprintf(stderr, "ERROR: Crash logs could not be moved. Connection interrupted.\n");
			break;
		}
	}
	idevice_disconnect(connection);
	free(ping);

	if (service) {
		lockdownd_service_descriptor_free(service);
		service = NULL;
	}

	if (device_error != IDEVICE_E_SUCCESS || attempts > 10) {
		fprintf(stderr, "ERROR: Failed to receive ping message from crash report mover.\n");
		lockdownd_client_free(lockdownd);
		idevice_free(device);
		return -1;
	}

	lockdownd_error = lockdownd_start_service(lockdownd, "com.apple.crashreportcopymobile", &service);
	if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
		lockdownd_client_free(lockdownd);
		idevice_free(device);
		return -1;
	}
	lockdownd_client_free(lockdownd);

	afc = NULL;
	afc_error = afc_client_new(device, service, &afc);
	if(afc_error != AFC_E_SUCCESS) {
		lockdownd_client_free(lockdownd);
		idevice_free(device);
		return -1;
	}

	if (service) {
		lockdownd_service_descriptor_free(service);
		service = NULL;
	}

	/* recursively copy crash reports from the device to a local directory */
	if (afc_client_copy_and_remove_crash_reports(afc, ".", target_directory) < 0) {
		fprintf(stderr, "ERROR: Failed to get crash reports from device.\n");
		afc_client_free(afc);
		idevice_free(device);
		return -1;
	}

	printf("Done.\n");

	afc_client_free(afc);
	idevice_free(device);

	return 0;
}


int test11() {
    idevice_t device = NULL;
    lockdownd_client_t lockdownd = NULL;
    afc_client_t afc = NULL;

    idevice_error_t device_error = IDEVICE_E_SUCCESS;
    lockdownd_error_t lockdownd_error = LOCKDOWN_E_SUCCESS;
    afc_error_t afc_error = AFC_E_SUCCESS;

    int i;
    const char* udid = NULL;

    extract_raw_crash_reports = 1;
    keep_crash_reports = 1;
    //target_directory = "C:\\Users\\Jiangfan\\Desktop\\crash";
    target_directory = "/Users/wujichao/Downloads/firefox/crash-get/crash4";

//    /* ensure a target directory was supplied */
//    if (!target_directory) {
//        print_usage(argc, argv);
//        return 0;
//    }

//    /* check if target directory exists */
//    if (!file_exists(target_directory)) {
//        fprintf(stderr, "ERROR: Directory '%s' does not exist.\n", target_directory);
//        print_usage(argc, argv);
//        return 0;
//    }

    device_error = idevice_new(&device, udid);
    if (device_error != IDEVICE_E_SUCCESS) {
        if (udid) {
            printf("No device found with udid %s, is it plugged in?\n", udid);
        } else {
            printf("No device found, is it plugged in?\n");
        }
        return -1;
    }

    lockdownd_error = lockdownd_client_new_with_handshake(device, &lockdownd, "idevicecrashreport");
    if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
        fprintf(stderr, "ERROR: Could not connect to lockdownd, error code %d\n", lockdownd_error);
        idevice_free(device);
        return -1;
    }

/// com.apple.crashreportmover

    /* start crash log mover service */
    lockdownd_service_descriptor_t service = NULL;
    lockdownd_error = lockdownd_start_service(lockdownd, "com.apple.crashreportmover", &service);
    if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
        lockdownd_client_free(lockdownd);
        idevice_free(device);
        return -1;
    }

    /* trigger move operation on device */
    idevice_connection_t connection = NULL;
    device_error = idevice_connect(device, service->port, &connection);
    if(device_error != IDEVICE_E_SUCCESS) {
        lockdownd_client_free(lockdownd);
        idevice_free(device);
        return -1;
    }

    /* read "ping" message which indicates the crash logs have been moved to a safe harbor */
    char *ping = malloc(4);
    memset(ping, '\0', 4);
    int attempts = 0;
    while ((strncmp(ping, "ping", 4) != 0) && (attempts < 10)) {
        uint32_t bytes = 0;
        device_error = idevice_connection_receive_timeout(connection, ping, 4, &bytes, 2000);
        if ((bytes == 0) && (device_error == IDEVICE_E_SUCCESS)) {
            attempts++;
            continue;
        } else if (device_error < 0) {
            fprintf(stderr, "ERROR: Crash logs could not be moved. Connection interrupted.\n");
            break;
        }
    }
    idevice_disconnect(connection);
    free(ping);

    if (service) {
        lockdownd_service_descriptor_free(service);
        service = NULL;
    }

    if (device_error != IDEVICE_E_SUCCESS || attempts > 10) {
        fprintf(stderr, "ERROR: Failed to receive ping message from crash report mover.\n");
        lockdownd_client_free(lockdownd);
        idevice_free(device);
        return -1;
    }

    lockdownd_error = lockdownd_start_service(lockdownd, "com.apple.crashreportcopymobile", &service);
    if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
        lockdownd_client_free(lockdownd);
        idevice_free(device);
        return -1;
    }
    lockdownd_client_free(lockdownd);

    afc = NULL;
    afc_error = afc_client_new(device, service, &afc);
    if(afc_error != AFC_E_SUCCESS) {
        lockdownd_client_free(lockdownd);
        idevice_free(device);
        return -1;
    }

    if (service) {
        lockdownd_service_descriptor_free(service);
        service = NULL;
    }

    /* recursively copy crash reports from the device to a local directory */
    if (afc_client_copy_and_remove_crash_reports(afc, ".", target_directory) < 0) {
        fprintf(stderr, "ERROR: Failed to get crash reports from device.\n");
        afc_client_free(afc);
        idevice_free(device);
        return -1;
    }

    printf("Done.\n");

    afc_client_free(afc);
    idevice_free(device);

    return 0;
}



static int afc_client_get_crash_report_list(afc_client_t afc, const char* device_directory, void *object, crash_file_name_cb cb)
{
    printf("dir: %s\n", device_directory);

    afc_error_t afc_error;
    int k;
    int res = -1;
    int crash_report_count = 0;
    char source_filename[512];

    if (!afc)
        return res;

    char** list = NULL;
    afc_error = afc_read_directory(afc, device_directory, &list);
    if (afc_error != AFC_E_SUCCESS) {
        fprintf(stderr, "ERROR: Could not read device directory '%s'\n", device_directory);
        return res;
    }

    /* ensure we have a trailing slash */
    strcpy(source_filename, device_directory);
    if (source_filename[strlen(source_filename)-1] != '/') {
        strcat(source_filename, "/");
    }
    int device_directory_length = strlen(source_filename);

    /* loop over file entries */
    for (k = 0; list[k]; k++) {
        printf("file: %s\n", list[k]);

        if (!strcmp(list[k], ".") || !strcmp(list[k], "..")) {
            continue;
        }

        char **fileinfo = NULL;
        struct stat stbuf;
        stbuf.st_size = 0;

        /* assemble absolute source filename */
        strcpy(((char*)source_filename) + device_directory_length, list[k]);

        /* get file information */
        afc_get_file_info(afc, source_filename, &fileinfo);
        if (!fileinfo) {
            printf("Failed to read information for '%s'. Skipping...\n", source_filename);
            continue;
        }

        /* parse file information */
        int i;
        for (i = 0; fileinfo[i]; i+=2) {
            if (!strcmp(fileinfo[i], "st_size")) {
                stbuf.st_size = atoll(fileinfo[i+1]);
            } else if (!strcmp(fileinfo[i], "st_ifmt")) {
                if (!strcmp(fileinfo[i+1], "S_IFREG")) {
                    stbuf.st_mode = S_IFREG;
                } else if (!strcmp(fileinfo[i+1], "S_IFDIR")) {
                    stbuf.st_mode = S_IFDIR;
                } else if (!strcmp(fileinfo[i+1], "S_IFLNK")) {
                    stbuf.st_mode = S_IFLNK;
                } else if (!strcmp(fileinfo[i+1], "S_IFBLK")) {
                    stbuf.st_mode = S_IFBLK;
                } else if (!strcmp(fileinfo[i+1], "S_IFCHR")) {
                    stbuf.st_mode = S_IFCHR;
                } else if (!strcmp(fileinfo[i+1], "S_IFIFO")) {
                    stbuf.st_mode = S_IFIFO;
                } else if (!strcmp(fileinfo[i+1], "S_IFSOCK")) {
                    stbuf.st_mode = S_IFSOCK;
                }
            } else if (!strcmp(fileinfo[i], "st_nlink")) {
                stbuf.st_nlink = atoi(fileinfo[i+1]);
            } else if (!strcmp(fileinfo[i], "st_mtime")) {
                stbuf.st_mtime = (time_t)(atoll(fileinfo[i+1]) / 1000000000);
            } else if (!strcmp(fileinfo[i], "LinkTarget")) {
                // link
            }
        }

        /* free file information */
        afc_dictionary_free(fileinfo);

        if (S_ISREG(stbuf.st_mode)) {
            printf("crash: %s\n", source_filename);
            cb(object, source_filename);
            crash_report_count++;
            res = 0;
        }
    }
    afc_dictionary_free(list);

    /* no reports, no error */
    if (crash_report_count == 0)
        res = 0;

    return res;
}


int get_crash_report_list(idevice_t device, lockdownd_client_t lockdownd, void *object, crash_file_name_cb cb)
{
    /* start crash log mover service */
    lockdownd_service_descriptor_t service = NULL;
    lockdownd_error_t lockdownd_error = lockdownd_start_service(lockdownd, "com.apple.crashreportmover", &service);
    if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
        fprintf(stderr, "ERROR: lockdownd_start_service(com.apple.crashreportmover), err: %d.\n", lockdownd_error);
        return -1;
    }

    /* trigger move operation on device */
    idevice_connection_t connection = NULL;
    idevice_error_t device_error = idevice_connect(device, service->port, &connection);
    if(device_error != IDEVICE_E_SUCCESS) {
        fprintf(stderr, "ERROR: idevice_connect(device), err: %d.\n", device_error);
        return -1;
    }

    /* read "ping" message which indicates the crash logs have been moved to a safe harbor */
    char *ping = malloc(4);
    memset(ping, '\0', 4);
    int attempts = 0;
    while ((strncmp(ping, "ping", 4) != 0) && (attempts < 10)) {
        uint32_t bytes = 0;
        device_error = idevice_connection_receive_timeout(connection, ping, 4, &bytes, 2000);
        if ((bytes == 0) && (device_error == IDEVICE_E_SUCCESS)) {
            attempts++;
            continue;
        } else if (device_error < 0) {
            fprintf(stderr, "ERROR: Crash logs could not be moved. Connection interrupted.\n");
            break;
        }
    }
    idevice_disconnect(connection);
    free(ping);

    if (service) {
        lockdownd_service_descriptor_free(service);
        service = NULL;
    }

    if (device_error != IDEVICE_E_SUCCESS || attempts > 10) {
        fprintf(stderr, "ERROR: Failed to receive ping message from crash report mover.\n");
        return -1;
    }

    lockdownd_error = lockdownd_start_service(lockdownd, "com.apple.crashreportcopymobile", &service);
    if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
        fprintf(stderr, "ERROR: lockdownd_start_service(com.apple.crashreportcopymobile), err: %d.\n", lockdownd_error);
        return -1;
    }

    afc_client_t afc = NULL;
    afc_error_t afc_error = afc_client_new(device, service, &afc);
    if(afc_error != AFC_E_SUCCESS) {
        fprintf(stderr, "ERROR: afc_client_new, err: %d.\n", afc_error);
        return -1;
    }

    if (service) {
        lockdownd_service_descriptor_free(service);
        service = NULL;
    }

    int a = afc_client_get_crash_report_list(afc, ".", object, cb);
    if (a != 0) {
        fprintf(stderr, "ERROR: Failed to get crash reports from device.\n");
        afc_client_free(afc);
        return -1;
    }

    int b = afc_client_get_crash_report_list(afc, "./Retired", object, cb);
    if (b != 0) {
        fprintf(stderr, "ERROR: Failed to get crash reports from device.\n");
        afc_client_free(afc);
        return -1;
    }

    printf("Done.\n");

    afc_client_free(afc);
    return 0;
}

int get_crash_report_detail(idevice_t device, lockdownd_client_t lockdownd, const char *source_filename, const char *target_filename)
{
    /* start crash log mover service */
    lockdownd_service_descriptor_t service = NULL;
    lockdownd_error_t lockdownd_error = lockdownd_start_service(lockdownd, "com.apple.crashreportmover", &service);
    if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
        fprintf(stderr, "ERROR: lockdownd_start_service(com.apple.crashreportmover), err: %d.\n", lockdownd_error);
        return -1;
    }

    /* trigger move operation on device */
    idevice_connection_t connection = NULL;
    idevice_error_t device_error = idevice_connect(device, service->port, &connection);
    if(device_error != IDEVICE_E_SUCCESS) {
        fprintf(stderr, "ERROR: idevice_connect(device), err: %d.\n", device_error);
        return -1;
    }

    /* read "ping" message which indicates the crash logs have been moved to a safe harbor */
    char *ping = malloc(4);
    memset(ping, '\0', 4);
    int attempts = 0;
    while ((strncmp(ping, "ping", 4) != 0) && (attempts < 10)) {
        uint32_t bytes = 0;
        device_error = idevice_connection_receive_timeout(connection, ping, 4, &bytes, 2000);
        if ((bytes == 0) && (device_error == IDEVICE_E_SUCCESS)) {
            attempts++;
            continue;
        } else if (device_error < 0) {
            fprintf(stderr, "ERROR: Crash logs could not be moved. Connection interrupted.\n");
            break;
        }
    }
    idevice_disconnect(connection);
    free(ping);

    if (service) {
        lockdownd_service_descriptor_free(service);
        service = NULL;
    }

    if (device_error != IDEVICE_E_SUCCESS || attempts > 10) {
        fprintf(stderr, "ERROR: Failed to receive ping message from crash report mover.\n");
        return -1;
    }

    lockdownd_error = lockdownd_start_service(lockdownd, "com.apple.crashreportcopymobile", &service);
    if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
        fprintf(stderr, "ERROR: lockdownd_start_service(com.apple.crashreportcopymobile), err: %d.\n", lockdownd_error);
        return -1;
    }

    afc_client_t afc = NULL;
    afc_error_t afc_error = afc_client_new(device, service, &afc);
    if(afc_error != AFC_E_SUCCESS) {
        fprintf(stderr, "ERROR: afc_client_new, err: %d.\n", afc_error);
        return -1;
    }

    /* copy file to host */
    uint64_t handle;
    afc_error = afc_file_open(afc, source_filename, AFC_FOPEN_RDONLY, &handle);
    if(afc_error != AFC_E_SUCCESS) {
//        if (afc_error == AFC_E_OBJECT_NOT_FOUND) {
//            continue;
//        }
        fprintf(stderr, "Unable to open device file '%s' (%d). Skipping...\n", source_filename, afc_error);
        return -1;
    }

    FILE* output = fopen(target_filename, "wb");
    if(output == NULL) {
        fprintf(stderr, "Unable to open local file '%s'. Skipping...\n", target_filename);
        afc_file_close(afc, handle);
        return -1;
    }

//	printf("%s: %s\n", (keep_crash_reports ? "Copy": "Move") , (char*)target_filename + strlen(target_directory));

    uint32_t bytes_read = 0;
    uint32_t bytes_total = 0;
    unsigned char data[0x1000];

    afc_error = afc_file_read(afc, handle, (char*)data, 0x1000, &bytes_read);
    while(afc_error == AFC_E_SUCCESS && bytes_read > 0) {
        fwrite(data, 1, bytes_read, output);
        bytes_total += bytes_read;
        afc_error = afc_file_read(afc, handle, (char*)data, 0x1000, &bytes_read);
    }
    afc_file_close(afc, handle);
    fclose(output);

//    if ((uint32_t)stbuf.st_size != bytes_total) {
//        fprintf(stderr, "File size mismatch. Skipping...\n");
//        return -1;
//    }
    return 0;
}
