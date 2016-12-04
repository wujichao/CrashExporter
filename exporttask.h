#ifndef EXPORT_TASK_H
#define EXPORT_TASK_H

#include <QThread>

class ExportTask : public QThread
{
    Q_OBJECT

public:
    ExportTask(QObject *parent = 0, QString path = "");
    ~ExportTask();

signals:
    void uploadFinish(QString result, QString error);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    QString path;
};

#endif // EXPORT_TASK_H
