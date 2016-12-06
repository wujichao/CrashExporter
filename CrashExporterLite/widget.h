#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <QModelIndex>
#include <set>
#include <QFileInfo>
#include "../devicemonitor.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

public slots:
    void onUploadFinish(QString result, QString error);
    void onDeviceEvent(int type, char *udid);
    void onExportFinish(QString result, QString error, QString udid);
    void onExportProgress(QString message);

protected:

private:
    Ui::Widget *ui;

    void updateIndicatorLabel(QString status);

    // monitor
    DeviceMonitor *monitor;
    void scanDevice();

    // export
    std::set<QString> taskSet;
    void startExportTask(QString udid, QStringList keywords);
    void restartExportTask(QString udid, QStringList keywords);

    // list
    QFileInfoList crashFiles;

    // upload

};

#endif // WIDGET_H
