#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include "mainwindow.h"

class NetworkThread : public QThread
{
public:
    void run();
    MainWindow *mainWindow;
private:
    QString getPassword();
    bool verifyClient();
    void startInputLoop();
    void updateClientIp(QString ip);
};

#endif // NETWORKTHREAD_H
