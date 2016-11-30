#ifndef IDEVICE_ID_H
#define IDEVICE_ID_H

#ifdef __cplusplus
extern "C" {
#endif

int get_current_device(char *udid, char *name);
int get_device_name(const char *udid, char *device_name);

#ifdef __cplusplus
}
#endif

#endif // IDEVICE_ID_H
