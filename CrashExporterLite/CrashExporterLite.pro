#-------------------------------------------------
#
# Project created by QtCreator 2016-12-04T14:50:52
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# 七牛
unix:INCLUDEPATH += "/usr/local/Cellar/openssl/1.0.2j/include"
unix:INCLUDEPATH += "/usr/local/include"
unix:LIBS += "/usr/local/Cellar/openssl/1.0.2j/lib/libssl.dylib"
unix:LIBS += "/usr/local/Cellar/openssl/1.0.2j/lib/libcrypto.dylib"
unix:LIBS += "/usr/lib/libcurl.dylib"

# libimobiledevice
unix:INCLUDEPATH += "/usr/local/include"
unix:LIBS += "/usr/local/Cellar/libplist/1.12/lib/libplist.dylib"
unix:LIBS += "/usr/local/Cellar/libplist/1.12/lib/libplist++.dylib"
unix:LIBS += "/usr/local/Cellar/libimobiledevice/HEAD-9df4e69/lib/libimobiledevice.dylib"
unix:LIBS += "/usr/local/Cellar/openssl/1.0.2j/lib/libssl.dylib"
unix:LIBS += "/usr/local/Cellar/openssl/1.0.2j/lib/libcrypto.dylib"

win32:INCLUDEPATH += "C:\Users\Jiangfan\Desktop\libimobiledevice-msvc-master\libimobiledevice-msvc-master\include"
win32:INCLUDEPATH += "C:\Users\Jiangfan\Desktop\vc\libplist-msvc-master\include"
win32:LIBS += "C:\Users\Jiangfan\Desktop\demo1\lib\imobiledevice.lib"

TARGET = CrashExporterLite
TEMPLATE = app

ICON = ../MyIcon.icns
RC_ICONS = ../MyIcon.ico

SOURCES += main.cpp\
        widget.cpp \
    qiniu/b64/b64.c \
    qiniu/b64/urlsafe_b64.c \
    qiniu/cJSON/cJSON.c \
    qiniu/qiniu/auth_mac.c \
    qiniu/qiniu/base_io.c \
    qiniu/qiniu/base.c \
    qiniu/qiniu/conf.c \
    qiniu/qiniu/fop.c \
    qiniu/qiniu/http.c \
    qiniu/qiniu/io.c \
    qiniu/qiniu/resumable_io.c \
    qiniu/qiniu/rs.c \
    upload.c \
    uploadtask.cpp \
    ../idevicecrashreport.c \
    ../utils.c \
    ../idevice_id.c \
    ../devicemonitor.cpp \
    ../exporttask.cpp

HEADERS  += widget.h \
    qiniu/b64/b64.h \
    qiniu/b64/urlsafe_b64.h \
    qiniu/cJSON/cJSON.h \
    qiniu/qiniu/base.h \
    qiniu/qiniu/conf.h \
    qiniu/qiniu/fop.h \
    qiniu/qiniu/http.h \
    qiniu/qiniu/io.h \
    qiniu/qiniu/resumable_io.h \
    qiniu/qiniu/rs.h \
    upload.h \
    uploadtask.h \
    ../utils.h \
    ../idevicecrashreport.h \
    ../idevice_id.h \
    ../devicemonitor.h \
    ../exporttask.h

FORMS    += widget.ui

DISTFILES += \
    qiniu/qiniu/qiniu.def \
    qiniu/qiniu/qiniu.dsp \
    qiniu/qiniu/qiniu.bat
