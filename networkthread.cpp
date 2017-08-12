#include "networkthread.h"
#include "mainwindow.h"
#include "fakeinput.h"
#include <QDebug>
#include <QDateTime>
#include <QAbstractSocket>
#include <QHostInfo>

QString serverVersion = "1";
QTcpSocket *socket;

// todo: change protocol, first byte = # of bytes sent

QString bytesToString(QByteArray bytes) {
    int len = 0;
    while(len < bytes.length() && bytes.at(len) != 0)
        len++;
    bytes.resize(len);

    return QString(bytes);
}

bool writeData(QByteArray data) {
    data.resize(1024);
    socket->write(data);
    return socket->waitForBytesWritten(60);
}

bool writeString(QString str) {
    return writeData(str.toUtf8());
}

qint64 readData(QByteArray *data) {
    int bytesLeft = data->length();
    int totalBytesRead = 0;
    while(bytesLeft > 0) {
        qint64 bytesRead = socket->read(data->data() + totalBytesRead, bytesLeft);

        if(bytesRead <= 0)
            break;
        else {
            totalBytesRead += bytesRead;
            bytesLeft -= bytesRead;
        }

        if(bytesLeft > 0 && socket->waitForReadyRead(50) == false)
            break;
    }
    return totalBytesRead;
}

QString readString() {
    QByteArray bytes(1024, 0);
    if(readData(&bytes) == 1024)
        return bytesToString(bytes);
    else
        return QString("");
}

void NetworkThread::run()
{
   FakeInput::initFakeInput();

   QTcpServer tcpServer;

   while(!tcpServer.isListening() && !tcpServer.listen(QHostAddress::Any, 9798)) {
        qInfo() << "Unable to start server on port 9798. Retrying...\n";
        this->sleep(1);
   }

   qInfo() << "Server started, listening for connection...";

   while(true) {
       updateClientIp("Not connected");

       tcpServer.waitForNewConnection(1000);
       socket = tcpServer.nextPendingConnection();
       tcpServer.setMaxPendingConnections(1);

       if(socket == 0)
           continue;
       else
           socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

       if( socket != 0 && verifyClient() ) {

           QString clientIp = socket->peerAddress().toString();
           int index = clientIp.lastIndexOf(':'); index = index==-1?0:index;
           clientIp = clientIp.right(clientIp.length() - index - 1);
           updateClientIp(clientIp);

           qInfo() << "Client verified\n";
           startInputLoop();
       }
       else if(socket != 0)
           qInfo() << "Could not verify client";

       delete socket;
       socket = 0;
   }

   FakeInput::freeFakeInput();
}

QString NetworkThread::getPassword()
{
    QString password;
    QMetaObject::invokeMethod( mainWindow, "getPassword", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, password));
    return password;
}

bool NetworkThread::verifyClient()
{
    if( !socket->waitForReadyRead(2000) ) {
        qInfo() << "Read timed out\n";
        return false;
    }

    QString desiredResponse("cow.emoji.WifiMouseClient "+getPassword());
    QString clientResponse = readString();
    qInfo() << clientResponse;

    if(clientResponse == desiredResponse) {
        QString str = "cow.emoji.WifiMouseServer Accepted "+serverVersion+" "+QHostInfo::localHostName();
        writeString(str);
        return true;
    }
    else {
        QString str = "cow.emoji.WifiMouseServer Pending "+serverVersion+" "+QHostInfo::localHostName();
        writeString(str);
        return false;
    }
}

void wcharToChar(wchar_t *wca, char *ca)
{
    int index = 0;
    while(wca[index] != '\0') {
        ca[index] = wca[index];
        index++;
    }
    ca[index] = '\0';
}

void specialKeyEvent(QString message)
{
    if(message.startsWith("Down ")) {
        message.remove("Down ");
        wchar_t str[message.length() + 1]; str[message.length()] = '\0';
        message.toWCharArray(str);
        char cstr[message.length() + 1];
        wcharToChar(str, cstr);
        FakeInput::keyDown(cstr);
    }
    else if(message.startsWith("Up ")){
        message.remove("Up ");
        wchar_t str[message.length() + 1]; str[message.length()] = '\0';
        message.toWCharArray(str);
        char cstr[message.length() + 1];
        wcharToChar(str, cstr);
        FakeInput::keyUp(cstr);
    }
    else {
        message.remove("Tap ");
        wchar_t str[message.length() + 1]; str[message.length()] = '\0';
        message.toWCharArray(str);
        char cstr[message.length() + 1];
        wcharToChar(str, cstr);
        FakeInput::keyTap(cstr);
    }
}

void NetworkThread::startInputLoop()
{
    int pingCount = 0;
    QString startPassword = getPassword();
    while(true) {
        if(!socket->waitForReadyRead(1000)) {
            qInfo() << "Read timed out...\n";
            break;
        }

        // Read out all messages sent
        for(QString message = readString(); message.length() > 0; message = readString()) {
            bool zoomEvent = false;

            if(message == "PING") {
                if(getPassword() != startPassword)
                    return;
                qInfo() << "Pinging... " << ++pingCount << "\n";
                writeString("PING");
                continue;
            }

            if(message.startsWith("MouseMove ")) {
                message.remove("MouseMove ");
                QStringList coords = message.split(",");
                int x = ((QString)coords.at(0)).toInt();
                int y = ((QString)coords.at(1)).toInt();
                FakeInput::mouseMove(x,y);
            }
            else if(message.startsWith("MouseScroll ")) {
                message.remove("MouseScroll ");
                FakeInput::mouseScroll( message.toInt() );
            } else if(message.startsWith("MouseDown ")) {
                message.remove("MouseDown ");
                FakeInput::mouseDown( message.toInt() );
            } else if(message.startsWith("MouseUp ")) {
                message.remove("MouseUp ");
                FakeInput::mouseUp( message.toInt() );
            } else if(message.startsWith("Backspace ")) {
                message.remove("Backspace");
                int n = abs( message.toInt() );
                while(n-- > 0)
                    FakeInput::keyTap("BackSpace");
            } else if(message.startsWith("TypeString ")) {
                message.remove(0, QString("TypeString ").length());
                message.replace("\uFFFF","\n");
                wchar_t str[message.length() + 1];
                message.toWCharArray(str);
                str[message.length()] = '\0';
                FakeInput::typeString(str);
            } else if(message.startsWith("SpecialKey ")) {
                message.remove("SpecialKey ");
                specialKeyEvent(message);
            } else if(message.startsWith("Zoom ")) {
                zoomEvent = true;
                message.remove("Zoom ");
                FakeInput::zoom(message.toInt());
            }

            if(!zoomEvent)
                FakeInput::stopZoom();
        }
    }
}

void NetworkThread::updateClientIp(QString ip)
{
    QMetaObject::invokeMethod(mainWindow, "setClientIp", Q_ARG(QString, ip));
}
