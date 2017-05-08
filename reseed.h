#ifndef RESEED_H
#define RESEED_H

#include <QDialog>

namespace Ui {
class reseed;
}

class reseed : public QDialog
{
    Q_OBJECT

public:
    explicit reseed(QWidget *parent = 0);
    ~reseed();

private slots:
        void on_buttonBox_accepted();
        void on_buttonBox_rejected();

private:
    Ui::reseed *ui;
};

#endif // RESEED_H
