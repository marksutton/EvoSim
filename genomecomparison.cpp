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

    //---- Set Auto Comapare
    autoComparison = true;

    //---- Set Column Colours
    first32 = QColor(51, 153, 51);
    last32 = QColor(51, 51, 153);
    spacerCol = QColor(242,242,242);
    highlight = QColor(215,206,152);

    //---- Render Genome Tables
    QFont fnt;
    fnt.setPointSize(8);
    fnt.setFamily("Arial");

    ui->genomeTableWidget->setFont(fnt);
    ui->genomeTableWidget->setColumnWidth(1,100);
    ui->genomeTableWidget->setColumnWidth(34,2);
    renderGenomesTable();

    ui->compareTableWidget->setFont(fnt);
    ui->compareTableWidget->setColumnWidth(1,100);
    ui->compareTableWidget->setColumnWidth(34,2);

    //---- Signal to capture name change
    connect(ui->genomeTableWidget, SIGNAL(cellChanged(int, int)), this, SLOT(updateGenomeName(int, int)));

    //---- Add Button Actions
    buttonActions();
    buttonUpdate();

    //---- Widget Styling
    widgetStyling();
}

/*---------------------------------------------------------------------------//
    Destructor
//---------------------------------------------------------------------------*/

GenomeComparison::~GenomeComparison()
{
    delete ui;
}

/*---------------------------------------------------------------------------//
    Widget Styling
//---------------------------------------------------------------------------*/

void GenomeComparison::widgetStyling()
{
    //---- Auto Button
    this->setStyleSheet("QPushButton {"
                        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #E0E0E0, stop: 1 #FFFFFF);"
                        "border-radius: 5px;"
                        "padding:3px;"
                        "border: 1px solid #4D4D4D;"
                        "}"

                        "QPushButton:hover {"
                        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #FFFFFF, stop: 1 #E0E0E0);"
                        "}"

                        "QPushButton#deleteButton:hover, QPushButton#resetButton:hover {"
                        "color: #FF0000;"
                        "}"

                        "QPushButton:disabled {"
                        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #E0E0E0, stop: 1 #FFFFFF);"
                        "border: 1px solid #BABABA;"
                        "}"

                        "QPushButton#compareButton {"
                        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #FFDD75, stop: 1 #FFF6DB);"
                        "}"

                        "QPushButton#compareButton:hover {"
                        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #FFF6DB, stop: 1 #FFDD75);"
                        "}"

                        "QPushButton#autoButton {"
                        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #FF8A8A, stop: 1 #FFD6D6);"
                        "}"

                        "QPushButton#autoButton:checked {"
                        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #BEDF9F, stop: 1 #E4F2D9);"
                        "}"
                        );

}

/*---------------------------------------------------------------------------//
    Buttons
//---------------------------------------------------------------------------*/

void GenomeComparison::buttonActions()
{
    connect(ui->compareButton, SIGNAL(pressed()), this, SLOT(compareGenomes()));
    connect(ui->autoButton, SIGNAL(toggled(bool)), this, SLOT(setAuto(bool)));
    connect(ui->resetButton, SIGNAL(pressed()), this, SLOT(resetTables()));
    connect(ui->deleteButton, SIGNAL(pressed()), this, SLOT(deleteGenome()));
}

void GenomeComparison::buttonUpdate()
{
    if (autoComparison == true) {
        ui->autoButton->setChecked(true);
    } else {
        ui->autoButton->setChecked(false);
    }

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

bool GenomeComparison::renderGenomesTable(){
    ui->genomeTableWidget->hide();
    ui->genomeTableWidget->clear();
    ui->genomeTableWidget->setRowCount(0);

    int i = 0;
    ui->genomeTableWidget->setHorizontalHeaderItem(i,new QTableWidgetItem(tr("")));
    ui->genomeTableWidget->setColumnWidth(i,20);
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
    if (numGenomes != 0) {
        for(int row=0; row<genomeList.count(); row++){
            //---- Add to table
            if (autoComparison == true && row!=0) {
                //---- Do comparison with last genome...
                QMap<QString,QString> genomeListMapA = genomeList[row-1];
                QMap<QString,QString> genomeListMapB = genomeList[row];
                QString compareMask;

                //---- Create Masks
                for (int i=0; i<64; i++)
                {
                    if (genomeListMapA["genome"].at(i) == genomeListMapB["genome"].at(i)) {
                        //---- Same bit
                        compareMask.append("0");
                    } else {
                        compareMask.append("1");
                    }
                }

                insertRow(
                            row,
                            genomeListMapB["name"],
                            genomeListMapB["genome"],
                            genomeListMapB["envColorR"].toInt(),
                            genomeListMapB["envColorG"].toInt(),
                            genomeListMapB["envColorB"].toInt(),
                            genomeListMapB["genomeColorR"].toInt(),
                            genomeListMapB["genomeColorG"].toInt(),
                            genomeListMapB["genomeColorB"].toInt(),
                            genomeListMapB["nonCodeColorR"].toInt(),
                            genomeListMapB["nonCodeColorG"].toInt(),
                            genomeListMapB["nonCodeColorB"].toInt(),
                            genomeListMapB["fitness"].toInt(),
                            ui->genomeTableWidget,
                            compareMask);
            } else {
                //---- No compare, just add...
                insertRow(
                            row,
                            genomeList[row]["name"],
                            genomeList[row]["genome"],
                            genomeList[row]["envColorR"].toInt(),
                            genomeList[row]["envColorG"].toInt(),
                            genomeList[row]["envColorB"].toInt(),
                            genomeList[row]["genomeColorR"].toInt(),
                            genomeList[row]["genomeColorG"].toInt(),
                            genomeList[row]["genomeColorB"].toInt(),
                            genomeList[row]["nonCodeColorR"].toInt(),
                            genomeList[row]["nonCodeColorG"].toInt(),
                            genomeList[row]["nonCodeColorB"].toInt(),
                            genomeList[row]["fitness"].toInt(),
                            ui->genomeTableWidget);
            }
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
        QTableWidget *table,
        QString comparisonMask)
{
    table->blockSignals(true);
    table->insertRow(row);

    QColor envColour = QColor(environmentR,environmentG,environmentB);
    QColor genomeColour = QColor(genomeR,genomeG,genomeB);
    QColor nonCodeColour = QColor(nonCodeR,nonCodeG,nonCodeB);

    if (table == ui->genomeTableWidget) {
        QTableWidgetItem *newItem = new QTableWidgetItem();
        newItem->setCheckState(Qt::Unchecked);
        newItem->setTextAlignment(Qt:: AlignCenter);
        table->setItem(row, 0, newItem);

        newItem = new QTableWidgetItem(genomeName);
        newItem->setTextAlignment(Qt:: AlignCenter);
        table->setItem(row, 1, newItem);

    } else {
        QString val;
        if (row == 0) { val = "A"; } else { val = "B"; }
        QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(val));
        newItem->setTextAlignment(Qt:: AlignCenter);
        newItem->setFlags(Qt::ItemIsEnabled);
        table->setItem(row, 0, newItem);

        newItem = new QTableWidgetItem(genomeName);
        newItem->setTextAlignment(Qt:: AlignCenter);
        newItem->setFlags(Qt::ItemIsEnabled);
        table->setItem(row, 1, newItem);
    }

    int col=2;
    for (int i=0; i<64; i++)
    {
        if (col==34) {
            QTableWidgetItem *newItem = new QTableWidgetItem(tr(""));
            newItem->setFlags(Qt::ItemIsEnabled);
            table->setItem(row, col, newItem);
            table->item(row, col)->setBackground(QBrush(spacerCol));
            col++;
        }

        QString bit = genomeStr.at(i);
        QTableWidgetItem *newItem = new QTableWidgetItem(bit);
        newItem->setTextAlignment(Qt:: AlignCenter);
        newItem->setFlags(Qt::ItemIsEnabled);

        table->setItem(row, col, newItem);
        if(i<32){
            table->item(row, col)->setForeground(QBrush(first32));
        } else {
            table->item(row, col)->setForeground(QBrush(last32));
        }

        if (comparisonMask.length() !=0 && comparisonMask.at(i) == QChar(49)) {
            //---- There is a mask, formate...
            table->item(row, col)->setBackground(QBrush(highlight));
        }

        col++;
    }

    //---- Add environment as cell background
    QTableWidgetItem *newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    newItem->setToolTip(QString(tr("%1,%2,%3").arg(environmentR).arg(environmentG).arg(environmentB)));
    table->setItem(row, col, newItem);
    table->item(row, col)->setBackgroundColor(envColour);    
    col++;

    //---- Add genome colour as cell background
    newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    newItem->setToolTip(QString(tr("%1,%2,%3").arg(genomeR).arg(genomeG).arg(genomeB)));
    table->setItem(row, col, newItem);
    table->item(row, col)->setBackgroundColor(genomeColour);
    col++;

    //---- Add non coded colour as cell background
    newItem = new QTableWidgetItem(tr(""));
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    newItem->setToolTip(QString(tr("%1,%2,%3").arg(nonCodeR).arg(nonCodeG).arg(nonCodeB)));
    table->setItem(row, col, newItem);
    table->item(row, col)->setBackgroundColor(nonCodeColour);
    col++;

    //---- Add Fitness
    newItem = new QTableWidgetItem();
    newItem->setTextAlignment(Qt:: AlignCenter);
    newItem->setFlags(Qt::ItemIsEnabled);
    newItem->setData(Qt::DisplayRole, fitness);
    table->setItem(row, col, newItem);

    table->blockSignals(false);
}

/*---------------------------------------------------------------------------//
    Table Actions
//---------------------------------------------------------------------------*/

void GenomeComparison::updateGenomeName(int row, int col) {
    if (col == 1) {
        QString newName = ui->genomeTableWidget->item(row, col)->text();
        genomeList[row].insert("name",newName);
    }
}


bool GenomeComparison::addGenomeCritter(Critter critter, quint8 *environment)
{
    int row = genomeList.count();
    quint32 genome;

    //---- Genome
    QString genomeStr;
    for (int j=0; j<64; j++)
        if (tweakers[j] & critter.genome) genomeStr.append("1"); else genomeStr.append("0");

    //---- Genome Colour
    genome = (quint32)(critter.genome & ((quint64)65536*(quint64)65536-(quint64)1));
    quint32 genomeB = bitcounts[genome & 2047] * 23;
    genome /=2048;
    quint32 genomeG = bitcounts[genome & 2047] * 23;
    genome /=2048;
    quint32 genomeR = bitcounts[genome] * 25;

    //---- Non coding Colour
    genome = (quint32)(critter.genome / ((quint64)65536*(quint64)65536));
    quint32 nonCodeB = bitcounts[genome & 2047] * 23;
    genome /=2048;
    quint32 nonCodeG = bitcounts[genome & 2047] * 23;
    genome /=2048;
    quint32 nonCodeR = bitcounts[genome] * 25;

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

    //---- Add to table
    if (autoComparison == true && row!=0) {
        //---- Do comparison with last genome...
        QMap<QString,QString> genomeListMapA = genomeList[row-1];
        QMap<QString,QString> genomeListMapB = genomeList[row];
        QString compareMask;

        //---- Create Masks
        for (int i=0; i<64; i++)
        {
            if (genomeListMapA["genome"].at(i) == genomeListMapB["genome"].at(i)) {
                //---- Same bit
                compareMask.append("0");
            } else {
                compareMask.append("1");
            }
        }

        insertRow(
                    row,
                    genomeListMapB["name"],
                    genomeListMapB["genome"],
                    genomeListMapB["envColorR"].toInt(),
                    genomeListMapB["envColorG"].toInt(),
                    genomeListMapB["envColorB"].toInt(),
                    genomeListMapB["genomeColorR"].toInt(),
                    genomeListMapB["genomeColorG"].toInt(),
                    genomeListMapB["genomeColorB"].toInt(),
                    genomeListMapB["nonCodeColorR"].toInt(),
                    genomeListMapB["nonCodeColorG"].toInt(),
                    genomeListMapB["nonCodeColorB"].toInt(),
                    genomeListMapB["fitness"].toInt(),
                    ui->genomeTableWidget,
                    compareMask);
    } else {
        //---- No compare, just add...
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
                    genomeListMap["nonCodeColorR"].toInt(),
                    genomeListMap["nonCodeColorG"].toInt(),
                    genomeListMap["nonCodeColorB"].toInt(),
                    genomeListMap["fitness"].toInt(),
                    ui->genomeTableWidget);
    }

    //---- Update Button States
    buttonUpdate();

    return true;
}

bool GenomeComparison::renderCompareTable() {
    //---- Reset Table
    ui->compareTableWidget->hide();
    ui->compareTableWidget->clear();
    ui->compareTableWidget->setRowCount(0);
    ui->compareTableWidget->setColumnWidth(0,20);
    ui->compareTableWidget->setColumnWidth(67,30);
    ui->compareTableWidget->setColumnWidth(68,30);
    ui->compareTableWidget->setColumnWidth(69,30);
    ui->compareTableWidget->setColumnWidth(70,30);

    //---- Setup
    QMap<QString,QString> genomeListMapA = compareList[0];
    QMap<QString,QString> genomeListMapB = compareList[1];
    QString compareMask;

    //---- Create Masks
    for (int i=0; i<64; i++)
    {
        if (genomeListMapA["genome"].at(i) == genomeListMapB["genome"].at(i)) {
            //---- Same bit
            compareMask.append("0");
        } else {
            compareMask.append("1");
        }
    }

    //---- Insert Rows
    insertRow(
                0,
                genomeListMapA["name"],
                genomeListMapA["genome"],
                genomeListMapA["envColorR"].toInt(),
                genomeListMapA["envColorG"].toInt(),
                genomeListMapA["envColorB"].toInt(),
                genomeListMapA["genomeColorR"].toInt(),
                genomeListMapA["genomeColorG"].toInt(),
                genomeListMapA["genomeColorB"].toInt(),
                genomeListMapA["nonCodeColorR"].toInt(),
                genomeListMapA["nonCodeColorG"].toInt(),
                genomeListMapA["nonCodeColorB"].toInt(),
                genomeListMapA["fitness"].toInt(),
                ui->compareTableWidget);

    insertRow(
                1,
                genomeListMapB["name"],
                genomeListMapB["genome"],
                genomeListMapB["envColorR"].toInt(),
                genomeListMapB["envColorG"].toInt(),
                genomeListMapB["envColorB"].toInt(),
                genomeListMapB["genomeColorR"].toInt(),
                genomeListMapB["genomeColorG"].toInt(),
                genomeListMapB["genomeColorB"].toInt(),
                genomeListMapB["nonCodeColorR"].toInt(),
                genomeListMapB["nonCodeColorG"].toInt(),
                genomeListMapB["nonCodeColorB"].toInt(),
                genomeListMapB["fitness"].toInt(),
                ui->compareTableWidget,
                compareMask);

    ui->compareTableWidget->show();

    return true;
}


QByteArray GenomeComparison::saveComparison()
{
    QByteArray outData;
    QDataStream out(&outData,QIODevice::WriteOnly);

    //---- Output setup variable states
    out<<autoComparison;

    //---- Output Manual Comparison Table Genome List
    out<<compareList;

    //---- Output Main Comparison Table Genome List
    out<<genomeList;

    return outData;
}

bool GenomeComparison::loadComparison(QByteArray inData)
{
    QDataStream in(&inData,QIODevice::ReadOnly);

    //---- Get setup variable states
    in>>autoComparison;

    //---- Reset All Tables
    resetTables();

    //---- Get Manual Comparison Table Genome List
    compareList.clear();
    in>>compareList;
    if (!compareList.empty()) {
        renderCompareTable();
    }
    //---- Get Main Comparison Table Genome List
    in>>genomeList;
    if (!genomeList.empty()) {
        renderGenomesTable();
    }

    //---- Update Button States
    buttonUpdate();

    return true;
}

/*---------------------------------------------------------------------------//
    Button Actions
//---------------------------------------------------------------------------*/

bool GenomeComparison::compareGenomes()
{
    //---- Are there any checked genomes?
    QList<int> checkedList = isGenomeChecked();
    int numChecked = checkedList.count();

    //---- Check that there two genomes checked
    if (numChecked == 0) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Genome Comparison: Error"));
        msgBox.setText(tr("You need to select 2 genomes from the table in order to compare."));
        msgBox.exec();
        return false;
    } else if (numChecked == 1) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Genome Comparison: Error"));
        msgBox.setText(tr("You need to select 1 more genome from the table to begin comparing."));
        msgBox.exec();
        return false;
    } else if (numChecked > 2) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Genome Comparison: Error"));
        msgBox.setText(tr("You can not select more than 2 genomes to compare."));
        msgBox.exec();
        return false;
    } else {
        //---- Compare...
        compareList.clear();        
        compareList.append(genomeList[checkedList[0]]);        
        compareList.append(genomeList[checkedList[1]]);
        renderCompareTable();
    }
    return true;
}

bool GenomeComparison::resetTables()
{
    genomeList.clear();
    renderGenomesTable();

    compareList.clear();
    renderCompareTable();

    buttonUpdate();
    return true;
}

bool GenomeComparison::deleteGenome()
{
    //---- Are there any checked genomes?
    QList<int> checkedList = isGenomeChecked();
    int numChecked = checkedList.count();

    if (numChecked == 0) {
        //---- Nothing Checked
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Genome Comparison: Error"));
        msgBox.setText(tr("You need to select at least 1 genome from the table to delete."));
        msgBox.exec();
        return false;
    } else {
        //---- Something Checked, Remove selected from genomeList
        for(int i=0;i<numChecked; i++)
        {
            genomeList.removeAt(checkedList[i-i]);
        }
        renderGenomesTable();
        buttonUpdate();
    }
    return true;
}

void GenomeComparison::setAuto(bool toggle)
{
    if (toggle) {
        ui->autoButton->setText(QString("Auto Compare ON"));
    } else {
        ui->autoButton->setText(QString("Auto Compare OFF"));
    }
    if (autoComparison != toggle){
        autoComparison = toggle;

        //---- Reload Genome Table
        renderGenomesTable();
    }
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
