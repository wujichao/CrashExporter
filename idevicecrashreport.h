#include <libimobiledevice/afc.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/libimobiledevice.h>

#ifndef IDEVICECRASHREPORT_H
#define IDEVICECRASHREPORT_H

typedef void(*crash_file_name_cb)(void *object, char *name);

#ifdef __cplusplus
extern "C" {
#endif

int test11();
int get_crash_report_list(idevice_t device,
                          lockdownd_client_t lockdownd,
                          void *object,
                          crash_file_name_cb cb);

int get_crash_report_detail(idevice_t device,
                            lockdownd_client_t lockdownd,
                            const char *source_filename,
                            const char *target_filename);

#ifdef __cplusplus
}
#endif

#endif // IDEVICECRASHREPORT_H
