#ifndef ENCRYPTUTILS_H
#define ENCRYPTUTILS_H

#include <QByteArray>
#include <QString>

class EncryptUtils
{
public:
    static QByteArray makeHash16(QByteArray toHash);
    static QByteArray decryptBytes(QByteArray data, QByteArray key, QByteArray iv);
    static QByteArray encryptBytes(QByteArray data, QByteArray key, QByteArray iv);
private:
};

#endif // ENCRYPTUTILS_H
