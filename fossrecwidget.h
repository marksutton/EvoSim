#ifndef FOSSRECWIDGET_H
#define FOSSRECWIDGET_H

#include <QWidget>
#include <QDockWidget>
#include <QTreeWidgetItem>
#include "fossilrecord.h"
#include <QList>
#include <QDir>

namespace Ui {
    class FossRecWidget;
}

class FossRecWidget : public QWidget
{
    Q_OBJECT

public:
    FossRecWidget(QWidget *parent = 0);
    ~FossRecWidget();
    QList <FossilRecord *> FossilRecords;
    void MakeRecords();
    void RefreshMe();
    void HideWarnLabel();
    QString LogDirectory;
    bool LogDirectoryChosen;
    void WriteFiles();
    QList<bool> GetSelections();
    int findclosest(int x, int y, int min);
    QByteArray SaveState();
    void RestoreState(QByteArray InArray);
    void NewItem(int x, int y, int s);
    void SelectedActive(bool s);
    void SelectedSparse(int s);
private:
    Ui::FossRecWidget *ui;

    int NextItem;


private slots:
    void on_DeleteButton_pressed();
    void on_NewButton_pressed();
    void on_SelectAllButton_pressed();
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_treeWidget_itemSelectionChanged();
};

#endif // FOSSRECWIDGET_H
