#include "helpipdialog.h"
#include "ui_helpipdialog.h"
#include <QCloseEvent>
#include <QHostAddress>
#include <QNetworkInterface>

HelpIpDialog::HelpIpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpIpDialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
}

HelpIpDialog::~HelpIpDialog()
{
    delete ui;
}

void HelpIpDialog::accept()
{
    this->hide();
}

void HelpIpDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    this->hide();
}

void HelpIpDialog::show()
{
    QDialog::show();
    QString ips = "All iPv4 addresses:<br/><b>";
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
            ips += address.toString() + "<br/>";
    }
    ips += "</b>";
    ui->help_ip_label->setText(ips);
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    this->adjustSize();
    this->setFixedSize(this->geometry().width(),this->geometry().height());
}
