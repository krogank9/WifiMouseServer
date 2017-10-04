#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include <QThread>
#include <QString>
#include "mainwindow.h"
#include "abstractedsocket.h"

class NetworkThread : public QThread
{
public:
    void run();
    MainWindow *mainWindow;
private:
    QByteArray getPassword();
    bool verifyClient(AbstractedSocket *socket);
    void startInputLoop(AbstractedSocket *socket);
    void updateClientIp(QString ip);
};

#endif // NETWORKTHREAD_H
