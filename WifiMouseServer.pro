#-------------------------------------------------
#
# Project created by QtCreator 2017-07-30T12:52:11
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT += svg
QT += network
QT += bluetooth

win32 {
    SOURCES += fakeinput-windows.cpp
}
macx {
    LIBS += -framework ApplicationServices
    SOURCES += fakeinput-mac.cpp
}
linux {
    LIBS += -lxdo
    SOURCES += fakeinput-linux.cpp
}

TARGET = WifiMouseServer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    fixedsvgwidget.cpp \
    rotatingsquare.cpp \
    networkthread.cpp \
    runguard.cpp \
    setpassworddialog.cpp \
    abstractedserver.cpp \
    encryptutils.cpp \
    aes.c

HEADERS  += mainwindow.h \
    fixedsvgwidget.h \
    rotatingsquare.h \
    networkthread.h \
    runguard.h \
    setpassworddialog.h \
    fakeinput.h \
    abstractedserver.h \
    encryptutils.h \
    aes.h

FORMS    += mainwindow.ui \
    setpassworddialog.ui

RESOURCES += \
    resources.qrc
