#include "networkthread.h"
#include "mainwindow.h"
#include "fakeinput.h"
#include "abstractedserver.h"
#include "encryptutils.h"
#include <QDebug>
#include <QDateTime>
#include <QAbstractSocket>
#include <QHostInfo>

QString serverVersion = "1";

/****************************************
 ****************************************
 **
 ** Encryption & socket read/write code
 **
 ****************************************
 ****************************************/

#define BLOCK_SIZE 512
QIODevice *socket;
bool socketIsBluetooth;
QEventLoop *eventLoop;

#define JAVA_INT_MAX_VAL 2147483647
long sessionIV;
QByteArray sessionPasswordHash;

// wait functions that will work for both TCP & Bluetooth IODevice
bool waitForBytesWritten(QIODevice *socket, int msecs)
{
    QTime stopWatch;
    stopWatch.start();
    while(stopWatch.elapsed() < msecs && socket->bytesToWrite() && socket->isOpen()) {
        eventLoop->processEvents();
    }
    return socket->bytesToWrite() == false;
}
bool waitForReadyRead(QIODevice *socket, int msecs)
{
    QTime stopWatch;
    stopWatch.start();
    while(stopWatch.elapsed() < msecs && socket->bytesAvailable() == false && socket->isOpen()) {
        eventLoop->processEvents();
    }
    return socket->bytesAvailable();
}


// write & write encrypted
bool writeDataUnencrypted(QByteArray data) {
    data.resize(BLOCK_SIZE);
    socket->write(data);
    return waitForBytesWritten(socket, 100);
}

bool writeDataEncrypted(QByteArray data) {
    if(socketIsBluetooth)
        return writeDataUnencrypted(data); // bluetooth already encrypted

    data.resize(BLOCK_SIZE);
    QByteArray iv = EncryptUtils::makeHash16(QString::number(sessionIV).toUtf8());
    QByteArray encrypted = EncryptUtils::encryptBytes(data, sessionPasswordHash, iv);
    return writeDataUnencrypted(encrypted);
}

// read & read encrypted
qint64 readDataUnencrypted(QByteArray *data) {
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

        if(bytesLeft > 0 && waitForReadyRead(socket, 50) == false)
            break;
    }
    return totalBytesRead;
}

qint64 readDataEncrypted(QByteArray *data) {
    qint64 result = readDataUnencrypted(data);
    if(socketIsBluetooth)
        return result; // bluetooth already encrypted

    QByteArray decrypted;
    if(result == BLOCK_SIZE) {
        sessionIV = ( sessionIV + 1 ) % JAVA_INT_MAX_VAL;
        QByteArray iv = EncryptUtils::makeHash16(QString::number(sessionIV).toUtf8());
        decrypted = EncryptUtils::decryptBytes(*data, sessionPasswordHash, iv);
    }
    (*data) = decrypted;
    return result;
}

// read & write strings
bool writeString(QString str, bool encrypt) {
    if(encrypt)
        return writeDataEncrypted(str.toUtf8());
    else
        return writeDataUnencrypted(str.toUtf8());
}
QString readString(bool decrypt) {
    QByteArray bytes(BLOCK_SIZE, 0);

    int result;
    if(decrypt)
        result = readDataEncrypted(&bytes);
    else
        result = readDataUnencrypted(&bytes);

    if(result == BLOCK_SIZE)
        return QString(bytes);
    else
        return QString("");
}

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

   QEventLoop threadEventLoop;
   eventLoop = &threadEventLoop;

   AbstractedServer server;

   int count = 0;
   while(true) {
       updateClientIp("Not connected");
       qInfo() << "Listening for connection... " << ++count;
       server.listenWithTimeout(1000);
       socket = server.nextPendingConnection();
       socketIsBluetooth = server.pendingIsBluetooth;

       if(socket == 0)
           continue;

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
    if( !waitForReadyRead(socket, 1000) ) {
        qInfo() << "Read timed out\n";
        return false;
    }

    sessionPasswordHash = getPassword();
    srand(time(NULL));
    sessionIV = rand() % JAVA_INT_MAX_VAL;

    // First inform we are a server and send sessionIV:
    if(readString(false) == "cow.emoji.WifiMouseClient") {
        QString hello_str = "cow.emoji.WifiMouseServer "+serverVersion+" "+QHostInfo::localHostName().replace(" ", "-")+" "+QString::number(sessionIV);
        writeString(hello_str, false);
    }
    else
        return false;

    if(!waitForReadyRead(socket, 1000)) {
        qInfo() << "Read timed out...\n";
        return false;
    }

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
        if(!waitForReadyRead(socket, 1000)) {
            qInfo() << "Read timed out...\n";
            break;
        }

        // Read out all messages sent
        for(QString message = readString(true); message.length() > 0; message = readString(true)) {
            bool zoomEvent = false;

            if(message == "PING") {
                if(memcmp(getPassword(), sessionPasswordHash.data(), 16) != 0)
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
            }

            if(!zoomEvent)
                FakeInput::stopZoom();

            if(message == "Quit")
                return;
        }
    }
}
