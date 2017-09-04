#ifndef ENCRYPTUTILS_H
#define ENCRYPTUTILS_H

#include <QByteArray>
#include <QString>

namespace EncryptUtils
{
    QByteArray makeHash16(QByteArray toHash);
    QByteArray decryptBytes(QByteArray data, QByteArray key, QByteArray iv);
    QByteArray encryptBytes(QByteArray data, QByteArray key, QByteArray iv);
}

#endif // ENCRYPTUTILS_H
