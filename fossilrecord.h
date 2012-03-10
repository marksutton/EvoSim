#ifndef FOSSILRECORD_H
#define FOSSILRECORD_H

#include <QList>
#include <QString>

struct Fossil
{
    quint64 genome;
    quint64 timestamp;
    quint8 env[3]; //environment recorded
};

//Encapsulates a virtual fossil locality - continuous record at some point on the grid
//Record is of genomes and environment
class FossilRecord
{
public:
    FossilRecord(int x, int y, int sparse, QString name);
    void WriteRecord(QString fname);
    void MakeFossil();
    int xpos; //grid pos
    int ypos;
    int sparsity; //quality of the record - iterations between records of fossil/env
    int startage; //iteration count when record started
    QString name;

    QList<Fossil *> fossils;
};

#endif // FOSSILRECORD_H
