#include "networkthread.h"
#include "mainwindow.h"
#include "fakeinput.h"
#include "abstractedserver.h"
#include "encryptutils.h"
#include "socketutils.h"
#include "fileutils.h"
#include <QDebug>
#include <QDateTime>
#include <QAbstractSocket>
#include <QHostInfo>

using namespace SocketUtils;

QString serverVersion = "1";

/****************************************
 ****************************************
 **
 ** Connection search & verify code
 **
 ****************************************
 ****************************************/

void NetworkThread::run()
{
   FakeInput::initFakeInput();

   AbstractedServer server;

   int count = 0;
   while(true) {
       updateClientIp("Not connected");
       qInfo() << "Listening for connection... " << ++count;
       server.listenWithTimeout(1000);

       QIODevice *socket = server.nextPendingConnection();
       if(socket == 0)
           continue;
       else
           setGlobalSocket(socket, server.pendingIsBluetooth);

       if( verifyClient() ) {
           updateClientIp(server.pendingSocketInfo);

           qInfo() << "Client verified\n";
           startInputLoop();
       }
       else
           qInfo() << "Could not verify client";

       delete socket;
   }

   FakeInput::freeFakeInput();
}

bool NetworkThread::verifyClient()
{
    srand(QDateTime::currentMSecsSinceEpoch());
    long sessionIV = rand() % JAVA_INT_MAX_VAL;
    SocketUtils::initSession(sessionIV, getPassword());

    // First inform we are a server and send sessionIV:
    if(readString(false) == "cow.emoji.WifiMouseClient") {
        qInfo() << "unencrypted verified";
        QString hello_str = "cow.emoji.WifiMouseServer "+serverVersion+" "+QHostInfo::localHostName().replace(" ", "-")+" "+QString::number(sessionIV);
        writeString(hello_str, false);
    }
    else
        return false;

    // Then, verify client by decoding its encrypted message:
    if(readString(true) == "cow.emoji.WifiMouseClient") {
        writeString("Verified", false);
        return true;
    }
    else {
        writeString("Wrong password", false);
        return false;
    }
}

QByteArray NetworkThread::getPassword()
{
    QByteArray password;
    QMetaObject::invokeMethod( mainWindow, "getPassword", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QByteArray, password));
    return password;
}

void NetworkThread::updateClientIp(QString ip)
{
    QMetaObject::invokeMethod(mainWindow, "setClientIp", Q_ARG(QString, ip));
}

/****************************************
 ****************************************
 **
 ** Input loop & server code
 **
 ****************************************
 ****************************************/

void NetworkThread::startInputLoop()
{
    int pingCount = 0;
    while(true) {
        if(!bytesAvailable() && !waitForReadyRead(1000)) {
            qInfo() << "Read timed out...\n";
            break;
        }

        QString message = readString(true);

        bool zoomEvent = false;

        if(message == "PING") {
            if( memcmp(getPassword().data(), getSessionHash().data(), 16) != 0)
                return;
            qInfo() << "Pinging... " << ++pingCount << "\n";
            writeString("PING", true);
            continue;
        }

        if(message.startsWith("MouseMove ")) {
            message.remove("MouseMove ");
            QStringList coords = message.split(",");
            int x = ((QString)coords.at(0)).toInt();
            int y = ((QString)coords.at(1)).toInt();
            FakeInput::mouseMove(x,y);
        }
        else if(message.startsWith("MouseSetPos ")) {
            message.remove("MouseSetPos ");
            QStringList coords = message.split(",");
            int x = ((QString)coords.at(0)).toInt();
            int y = ((QString)coords.at(1)).toInt();
            FakeInput::mouseSetPos(x,y);
        } else if(message.startsWith("MouseScroll ")) {
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
            FakeInput::typeString(message);
        } else if(message.startsWith("SpecialKey ")) {
            message.remove("SpecialKey ");
            if(message.startsWith("Down "))
                FakeInput::keyDown(message.remove("Down "));
            else if(message.startsWith("Up "))
                FakeInput::keyUp(message.remove("Up "));
            else
                FakeInput::keyTap(message.remove("Tap "));
        } else if(message.startsWith("Zoom ")) {
            zoomEvent = true;
            message.remove("Zoom ");
            FakeInput::zoom(message.toInt());
        } else if(message.startsWith("Power ")) {
            message.remove("Power ");
            if(message == "Shutdown")
                FakeInput::shutdown();
            else if(message == "Restart")
                FakeInput::restart();
            else if(message == "Sleep")
                FakeInput::sleep();
            else if(message == "Logout")
                FakeInput::logout();
        } else if(message.startsWith("FileManager ")) {
            message = message.remove(0, QString("FileManager ").length());
            FileUtils::fileManagerCommand(message);
        } else if(message.startsWith("ScreenMirror ")) {
            message = message.remove("ScreenMirror ");
            FileUtils::sendScreenJPG( message );
        }
        else if(message == "Quit")
            break;

        if(!zoomEvent)
            FakeInput::stopZoom();
    }
    FakeInput::stopZoom();
}
