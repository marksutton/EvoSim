#ifndef ANALYSISTOOLS_H
#define ANALYSISTOOLS_H
#include <QString>
#include <QMap>
#include <QList>

#define SCALE 100

class logged_species
{
public:
    logged_species();
    quint64 start;
    quint64 end;
    quint64 parent;
    int maxsize;
    quint64 totalsize;
    int occurrences;
    quint64 lastgenome;
    quint64 genomes[SCALE];
    int sizes[SCALE];
};

class AnalysisTools
{
public:
    AnalysisTools();
    QString GenerateTree(QString filename);
private:
    void MakeListRecursive(QList<quint64> *magiclist, QMap <quint64, logged_species> *species_list, quint64 ID, int insertpos);
    QString ReturnBinary(quint64 genome);
};


#endif // ANALYSISTOOLS_H
