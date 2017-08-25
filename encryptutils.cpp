#include "encryptutils.h"
#include <QByteArray>
#include <QCryptographicHash>

extern "C" {
    #include "aes.h"
}

QByteArray EncryptUtils::makeHash16(QByteArray toHash)
{
    QCryptographicHash hashGen(QCryptographicHash::Sha256);
    hashGen.addData(toHash);
    QByteArray hash = hashGen.result();
    hash.resize(16);
    return hash;
}

QByteArray EncryptUtils::decryptBytes(QByteArray input, QByteArray key, QByteArray iv)
{
    QByteArray output(input.length(), 0);
    AES_CBC_decrypt_buffer((uint8_t*)output.data(), (uint8_t*)input.data(), input.length(), (uint8_t*)key.data(), (uint8_t*)iv.data());
    return output;
}

QByteArray EncryptUtils::encryptBytes(QByteArray input, QByteArray key, QByteArray iv)
{
    QByteArray output(input.length(), 0);
    AES_CBC_encrypt_buffer((uint8_t*)output.data(), (uint8_t*)input.data(), input.length(), (uint8_t*)key.data(), (uint8_t*)iv.data());
    return output;
}
