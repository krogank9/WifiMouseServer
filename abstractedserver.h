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
    explicit AbstractedServer(QObject *parent = 0);
    ~AbstractedServer();

    QIODevice *nextPendingConnection();
    void listenWithTimeout(qint16 timeoutMs, bool *shouldQuit = 0);
    void trySetupServers();

    QString pendingSocketInfo;
    bool pendingIsBluetooth;

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
