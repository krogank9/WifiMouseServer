#include "mainwindow.h"
#include "networkthread.h"
#include "runguard.h"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    RunGuard guard("WifiMouseServer");
    if( !guard.tryToRun() ) {
        QMessageBox error;
        error.setText("Another instance of WifiMouseServer is already running.");
        error.exec();
        return 0;
    }

    MainWindow w;
    NetworkThread networkThread;
    networkThread.mainWindow = &w;

    networkThread.start();
    int result = a.exec();

    networkThread.exit();
    networkThread.requestInterruption();
    networkThread.wait(5000);

    return result;
}
