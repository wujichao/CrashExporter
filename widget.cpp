#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QTableWidgetSelectionRange>
#include <QHeaderView>
#include <qDebug>
#include "idevicecrashreport.h"
#include <vector>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <devicemonitor.h>

void get_crash_list(void *object, char *name)
{
    printf("insert row: name %s\n", name);
    if (!strstr(name, "MGJ") && !strstr(name, "Mogujie")) {
        printf("skip\n");
        return;
    }
    if (!strstr(name, ".ips")) {
        printf("skip\n");
        return;
    }

    int pos_name_start = -1;
    int pos_name_end = -1;
    int pos_date_start = -1;
    int pos_date_end = -1;
    for (int i = strlen(name)-1; i >= 0; i--) {
        char c = name[i];
        if (c == '.' && i != 0) {
            pos_date_end = i;
        }
        if (c == '-') {
            pos_name_end = i;
            pos_date_start = i+1;
        }
        if (c == '/' && pos_name_start == -1) {
            pos_name_start = i+1;
        }
    }

    if (pos_name_start && pos_name_end && pos_date_start && pos_date_end) {
        char *bundle = (char*)malloc(pos_name_end-pos_name_start+1);
        strncpy(bundle, name+pos_name_start, pos_name_end-pos_name_start);
        bundle[pos_name_end-pos_name_start] = 0;

        char date[50] = {0};
        int offset = pos_date_start;
        if (pos_date_end-pos_date_start == 17) {
            int i = 0;
            for (i = 0; i < pos_date_end-pos_date_start + 2; i++) {
                if (i == 13 || i == 16) {
                    date[i] = ':';
                } else {
                    date[i] = name[offset];
                    offset += 1;
                }
            }
        } else {
            strncpy(date, name+pos_date_start, pos_date_end-pos_date_start);
        }

        printf("%s %s\n", bundle, date);

        Widget *widget = static_cast<Widget*>(object);
        widget->insertRow(QString(bundle), QString(date));

        CrashItem item;
        item.path = QString(name);
        item.bundle = QString(bundle);
        item.date = QString(date);
        widget->crashItems.push_back(item);

        free(bundle);
    }
}

Widget::Widget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::Widget)
{
    setbuf(stdout, NULL); //disable stdout buffer

    ui->setupUi(this);

    connect(ui->exportAllButton, &QPushButton::clicked, this, &Widget::onClickExportAllButton);
    connect(ui->exportSelectButton, &QPushButton::clicked, this, &Widget::onClickExportSelectButton);
    connect(ui->tableWidget, &QTableWidget::cellClicked, this, &Widget::onCellClicked);

    monitor = new DeviceMonitor();
    connect(monitor, &DeviceMonitor::idevice_event, this, &Widget::onDeviceEvent);
    monitor->start();

    setupTableWidget();

    ui->detailView->append("Crash详情");
}

void Widget::onCellClicked(int row, int column)
{
//    printf("%d %d\n", row, column);
//    qDebug()<< crashItems[row].path;
//    QString path = crashItems[row].path;
//    QString filename = path.split('/').last();

//    char source_filename[500] = {0};
//    strcpy(source_filename, path.toLatin1().data());

//    QString finalPath = QDir::temp().filePath(filename);
//    char target_filename[500] = {0};
//    strcpy(target_filename, finalPath.toLatin1().data());

//    int result = get_crash_report_detail(source_filename, target_filename);
//    if (result == 0) {
//        QFile f(target_filename);
//        if (!f.open(QFile::ReadOnly | QFile::Text)) {
//            qDebug() << "open file error" << QString(target_filename);
//        }
//        QTextStream in(&f);
//        ui->detailView->setText(in.readAll());
//        qDebug() << f.size();
//    }
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

void Widget::clearContents()
{
    ui->tableWidget->clear();
    crashItems.clear();
    ui->detailView->clear();

    QStringList listHeaders;
    listHeaders<<tr("App")<<tr("Date");
    ui->tableWidget->setHorizontalHeaderLabels(listHeaders);
}

void Widget::onClickExportAllButton()
{
    QMessageBox::information(this, tr("送餐"), tr("叮咚！外卖已送达"));
}

void Widget::onClickExportSelectButton()
{
    QMessageBox::information(this, tr("送餐"), tr("叮咚！外卖已送达"));
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

void Widget::onDeviceEvent(int type, char *udid)
{
    printf("idevice_event_cb: event: %s, udid: %s\n",
           type==IDEVICE_DEVICE_ADD ? "connect" : "disconnect",
           udid);

    clearContents();

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

    // update ui
    //
    printf("device name: %s\n", name);
    QString str;
    str.sprintf("device_name: %s udid: %s\n", name, udid);
    updateIndicatorLabel(str);
    free(name);

    // 检查当前有多少个设备连接, 一个设备也可能既通过wifi又通过usb连接
    // 暂不能判断是否是usb或者是wifi
    int count = 0;
    char **dev_list = NULL;

    if (idevice_get_device_list(&dev_list, &count) < 0) {
        fprintf(stderr, "ERROR: Unable to retrieve device list!\n");
        return;
    }
    idevice_device_list_free(dev_list);

    printf("count %d\n", count);
    if (count != 1) {
        fprintf(stderr, "ERROR: device_list_count(%d) != 1!\n", count);
        QMessageBox::information(this, QObject::tr("错误"), QObject::tr("请关掉手机上的wifi, 并且电脑只连接一台手机"));
        return;//同时连接零个或多个设备
    }

    // TODO export task
    const char *keywords[] = {"MGJ", "Mogujie"};
    copy_crash_reports("/Users/wujichao/Downloads/firefox/crash-get/crash5", keywords, 2);

    if (udid) free(udid);
}
