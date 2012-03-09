#include "genomecomparison.h"
#include "ui_genomecomparison.h"

#include <QDebug>
#include <QMessageBox>

/*---------------------------------------------------------------------------//
    Constructor
//---------------------------------------------------------------------------*/

GenomeComparison::GenomeComparison(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GenomeComparison)
{
    ui->setupUi(this);

    //---- Set Column Colours
    first32 = QColor(51, 153, 51);
    last32 = QColor(51, 51, 153);
    spacerCol = QColor(242,242,242);

    //---- Render Genome Tables
    QFont fnt;
    fnt.setPointSize(8);
    fnt.setFamily("Arial");

    ui->genomeTableWidget->setFont(fnt);
    ui->genomeTableWidget->setColumnWidth(1,100);
    ui->genomeTableWidget->setColumnWidth(34,2);
    renderTable();

    ui->compareTableWidget->setFont(fnt);
    ui->compareTableWidget->setColumnWidth(1,100);
    ui->compareTableWidget->setColumnWidth(34,2);

    //---- Add Button Actions
    buttonActions();
    buttonUpdate();
}

/*---------------------------------------------------------------------------//
    Destructor
//---------------------------------------------------------------------------*/

GenomeComparison::~GenomeComparison()
{
    delete ui;
}

/*---------------------------------------------------------------------------//
    Buttons
//---------------------------------------------------------------------------*/

void GenomeComparison::buttonActions()
{
    connect(ui->resetButton, SIGNAL(pressed()), this, SLOT(resetTable()));
    connect(ui->deleteButton, SIGNAL(pressed()), this, SLOT(deleteGenome()));
}

void GenomeComparison::buttonUpdate()
{
    if (genomeList.count() != 0) {
        //---- Activate Buttons
        ui->compareButton->setEnabled(true);
        ui->resetButton->setEnabled(true);
        ui->deleteButton->setEnabled(true);
    } else {
        ui->compareButton->setEnabled(false);
        ui->resetButton->setEnabled(false);
        ui->deleteButton->setEnabled(false);
    }
}

/*---------------------------------------------------------------------------//
    Tables
//---------------------------------------------------------------------------*/

bool GenomeComparison::renderTable(){
    qDebug() << "Rendering Table....";
    ui->genomeTableWidget->hide();
    ui->genomeTableWidget->clear();
    ui->genomeTableWidget->setRowCount(0);

    int i = 0;
    ui->genomeTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("")));
    i++;
    ui->genomeTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("Name")));
    i++;
    for(int x=0;x<64;x++)
    {
        if (x==32) {
            ui->genomeTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("")));
            i++;
        }
        if (x<33) {
            QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(x+1));
            ui->genomeTableWidget->setHorizontalHeaderItem(i,newItem);
            i++;
        } else {
            QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(x+1));
            ui->genomeTableWidget->setHorizontalHeaderItem(i,newItem);
            i++;
        }
    }
    ui->genomeTableWidget->setColumnWidth(i,30);
    ui->genomeTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("E.")));
    i++;
    ui->genomeTableWidget->setColumnWidth(i,30);
    ui->genomeTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("G.")));
    i++;
    ui->genomeTableWidget->setColumnWidth(i,30);
    ui->genomeTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("N.")));
    i++;
    ui->genomeTableWidget->setColumnWidth(i,30);
    ui->genomeTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("F.")));

    //----- Add rows if genomeList.count() != 0
    int numGenomes = genomeList.count();
    qDebug() << "Num Genomes to add to table = " << numGenomes;
    if (numGenomes != 0) {
        for (int i=0; i<numGenomes; i++)
        {

            QMap<QString,QString> genomeListMap = genomeList.at(i);
            qDebug() << "Add to table at Row(" << i << ")";
            insertRow(
                        i,
                        genomeListMap["name"],
                        genomeListMap["genome"],
                        genomeListMap["envColorR"].toInt(),
                        genomeListMap["envColorG"].toInt(),
                        genomeListMap["envColorB"].toInt(),
                        genomeListMap["genomeColorR"].toInt(),
                        genomeListMap["genomeColorG"].toInt(),
                        genomeListMap["genomeColorB"].toInt(),
                        genomeListMap["nonCodeColor"].toInt(),
                        genomeListMap["nonCodeColor"].toInt(),
                        genomeListMap["nonCodeColor"].toInt(),
                        genomeListMap["fitness"].toInt(),
                        ui->genomeTableWidget);
        }

    }

    ui->genomeTableWidget->show();
    return true;
}

void GenomeComparison::insertRow(
        int row,
        QString genomeName,
        QString genomeStr,
        int environmentR,
        int environmentG,
        int environmentB,
        int genomeR,
        int genomeG,
        int genomeB,
        int nonCodeR,
        int nonCodeG,
        int nonCodeB,
        int fitness,
        QTableWidget *table)
{
    table->insertRow(row);

    QColor envColour = QColor(environmentR,environmentG,environmentB);
    QColor genomeColour = QColor(genomeR,genomeG,genomeB);
    QColor nonCodeColour = QColor(nonCodeR,nonCodeG,nonCodeB);

    QTableWidgetItem *newItem = new QTableWidgetItem();
    newItem->setCheckState(Qt::Unchecked);
    newItem->setTextAlignment(Qt:: AlignCenter);
    table->setItem(row, 0, newItem);

    newItem = new QTableWidgetItem(genomeName);
    newItem->setTextAlignment(Qt:: AlignCenter);
    table->setItem(row, 1, newItem);

    int col=2;
    for (int i=0; i<64; i++)
    {
        if (col==34) {
            newItem = new QTableWidgetItem(tr(""));
            newItem->setFlags(Qt::ItemIsEnabled);
            table->setItem(row, col, newItem);
            table->item(row, col)->setBackground(QBrush(spacerCol));
            col++;
        }

        QString bit = genomeStr.at(i);
        newItem = new QTableWidgetItem(bit);
        newItem->setTextAlignment(Qt:: AlignCenter);
        newItem->setFlags(Qt::ItemIsEnabled);
        table->setItem(row, col, newItem);
        if(i<32){
            table->item(row, col)->setForeground(QBrush(first32));
        } else {
            table->item(row, col)->setForeground(QBrush(last32));
        }
        col++;
    }

    //---- Add environment as cell background
    newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    table->setItem(row, col, newItem);
    table->item(row, col)->setBackgroundColor(envColour);
    col++;

    //---- Add genome colour as cell background
    newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    table->setItem(row, col, newItem);
    table->item(row, col)->setBackgroundColor(genomeColour);
    col++;

    //---- Add non coded colour as cell background
    newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    table->setItem(row, col, newItem);
    table->item(row, col)->setBackgroundColor(nonCodeColour);
    col++;

    //---- Add Fitness
    newItem = new QTableWidgetItem();
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    newItem->setData(Qt::DisplayRole, fitness);
    table->setItem(row, col, newItem);
}

/*---------------------------------------------------------------------------//
    Actions
//---------------------------------------------------------------------------*/

bool GenomeComparison::addGenomeCritter(Critter critter, quint8 *environment)
{
    int row = genomeList.count();

    //---- Genome
    QString genomeStr;
    for (int j=0; j<64; j++)
        if (tweakers[j] & critter.genome) genomeStr.append("1"); else genomeStr.append("0");

    //---- Genome Colour
    quint32 genome = (quint32)(critter.genome & ((quint64)65536*(quint64)65536-(quint64)1));
    quint32 genomeB = bitcounts[genome & 2047] * 23;
    genome /=2048;
    quint32 genomeG = bitcounts[genome & 2047] * 23;
    genome /=2048;
    quint32 genomeR = bitcounts[genome] * 25;

    //---- Non coding Colour
    quint32 nonCode = (quint32)(critter.genome / ((quint64)65536*(quint64)65536));
    quint32 nonCodeB = bitcounts[nonCode & 2047] * 23;
    genome /=2048;
    quint32 nonCodeG = bitcounts[nonCode & 2047] * 23;
    genome /=2048;
    quint32 nonCodeR = bitcounts[nonCode] * 25;

    //---- Fitness
    int fitness = critter.fitness;

    QMap<QString,QString> genomeListMap;
    genomeListMap.insert("name",QString(tr("Genome %1").arg(row+1)));
    genomeListMap.insert("genome",genomeStr);
    genomeListMap.insert("envColorR",QString("%1").arg(environment[0]));
    genomeListMap.insert("envColorG",QString("%1").arg(environment[1]));
    genomeListMap.insert("envColorB",QString("%1").arg(environment[2]));
    genomeListMap.insert("genomeColorR",QString("%1").arg(genomeR));
    genomeListMap.insert("genomeColorG",QString("%1").arg(genomeG));
    genomeListMap.insert("genomeColorB",QString("%1").arg(genomeB));
    genomeListMap.insert("nonCodeColorR",QString("%1").arg(nonCodeR));
    genomeListMap.insert("nonCodeColorG",QString("%1").arg(nonCodeG));
    genomeListMap.insert("nonCodeColorB",QString("%1").arg(nonCodeB));
    genomeListMap.insert("fitness",QString("%1").arg(fitness));
    genomeList.append(genomeListMap);
    qDebug() << "Add Genome Map = " << genomeListMap;

    //---- Add to table
    insertRow(
                row,
                genomeListMap["name"],
                genomeListMap["genome"],
                genomeListMap["envColorR"].toInt(),
                genomeListMap["envColorG"].toInt(),
                genomeListMap["envColorB"].toInt(),
                genomeListMap["genomeColorR"].toInt(),
                genomeListMap["genomeColorG"].toInt(),
                genomeListMap["genomeColorB"].toInt(),
                genomeListMap["nonCodeColor"].toInt(),
                genomeListMap["nonCodeColor"].toInt(),
                genomeListMap["nonCodeColor"].toInt(),
                genomeListMap["fitness"].toInt(),
                ui->genomeTableWidget);

    //---- Update Button States
    buttonUpdate();

    return true;
}

bool GenomeComparison::resetTable()
{
    qDebug() << "Reseting Table...";
    genomeList.clear();
    renderTable();
    buttonUpdate();
    return true;
}

bool GenomeComparison::deleteGenome()
{
    qDebug() << "Delete a Genome...";

    //---- Are there any checked genomes?
    QList<int> checkedList = isGenomeChecked();
    int numChecked = checkedList.count();

    if (numChecked == 0) {
        //---- Nothing Checked
        qDebug() << "Error: no genomes checked!";
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Genome Comparison: Error"));
        msgBox.setText(tr("You need to select at least 1 genome from the table to delete."));
        msgBox.exec();
        return false;
    } else {
        //---- Something Checked
        qDebug() << "Num genomes checked = " << numChecked;

        //---- Remove selected from genomeList
        for(int i=0;i<numChecked; i++)
        {
            qDebug() << "Removing = " << checkedList[i-i];
            genomeList.removeAt(checkedList[i-i]);
        }

        renderTable();
        buttonUpdate();
    }
    return true;
}

/*---------------------------------------------------------------------------//
    Tables Functions
//---------------------------------------------------------------------------*/

QList<int> GenomeComparison::isGenomeChecked()
{
    QList<int> checkedList;

    for (int i=0; i<genomeList.count(); i++)
    {
        if (ui->genomeTableWidget->item(i,0)->checkState() == Qt::Checked) {
            //---- Yes, add to list
            checkedList.append(i);
        }
    }

    return checkedList;
}
