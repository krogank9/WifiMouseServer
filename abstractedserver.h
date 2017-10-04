#ifndef ABSTRACTEDSERVER_H
#define ABSTRACTEDSERVER_H

#include <QTcpServer>
#include <QBluetoothServer>
#include <QBluetoothServiceInfo>
#include <QEventLoop>
#include <QTimer>
#include "abstractedsocket.h"

class AbstractedServer : public QObject
{
    Q_OBJECT

public:
    AbstractedServer();
    ~AbstractedServer();

    AbstractedSocket *nextPendingConnection();
    void listenWithTimeout(qint16 timeoutMs);
    void trySetupServers();

    QString pendingSocketInfo;
    bool pendingIsBluetooth;

private:
    QEventLoop eventLoop;
    QTimer timeoutTimer;

    bool registerBluetoothService();
    bool bluetoothServerListen();
    bool tcpServerListen();
    QBluetoothServer bluetoothServer;
    QBluetoothServiceInfo serviceInfo;
    QTcpServer tcpServer;

    QIODevice *pendingSocket;

public slots:
    void newTcpConnection();
    void newBluetoothConnection();
    void timeout();

signals:
    void newConnection();
};

#endif // ABSTRACTEDSERVER_H
