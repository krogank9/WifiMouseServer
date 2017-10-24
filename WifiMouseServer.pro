#-------------------------------------------------
#
# Project created by QtCreator 2017-07-30T12:52:11
#
#-------------------------------------------------

QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

QT += svg
QT += network
QT += bluetooth

win32 {
    SOURCES += fakeinput-windows.cpp
}
macx {
    LIBS += -framework ApplicationServices
    SOURCES += fakeinput-mac.cpp
    ICON = $${PWD}/MacIcon.icns
    #note: on Mac had to edit ~/Qt/ver/mkspecs/macx-clang/Info.plist to add
    #LSUIElement 1 to hide from dock
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
    aes.c \
    fileutils.cpp \
    abstractedsocket.cpp \
    helpipdialog.cpp

HEADERS  += mainwindow.h \
    fixedsvgwidget.h \
    rotatingsquare.h \
    networkthread.h \
    runguard.h \
    setpassworddialog.h \
    fakeinput.h \
    abstractedserver.h \
    encryptutils.h \
    aes.h \
    fileutils.h \
    abstractedsocket.h \
    helpipdialog.h

FORMS    += mainwindow.ui \
    setpassworddialog.ui \
    helpipdialog.ui

RESOURCES += \
    resources.qrc
