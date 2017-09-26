#ifndef LOGSPECIESDATAITEM_H
#define LOGSPECIESDATAITEM_H
#include <QtGlobal>

class LogSpeciesDataItem
{
public:
    LogSpeciesDataItem();
    quint64 generation;
    quint64 sample_genome;
    quint32 size; //number of critters
    quint32 genomic_diversity; //number of genomes
    quint16 cells_occupied; //number of cells found in - 1 (as real range is 1-65536, to fit in 0-65535)
    quint8 geographical_range; //max distance between outliers
    quint8 centroid_range_x; //mean of x positions
    quint8 centroid_range_y; //mean of y positions
    quint16 mean_fitness; //mean of all critter fitnesses, stored as x1000
    quint8 min_env[3]; //min red, green, blue found in
    quint8 max_env[3]; //max red, green, blue found in
    quint8 mean_env[3]; //mean environmental colour found in
};

#endif // LOGSPECIESDATAITEM_H