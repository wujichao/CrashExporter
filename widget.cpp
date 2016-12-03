#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QTableWidgetSelectionRange>
#include <QHeaderView>
#include <qDebug>
#include "idevicecrashreport.h"
#include "idevice_id.h"
#include <vector>
#include <QDir>
#include <QFile>

void idevice_event_cb(const idevice_event_t *event, void *user_data)
{
    printf("idevice_event_cb: event: %s, udid: %s\n",
           event->event==IDEVICE_DEVICE_ADD ? "connect" : "disconnect",
           event->udid);

    Widget *widget = static_cast<Widget*>(user_data);

    if (event->event == IDEVICE_DEVICE_ADD) {

        // 这里有坑, wifi的也混进来了
        // 暂时不支持判断是否是通过wifi连接
        // https://github.com/libimobiledevice/libimobiledevice/issues/152
        // 说不定要整个结构重构

        if (widget->client) {
            printf("已有设备连接\n");
            return;
        }

        // for double check
        //
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
            return;//同时连接零个或多个设备
        }

        // connect device
        //
        idevice_t device = NULL;
        if (idevice_new(&device, event->udid) != IDEVICE_E_SUCCESS) {
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

        widget->client = lockdown;
        widget->device = device;

        // update ui
        //
        printf("device name: %s\n", name);
        QString str;
        str.sprintf("event: %d  device_name: %s udid: %s\n", event->event, name, event->udid);
        widget->updateIndicatorLabel(str);

        free(name);

    } else {

        if (!widget->client) {
            printf("没有设备连接\n");
            return;
        }

        // free device
        //
        lockdownd_client_free(widget->client);
        idevice_free(widget->device);
        widget->client = NULL;
        widget->device = NULL;

        // update ui
        //
        widget->updateIndicatorLabel(QObject::tr("disconnect"));
    }
}

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
        memset(bundle+pos_name_end-pos_name_start, 0, 1);
        char *date = (char*)malloc(pos_date_end-pos_date_start+1);
        strncpy(date, name+pos_date_start, pos_date_end-pos_date_start);
        memset(date+pos_date_end-pos_date_start, 0, 1);

        printf("%s %s\n", bundle, date);

        Widget *widget = static_cast<Widget*>(object);
        widget->insertRow(QString(bundle), QString(date));

        CrashItem item;
        item.path = QString(name);
        item.bundle = QString(bundle);
        item.date = QString(date);
        widget->crashItems.push_back(item);

        free(bundle);
        free(date);
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

    InitScoresTable();

    idevice_error_t r = idevice_event_subscribe(idevice_event_cb, this);
    printf("idevice_event_subscribe: %d\n", r);

    ui->detailView->append("s");
}

void Widget::onCellClicked(int row, int column)
{
    printf("%d %d\n", row, column);
    qDebug()<< crashItems[row].path;
    QString path = crashItems[row].path;
    QString filename = path.split('/').last();


    char source_filename[500] = {0};
    strcpy(source_filename, path.toLatin1().data());



    QString finalPath = QDir::temp().filePath(filename);
    char target_filename[500] = {0};
    strcpy(target_filename, finalPath.toLatin1().data());

    int result = get_crash_report_detail(device, client, source_filename, target_filename);
    if (result == 0) {
        QFile f(target_filename);
        if (!f.open(QFile::ReadOnly | QFile::Text)) {
           qDebug() << "open file error" << QString(target_filename);
        }
        QTextStream in(&f);
        ui->detailView->setText(in.readAll());
        qDebug() << f.size() << in.readAll();
    }
}

Widget::~Widget()
{
    idevice_event_unsubscribe();
    delete ui;
}

void Widget::updateIndicatorLabel(QString status)
{
    ui->indicatorLabel->setText(status);
}

void Widget::onClickExportAllButton()
{
//   test11();
    get_crash_report_list(device, client, this, get_crash_list);
//    QMessageBox::information(this, tr("送餐"), tr("叮咚！外卖已送达"));
}

void Widget::onClickExportSelectButton()
{
    QMessageBox::information(this, tr("送餐"), tr("叮咚！外卖已送达"));
}

void Widget::InitScoresTable()
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

    ui->tableWidget->setRowHeight(row, 20);
}
