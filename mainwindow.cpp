#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "encryptutils.h"
#include <QTimer>
#include <QPainter>
#include <QtNetwork/QNetworkInterface>
#include <QSettings>
#include <QCryptographicHash>
#include <QMessageBox>
#include "fakeinput.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    programIcon(new QIcon(":/images/icon64.png"))
{
    ui->setupUi(this);

    setPasswordDialog = new SetPasswordDialog(this);

    loadSettings();

    this->setWindowIcon(*programIcon);
    this->setFixedSize(this->geometry().width(),this->geometry().height());
    this->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);

    rotatingSquare = new RotatingSquare(0, 0, 0, this->width()/7, 35, 35, 35);
    loadSvgs();
    startSvgAnimation();

    updateServerIp();

    createActions();
    createTrayIcon();

    if(ui->startMinimizedCheck->isChecked() == false)
        this->show();
}

MainWindow::~MainWindow()
{
    delete rotatingSquare;
    delete ui;
    delete programIcon;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::loadSettings()
{
    QSettings settings("WifiMouse", QSettings::NativeFormat);
    serverPassword = settings.value("pass","").toByteArray();
    if(serverPassword.length() != 16)
        serverPassword = EncryptUtils::makeHash16("");
    ui->startMinimizedCheck->setChecked( settings.value("startMinimized", false).toBool() );
}

void MainWindow::saveSettings()
{
    QSettings settings("WifiMouse", QSettings::NativeFormat);
    settings.setValue("pass", serverPassword);
    settings.setValue("startMinimized", ui->startMinimizedCheck->isChecked());
}

QByteArray MainWindow::getPassword()
{
    return serverPassword;
}

void MainWindow::setPassword(QString newPassword)
{
    // Store password as 16 length hash ready to be used as AES key
    serverPassword = EncryptUtils::makeHash16(newPassword.toUtf8());
    saveSettings();
}

void MainWindow::setClientIp(QString ip)
{
    clientIpAction->setText(ip);

    if(ip == "Not connected" && statusWidget->getRenderer() == connectedSvg)
        statusWidget->setRenderer(listening1Svg);
    else if(ip != "Not connected")
        statusWidget->setRenderer(connectedSvg);
}

void MainWindow::updateServerIp()
{
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        qInfo() << address.toString();
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
            ui->serverIpLabel->setText("Server IP: "+address.toString());
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.translate(rotatingSquare->x, rotatingSquare->y);
    p.rotate(rotatingSquare->rotation);
    logoSvg->render(&p, QRectF(rotatingSquare->size/-2,rotatingSquare->size/-2,rotatingSquare->size,rotatingSquare->size));
}

void MainWindow::loadSvgs()
{
    listening1Svg = new QSvgRenderer(this); listening1Svg->load((QString)":/images/listening1.svg");
    listening2Svg = new QSvgRenderer(this); listening2Svg->load((QString)":/images/listening2.svg");
    listening3Svg = new QSvgRenderer(this); listening3Svg->load((QString)":/images/listening3.svg");
    logoSvg = new QSvgRenderer(this); logoSvg->load((QString)":/images/logo.svg");
    connectedSvg = new QSvgRenderer(this); connectedSvg->load((QString)":/images/connected.svg");
    statusWidget = new FixedSvgWidget(this);
    statusWidget->setRenderer(listening3Svg);

    statusWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->statusLayout->addWidget(statusWidget);
}

void MainWindow::updateListeningAnimation()
{
    if(this->isVisible() == false)
        return;

    if(statusWidget->getRenderer() == listening1Svg)
        statusWidget->setRenderer(listening2Svg);
    else if(statusWidget->getRenderer() == listening2Svg)
        statusWidget->setRenderer(listening3Svg);
    else if(statusWidget->getRenderer() == listening3Svg)
        statusWidget->setRenderer(listening1Svg);

    statusWidget->repaint();

    updateServerIp();
}

void MainWindow::updateLogoAnimation()
{
    if(this->isVisible() == false)
        return;

    rotatingSquare->update(this->width(), this->height());
    this->repaint();
}

void MainWindow::startSvgAnimation()
{
    QTimer *listeningTimer = new QTimer(this);
    connect(listeningTimer, SIGNAL(timeout()), this, SLOT(updateListeningAnimation()));
    listeningTimer->start(1500);

    QTimer *logoTimer = new QTimer(this);
    connect(logoTimer, SIGNAL(timeout()), this, SLOT(updateLogoAnimation()));
    logoTimer->start(10);//120fps to ensure smooth animation
}

void MainWindow::clickMaximized()
{
    this->show();
    //trayIcon->hide();
}

void MainWindow::clickMinimized()
{
    this->hide();
    //trayIcon->show();
}

void MainWindow::clickQuit()
{
    this->close();
}

void MainWindow::clickSetPassword()
{
    if(!setPasswordDialog->isVisible())
        setPasswordDialog->show();
}

void MainWindow::createActions()
{
    quitAction = new QAction(tr("Quit"),this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(clickQuit()));
    connect(ui->quitButton, SIGNAL(released()), this, SLOT(clickQuit()));

    maximizeAction = new QAction(tr("Maximize"),this);
    connect(maximizeAction, SIGNAL(triggered()), this, SLOT(clickMaximized()));
    connect(ui->minimizeButton, SIGNAL(released()), this, SLOT(clickMinimized()));

    passwordAction = new QAction(tr("Set password"),this);
    connect(passwordAction, SIGNAL(triggered()), this, SLOT(clickSetPassword()));
    connect(ui->passwordButton, SIGNAL(released()), this, SLOT(clickSetPassword()));

    connect(ui->startMinimizedCheck, SIGNAL(released()), this, SLOT(saveSettings()));

    clientTitleAction = new QAction("Client IP", this);
    clientTitleAction->setEnabled(false);
    clientIpAction = new QAction("Not connected", this);
}

void MainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(*programIcon);
    trayIcon->show();

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(clientTitleAction);
    trayIconMenu->addAction(clientIpAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(passwordAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayIconMenu);
}
