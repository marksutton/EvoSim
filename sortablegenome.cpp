/**
 * @file
 * Sortable Genome
 *
 * All REvoSim code is released under the GNU General Public License.
 * See LICENSE.md files in the programme directory.
 *
 * All REvoSim code is Copyright 2018 by Mark Sutton, Russell Garwood,
 * and Alan R.T. Spencer.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version. This program is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY.
 */

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

