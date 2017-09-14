#include "socketutils.h"
#include "encryptutils.h"
#include "abstractedserver.h"
#include <QEventLoop>
#include <QTime>
#include <QDebug>
#include <QTextCodec>
#include <QMutex>
#include <QWaitCondition>

qint64 MIN(qint64 a, qint64 b) {
    return a<b?a:b;
}
qint64 MAX(qint64 a, qint64 b) {
    return a>b?a:b;
}

QMutex sleepMutex;
void sleepMS(qint64 ms) {
    sleepMutex.lock();
    QWaitCondition sleepSimulator;
    sleepSimulator.wait(&sleepMutex, ms);
}

QByteArray intToBytes(unsigned int n) {
    QByteArray bytes(4, 0);
    for(int i=0; i<bytes.length(); i++)
        *(bytes.data() + i) = 0xFF & (n >> 8*i);
    return bytes;
}

unsigned int bytesToInt(QByteArray bytes) {
    if(bytes.length() < 4)
        return 0;
    int n = int((unsigned char)(bytes.at(0)) << 0 |
                (unsigned char)(bytes.at(1)) << 8 |
                (unsigned char)(bytes.at(2)) << 16 |
                (unsigned char)(bytes.at(3)) << 24);
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

bool bytesAvailable() { return socket->bytesAvailable() > 0; }

// wait functions that will work for both TCP & Bluetooth IODevice
bool waitForBytesWritten(int msecs)
{
    QEventLoop eventLoop;
    QTime stopWatch;
    stopWatch.start();

    eventLoop.processEvents();
    while(stopWatch.elapsed() < msecs && socket->bytesToWrite() && socket->isOpen()) {
        sleepMS(50); // sleep so loop doesn't take 100% cpu
        eventLoop.processEvents();
    }
    return socket->bytesToWrite() == false;
}
bool waitForReadyRead(int msecs)
{
    QEventLoop eventLoop;
    QTime stopWatch;
    stopWatch.start();

    eventLoop.processEvents();
    while(stopWatch.elapsed() < msecs && socket->bytesAvailable() == false && socket->isOpen()) {
        sleepMS(50); // sleep so loop doesn't take 100% cpu
        eventLoop.processEvents();
    }
    return bytesAvailable();
}

bool writeAllData(QByteArray data) {
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

    return wroteSoFar == data.size();
}

// write & write encrypted
bool writeDataUnencrypted(QByteArray data) {
    return writeAllData(intToBytes(data.size())) && writeAllData(data) && socket->waitForBytesWritten(1000);
}

bool writeDataEncrypted(QByteArray data) {
    if(socketIsBluetooth)
        return writeDataUnencrypted(data); // bluetooth already encrypted

    int originalSize = data.size();
    data = intToBytes(originalSize) + data;
    // AES encryption requires 16 byte blocks
    int padding = data.size() > 16 ? 16-(data.size()%16) : 16 - data.size();
    data.resize(data.size() + padding);

    sessionIV = ( sessionIV + 1 ) % JAVA_INT_MAX_VAL;
    QByteArray iv = EncryptUtils::makeHash16(QString::number(sessionIV).toUtf8());
    QByteArray encrypted = EncryptUtils::encryptBytes(data, sessionPasswordHash, iv);

    return writeDataUnencrypted(encrypted);
}

bool readAllData(QByteArray *data) {
    if(!bytesAvailable() && !waitForReadyRead(1000))
        return false;
    int bytesLeft = data->length();
    int totalBytesRead = 0;
    while(bytesLeft > 0) {
        qint64 bytesRead;
        do {
            bytesRead = socket->read(data->data() + totalBytesRead, bytesLeft);

            if(bytesRead < 0) {
                qInfo() << "socket read error";
                return false;
            }
            else {
                totalBytesRead += bytesRead;
                bytesLeft -= bytesRead;
            }
        }
        while(bytesRead > 0 && bytesLeft > 0);

        // No bytes left to read. If Block hasn't finished reading, try wait
        if(bytesLeft > 0 && !waitForReadyRead(1000)) {
            socket->close();
            return false;
        }
    }
    return true;
}

// read & read encrypted
QByteArray readDataUnencrypted() {
    // Get data size
    QByteArray dataLengthBytes(4, 0);
    if( readAllData(&dataLengthBytes) == false )
        return QByteArray();
    int dataLength = bytesToInt(dataLengthBytes);

    // Read data
    QByteArray data(dataLength, 0);
    if( readAllData(&data) )
        return data;
    else
        return QByteArray();
}

QByteArray readDataEncrypted() {
    QByteArray data = readDataUnencrypted();
    if(socketIsBluetooth)
        return data; // bluetooth protocol automatically encrypts/decrypts

    sessionIV = ( sessionIV + 1 ) % JAVA_INT_MAX_VAL;
    QByteArray iv = EncryptUtils::makeHash16(QString::number(sessionIV).toUtf8());

    if(data.size() % 16 == 0)
        data = EncryptUtils::decryptBytes(data, sessionPasswordHash, iv);
    else
        return data;

    // First 4 bytes of encrypted data are its real length minus padding
    qint64 realDataLength = MAX( MIN( bytesToInt(data), data.length() - 4 ), 0 );
    if(data.size() >= 4) {
        data.remove(0, 4);
        data.resize(realDataLength);
    }

    return data;
}

// read & write strings
bool writeString(QString str, bool encrypt) {
    if(encrypt)
        return writeDataEncrypted(str.toUtf8());
    else
        return writeDataUnencrypted(str.toUtf8());
}
QString readString(bool decrypt) {
    QByteArray data;
    if(decrypt)
        data = readDataEncrypted();
    else
        data = readDataUnencrypted();
    return QString::fromUtf8(data);
}

}
