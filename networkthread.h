#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QEventLoop>
#include "mainwindow.h"

class NetworkThread : public QThread
{
public:
    void run();
    MainWindow *mainWindow;
private:
    QByteArray getPassword();
    bool verifyClient();
    void startInputLoop();
    void updateClientIp(QString ip);
};

#endif // NETWORKTHREAD_H
