#ifndef RESEED_H
#define RESEED_H

#include <QDialog>
#include <QMessageBox>
#include <QLabel>
#include <QRadioButton>

namespace Ui {
class reseed;
}

class reseed : public QDialog
{
    Q_OBJECT

public:
    explicit reseed(QWidget *parent = 0);
    ~reseed();

    QList<QRadioButton *> radios;

private slots:
        void on_buttonBox_accepted();
        void on_buttonBox_rejected();
        void radio_toggled();

private:
    Ui::reseed *ui;
};

#endif // RESEED_H
