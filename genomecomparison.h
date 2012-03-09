#ifndef GENOMECOMPARISON_H
#define GENOMECOMPARISON_H

#include <QWidget>
#include <QRgb>

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

    //---- Setters
    bool setGenome(QString genomeStr, QColor envColour, QColor genomeColour, QColor nonCodeColour, int fitness);


private:
    QList<QString> genomeList;
    QList<int> fitnessList;
};

#endif // GENOMECOMPARISON_H
