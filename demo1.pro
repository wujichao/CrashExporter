#-------------------------------------------------
#
# Project created by QtCreator 2016-11-27T21:11:02
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = demo1
TEMPLATE = app

unix:INCLUDEPATH += "/usr/local/include"
unix:LIBS += "/usr/local/Cellar/libplist/1.12/lib/libplist.dylib"
unix:LIBS += "/usr/local/Cellar/libplist/1.12/lib/libplist++.dylib"
unix:LIBS += "/usr/local/Cellar/libimobiledevice/HEAD-23069d1/lib/libimobiledevice.dylib"
unix:LIBS += "/usr/local/Cellar/openssl/1.0.2j/lib/libssl.dylib"
unix:LIBS += "/usr/local/Cellar/openssl/1.0.2j/lib/libcrypto.dylib"

win32:INCLUDEPATH += "C:\Users\Jiangfan\Desktop\libimobiledevice-msvc-master\libimobiledevice-msvc-master\include"
win32:INCLUDEPATH += "C:\Users\Jiangfan\Desktop\vc\libplist-msvc-master\include"
win32:LIBS += "C:\Users\Jiangfan\Desktop\demo1\lib\imobiledevice.lib"


SOURCES += main.cpp\
        widget.cpp \
    idevicecrashreport.c \
    utils.c \
    idevice_id.c

HEADERS  += widget.h \
    utils.h \
    idevicecrashreport.h \
    idevice_id.h

FORMS    += widget.ui
