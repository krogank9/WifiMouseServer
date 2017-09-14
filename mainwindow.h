#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "fixedsvgwidget.h"
#include "rotatingsquare.h"
#include "setpassworddialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    Ui::MainWindow *ui;
    QIcon *programIcon;
    SetPasswordDialog *setPasswordDialog;

    QByteArray serverPassword;

    void updateServerIp();

    void loadSvgs();
    void startSvgAnimation();
    FixedSvgWidget *logoWidget;
    FixedSvgWidget *statusWidget;
    QSvgRenderer *listening1Svg;
    QSvgRenderer *listening2Svg;
    QSvgRenderer *listening3Svg;
    QSvgRenderer *connectedSvg;
    QSvgRenderer *logoSvg;

    RotatingSquare *rotatingSquare;

    void createTrayIcon();
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    void createActions();
    QAction *quitAction;
    QAction *maximizeAction;
    QAction *passwordAction;
    QAction *clientTitleAction;
    QAction *clientIpAction;

public Q_SLOTS:
    void updateListeningAnimation();
    void updateLogoAnimation();
    void clickMaximized();
    void clickMinimized();
    void clickQuit();
    void clickSetPassword();

    QByteArray getPassword();
    void setPassword(QString newPassword);
    void setClientIp(QString ip);

    void loadSettings();
    void saveSettings();
};

#endif // MAINWINDOW_H
