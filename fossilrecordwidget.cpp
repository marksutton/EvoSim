#include "fossilrecordwidget.h"
#include "ui_fossilrecordwidget.h"


FossilRecordWidget::FossilRecordWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FossilRecordWidget)
{
    ui->setupUi(this);

    ui->treeWidget->setColumnWidth(0,150);
    ui->treeWidget->setColumnWidth(1,40);
    ui->treeWidget->setColumnWidth(2,40);
    ui->treeWidget->setColumnWidth(3,40);
    ui->treeWidget->setColumnWidth(4,40);

    ui->treeWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);
    FossilRecords.clear();

    RefreshMe();
    NextItem=0;

}

void FossilRecordWidget::RefreshMe()
{
    //write in all the current records
    ui->treeWidget->clear();

    if (FossilRecords.count()==0) return;
    ui->treeWidget->setUniformRowHeights(true);
    ui->treeWidget->setUpdatesEnabled(false);

    for (int i=0; i<FossilRecords.count(); i++)
    {
        QStringList strings;
        QString temp;

        strings.append(FossilRecords[i]->name);
        temp.sprintf("%d",FossilRecords[i]->xpos); strings.append(temp);
        temp.sprintf("%d",FossilRecords[i]->ypos); strings.append(temp);
        temp.sprintf("%d",FossilRecords[i]->startage); strings.append(temp);
        temp.sprintf("%d",FossilRecords[i]->sparsity); strings.append(temp);

        QTreeWidgetItem *item=new QTreeWidgetItem(strings);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->treeWidget->addTopLevelItem(item);
    }
   ui->treeWidget->setUpdatesEnabled(true);

   return;
}

FossilRecordWidget::~FossilRecordWidget()
{
    delete ui;
}

void FossilRecordWidget::MakeRecords()
{
    for (int i=0; i<FossilRecords.count(); i++)
        FossilRecords[i]->MakeFossil();
}

void FossilRecordWidget::on_DeleteButton_pressed()
{
    QList<QTreeWidgetItem *> allitems;

    for (int i=0; i<ui->treeWidget->topLevelItemCount(); i++)
        allitems.append(ui->treeWidget->topLevelItem(i));

    for (int i=0; i<allitems.count(); i++)
    if (allitems[i]->isSelected())
    {
        //item i to be deleted
        FossilRecords[i]->xpos=-1; //mark for death
    }

    QMutableListIterator<FossilRecord *> i(FossilRecords);
    while (i.hasNext())
    {
        FossilRecord *item = i.next();
        if (item->xpos==-1)
       {
           delete item;
           i.remove();
       }

    }
    RefreshMe();
}

void FossilRecordWidget::on_NewButton_pressed()
{
    NewItem(50,50,10);
}

void FossilRecordWidget::NewItem(int x, int y, int s)
{
    QString String;
    String.sprintf("Record %d",NextItem++);
    FossilRecords.append(new FossilRecord(50,50,10,String));
    RefreshMe();
}

void FossilRecordWidget::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    //find index
    int index=ui->treeWidget->indexOfTopLevelItem(item);
    if (column==0) //
}
