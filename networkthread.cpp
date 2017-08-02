#include "networkthread.h"
#include "mainwindow.h"
#include "fakeinput.h"
#include <QDebug>
#include <QDateTime>
#include <QAbstractSocket>

NetworkThread::NetworkThread()
{
    FakeInput::initFakeInput();
}

NetworkThread::~NetworkThread()
{
    FakeInput::freeFakeInput();
}

void NetworkThread::run()
{
   QTcpServer tcpServer;
   while(true) {
       updateClientIp("Not connected");

       if(!tcpServer.isListening() && !tcpServer.listen(QHostAddress::Any, 9798)) {
           qInfo() << "Unable to start server on port 9798. Retrying...\n";
           this->sleep(1);
           continue;
       }
       qInfo() << "Server started, listening for connection...";
       tcpServer.waitForNewConnection(-1);
       QTcpSocket *socket = tcpServer.nextPendingConnection();

       if( socket != 0 && verifyClient(socket) ) {
           QString clientIp = socket->peerAddress().toString();
           int index = clientIp.lastIndexOf(':'); index = index==-1?0:index;
           clientIp = clientIp.right(clientIp.length() - index - 1);
           updateClientIp(clientIp);

           qInfo() << "Client verified\n";
           startInputLoop(socket);
       }
       else
           qInfo() << "Could not verify client";

       delete socket;
   }
}

QString NetworkThread::getPassword()
{
    QString password;
    QMetaObject::invokeMethod( mainWindow, "getPassword", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, password));
    return password;
}

bool NetworkThread::verifyClient(QTcpSocket *socket)
{
    if( !socket->waitForReadyRead(2000) )
        return false;

    QString desiredResponse("cow.emoji.WifiMouseClient "+getPassword()+"\n");
    QString clientResponse( socket->readLine() );

    if(clientResponse == desiredResponse) {
        socket->write("cow.emoji.WifiMouseServer Accepted\n");
        socket->waitForBytesWritten(60);
    }
    else {
        socket->write("cow.emoji.WifiMouseServer Pending\n");
        socket->waitForBytesWritten(60);
        return false;
    }

    return true;
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

void NetworkThread::startInputLoop(QTcpSocket *socket)
{
    QString startPassword = getPassword();
    while(true) {
        if(!socket->waitForReadyRead(2000)) {
            qInfo() << "Read timed out...\n";
            break;
        }

        // Read out all lines
        for(QString message = socket->readLine(); message.length() > 0; message = socket->readLine()) {
            message = message.left(message.length() - 1); // remove \n at end of each line

            if(message == "PING") {
                if(getPassword() != startPassword)
                    return;
                socket->write("PING");
                socket->waitForBytesWritten();
            }
            else if(message.startsWith("MouseMove ")) {
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
            }
        }
    }
}

void NetworkThread::updateClientIp(QString ip)
{
    QMetaObject::invokeMethod(mainWindow, "setClientIp", Q_ARG(QString, ip));
}
