#ifndef CRITTER_H
#define CRITTER_H

#include <QtGlobal>

class Critter
{
public:
    Critter();
    void initialise(quint64 gen, quint8 *env, int x, int y, int z);
    int recalc_fitness(quint8 *env);
    int breed_with_parallel(int xpos, int ypos, Critter *partner, int *newgenomecount_local);
    bool iterate_parallel(int *KillCount_local, int addfood);

    int xpos, ypos, zpos;

    quint64 genome;
    quint32 ugenecombo;
    int age; //start off positive - 0 is dead - reduces each time
    int fitness;
    int energy; //breeding energy
};


#endif // CRITTER_H
