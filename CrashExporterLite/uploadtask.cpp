#include "uploadtask.h"
#include "upload.h"
#include <QFile>
#include "libzippp.h"
#include <QDir>
#include <qDebug>

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
    // create zip
    //
    QString zip_path = path + ".zip";
    libzippp::ZipArchive z1(zip_path.toStdString());
    z1.open(libzippp::ZipArchive::WRITE);

    QDir dir(path);
    QFileInfoList files = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);

    bool zip_result = true;
    for (int i = 0; i < files.length(); i++) {
        QFileInfo file = files[i];
        zip_result = z1.addFile(file.fileName().toStdString(), file.filePath().toStdString());
        qDebug() << "archive " + file.fileName() << ", " << zip_result;
    }

    z1.close();
    qDebug() << path;

    // upload
    //
    QStringList parts = zip_path.split("/");
    QString filename = "crash_" + parts.at(parts.size()-1);

    char *result = NULL;
    char *error = NULL;
    int r = uploadFile(filename.toUtf8().data(), zip_path.toUtf8().data(),
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
