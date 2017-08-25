#include "setpassworddialog.h"
#include "ui_setpassworddialog.h"
#include <QCloseEvent>
#include <QPushButton>

SetPasswordDialog::SetPasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetPasswordDialog)
{
    ui->setupUi(this);
}

SetPasswordDialog::~SetPasswordDialog()
{
    delete ui;
}

void SetPasswordDialog::accept()
{
    QMetaObject::invokeMethod(this->parent(), "setPassword", Q_ARG(QString, ui->setPasswordEdit->text()));
    ui->setPasswordEdit->setText("");
    this->hide();
}

void SetPasswordDialog::reject()
{
    ui->setPasswordEdit->setText("");
    this->hide();
}

void SetPasswordDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    ui->setPasswordEdit->setText("");
    this->hide();
}

void SetPasswordDialog::show()
{
    QDialog::show();
    ui->setPasswordEdit->setFocus(Qt::OtherFocusReason);
    ui->setPasswordButtons->button(QDialogButtonBox::Ok)->setDefault(true);
}
