#ifndef EXPORT_TASK_H
#define EXPORT_TASK_H

#include <QThread>

class ExportTask : public QThread
{
    Q_OBJECT

public:
    ExportTask(QString udid, QStringList keywords, QObject *parent = 0);
    ~ExportTask();

    void export_progress_proxy(char *message);

signals:
    void exportFinish(QString result, QString error);
    void exportProgress(QString message);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    QString udid;
    QStringList keywords;
};

#endif // EXPORT_TASK_H
