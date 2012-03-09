#ifndef SORTABLEGENOME_H
#define SORTABLEGENOME_H
#include "QtGlobal"

class sortablegenome
{
public:
    sortablegenome(quint64 genome, int f, int c);
    bool operator<(const sortablegenome& rhs) const;
    bool operator==(const sortablegenome& rhs) const;
    int fit;
    quint64 genome;
    int group;
    int count;
};

#endif // SORTABLEGENOME_H
