#ifndef SOCKETUTILS_H
#define SOCKETUTILS_H

#include <QIODevice>

#define BLOCK_SIZE 512
#define JAVA_INT_MAX_VAL 2147483647

namespace SocketUtils
{

void setGlobalSocket(QIODevice *socket, bool bluetooth);
void initSession(long sessionIV, QByteArray sessionPasswordHash);
QByteArray getSessionHash();

bool waitForBytesWritten(int msecs);
bool waitForReadyRead(int msecs);
bool writeDataUnencrypted(QByteArray data);
bool writeDataEncrypted(QByteArray data);
qint64 readDataUnencrypted(QByteArray *data);
qint64 readDataEncrypted(QByteArray *data);
bool writeString(QString str, bool encrypt);
QString readString(bool decrypt);

}

#endif // SOCKETUTILS_H
