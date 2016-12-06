#ifndef UPLOAD_TASK_H
#define UPLOAD_TASK_H

#include <QThread>

class UploadTask : public QThread
{
    Q_OBJECT

public:
    UploadTask(QObject *parent = 0, QString path = "");
    ~UploadTask();

signals:
    void uploadFinish(QString result, QString error);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    QString path;
};

#endif // UPLOAD_TASK_H
