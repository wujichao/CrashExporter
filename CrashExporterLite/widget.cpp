#include "widget.h"
#include "ui_widget.h"
#include "upload.h"
#include <QTimer>
#include <qDebug>
#include "uploadtask.h"

#include <QThread>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    setbuf(stdout, NULL); //disable stdout buffer

    startTimer(1000);

    UploadTask *task = new UploadTask(this, "/Users/wujichao/Downloads/firefox/qt/《Qt Creator快速入门》代码/版权声明.txt");
    connect(task, &UploadTask::uploadFinish, this, &Widget::onUploadFinish);
    connect(task, &UploadTask::finished, task, &QObject::deleteLater);
    task->start();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::timerEvent(QTimerEvent *event)
{
    // 可以开启多个定时器, 通过id区分
    // event->timerId()
    ui->label->setText(tr("%1").arg(qrand()%10));
}

void Widget::onUploadFinish(QString result, QString error)
{
    qDebug() << "onUploadFinish" << result << error;
}
