#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QTableWidgetSelectionRange>
#include <QHeaderView>
#include <qDebug>
#include <vector>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QTimer>
#include <QScrollBar>
#include "devicemonitor.h"
#include "exporttask.h"

Widget::Widget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::Widget)
{
    setbuf(stdout, NULL); //disable stdout buffer

    ui->setupUi(this);

    settings = new QSettings("com.jichaowu", "CrashExporter", this);
    qDebug() << settings;

    connect(ui->exportAllButton, &QPushButton::clicked, this, &Widget::onClickExportAllButton);
    connect(ui->exportSelectButton, &QPushButton::clicked, this, &Widget::onClickExportSelectButton);
    connect(ui->tableWidget, &QTableWidget::cellClicked, this, &Widget::onCellClicked);

    monitor = new DeviceMonitor();
    connect(monitor, &DeviceMonitor::idevice_event, this, &Widget::onDeviceEvent);
    monitor->start();

    setupTableWidget();

    ui->detailView->append("Crash详情");

    loadKeywords();
    connect(ui->refreshButton, &QPushButton::clicked, this, &Widget::onClickRefreshButton);
//    QTimer::singleShot(300, this, &Widget::showTips);
}

Widget::~Widget()
{
    delete monitor;
    delete ui;
}

void Widget::showTips()
{
    QMessageBox::information(this, "", "请将手机关闭Wifi后, 使用数据线连接到电脑上, 稍等片刻");
}

void Widget::updateIndicatorLabel(QString status)
{
    ui->indicatorLabel->setText(status);
}

void Widget::clearContents()
{
    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);
    ui->detailView->clear();

    QStringList listHeaders;
    listHeaders<<tr("App")<<tr("Date");
    ui->tableWidget->setHorizontalHeaderLabels(listHeaders);
}

void Widget::onClickExportAllButton()
{
    std::vector<QFileInfo> files;

    foreach (QFileInfo file, crashFiles){
        if (file.isDir()) {
            qDebug() << "DIR: " << file.fileName();
            continue;
        }
        files.push_back(file);
    }

    saveCrashFiles(files);
}

void Widget::onClickExportSelectButton()
{
    std::vector<QFileInfo> files;

    QModelIndexList selection = ui->tableWidget->selectionModel()->selectedRows();
    for(int i = 0; i < selection.count(); i++) {
        QModelIndex index = selection.at(i);
        qDebug() << index.row();

        QFileInfo file = crashFiles[index.row()];
        files.push_back(file);
    }

    saveCrashFiles(files);
}

void Widget::saveCrashFiles(std::vector<QFileInfo> &files)
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    QDir::homePath(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir == NULL) {
        return;
    }
    QDir d(dir);

    for ( auto i = files.begin(); i != files.end(); i++ ) {
        QFileInfo file = *i;
        QFile::copy(file.filePath(), d.filePath(file.fileName()));
    }

    QMessageBox::information(this, "", "导出成功! 日志已导出至: " + dir);
}

void Widget::setupTableWidget()
{
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setShowGrid(false);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QFont fnt;
    fnt.setPointSize(12);
    ui->tableWidget->setFont(fnt);

    //记录四个列首字符串
    QStringList listHeaders;
    //添加四个字符串到字符串列表
    listHeaders<<tr("App")<<tr("Date");
    ui->tableWidget->setHorizontalHeaderLabels(listHeaders);

//    insertRow(tr("ExcResource"), tr("16/11/25 15:52:12"));
//    insertRow(tr("JDMobile"), tr("16/11/22 12:57:54"));
}

void Widget::insertRow(QString title, QString date)
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    QTableWidgetItem *item1 = new QTableWidgetItem(title);
    ui->tableWidget->setItem(row, 0, item1);

    QTableWidgetItem *item2 = new QTableWidgetItem(date);
    ui->tableWidget->setItem(row, 1, item2);

    ui->tableWidget->setRowHeight(row, 22);
}

void Widget::onCellClicked(int row, int column)
{
    printf("onCellClicked row: %d, column %d\n", row, column);

    QFileInfo fileInfo = crashFiles[row];
    QFile f(fileInfo.filePath());
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "open file error" << QString(fileInfo.filePath());
    }
    QTextStream in(&f);
    ui->detailView->setText(in.readAll());
}

void Widget::onDeviceEvent(int type, char *udid)
{
    printf("idevice_event_cb: event: %s, udid: %s\n",
           type==IDEVICE_DEVICE_ADD ? "connect" : "disconnect",
           udid);

    QString message;
    message.sprintf("idevice_event_cb: event: %s, udid: %s\n",
           type==IDEVICE_DEVICE_ADD ? "connect" : "disconnect",
           udid);
    console_log(message);

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
    console_log(str);
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
    console_log("设备列表: \n");
    QStringList keywords = loadKeywords();

    for (int i = 0; i < count; i++) {
        char *id = dev_list[i];
        printf("udid: %s\n", id);

        QString log; log.sprintf("udid: %s\n", id);
        console_log(log);

        QString qudid; qudid.sprintf("%s", id);
        startExportTask(qudid, keywords);
    }

    idevice_device_list_free(dev_list);

//    if (count != 1) {
//        fprintf(stderr, "ERROR: device_list_count(%d) != 1!\n", count);
//        QMessageBox::information(this, QObject::tr("错误"), QObject::tr("请关掉手机上的wifi, 并且电脑只连接一台手机"));
//        return;//同时连接零个或多个设备
//    }
}

void Widget::startExportTask(QString udid, QStringList keywords)
{
    qDebug() << "startExportTask " << udid;
    console_log("startExportTask " + udid + "\n");

    bool is_in = taskSet.find(udid) != taskSet.end();
    if(is_in) {
        qDebug() << "is_in: " << udid;
        console_log("is_in: " + udid + "\n");
        return;
    }
    taskSet.insert(udid);

    qDebug() << "export udid:" << udid;
    console_log("export udid:" + udid + "\n");

    ExportTask *task = new ExportTask(udid, keywords, this);

    connect(task, &ExportTask::exportFinish, this, &Widget::onExportFinish);
    connect(task, &ExportTask::exportProgress, this, &Widget::onExportProgress);
    connect(task, &ExportTask::finished, task, &QObject::deleteLater);
    task->start();
}

void Widget::restartExportTask(QString udid, QStringList keywords)
{
    std::set<QString>::iterator it = taskSet.find(udid);
    if(it != taskSet.end()) {
        taskSet.erase(it);
    }
    startExportTask(udid, keywords);
}

void Widget::onExportFinish(QString result, QString error, QString udid)
{
    qDebug() << "exportFinish "<< result << error;
    console_log("exportFinish result: " + result + ", error: " + error + "\n");

    if (error != NULL) {
        std::set<QString>::iterator it = taskSet.find(udid);
        if(it != taskSet.end()) {
            taskSet.erase(it);
        }
        return;
    }

    QDir dir(result);
    QFileInfoList files = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    foreach (QFileInfo file, files){
        if (file.isDir()){
            qDebug() << "DIR: " << file.fileName();
        }else{
            qDebug() << "FILE: " << file.fileName();
        }
        console_log(file.fileName() + "\n");
    }
    console_log("导出成功!\n");
    crashFiles = files;
    clearContents();
    updateTableWidgets();
}

void Widget::onExportProgress(QString message)
{
   console_log(message);
}

void Widget::console_log(QString message)
{
    ui->consoleView->insertPlainText(message);

    QScrollBar *sb = ui->consoleView->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void Widget::updateTableWidgets()
{
    if (crashFiles.length() == 0) {
        QMessageBox::information(this, "", "没有找到关键词相关Crash, 请调整关键词后重试");
        return;
    }

    foreach (QFileInfo file, crashFiles){
        if (file.isDir()) {
            qDebug() << "DIR: " << file.fileName();
            continue;
        }

        QString filename = file.fileName();
        QRegExp rx("^([^-]+).(\\d{4}-\\d{2}-\\d{2})-(\\d{2})(\\d{2})(\\d{2})");
        int pos = rx.indexIn(filename);
        //qDebug() <<pos;
        QStringList list = rx.capturedTexts();
        //qDebug() <<list;

        if (pos != -1) {
            QString bundle = list[1];
            QString date = QString("%1 %2:%3:%4").arg(list[2], list[3], list[4], list[5]);
            //qDebug() << bundle << date;
            insertRow(bundle, date);
        }
    }
}

void Widget::onClickRefreshButton()
{
    taskSet.clear();
    saveKeywords();
    scanDevice();
}

QStringList Widget::loadKeywords()
{
    QString keywords = settings->value("keywords", "MGJ,Mogujie").toString();
    ui->keywordsField->setText(keywords);
    if (keywords.length() == 0) {
        QStringList list;
        return list;
    } else {
        QStringList list = keywords.split(",");
        return list;
    }
}

void Widget::saveKeywords()
{
    QString keywords = ui->keywordsField->text();
    settings->setValue("keywords", keywords);
}
