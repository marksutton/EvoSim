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
    bool resetTable();
    bool deleteGenome();

private:
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
            QTableWidget *table);
    void buttonUpdate();
    void buttonActions();
    QList<int> isGenomeChecked();

    QList< QMap<QString,QString> > genomeList;
    QList< QMap<QString,QString> > compareList;

    QColor first32;
    QColor last32;
    QColor spacerCol;
};

#endif // GENOMECOMPARISON_H
