#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QDir>
#include <QString>
#include "abstractedsocket.h"

namespace FileUtils
{
    void fileManagerCommand(QString command, AbstractedSocket *socket);
    void sendScreenJPG(QString opts, AbstractedSocket *socket);
    QString downloadUrlToString(QString urlStr);
}

#endif // FILEUTILS_H
