#ifndef IDEVICE_ID_H
#define IDEVICE_ID_H

#ifdef __cplusplus
extern "C" {
#endif

int uploadFile(const char *uploadName, const char *loaclFilePath, char **result, char **error);

#ifdef __cplusplus
}
#endif

#endif // IDEVICE_ID_H
