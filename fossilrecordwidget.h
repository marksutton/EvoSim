#ifndef FOSSILRECORDWIDGET_H
#define FOSSILRECORDWIDGET_H

#include <QDockWidget>
#include "fossilrecord.h"
#include <QList>

namespace Ui {
    class FossilRecordWidget;
}

class FossilRecordWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit FossilRecordWidget(QWidget *parent = 0);
    ~FossilRecordWidget();
    QList <FossilRecord *> FossilRecords;
    void MakeRecords();

private:
    Ui::FossilRecordWidget *ui;
    void RefreshMe();
    int NextItem;
    void NewItem(int x, int y, int s);

private slots:
    void on_DeleteButton_pressed();
    void on_NewButton_pressed();
};

#endif // FOSSILRECORDWIDGET_H
