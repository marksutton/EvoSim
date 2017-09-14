#ifndef LOGSPECIES_H
#define LOGSPECIES_H
#include <QList>
#include <QString>
#include "logspeciesdataitem.h"

//used to log all species that appear. species class is for internal simulation purposes. This is for output.
class LogSpecies
{

public:
    LogSpecies();
    ~LogSpecies();

    quint64 ID;
    LogSpecies *parent;
    quint64 time_of_first_appearance;
    quint64 time_of_last_appearance;
    QList<LogSpeciesDataItem *>data_items;
    QList<LogSpecies *>children;
    quint32 maxsize;
    QString newickstring(int childindex, quint64 last_time_base, bool killfluff);
    bool isfluff();
    int maxsize_inc_children();
    QString dump_data(int childindex, quint64 last_time_base, bool killfluff, quint64 parentid=0);
    QString my_data_line(quint64 start, quint64 end, quint64 myid, quint64 pid);
};

#endif // LOGSPECIES_H
