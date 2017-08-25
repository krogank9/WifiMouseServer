#include "abstractedserver.h"
#include <QEventLoop>
#include <QTime>
#include <QBluetoothSocket>
#include <QTcpSocket>

AbstractedServer::AbstractedServer()
: bluetoothServer(QBluetoothServiceInfo::RfcommProtocol),
  pendingSocket(0)
{
    tcpServer.setMaxPendingConnections(1);
    bluetoothServer.setMaxPendingConnections(1);

    QObject::connect(&tcpServer, SIGNAL(newConnection()), this, SLOT(newTcpConnection()));
    QObject::connect(&bluetoothServer, SIGNAL(newConnection()), this, SLOT(newBluetoothConnection()));

    trySetupServers();
}

AbstractedServer::~AbstractedServer()
{
    if(pendingSocket != 0)
        delete pendingSocket;
}

void AbstractedServer::newTcpConnection()
{
    QTcpSocket *tcpSocket = tcpServer.nextPendingConnection();
    tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    if(pendingSocket == 0) {
        pendingSocket = tcpSocket;
        pendingIsBluetooth = false;

        QString clientIp = tcpSocket->peerAddress().toString();
        int index = clientIp.lastIndexOf(':'); index = index==-1?0:index;
        clientIp = clientIp.right(clientIp.length() - index - 1);
        pendingSocketInfo = clientIp;

        emit newConnection();
    }
    else
        delete tcpSocket;
}

void AbstractedServer::newBluetoothConnection()
{
    qInfo() << "new bluetooth connection";
    QBluetoothSocket *bluetoothSocket = bluetoothServer.nextPendingConnection();

    if(pendingSocket == 0) {
        pendingSocket = bluetoothSocket;
        pendingIsBluetooth = true;
        pendingSocketInfo = "Connected via Bluetooth";
        emit newConnection();
    }
    else
        delete bluetoothSocket;
}

QIODevice *AbstractedServer::nextPendingConnection()
{
    QIODevice *ret = pendingSocket;
    pendingSocket = 0;
    return ret;
}

bool AbstractedServer::registerBluetoothService()
{
    serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceName, tr("WifiMouseServer"));
    serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceDescription, tr("WifiMouseServer"));
    serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceProvider, tr("wifi-mouse.xyz"));

    static const QLatin1String serviceUuid("c05efa2b-5e9f-4a39-9705-72ccf47d2eb8");
    serviceInfo.setServiceUuid(QBluetoothUuid(serviceUuid));

    QBluetoothServiceInfo::Sequence publicBrowse;
    publicBrowse << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::PublicBrowseGroup));
    serviceInfo.setAttribute(QBluetoothServiceInfo::BrowseGroupList, publicBrowse);

    QBluetoothServiceInfo::Sequence protocolDescriptorList;
    QBluetoothServiceInfo::Sequence protocol;
    protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::L2cap));
    protocolDescriptorList.append(QVariant::fromValue(protocol));
    protocol.clear();
    protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::Rfcomm))
             << QVariant::fromValue(quint8(bluetoothServer.serverPort()));
    protocolDescriptorList.append(QVariant::fromValue(protocol));
    serviceInfo.setAttribute(QBluetoothServiceInfo::ProtocolDescriptorList, protocolDescriptorList);

    return serviceInfo.registerService();
}

bool AbstractedServer::bluetoothServerListen()
{
    if(!bluetoothServer.isListening() && !bluetoothServer.listen()) {
        qInfo() << "Unable to start bluetooth server.\n";
        return false;
    }
    else if(serviceInfo.isRegistered() == false) {
        if( !registerBluetoothService() ) {
            qInfo() << "Couldn't register service";
            return false;
        }
    }

    return true;
}

bool AbstractedServer::tcpServerListen()
{
    if(!tcpServer.isListening() && !tcpServer.listen(QHostAddress::Any, 9798)) {
         qInfo() << "Unable to start TCP server on port 9798.\n";
         return false;
    }
    return true;
}

void AbstractedServer::listenWithTimeout(qint16 timeoutMs)
{
    trySetupServers();
    QTime stopWatch;
    stopWatch.start();
    while(stopWatch.elapsed() < timeoutMs && pendingSocket == 0) {
        eventLoop.processEvents();
    }
}

void AbstractedServer::trySetupServers()
{
    // these can be recalled if initial setup failed
    bluetoothServerListen();
    tcpServerListen();
}
