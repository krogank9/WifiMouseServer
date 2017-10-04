#ifndef ABSTRACTEDSOCKET_H
#define ABSTRACTEDSOCKET_H

#include <QIODevice>
#include <QEventLoop>
#include <QTimer>

const int JAVA_INT_MAX_VAL = 2147483647;

class AbstractedSocket : public QObject
{
    Q_OBJECT

public:
    AbstractedSocket(QIODevice *socket, bool bluetooth);
    ~AbstractedSocket();

    void initSession(long iv, QByteArray passHash);
    QByteArray getSessionHash();

    bool waitForBytesWritten(qint64 ms);
    bool waitForReadyRead(qint64 ms);

    bool writeDataUnencrypted(QByteArray data);
    bool writeDataEncrypted(QByteArray data);
    QByteArray readDataUnencrypted();
    QByteArray readDataEncrypted();
    bool writeString(QString str, bool encrypt);
    QString readString(bool decrypt);

private:
    bool readAllData(QByteArray *data);
    bool writeAllData(QByteArray data);

    QIODevice *socket;
    bool socketIsBluetooth;
    long sessionIV;
    QByteArray sessionPasswordHash;

    QEventLoop eventLoop;
    QTimer timeoutTimer;

public slots:
    void timeout();
    void readyRead();
    void bytesWritten();
};

#endif // ABSTRACTEDSOCKET_H
