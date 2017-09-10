#include "fileutils.h"
#include "socketutils.h"
#include <QFileInfoList>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QIODevice>
#include <QPixmap>
#include <QCursor>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QBuffer>
#include <QDebug>
#include <QRect>

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

void sendFileForDownload(QString name)
{
    QFileInfo info = getFileInfo(name);
    QFile file(info.absoluteFilePath());
    if(file.exists() && file.open(QIODevice::ReadOnly)) {
        writeString("Sending", true);
        QByteArray fileBytes = file.readAll();
        file.close();
        writeDataEncrypted(fileBytes);
    }
    else {
        writeString("Failed", true);
    }
}

void receiveSentFile(QString name)
{
    // Make sure ready to receive file & can write
    QFile toWrite(dir.absolutePath() + "/" + name);
    if( toWrite.open(QIODevice::WriteOnly) )
        writeString("Ready", true);
    else
        writeString("Failed", true);

    // Receive the file
    QByteArray fileBytes = readDataEncrypted();

    // Write it to a file with given name
    int writtenSoFar = 0;
    while(writtenSoFar < fileBytes.length()) {
        int remainingBytes = fileBytes.size() - writtenSoFar;
        int writtenThisTime = toWrite.write(fileBytes.data() + writtenSoFar, remainingBytes);

        if(writtenThisTime <= 0) {
            writeString("Failed", true);
            return;
        }
        else
            writtenSoFar += writtenThisTime;
    }
    toWrite.close();

    writeString("Success", true);
    qInfo() << "fileSize" << fileBytes.length();
    qInfo() << "successfully wrote file";
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
        sendFileForDownload(command.remove(0, QString("Download ").length()));
    } else if(command.startsWith("Send ")) {
        receiveSentFile(command.remove(0, QString("Send ").length()));
    }
}

void sendScreenJPG(QString opts)
{
    QStringList optList = opts.split(" ");
    int quality = optList.at(0).toInt();
    QRect cropRect;
    bool cropped = false;
    if(optList.length() == 5) {
        cropped = true;
        cropRect.setX(optList.at(1).toInt());
        cropRect.setY(optList.at(2).toInt());
        cropRect.setWidth(optList.at(3).toInt());
        cropRect.setHeight(optList.at(4).toInt());
    }

    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap screenshot = screen->grabWindow(0);
    if(cropped) {
        QPainter painterFrame(&screenshot);
        painterFrame.setCompositionMode(QPainter::CompositionMode_Source);
        painterFrame.fillRect(0, 0, cropRect.left(), screenshot.height(), Qt::white); // left
        painterFrame.fillRect(0, 0, screenshot.width(), cropRect.top(), Qt::white); // top
        painterFrame.fillRect(cropRect.right(), 0, screenshot.width() - cropRect.right(), screenshot.height(), Qt::white); // right
        painterFrame.fillRect(0, cropRect.bottom(), screenshot.width(), screenshot.height() - cropRect.bottom(), Qt::white); // bottom
        painterFrame.end();
    }
    QByteArray jpgBytes;
    QBuffer buffer(&jpgBytes);
    buffer.open(QIODevice::WriteOnly);
    screenshot.save(&buffer, "JPG", quality);

    qInfo() << "quality" << quality;

    writeDataEncrypted(jpgBytes);
}

}
