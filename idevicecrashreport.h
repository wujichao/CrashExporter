#include <libimobiledevice/afc.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/libimobiledevice.h>

#ifndef IDEVICECRASHREPORT_H
#define IDEVICECRASHREPORT_H

#ifdef __cplusplus
extern "C" {
#endif

int test11();
int get_crash_report_list(idevice_t device, lockdownd_client_t lockdownd);

#ifdef __cplusplus
}
#endif

#endif // IDEVICECRASHREPORT_H
