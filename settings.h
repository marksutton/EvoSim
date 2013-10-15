#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include "ui_dialog.h"

class SettingsImpl : public QDialog, public Ui::Settings
{
Q_OBJECT
public:
        SettingsImpl( QWidget * parent = 0);
        bool RedoImages;
private slots:
        void on_buttonBox_accepted();
        void on_buttonBox_rejected();
};

#endif // SETTINGS_H
