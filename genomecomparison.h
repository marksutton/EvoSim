#ifndef GENOMECOMPARISON_H
#define GENOMECOMPARISON_H

#include <QWidget>
#include <QTableWidget>

#include "simmanager.h"

namespace Ui {
class GenomeComparison;
}

class GenomeComparison : public QWidget
{
    Q_OBJECT
    
public:
    explicit GenomeComparison(QWidget *parent = 0);
    ~GenomeComparison();
    Ui::GenomeComparison *ui;

    //---- Add to Table
    bool addGenomeCritter(Critter critter, quint8 *environment);

private slots:
    //---- Actions
    bool resetTable();
    bool deleteGenome();
    bool compareGenomes();
    void setAuto(bool toggle);

private:
    //---- Tables
    bool renderTable();
    void insertRow(
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
            QString comparisonMask = QString(""));

    //---- Buttons
    void buttonUpdate();
    void buttonActions();

    //---- Table Functions
    QList<int> isGenomeChecked();

    //---- Vars
    QList< QMap<QString,QString> > genomeList, compareList;
    QColor first32, last32, spacerCol, highlight;
    bool autoComparison;

};

#endif // GENOMECOMPARISON_H
