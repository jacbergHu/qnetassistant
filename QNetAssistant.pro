#-------------------------------------------------
#
# Project created by QtCreator 2020-12-02T17:00:43
#
#-------------------------------------------------

QT       += core gui network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qnetassistant
TEMPLATE = app
CONFIG += c++11
QMAKE_LFLAGS += -no-pie

DESTDIR = $$PWD/../Build/Debug
MOC_DIR = $$PWD/../Build/mocfiles
OBJECTS_DIR = $$PWD/../Build/objfiles
UI_DIR      = $$PWD/../Build/uifiles
RCC_DIR     = $$PWD/../Build/rcfiles

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
    serialwidget.cpp \
    networkwidget.cpp \
    newsubwindow.cpp \
    tcpserver.cpp \
    qnetassistant.cpp

HEADERS += \
    serialwidget.h \
    networkwidget.h \
    newsubwindow.h \
    globalfunction.h \
    tcpserver.h \
    qnetassistant.h

FORMS += \
    serialwidget.ui \
    networkwidget.ui \
    qnetassistant.ui

RESOURCES += \
    resnetassistant.qrc

TRANSLATIONS = zh_hans.ts
