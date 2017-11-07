#include "fileutils.h"
#include <QEventLoop>
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
#include <QNetworkRequest>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslConfiguration>

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

void home(AbstractedSocket *socket)
{
    dir = QDir::home();
    socket->writeString("Success", true);
}

void up(AbstractedSocket *socket)
{
    socket->writeString(dir.cdUp()?"Success":"Failed", true);
}

void refresh(AbstractedSocket *socket)
{
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::NoFilter, QDir::DirsFirst | QDir::Name);
    socket->writeString(QString::number(fileInfoList.length()), true);
    for(int i=0; i<fileInfoList.length(); i++) {
        QFileInfo info = fileInfoList.at(i);
        socket->writeString(info.fileName(), true);
        socket->writeString(info.isDir()? "true":"false", true);
    }
}

void open(QString name, AbstractedSocket *socket)
{
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::NoFilter, QDir::DirsFirst | QDir::Name);

    QFileInfo file = getFileInfo(name);

    if(file.fileName().length() == 0) {
        socket->writeString("Failed", true);
        return;
    }
    else if(file.isDir()) {
        dir.cd(file.fileName());
    }
    else {
        QDesktopServices::openUrl( QUrl::fromLocalFile(file.absoluteFilePath()) );
    }

    socket->writeString("Success", true);
}

void copy(QString name, AbstractedSocket *socket)
{
    QFileInfo file = getFileInfo(name);
    if(file.fileName().length() > 0) {
        copied = file;
        cutting = false;
        qInfo() << "copying " << file.fileName();
        socket->writeString("Success", true);
    }
    else {
        socket->writeString("Failed", true);
    }
}

void cut(QString name, AbstractedSocket *socket)
{
    QFileInfo file = getFileInfo(name);
    if(file.fileName().length() > 0) {
        copied = file;
        cutting = true;
        socket->writeString("Success", true);
    }
    else {
        socket->writeString("Failed", true);
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

void paste(QString dirName, AbstractedSocket *socket)
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
                socket->writeString("Success", true);
                return;
            }
        }
        else {
            if( cpDir(dirToCopy.absolutePath(), destinationName) ) {
                socket->writeString("Success", true);
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
                socket->writeString("Success", true);
                return;
            }
        }
        else {
            if( file.copy(destinationName) ) {
                qInfo() << "pasted to" << destinationName;
                socket->writeString("Success", true);
                return;
            }
        }
    }

    socket->writeString("Failed", true);
}

void deleteCmd(QString name, AbstractedSocket *socket)
{
    QFileInfo info = getFileInfo(name);
    if(info.fileName().length() == 0) {
        socket->writeString("Failed", true);
        return;
    }

    if(info.isDir()) {
        QDir dir(info.absoluteFilePath());
        if(dir.removeRecursively()) {
            socket->writeString("Success", true);
        }
        else {
            socket->writeString("Failed", true);
        }
    }
    else {
        QFile file(info.absoluteFilePath());
        if(file.remove()) {
            socket->writeString("Success", true);
        }
        else {
            socket->writeString("Failed", true);
        }
    }
}

void details(QString name, AbstractedSocket *socket)
{
    QFileInfo info = getFileInfo(name);
    if(info.fileName().length() == 0) {
        socket->writeString("Failed", true);
        return;
    }

    QString details = "";
    details += "Name: " + info.fileName() + "\n";
    details += "Location: " + info.absolutePath() + "\n";
    details += "Size: " + QString::number(info.size()) + "\n";
    details += "Created: " + info.created().toString() + "\n";
    details += "Modified: " + info.lastModified().toString();

    socket->writeString(details, true);
}

void sendFileForDownload(QString name, AbstractedSocket *socket)
{
    qInfo() << "sending file";
    QFileInfo info = getFileInfo(name);
    QFile file(info.absoluteFilePath());
    if(file.exists() && file.open(QIODevice::ReadOnly)) {
        socket->writeString("Sending", true);
        QByteArray fileBytes = file.readAll();
        file.close();
        if( socket->writeDataEncrypted(fileBytes) )
            qInfo() << "sent file";
        else
            qInfo() << "failed to send file";
    }
    else {
        socket->writeString("Failed", true);
    }
}

void receiveSentFile(QString name, AbstractedSocket *socket)
{
    // Make sure ready to receive file & can write
    QFile toWrite(dir.absolutePath() + "/" + name);
    if( toWrite.open(QIODevice::WriteOnly) )
        socket->writeString("Ready", true);
    else
        socket->writeString("Failed", true);

    // Receive the file
    QByteArray fileBytes = socket->readDataEncrypted();
    qInfo() << fileBytes.length();
    socket->writeString("Success", true);

    // Write it to a file with given name
    int writtenSoFar = 0;
    while(writtenSoFar < fileBytes.length()) {
        int remainingBytes = fileBytes.size() - writtenSoFar;
        int writtenThisTime = toWrite.write(fileBytes.data() + writtenSoFar, remainingBytes);

        if(writtenThisTime <= 0) {
            socket->writeString("Failed", true);
            return;
        }
        else
            writtenSoFar += writtenThisTime;
    }
    toWrite.close();

    qInfo() << "fileSize" << fileBytes.length();
    qInfo() << "successfully wrote file";
}

void fileManagerCommand(QString command, AbstractedSocket *socket)
{
    if(command == "Refresh") {
        refresh(socket);
    } else if(command == "Home") {
        home(socket);
    } else if(command == "Up") {
        up(socket);
    } else if(command.startsWith("Open ")) {
        open(command.remove(0, QString("Open ").length()), socket);
    } else if(command.startsWith("Copy ")) {
        copy(command.remove(0, QString("Copy ").length()), socket);
    } else if(command.startsWith("Cut ")) {
        cut(command.remove(0, QString("Cut ").length()), socket);
    } else if(command.startsWith("Paste ")) {
        paste(command.remove(0, QString("Paste ").length()), socket);
    } else if(command.startsWith("Delete ")) {
        deleteCmd(command.remove(0, QString("Delete ").length()), socket);
    } else if(command.startsWith("Details ")) {
        details(command.remove(0, QString("Details ").length()), socket);
    } else if(command.startsWith("Download ")) {
        sendFileForDownload(command.remove(0, QString("Download ").length()), socket);
    } else if(command.startsWith("Send ")) {
        receiveSentFile(command.remove(0, QString("Send ").length()), socket);
    }
}

QPixmap grabScreens() {
  auto screens = QGuiApplication::screens();
  QList<QPixmap> scrs;
  int w = 0, h = 0, p = 0;
  foreach (auto scr, screens) {
    QPixmap pix = scr->grabWindow(0);
    w += pix.width();
    if (h < pix.height()) h = pix.height();
    scrs << pix;
  }
  QPixmap final(w, h);
  QPainter painter(&final);
  final.fill(Qt::black);
  foreach (auto scr, scrs) {
    painter.drawPixmap(QPoint(p, 0), scr);
    p += scr.width();
  }
  return final;
}

void sendScreenJPG(QString opts, AbstractedSocket *socket)
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

    QPixmap screenshot = grabScreens();
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

    socket->writeDataEncrypted(jpgBytes);
}

}
