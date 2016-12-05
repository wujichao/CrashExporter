#include <libimobiledevice/afc.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/libimobiledevice.h>

#ifndef IDEVICECRASHREPORT_H
#define IDEVICECRASHREPORT_H

typedef void(*export_progress_callback)(void *object, char *message);

#ifdef __cplusplus
extern "C" {
#endif

int copy_crash_reports(char *target_directory, const char **keywords, int keywords_len,
                       export_progress_callback progress_callback, void *object);

#ifdef __cplusplus
}
#endif

#endif // IDEVICECRASHREPORT_H
