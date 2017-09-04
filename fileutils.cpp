#include "fileutils.h"
#include "socketutils.h"
#include <QFileInfoList>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QIODevice>
#include <QDebug>

using namespace SocketUtils;

QDir dir = QDir::home();
QFileInfo copied;
bool cutting;

QFileInfo getFileInfo(QString name)
{
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::NoFilter, QDir::DirsFirst | QDir::Name);
    for(int i=0; i<fileInfoList.length(); i++) {
        QFileInfo info = fileInfoList.at(i);
        if(name.length() > 0 && name == info.fileName())
            return info;
    }
    return QFileInfo();
}

namespace FileUtils
{

void home()
{
    dir = QDir::home();
    writeString("Success", true);
}

void up()
{
    if(dir.cdUp())
        writeString("Success", true);
    else
        writeString("Failed", true);
}

void refresh()
{
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::NoFilter, QDir::DirsFirst | QDir::Name);
    SocketUtils::writeString(QString::number(fileInfoList.length()), true);
    for(int i=0; i<fileInfoList.length(); i++) {
        QFileInfo info = fileInfoList.at(i);
        writeString(info.fileName(), true);
        writeString(info.isDir()? "true":"false", true);
    }
}

void open(QString name)
{
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::NoFilter, QDir::DirsFirst | QDir::Name);

    QFileInfo file = getFileInfo(name);

    if(file.fileName().length() == 0) {
        writeString("Failed", true);
        return;
    }
    else if(file.isDir()) {
        dir.cd(file.fileName());
    }
    else {
        QDesktopServices::openUrl( QUrl("file://" + file.absoluteFilePath()) );
    }

    writeString("Success", true);
}

void copy(QString name)
{
    QFileInfo file = getFileInfo(name);
    if(file.fileName().length() > 0) {
        copied = file;
        cutting = false;
        qInfo() << "copying " << file.fileName();
        writeString("Success", true);
    }
    else {
        writeString("Failed", true);
    }
}

void cut(QString name)
{
    QFileInfo file = getFileInfo(name);
    if(file.fileName().length() > 0) {
        copied = file;
        cutting = true;
        writeString("Success", true);
    }
    else {
        writeString("Failed", true);
    }
}

bool cpDir(const QString &srcPath, const QString &dstPath)
{
    QDir parentDstDir(QFileInfo(dstPath).path());
    if (!parentDstDir.mkdir(QFileInfo(dstPath).fileName()))
        return false;

    QDir srcDir(srcPath);
    foreach(const QFileInfo &info, srcDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
        QString srcItemPath = srcPath + "/" + info.fileName();
        QString dstItemPath = dstPath + "/" + info.fileName();
        if (info.isDir()) {
            if (!cpDir(srcItemPath, dstItemPath)) {
                return false;
            }
        } else if (info.isFile()) {
            if (!QFile::copy(srcItemPath, dstItemPath)) {
                return false;
            }
        } else {
            qDebug() << "Unhandled item" << info.filePath() << "in cpDir";
        }
    }
    return true;
}

void paste(QString dirName)
{
    QFileInfo dirInfo = getFileInfo(dirName);
    if(dirInfo.isDir() == false)
        dirName = "";
    if(dirName.endsWith("/") == false && dirName.length() > 0)
        dirName += "/";

    QString destinationPath = dir.absolutePath();
    if(destinationPath.endsWith("/") == false)
        destinationPath += "/";
    destinationPath += dirName;
    QString destinationName = destinationPath + copied.fileName();

    if( copied.isDir() ) {
        QDir dirToCopy(copied.absoluteFilePath());
        if(cutting) {
            if( dirToCopy.rename(dirToCopy.absolutePath(), destinationName) ) {
                cutting = false;
                writeString("Success", true);
                return;
            }
        }
        else {
            if( cpDir(dirToCopy.absolutePath(), destinationName) ) {
                writeString("Success", true);
                return;
            }
        }
    }
    else {
        QFile file(copied.absoluteFilePath());
        if(cutting) {
            if( file.rename(destinationName) ) {
                cutting = false;
                copied = QFileInfo(destinationName);
                writeString("Success", true);
                return;
            }
        }
        else {
            if( file.copy(destinationName) ) {
                qInfo() << "pasted to" << destinationName;
                writeString("Success", true);
                return;
            }
        }
    }

    writeString("Failed", true);
}

void deleteCmd(QString name)
{
    QFileInfo info = getFileInfo(name);
    if(info.fileName().length() == 0) {
        writeString("Failed", true);
        return;
    }

    if(info.isDir()) {
        QDir dir(info.absoluteFilePath());
        if(dir.removeRecursively()) {
            writeString("Success", true);
        }
        else {
            writeString("Failed", true);
        }
    }
    else {
        QFile file(info.absoluteFilePath());
        if(file.remove()) {
            writeString("Success", true);
        }
        else {
            writeString("Failed", true);
        }
    }
}

void details(QString name)
{
    QFileInfo info = getFileInfo(name);
    if(info.fileName().length() == 0) {
        writeString("Failed", true);
        return;
    }

    QString details = "";
    details += "Name: " + info.fileName() + "\n";
    details += "Location: " + info.absolutePath() + "\n";
    details += "Size: " + QString::number(info.size()) + "\n";
    details += "Created: " + info.created().toString() + "\n";
    details += "Modified: " + info.lastModified().toString();

    writeString(details, true);
}

void download(QString name)
{
    QFileInfo info = getFileInfo(name);
    QFile file(info.absoluteFilePath());
    int fileSize = file.size();
    writeString(QString::number(fileSize), true);
    if(file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray bytes = file.readAll();
        long readSoFar = 0;
        while(readSoFar < fileSize) {
            if(bytes.size() - readSoFar >= BLOCK_SIZE) {
                QByteArray block(bytes.data() + readSoFar, BLOCK_SIZE);
                writeDataEncrypted(block);
            }
            else {
                QByteArray block(bytes.data() + readSoFar, bytes.size() - readSoFar);
                block.resize(BLOCK_SIZE);
                writeDataEncrypted(block);
            }
            readSoFar += BLOCK_SIZE;
        }
        file.close();
    }
}

void receiveSent(QString name)
{
}

void fileManagerCommand(QString command)
{
    if(command == "Refresh") {
        refresh();
    } else if(command == "Home") {
        home();
    } else if(command == "Up") {
        up();
    } else if(command.startsWith("Open ")) {
        open(command.remove(0, QString("Open ").length()));
    } else if(command.startsWith("Copy ")) {
        copy(command.remove(0, QString("Copy ").length()));
    } else if(command.startsWith("Cut ")) {
        cut(command.remove(0, QString("Cut ").length()));
    } else if(command.startsWith("Paste ")) {
        paste(command.remove(0, QString("Paste ").length()));
    } else if(command.startsWith("Delete ")) {
        deleteCmd(command.remove(0, QString("Delete ").length()));
    } else if(command.startsWith("Details ")) {
        details(command.remove(0, QString("Details ").length()));
    } else if(command.startsWith("Download ")) {
        download(command.remove(0, QString("Download ").length()));
    } else if(command.startsWith("Send ")) {
        receiveSent(command.remove(0, QString("Send ").length()));
    }
}

}
