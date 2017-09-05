#include "socketutils.h"
#include "encryptutils.h"
#include <QEventLoop>
#include <QTime>
#include <QDebug>

QByteArray longToBytes(long n) {
    QByteArray bytes(4, 0);
    for(int i=0; i<bytes.length(); i++)
        *(bytes.data() + i) = 0xff & (n >> 8*i);
    return bytes;
}

long bytesToLong(QByteArray bytes) {
    long n = 0;
    for(int i=0; i<bytes.length(); i++)
        n += bytes.at(i) << 8*i;
    return n;
}

namespace SocketUtils
{

QIODevice *socket;
bool socketIsBluetooth;

long sessionIV;
QByteArray sessionPasswordHash;

void setGlobalSocket(QIODevice *socket, bool bluetooth)
{
    SocketUtils::socket = socket;
    socketIsBluetooth = bluetooth;
}

void initSession(long sessionIV, QByteArray sessionPasswordHash)
{
    SocketUtils::sessionIV = sessionIV;
    SocketUtils::sessionPasswordHash = sessionPasswordHash;
}

QByteArray getSessionHash()
{
    return sessionPasswordHash;
}

// wait functions that will work for both TCP & Bluetooth IODevice
bool waitForBytesWritten(int msecs)
{
    QEventLoop eventLoop;
    QTime stopWatch;
    stopWatch.start();
    while(stopWatch.elapsed() < msecs && socket->bytesToWrite() && socket->isOpen()) {
        eventLoop.processEvents();
    }
    return socket->bytesToWrite() == false;
}
bool waitForReadyRead(int msecs)
{
    QEventLoop eventLoop;
    QTime stopWatch;
    stopWatch.start();
    while(stopWatch.elapsed() < msecs && socket->bytesAvailable() == false && socket->isOpen()) {
        eventLoop.processEvents();
    }
    return socket->bytesAvailable();
}

// write & write encrypted
bool writeDataUnencrypted(QByteArray data) {
    data.resize(BLOCK_SIZE);
    int wroteSoFar = 0;
    while(wroteSoFar < data.size()) {
        int wroteThisTime = socket->write(data.data() + wroteSoFar, data.size() - wroteSoFar);
        if(wroteThisTime <= 0)
            break;
        else
            wroteSoFar += wroteThisTime;
    }

    if(wroteSoFar != data.size())
        socket->close();

    return waitForBytesWritten(100);
}

bool writeDataEncrypted(QByteArray data) {
    if(socketIsBluetooth)
        return writeDataUnencrypted(data); // bluetooth already encrypted

    data.resize(BLOCK_SIZE);
    sessionIV = ( sessionIV + 1 ) % JAVA_INT_MAX_VAL;
    QByteArray iv = EncryptUtils::makeHash16(QString::number(sessionIV).toUtf8());
    QByteArray encrypted = EncryptUtils::encryptBytes(data, sessionPasswordHash, iv);
    return writeDataUnencrypted(encrypted);
}

// read & read encrypted
qint64 readDataUnencrypted(QByteArray *data) {
    int bytesLeft = data->length();
    int totalBytesRead = 0;
    while(bytesLeft > 0) {
        qint64 bytesRead;
        do {
            bytesRead = socket->read(data->data() + totalBytesRead, bytesLeft);

            if(bytesRead < 0) {
                qInfo() << "socket read error";
                return totalBytesRead;
            }
            else {
                totalBytesRead += bytesRead;
                bytesLeft -= bytesRead;
            }
        }
        while(bytesRead > 0 && bytesLeft > 0);

        // No bytes left to read. If Block hasn't finished reading, try wait
        if(bytesLeft > 0 && !waitForReadyRead(1000)) {
            // Close socket if stream disrupted
            if(totalBytesRead > 0)
                socket->close();
            qInfo() << "failed to read a full block. " << totalBytesRead;
            break;
        }
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

}
