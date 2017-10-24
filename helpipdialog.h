#ifndef HELPIPDIALOG_H
#define HELPIPDIALOG_H

#include <QDialog>

namespace Ui {
class HelpIpDialog;
}

class HelpIpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpIpDialog(QWidget *parent = 0);
    ~HelpIpDialog();

    void closeEvent(QCloseEvent *event);
    void show();

private:
    Ui::HelpIpDialog *ui;

private Q_SLOTS:
    void accept();
};

#endif // HELPIPDIALOG_H
