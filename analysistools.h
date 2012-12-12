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
    QList <float> avsizes;
    QList <float> avchanges;
};

class stasis_species
{
public:
    stasis_species();
    quint64 ID;
    qint64 start;
    qint64 end;
    QList <quint64> genomes;
    QList <quint64> genome_sample_times;
    QList <float> resampled_average_genome_changes;
};

class AnalysisTools
{
public:
    AnalysisTools();
    QString GenerateTree(QString filename);
    QString ExtinctOrigin(QString filename);
    QString SpeciesRatesOfChange(QString filename);
    QString Stasis(QString filename, int slot_count, float percentilecut, int qualifyingslotcount);
    QString CountPeaks(int r,int g,int b);

    int find_closest_index(QList <quint64>time_list, float look_for, float slot_width);
private:
    void MakeListRecursive(QList<quint64> *magiclist, QMap <quint64, logged_species> *species_list, quint64 ID, int insertpos);
    QString ReturnBinary(quint64 genome);
};


#endif // ANALYSISTOOLS_H
