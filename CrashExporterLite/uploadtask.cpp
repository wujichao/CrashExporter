#include "uploadtask.h"
#include "upload.h"
#include <QFile>

UploadTask::UploadTask(QObject *parent, QString path)
    : QThread(parent),
      path(path)
{

}

UploadTask::~UploadTask()
{

}

void UploadTask::run()
{
    QStringList parts = path.split("/");
    QString filename = parts.at(parts.size()-1);

    char *result = NULL;
    char *error = NULL;
    int r = uploadFile(filename.toUtf8().data(), path.toUtf8().data(),
                       &result, &error);

    if (r == 0) {
        QString r; r.sprintf("%s",result);
        emit uploadFinish(r, NULL);
    } else {
        QString e; e.sprintf("%s",error);
        emit uploadFinish(NULL, e);
    }

    if (result) {
        free(result);
    }
    if (error) {
        free(error);
    }
}
