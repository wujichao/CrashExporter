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

const char **_keywords;
int _keywords_len = 0;

void *_userinfo;
export_progress_callback _progress_callback;


void notify_progress (const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    va_end (args);

    printf("%s", buffer);
    _progress_callback(_userinfo, buffer);
}

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
    notify_progress("dir %s\n", device_directory);

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
        notify_progress("%s \n", list[k]);

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
            notify_progress("Failed to read information for '%s'. Skipping...\n", source_filename);
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
                notify_progress("Link: %s\n", (char*)target_filename + strlen(target_directory));

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
            if (strcmp(source_filename, "./Retired") == 0) {
                res = afc_client_copy_and_remove_crash_reports(afc, source_filename, host_directory);

                /* remove directory from device */
                if (!keep_crash_reports)
                    afc_remove_path(afc, source_filename);
            } else {
                notify_progress("skip dir: %s", source_filename);
            }
		} else if (S_ISREG(stbuf.st_mode)) {
            
            if (!strstr(list[k], ".ips")) {
                notify_progress("skip, not .ips\n");
                continue;
            }
            int skip = 1;
            int i = 0;
            for (i = 0; i < _keywords_len; i++) {
                if (strstr(list[k], _keywords[i])) {
                    skip = 0;
                    break;
                }
            }

            // blacklist
            if (_keywords_len == 0) {
                if (strstr(list[k], "backupd") || strstr(list[k], "JetsamEvent") || strstr(list[k], "CoreTime")) {
                    skip = 1;
                } else {
                    skip = 0;
                }
            }

            if (skip) {
                notify_progress("skip, no keyword\n");
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

int copy_crash_reports(char *target_directory, const char **keywords, int keywords_len,
                       export_progress_callback progress_callback, void *object)
{
    _keywords = keywords;
    _keywords_len = keywords_len;
    _userinfo = object;
    _progress_callback = progress_callback;

    int i = 0;
    for (i = 0; i < keywords_len; i++) {
        printf("keywords: %s\n", _keywords[i]);
    }

    idevice_t device = NULL;
    lockdownd_client_t lockdownd = NULL;
    afc_client_t afc = NULL;

    idevice_error_t device_error = IDEVICE_E_SUCCESS;
    lockdownd_error_t lockdownd_error = LOCKDOWN_E_SUCCESS;
    afc_error_t afc_error = AFC_E_SUCCESS;

    const char* udid = NULL;

    extract_raw_crash_reports = 1;
    keep_crash_reports = 1;

    device_error = idevice_new(&device, udid);
    if (device_error != IDEVICE_E_SUCCESS) {
        if (udid) {
            notify_progress("No device found with udid %s, is it plugged in?\n", udid);
        } else {
            notify_progress("No device found, is it plugged in?\n");
        }
        return -1;
    }

    lockdownd_error = lockdownd_client_new_with_handshake(device, &lockdownd, "idevicecrashreport");
    if (lockdownd_error != LOCKDOWN_E_SUCCESS) {
        fprintf(stderr, "ERROR: Could not connect to lockdownd, error code %d\n", lockdownd_error);
        idevice_free(device);
        return -1;
    }

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
        // 一般到这里的是wifi连接 导致导不出来
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

    notify_progress("Done.\n");

    afc_client_free(afc);
    idevice_free(device);

    return 0;
}
