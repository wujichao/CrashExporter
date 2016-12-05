#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <QModelIndex>
#include <devicemonitor.h>
#include <set>
#include <QFileInfo>

struct CrashItem {
    QString path;
    QString bundle;
    QString date;
};

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
    void onClickExportAllButton();
    void onClickExportSelectButton();
    void onCellClicked(int row, int column);
    void onDeviceEvent(int type, char *udid);
    void onExportFinish(QString result, QString error);

private:
    Ui::Widget *ui;

    void updateIndicatorLabel(QString status);
    void setupTableWidget();
    void updateTableWidgets();
    void clearContents();
    void insertRow(QString title, QString date);

    // monitor
    DeviceMonitor *monitor;

    // export
    std::set<QString> taskSet;
    void startExportTask(QString udid, QStringList keywords);
    void restartExportTask(QString udid, QStringList keywords);

    // list
    QFileInfoList crashFiles;
};

#endif // WIDGET_H
