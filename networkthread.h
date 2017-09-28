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
    NetworkThread(QObject *parent = nullptr);
    ~NetworkThread();
    void run();
    MainWindow *mainWindow;
    bool shouldQuit;
private:
    QByteArray getPassword();
    bool verifyClient();
    void startInputLoop();
    void updateClientIp(QString ip);
};

#endif // NETWORKTHREAD_H
