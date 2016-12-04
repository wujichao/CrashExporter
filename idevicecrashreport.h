#include <libimobiledevice/afc.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/libimobiledevice.h>

#ifndef IDEVICECRASHREPORT_H
#define IDEVICECRASHREPORT_H

typedef void(*crash_file_name_cb)(void *object, char *name);

#ifdef __cplusplus
extern "C" {
#endif

int copy_crash_reports(char *target_directory, char **keyworks, int keywords_len);

#ifdef __cplusplus
}
#endif

#endif // IDEVICECRASHREPORT_H
