#include "devicemonitor.h"

void idevice_event_callback(const idevice_event_t *event, void *user_data)
{
    printf("monitor: idevice_event_cb: event: %s, udid: %s\n",
           event->event==IDEVICE_DEVICE_ADD ? "connect" : "disconnect",
           event->udid);

    DeviceMonitor *monitor = static_cast<DeviceMonitor*>(user_data);
    monitor->idevice_event_proxy(event);
}

DeviceMonitor::DeviceMonitor(QObject *parent) : QObject(parent)
{

}

DeviceMonitor::~DeviceMonitor()
{
    idevice_event_unsubscribe();
}

void DeviceMonitor::start()
{
    idevice_error_t r = idevice_event_subscribe(idevice_event_callback, this);
    printf("monitor: idevice_event_subscribe: %d\n", r);
}

void DeviceMonitor::idevice_event_proxy(const idevice_event_t *event)
{
    emit idevice_event(event->event, strdup(event->udid));
}
