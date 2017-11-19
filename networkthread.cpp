#include "networkthread.h"
#include "mainwindow.h"
#include "fakeinput.h"
#include "abstractedserver.h"
#include "abstractedsocket.h"
#include "encryptutils.h"
#include "fileutils.h"
#include <QDebug>
#include <QDateTime>
#include <QAbstractSocket>
#include <QHostInfo>

QString serverVersion = "2";

/****************************************
 ****************************************
 **
 ** Listen for connections & verify clients
 **
 ****************************************
 ****************************************/

void NetworkThread::run()
{
   AbstractedServer server;

   FakeInput::initFakeInput();

   int count = 0;
   while(!isInterruptionRequested()) {
       updateClientIp("Not connected");
       qInfo() << "Listening for connection... " << ++count;
       server.listenWithTimeout(1000);

       AbstractedSocket *socket = server.nextPendingConnection();
       if(socket == 0)
           continue;

       if( verifyClient(socket) ) {
           updateClientIp(server.pendingSocketInfo);

           qInfo() << "Client verified\n";
           startInputLoop(socket);
       }
       else
           qInfo() << "Could not verify client";

       delete socket;
   }

   FakeInput::freeFakeInput();
}

bool NetworkThread::verifyClient(AbstractedSocket *socket)
{
    srand(QDateTime::currentMSecsSinceEpoch());
    long sessionIV = rand() % JAVA_INT_MAX_VAL;
    socket->initSession(sessionIV, getPassword());

    // First inform we are a server and send sessionIV:
    if(socket->readString(false) == "cow.emoji.WifiMouseClient") {
        qInfo() << "unencrypted verified";
        QString hello_str = "cow.emoji.WifiMouseServer "+serverVersion+" "+QHostInfo::localHostName().replace(" ", "-")+" "+FakeInput::getOsName()+" "+QString::number(sessionIV);
        socket->writeString(hello_str, false);
    }
    else
        return false;

    // Then, verify client by decoding its encrypted message:
    if(socket->readString(true) == "cow.emoji.WifiMouseClient") {
        socket->writeString("Verified", false);
        return true;
    }
    else {
        socket->writeString("Wrong password", false);
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
 ** Input loop entered once client is verified
 **
 ****************************************
 ****************************************/

void specialKeyCombo(QString comboString) {
    QStringList keyList = comboString.split(" ");

    for(int i=0; i<keyList.length(); i++)
        FakeInput::keyDown(keyList.at(i));
    for(int i=keyList.length()-1; i>=0; i--)
        FakeInput::keyUp(keyList.at(i));
}

void NetworkThread::startInputLoop(AbstractedSocket *socket)
{
    int pingCount = 0;
    while(!isInterruptionRequested()) {
        if( !socket->waitForReadyRead(1000) ) {
            qInfo() << "Read timed out...\n";
            break;
        }

        QString message = socket->readString(true);
        if(message.startsWith("MouseMove") == false
        && message.startsWith("PING") == false)
            qInfo() << message;

        if(message == "PING") {
            if( memcmp(getPassword().data(), socket->getSessionHash().data(), 16) != 0)
                break;
            //qInfo() << "Pinging... " << ++pingCount << "\n";
            socket->writeString("PING", true);
        }
        else if(message.startsWith("MouseMove ")) {
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
        } else if(message.startsWith("SpecialKeyCombo ")) {
            message.remove("SpecialKeyCombo ");
            specialKeyCombo(message);
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
            else if(message == "Blank")
                FakeInput::blank_screen();
            else if(message == "Lock")
                FakeInput::lock_screen();
        } else if(message.startsWith("FileManager ")) {
            message = message.remove(0, QString("FileManager ").length());
            FileUtils::fileManagerCommand(message, socket);
        } else if(message.startsWith("ScreenMirror ")) {
            message = message.remove("ScreenMirror ");
            FileUtils::sendScreenJPG(message, socket);
        } else if(message.startsWith("Command ")) {
            message = message.remove(0, QString("Command ").length());
            if(message.startsWith("Run ")) {
                message = message.remove(0, QString("Run ").length());
                QString result = FakeInput::runCommandForResult(message);
                socket->writeString(result, true);
            }
            else if(message.startsWith("Suggest ")) {
                message = message.remove(0, QString("Suggest ").length());
                QString suggestions = FakeInput::getCommandSuggestions(message);
                socket->writeString(suggestions, true);
            }
        }
        else if(message == "GetApplications") {
            socket->writeString(FakeInput::getApplicationNames(), true);
        }
        else if(message.startsWith("StartApplication ")) {
            message = message.remove(0, QString("StartApplication ").length());
            FakeInput::startApplicationByName(message);
        }
        else if(message == "GetCpuUsage") {
            socket->writeString(FakeInput::getCpuUsage(), true);
        }
        else if(message == "GetRamUsage") {
            socket->writeString(FakeInput::getRamUsage(), true);
        }
        else if(message == "GetTasks") {
            socket->writeString(FakeInput::getProcesses(), true);
        }
        else if(message.startsWith("KillPID ")) {
            message = message.remove("KillPID ");
            qInfo() << "Killing PID" << message;
            FakeInput::killProcess(message);
        }
        else if(message == "Quit")
            break;
    }
}
