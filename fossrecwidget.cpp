#include "fossrecwidget.h"
#include "ui_fossrecwidget.h"
#include "simmanager.h"
#include <QInputDialog>
#include <QMessageBox>

FossRecWidget::FossRecWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FossRecWidget)
{
    ui->setupUi(this);
    ui->treeWidget->setColumnWidth(0,150);
    ui->treeWidget->setColumnWidth(1,40);
    ui->treeWidget->setColumnWidth(2,40);
    ui->treeWidget->setColumnWidth(3,40);
    ui->treeWidget->setColumnWidth(4,50);
    ui->treeWidget->setColumnWidth(5,50);
    ui->treeWidget->setColumnWidth(6,30);

    ui->treeWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);
    FossilRecords.clear();

    RefreshMe();
    NextItem=0;

    LogDirectoryChosen=false;
}

void FossRecWidget::HideWarnLabel()
{
    ui->WarnLabel->setVisible(false);
}

FossRecWidget::~FossRecWidget()
{
    delete ui;

}

void FossRecWidget::RefreshMe()
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
        temp.sprintf("%d",FossilRecords[i]->recorded); strings.append(temp);
        if (FossilRecords[i]->recording) strings.append("Yes"); else strings.append("No");

        QTreeWidgetItem *item=new QTreeWidgetItem(strings);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->treeWidget->addTopLevelItem(item);
    }
   ui->treeWidget->setUpdatesEnabled(true);

   return;
}

void FossRecWidget::MakeRecords()
{
    if (!LogDirectoryChosen) return;
    for (int i=0; i<FossilRecords.count(); i++)
        FossilRecords[i]->MakeFossil();
}

void FossRecWidget::on_DeleteButton_pressed()
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

void FossRecWidget::on_NewButton_pressed()
{
    NewItem(50,50,10);
}

void FossRecWidget::NewItem(int x, int y, int s)
{
    QString String;
    String.sprintf("Record %d",NextItem++);
    FossilRecords.append(new FossilRecord(50,50,10,String));
    RefreshMe();
}

void FossRecWidget::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    //find index
    int index=ui->treeWidget->indexOfTopLevelItem(item);

    if (column==0)
    //Name
    {
        QString newname=QInputDialog::getText(this,"","New unique name for record item",QLineEdit::Normal,FossilRecords[index]->name);
        if (newname=="") return;

        bool ok=true;
        //Check name not in use
        for (int i=0; i<FossilRecords.count(); i++)
        {
            if (i!=index) if (FossilRecords[i]->name==newname) ok=false;
        }
        if (ok) FossilRecords[index]->name=newname;
        else
        {
            QMessageBox::warning(this,"Warning","A record with that name already exists");
        }
    }

    if (column==1)
    //Xpos
    {
        bool ok;
        int newint=QInputDialog::getInt(this,"","New X coordinate",FossilRecords[index]->xpos,0,gridX-1,1,&ok);
        if (!ok) return;
        else FossilRecords[index]->xpos=newint;
    }

    if (column==2)
    //Ypos
    {
        bool ok;
        int newint=QInputDialog::getInt(this,"","New Y coordinate",FossilRecords[index]->ypos,0,gridY-1,1,&ok);
        if (!ok) return;
        else FossilRecords[index]->ypos=newint;
    }

    //3 is start -can't edit
    //4 is count of records - can't edit

    if (column==4)
    //Sparsity
    {
        bool ok;
        int newint=QInputDialog::getInt(this,"","New Sparsity",FossilRecords[index]->sparsity,1,100000000,1,&ok);
        if (!ok) return;
        else FossilRecords[index]->sparsity=newint;
    }

    if (column==6)
    //Active
        FossilRecords[index]->recording = !FossilRecords[index]->recording;

    RefreshMe();
}

void FossRecWidget::WriteFiles()
{
    if (!LogDirectoryChosen) return;
    for (int i=0; i<FossilRecords.count(); i++)
    {
        qDebug()<<LogDirectory + "/" + FossilRecords[i]->name + ".csv";
        FossilRecords[i]->WriteRecord(LogDirectory + "/" + FossilRecords[i]->name + ".csv");
        qDebug()<<"Written";
    }
}
