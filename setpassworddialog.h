#ifndef SETPASSWORDDIALOG_H
#define SETPASSWORDDIALOG_H

#include <QDialog>

namespace Ui {
class SetPasswordDialog;
}

class SetPasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetPasswordDialog(QWidget *parent = 0);
    ~SetPasswordDialog();

    void closeEvent(QCloseEvent *event);
    void show();

private:
    Ui::SetPasswordDialog *ui;

private Q_SLOTS:
    void accept();
    void reject();
};

#endif // SETPASSWORDDIALOG_H
