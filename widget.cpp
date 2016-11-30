#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QTableWidgetSelectionRange>
#include <QHeaderView>
#include "idevicecrashreport.h"
#include "idevice_id.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    connect(ui->exportAllButton, &QPushButton::clicked, this, &Widget::onClickExportAllButton);
    connect(ui->exportSelectButton, &QPushButton::clicked, this, &Widget::onClickExportSelectButton);

    InitScoresTable();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onClickExportAllButton()
{
    test11();
    QMessageBox::information(this, tr("送餐"), tr("叮咚！外卖已送达"));
}

void Widget::onClickExportSelectButton()
{
    char udid[100];
    char device_name[100];
    get_device_id(udid, device_name);

    printf("%s %s", udid, device_name);

    QMessageBox::information(this, tr("送餐"), tr("叮咚！外卖已送达"));
}

void Widget::InitScoresTable()
{
    //初始设置表格为 2 行 4 列的
    ui->tableWidget->setRowCount(2);
    ui->tableWidget->setColumnCount(2);

    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setShowGrid(false);
    ui->tableWidget->resizeRowsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QFont fnt;
    fnt.setPointSize(12);
    ui->tableWidget->setFont(fnt);

    //记录四个列首字符串
    QStringList listHeaders;
    //添加四个字符串到字符串列表
    listHeaders<<tr("App")<<tr("Date");
    ui->tableWidget->setHorizontalHeaderLabels(listHeaders);

    //初始时添加两行学生成绩记录
    //第0行的行首和单元格
    SetTableRow(0, tr("ExcResource_Mogujie4iPhone"), tr("16/11/25 15:52:12"));
    //第1行的行首和单元格
    SetTableRow(1, tr("JDMobile"), tr("16/11/22 12:57:54"));
}

void Widget::SetTableRow(int row, QString title, QString date)
{
    QTableWidgetItem *item1 = new QTableWidgetItem(title);
    ui->tableWidget->setItem(row, 0, item1);

    QTableWidgetItem *item2 = new QTableWidgetItem(date);
    ui->tableWidget->setItem(row, 1, item2);
}
