#include "exporttask.h"
#include <QFile>
#include <QDir>
#include "idevicecrashreport.h"

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
    copy_crash_reports(path, c_keywords, s);

    // TODO:
    // c_keywords leaks

    emit exportFinish(dir, NULL);

    if (path) {
        free(path);
    }
}
