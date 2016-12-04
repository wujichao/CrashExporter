#ifndef DEVICEMONITOR_H
#define DEVICEMONITOR_H

#include <QObject>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

class DeviceMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DeviceMonitor(QObject *parent = 0);
    ~DeviceMonitor();
    void start();

    void idevice_event_proxy(const idevice_event_t *event);
signals:
    void idevice_event(int type, char *udid);

public slots:
};

#endif // DEVICEMONITOR_H
