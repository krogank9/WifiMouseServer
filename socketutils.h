#ifndef SOCKETUTILS_H
#define SOCKETUTILS_H

#include <QIODevice>

#define CHUNK_SIZE 512
#define JAVA_INT_MAX_VAL 2147483647

void sleepMS(qint64 ms);

namespace SocketUtils
{

void setGlobalSocket(QIODevice *socket, bool bluetooth);
void initSession(long sessionIV, QByteArray sessionPasswordHash);
QByteArray getSessionHash();

bool writeAllData(QByteArray data);
bool readAllData(QByteArray *data);

bool bytesAvailable();
bool waitForBytesWritten(int msecs, bool *shouldQuit = 0);
bool waitForReadyRead(int msecs, bool *shouldQuit = 0);
bool writeDataUnencrypted(QByteArray data);
bool writeDataEncrypted(QByteArray data);
QByteArray readDataUnencrypted();
QByteArray readDataEncrypted();
bool writeString(QString str, bool encrypt);
QString readString(bool decrypt);

}

#endif // SOCKETUTILS_H
