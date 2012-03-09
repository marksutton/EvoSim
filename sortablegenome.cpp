#include "sortablegenome.h"

sortablegenome::sortablegenome(quint64 gen, int f, int c)
{
    fit=f;
    count=c;
    genome=gen;
    group=0;
 }

bool sortablegenome::operator<(const sortablegenome& rhs) const
{
    if (count>rhs.count) return true; //to reverse the sort
    else return false;
}

bool sortablegenome::operator==(const sortablegenome& rhs) const
{
    if (genome==rhs.genome) return true;
    else return false;
}

