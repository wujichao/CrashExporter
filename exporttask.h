#ifndef EXPORT_TASK_H
#define EXPORT_TASK_H

#include <QThread>

class ExportTask : public QThread
{
    Q_OBJECT

public:
    ExportTask(QString udid, QStringList keywords, QObject *parent = 0);
    ~ExportTask();

signals:
    void exportFinish(QString result, QString error);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    QString udid;
    QStringList keywords;
};

#endif // EXPORT_TASK_H
