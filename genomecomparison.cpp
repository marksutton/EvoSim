#include "genomecomparison.h"
#include "ui_genomecomparison.h"

#include <QDebug>

GenomeComparison::GenomeComparison(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GenomeComparison)
{
    ui->setupUi(this);

    QFont fnt;
    fnt.setPointSize(8);
    fnt.setFamily("Arial");

    ui->tableWidget->setFont(fnt);
    ui->tableWidget->setColumnWidth(1,100);

    int i = 0;
    ui->tableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("")));
    i++;
    ui->tableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("Name")));
    i++;
    for(int x=0;x<64;x++)
    {
        if (x==32) {
            ui->tableWidget->setColumnWidth(i,2);
            ui->tableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("")));
            i++;
        }
        if (x<33) {
            QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(x+1));
            ui->tableWidget->setHorizontalHeaderItem(i,newItem);
            i++;
        } else {
            QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(x+1));
            ui->tableWidget->setHorizontalHeaderItem(i,newItem);
            i++;
        }
    }
    ui->tableWidget->setColumnWidth(i,30);
    ui->tableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("E.")));
    i++;
    ui->tableWidget->setColumnWidth(i,30);
    ui->tableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("G.")));
    i++;
    ui->tableWidget->setColumnWidth(i,30);
    ui->tableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("N.")));
    i++;
    ui->tableWidget->setColumnWidth(i,30);
    ui->tableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("F.")));
}

GenomeComparison::~GenomeComparison()
{
    delete ui;
}

bool GenomeComparison::setGenome(QString genomeStr, QColor envColour, QColor genomeColour, QColor nonCodeColour, int fitness){
    int row = genomeList.count();
    ui->tableWidget->insertRow(row);
    genomeList.append(genomeStr);
    fitnessList.append(fitness);

    qDebug() << "Genome: " << genomeStr;

    //---- Add to table

    QTableWidgetItem *newItem = new QTableWidgetItem();
    newItem->setCheckState(Qt::Unchecked);
    newItem->setTextAlignment(Qt:: AlignCenter);
    ui->tableWidget->setItem(row, 0, newItem);

    newItem = new QTableWidgetItem(tr("Species %1").arg(row+1));
    newItem->setTextAlignment(Qt:: AlignCenter);
    ui->tableWidget->setItem(row, 1, newItem);

    int col=2;
    for (int i=0; i<64; i++)
    {
        if (col==34) {
            newItem = new QTableWidgetItem(tr(""));
            newItem->setFlags(Qt::ItemIsEnabled);
            ui->tableWidget->setItem(row, col, newItem);
            ui->tableWidget->item(row, col)->setBackground(QColor(242,242,242));
            col++;
        }

        QString bit = genomeStr.at(i);
        newItem = new QTableWidgetItem(bit);
        newItem->setTextAlignment(Qt:: AlignCenter);
        newItem->setFlags(Qt::ItemIsEnabled);
        ui->tableWidget->setItem(row, col, newItem);
        if(i<32){
            ui->tableWidget->item(row, col)->setForeground(QColor(51, 153, 51));
        } else {
            ui->tableWidget->item(row, col)->setForeground(QColor(51, 51, 153));
        }
        col++;
    }

    //---- Add environment as cell background
    newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    ui->tableWidget->setItem(row, col, newItem);
    ui->tableWidget->item(row, col)->setBackgroundColor(envColour);
    col++;

    //---- Add genome colour as cell background
    newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    ui->tableWidget->setItem(row, col, newItem);
    ui->tableWidget->item(row, col)->setBackgroundColor(genomeColour);
    col++;

    //---- Add non coded colour as cell background
    newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    ui->tableWidget->setItem(row, col, newItem);
    ui->tableWidget->item(row, col)->setBackgroundColor(nonCodeColour);
    col++;

    //---- Add Fitness
    newItem = new QTableWidgetItem();
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    newItem->setData(Qt::DisplayRole, fitness);
    ui->tableWidget->setItem(row, col, newItem);

    return true;
}
