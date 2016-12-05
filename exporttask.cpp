#include "exporttask.h"
#include <QFile>
#include <QDir>
#include <qDebug>
#include "idevicecrashreport.h"

void ExportTask_export_progress_callback(void *object, char *message)
{
    ExportTask *task = static_cast<ExportTask*>(object);
    task->export_progress_proxy(message);
}

ExportTask::ExportTask(QString udid, QStringList keywords, QObject *parent)
    : QThread(parent),
      udid(udid),
      keywords(keywords)
{

}

ExportTask::~ExportTask()
{

}

void ExportTask::run()
{
    QDir tempDir = QDir::temp();
    bool success = tempDir.mkpath(udid);
    if (!success) {
        emit exportFinish(NULL, "create dir error");
        return;
    }
    QString dir = tempDir.filePath(udid);
    char *path = strdup(dir.toUtf8().data());

    //const char *keywords[] = {"MGJ", "Mogujie"};

    int s = keywords.length();
    const char *c_keywords[s];
    for (int i = 0; i < s; i++) {
        c_keywords[i] = strdup(keywords[i].toUtf8().data());
    }

    // TODO: get error message
    copy_crash_reports(path, c_keywords, s, ExportTask_export_progress_callback, this);

    // TODO:
    // c_keywords leaks

    emit exportFinish(dir, NULL);

    if (path) {
        free(path);
    }
}

void ExportTask::export_progress_proxy(char *message)
{
    QString s;
    s.sprintf("%s", message);

    emit exportProgress(s);
}
