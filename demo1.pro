#-------------------------------------------------
#
# Project created by QtCreator 2016-11-27T21:11:02
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = demo1
TEMPLATE = app

INCLUDEPATH += "/usr/local/include"
LIBS += "/usr/local/Cellar/libplist/1.12/lib/libplist.dylib"
LIBS += "/usr/local/Cellar/libplist/1.12/lib/libplist++.dylib"
LIBS += "/usr/local/Cellar/libimobiledevice/HEAD-23069d1/lib/libimobiledevice.dylib"
LIBS += "/usr/local/Cellar/openssl/1.0.2j/lib/libssl.dylib"
LIBS += "/usr/local/Cellar/openssl/1.0.2j/lib/libcrypto.dylib"

SOURCES += main.cpp\
        widget.cpp \
    idevicecrashreport.c \
    utils.c

HEADERS  += widget.h \
    utils.h \
    idevicecrashreport.h

FORMS    += widget.ui
