#include "widget.h"
#include "ui_widget.h"
#include "upload.h"
#include <QTimer>
#include <qDebug>
#include "uploadtask.h"
#include "../exporttask.h"
#include <QMessageBox>
#include <QDir>
#include <QThread>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    setbuf(stdout, NULL); //disable stdout buffer

    monitor = new DeviceMonitor();
       connect(monitor, &DeviceMonitor::idevice_event, this, &Widget::onDeviceEvent);
       monitor->start();
}

Widget::~Widget()
{
    delete monitor;
    delete ui;
}

void Widget::updateIndicatorLabel(QString status)
{
    ui->indicatorLabel->setText(status);
}

void Widget::onDeviceEvent(int type, char *udid)
{
    printf("idevice_event_cb: event: %s, udid: %s\n",
           type==IDEVICE_DEVICE_ADD ? "connect" : "disconnect",
           udid);

    // 拔掉的设备重新连接时重试
    if (type == IDEVICE_DEVICE_REMOVE) {
        QString qudid; qudid.sprintf("%s", udid);
        std::set<QString>::iterator it = taskSet.find(qudid);
        if(it != taskSet.end()) {
            taskSet.erase(it);
        }
    }

    scanDevice();

    if (udid) free(udid);
}

void Widget::scanDevice()
{
    // connect device
    //
    idevice_t device = NULL;
    if (idevice_new(&device, NULL) != IDEVICE_E_SUCCESS) {
        fprintf(stderr, "ERROR: Could not connect to device\n");
        return;
    }

    lockdownd_client_t lockdown = NULL;
    lockdownd_error_t lerr = lockdownd_client_new_with_handshake(device, &lockdown, "idevicename");
    if (lerr != LOCKDOWN_E_SUCCESS) {
        idevice_free(device);
        fprintf(stderr, "ERROR: Could not connect to lockdownd, error code %d\n", lerr);
        return;
    }

    // getting device name
    char* name = NULL;
    lockdownd_error_t nerr = lockdownd_get_device_name(lockdown, &name);
    if (nerr != LOCKDOWN_E_SUCCESS) {
        fprintf(stderr, "ERROR: Could not get device name, lockdown error %d\n", lerr);
        return;
    }

    // update device name
    //
    printf("device name: %s\n", name);
    QString str;
    str.sprintf("已连接: %s\n", name);
    updateIndicatorLabel(str);
#ifndef WIN32
    free(name);
#endif

    // 检查当前有多少个设备连接, 一个设备也可能既通过wifi又通过usb连接
    // 暂不能判断是否是usb或者是wifi
    int count = 0;
    char **dev_list = NULL;

    if (idevice_get_device_list(&dev_list, &count) < 0) {
        fprintf(stderr, "ERROR: Unable to retrieve device list!\n");
        return;
    }

    printf("count %d\n", count);
    QStringList keywords;
    keywords.append("MGJ");
    keywords.append("Mogujie");

    for (int i = 0; i < count; i++) {
        char *id = dev_list[i];
        printf("udid: %s\n", id);

        QString qudid; qudid.sprintf("%s", id);
        startExportTask(qudid, keywords);
    }

    idevice_device_list_free(dev_list);
}

void Widget::startExportTask(QString udid, QStringList keywords)
{
    qDebug() << "startExportTask " << udid;
    updateIndicatorLabel("开始导出: " + udid + "\n");

    bool is_in = taskSet.find(udid) != taskSet.end();
    if(is_in) {
        qDebug() << "is_in: " << udid;
        return;
    }
    taskSet.insert(udid);

    qDebug() << "export udid:" << udid;

    ExportTask *task = new ExportTask(udid, keywords, this);

    connect(task, &ExportTask::exportFinish, this, &Widget::onExportFinish);
    connect(task, &ExportTask::exportProgress, this, &Widget::onExportProgress);
    connect(task, &ExportTask::finished, task, &QObject::deleteLater);
    task->start();
}

void Widget::onExportFinish(QString result, QString error, QString udid)
{
    qDebug() << "exportFinish "<< result << error;
    if (error != NULL) {
        updateIndicatorLabel("导出失败: " + error + "\n");
        QMessageBox::information(this, "", "导出失败, 请将手机关闭Wifi后, 拔掉再重新连接到到电脑上");

        std::set<QString>::iterator it = taskSet.find(udid);
        if(it != taskSet.end()) {
            taskSet.erase(it);
        }
        return;
    }

    updateIndicatorLabel("导出成功: " + result + "\n");

    QDir dir(result);
    QFileInfoList files = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    foreach (QFileInfo file, files){
        if (file.isDir()){
            qDebug() << "DIR: " << file.fileName();
        }else{
            qDebug() << "FILE: " << file.fileName();
        }
    }
    crashFiles = files;

    updateIndicatorLabel("上传...\n");
    UploadTask *task = new UploadTask(this, result);
    connect(task, &UploadTask::uploadFinish, this, &Widget::onUploadFinish);
    connect(task, &UploadTask::finished, task, &QObject::deleteLater);
    task->start();
}

void Widget::onExportProgress(QString message)
{
   updateIndicatorLabel(message);
}

void Widget::onUploadFinish(QString result, QString error)
{
    qDebug() << "onUploadFinish" << result << error;
    if (error != NULL) {
        updateIndicatorLabel("上传失败: " + error + ", 请关掉重试\n");
        return;
    }
    updateIndicatorLabel("上传成功: " + result + "\n");
}
