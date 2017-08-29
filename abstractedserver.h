#ifndef ABSTRACTEDSERVER_H
#define ABSTRACTEDSERVER_H

#include <QTcpServer>
#include <QBluetoothServer>
#include <QBluetoothServiceInfo>
#include <QEventLoop>

class AbstractedServer : public QObject
{
    Q_OBJECT

public:
    AbstractedServer();
    ~AbstractedServer();

    QIODevice *nextPendingConnection();
    void listenWithTimeout(qint16 timeoutMs);
    void trySetupServers();

    QString pendingSocketInfo;
    bool pendingIsBluetooth = false;

private:
    QEventLoop eventLoop;

    bool registerBluetoothService();
    bool bluetoothServerListen();
    bool tcpServerListen();
    QTcpServer tcpServer;
    QBluetoothServer bluetoothServer;
    QBluetoothServiceInfo serviceInfo;

    QIODevice *pendingSocket;

public slots:
    void newTcpConnection();
    void newBluetoothConnection();

signals:
    void newConnection();
};

#endif // ABSTRACTEDSERVER_H