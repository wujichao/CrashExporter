#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>

#include "idevicecrashreport.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    //添加关联代码，必须放在 setupUi 函数之后
    //connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(FoodIsComing()));
    // 新方式
    connect(ui->exportAllButton, &QPushButton::clicked, this, &Widget::FoodIsComing);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::FoodIsComing()
{
    test11();
    QMessageBox::information(this, tr("送餐"), tr("叮咚！外卖已送达"));
}
