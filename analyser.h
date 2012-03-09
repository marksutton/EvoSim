#ifndef ANALYSER_H
#define ANALYSER_H
#include <QString>
#include <QList>
#include <sortablegenome.h>

class Analyser
{
public:
    Analyser();
    void AddGenome(quint64 genome, int fitness);
    QString SortedSummary();
    QString Groups();

private:
    QList <sortablegenome> genomes;
    QList <int>unusedgroups;
    int Spread(int position, int group);

};

#endif // ANALYSER_H
