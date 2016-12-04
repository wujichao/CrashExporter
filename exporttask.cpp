#include "exporttask.h"
#include <QFile>

ExportTask::ExportTask(QObject *parent, QString path)
    : QThread(parent),
      path(path)
{

}

ExportTask::~ExportTask()
{

}

void ExportTask::run()
{
    emit uploadFinish(NULL, NULL);
}
