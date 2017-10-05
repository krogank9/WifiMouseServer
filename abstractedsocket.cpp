#include "abstractedsocket.h"
#include "encryptutils.h"
#include <QDateTime>
#include <QThread>
#include <QDebug>

const int IO_MAX_CHUNK = 1024*30; // 30 kb

/*********************
 * Helper functions  *
 *********************/

qint64 MIN(qint64 a, qint64 b) { return a<b?a:b; }
qint64 MAX(qint64 a, qint64 b) { return a>b?a:b; }

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

/*********************
 * Constructor/misc  *
 *********************/

AbstractedSocket::AbstractedSocket(QIODevice *socket, bool bluetooth)
{
    this->socket = socket;
    this->socketIsBluetooth = bluetooth;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, SIGNAL(timeout()), this, SLOT(timeout()));
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    QObject::connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten()));
}

AbstractedSocket::~AbstractedSocket()
{
    delete socket;
}

void AbstractedSocket::timeout()
{
    eventLoop.quit();
}

void AbstractedSocket::bytesWritten()
{
    eventLoop.quit();
}

void AbstractedSocket::readyRead()
{
    eventLoop.quit();
}

void AbstractedSocket::initSession(long iv, QByteArray passHash) {
    sessionIV = iv;
    sessionPasswordHash = passHash;
}

QByteArray AbstractedSocket::getSessionHash() {
    return sessionPasswordHash;
}

/*********************
 * Public functions  *
 *********************/

bool AbstractedSocket::waitForBytesWritten(qint64 ms) {
    qint64 until = QDateTime::currentMSecsSinceEpoch() + ms;

    while(socket->bytesToWrite() > 0
          && !QThread::currentThread()->isInterruptionRequested()
          && QDateTime::currentMSecsSinceEpoch() < until) {
        timeoutTimer.setInterval(until - QDateTime::currentMSecsSinceEpoch());
        timeoutTimer.start();

        eventLoop.exec();

        timeoutTimer.stop();
    }

    return socket->bytesToWrite() == 0;
}

bool AbstractedSocket::waitForReadyRead(qint64 ms) {
    qint64 until = QDateTime::currentMSecsSinceEpoch() + ms;

    while(socket->bytesAvailable() == 0
          && !QThread::currentThread()->isInterruptionRequested()
          && QDateTime::currentMSecsSinceEpoch() < until) {
        timeoutTimer.setInterval(until - QDateTime::currentMSecsSinceEpoch());
        timeoutTimer.start();

        eventLoop.exec();

        timeoutTimer.stop();
    }

    return socket->bytesAvailable() > 0;
}

bool AbstractedSocket::writeDataUnencrypted(QByteArray data) {
    // Note: bluetooth fucks up if you send data size and data in 2 writes.
    return writeAllData(intToBytes(data.size()) + data);
}

bool AbstractedSocket::writeDataEncrypted(QByteArray data) {
    if(socketIsBluetooth)
        return writeDataUnencrypted(data); // bluetooth already encrypted

    // AES encryption requires 16 byte blocks
    int padding = 16-(data.size()%16);
    data.resize(data.size() + padding);

    sessionIV = ( sessionIV + 1 ) % JAVA_INT_MAX_VAL;
    QByteArray iv = EncryptUtils::makeHash16(QString::number(sessionIV).toUtf8());
    QByteArray encrypted = EncryptUtils::encryptBytes(data, sessionPasswordHash, iv);

    return writeDataUnencrypted(encrypted);
}

QByteArray AbstractedSocket::readDataUnencrypted() {
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

QByteArray AbstractedSocket::readDataEncrypted() {
    QByteArray data = readDataUnencrypted();
    if(socketIsBluetooth)
        return data; // bluetooth protocol automatically encrypts/decrypts

    sessionIV = ( sessionIV + 1 ) % JAVA_INT_MAX_VAL;
    QByteArray iv = EncryptUtils::makeHash16(QString::number(sessionIV).toUtf8());

    if(data.size() % 16 == 0)
        data = EncryptUtils::decryptBytes(data, sessionPasswordHash, iv);
    else
        return QByteArray();

    return data;
}

bool AbstractedSocket::writeString(QString str, bool encrypt) {
    if(encrypt)
        return writeDataEncrypted(str.toUtf8());
    else
        return writeDataUnencrypted(str.toUtf8());
}

QString AbstractedSocket::readString(bool decrypt) {
    QByteArray data;
    if(decrypt)
        data = readDataEncrypted();
    else
        data = readDataUnencrypted();
    return QString::fromUtf8(data);
}

/*********************
 * Private functions *
 *********************/

bool AbstractedSocket::writeAllData(QByteArray data) {
    int wroteSoFar = 0;
    while(wroteSoFar < data.size()) {
        int bytesLeft = data.size() - wroteSoFar;
        int wroteThisTime = socket->write(data.data() + wroteSoFar, MIN(bytesLeft, IO_MAX_CHUNK));

        if(wroteThisTime <= 0 || !waitForBytesWritten(1000))
            break;
        else
            wroteSoFar += wroteThisTime;
    }

    if(wroteSoFar != data.size()) {
        socket->close();
        qInfo() << "Failed to write data. Closing.";
        return false;
    }
    else
        return socket->bytesToWrite() == 0 || waitForBytesWritten(1000);
}

bool AbstractedSocket::readAllData(QByteArray *data) {
    if(socket->bytesAvailable() == 0 && !waitForReadyRead(2500))
        return false;
    int readSoFar = 0;
    while(readSoFar < data->size()) {
        qint64 bytesRead;
        do {
            int bytesLeft = data->size() - readSoFar;
            // server doesn't seem to need max chunk read size. mobile glitches out for reading tho
            bytesRead = socket->read(data->data() + readSoFar, bytesLeft);

            if(bytesRead < 0)
                return false;
            else
                readSoFar += bytesRead;
        }
        while(bytesRead > 0 && readSoFar < data->size());

        // No bytes left to read. If Block hasn't finished reading, try wait
        if(readSoFar < data->size() && !waitForReadyRead(2500)) {
            socket->close();
            qInfo() << "Failed to read data. Closing.";
            return false;
        }
    }
    return true;
}
