#include "critter.h"
#include "simmanager.h"

//undefine to use old genetic distance methods

//#define SLOTS
#define MONEYBACK

Critter::Critter()
{
    genome=0;
    age=0;  //this is the tested flag for dead
    fitness=0;
    energy=0;

}

void Critter::initialise(quint64 gen, quint8 *env, int x, int y, int z)
{
    //Restart a slot - set up properly
    genome=gen;
    //age=START_AGE;
    age=startAge;
    energy=0; //start with 0 energy
    recalc_fitness(env);
    xpos=x; ypos=y; zpos=z;

    quint32 gen2 = genome>>32;
    ugenecombo = (gen2>>16) ^ (gen2 & 65535); //for breed testing - work out in advance for speed
}

void Critter::recalc_fitness(quint8 *env)
{
    quint32 lowergenome=(quint32)(genome & ((quint64)65536*(quint64)65536-(quint64)1));

    quint32 answer=lowergenome ^ xormasks[env[0]][0]; //apply redmask
    quint32 a2=answer/65536;
    answer &= (quint32) 65535;
    int finalanswer=bitcounts[answer];
    finalanswer+=bitcounts[a2];

    answer=lowergenome ^ xormasks[env[1]][1]; //apply greenmask
    a2=answer/65536;
    answer &= (unsigned int) 65535;
    finalanswer+=bitcounts[answer];
    finalanswer+=bitcounts[a2];

    answer=lowergenome ^ xormasks[env[2]][2]; //apply bluemask
    a2=answer/65536;
    answer &= (unsigned int) 65535;
    finalanswer+=bitcounts[answer];
    finalanswer+=bitcounts[a2];

    if (finalanswer>=target+settleTolerance) {fitness=0; age=0; return;} // no use
    if (finalanswer<=target-settleTolerance) {fitness=0; age=0; return;} // no use

    //These next two SHOULD do reverse of the abs of finalanswer (i.e 0=20, 20=0)
    if (finalanswer<target) fitness = settleTolerance - (target - finalanswer);
    else fitness = settleTolerance + target - finalanswer;
}


bool Critter::iterate_parallel(int *KillCount_local, int addfood)
{

    if (age)
    {
        if ((--age)==0)
        {
            (*KillCount_local)++;
            totalfit[xpos][ypos]-=fitness;
            fitness=0;
            if (maxused[xpos][ypos]==zpos)
            {
                for (int n=zpos-1; n>=0; n--)
                if (critters[xpos][ypos][n].age>0)
                {
                     maxused[xpos][ypos]=n;
                     goto past;
                }
                maxused[xpos][ypos]=-1;
    past:   ;
            }
            return false;
        }
        energy +=  fitness * addfood;
        //energy+= (fitness * food) / totalfit[xpos][ypos];]

//non-slot version - try breeding if our energy is high enough
            if (energy>(breedThreshold+breedCost)) {energy-=breedCost; return true;}
            else return false;

    }
    return false;
}

int Critter::breed_with_parallel(int xpos, int ypos, Critter *partner, int *newgenomecount_local)
//newgenomecount_local is position in genome array - arranged so can never overlap
//returns 1 if it fails (so I can count fails)
{
    //do some breeding!


    int t1=0;
    // - determine success.. use genetic similarity
    quint64 cg1x = genome ^ partner->genome; //XOR the two to compare

   //Coding half
    quint32 g1xl = quint32(cg1x & ((quint64)65536*(quint64)65536-(quint64)1)); //lower 32 bits
    t1 = bitcounts[g1xl/(quint32)65536] +  bitcounts[g1xl & (quint32)65535];

    //non-Coding half
    quint32 g1xu = quint32(cg1x / ((quint64)65536*(quint64)65536)); //upper 32 bits
    t1 += bitcounts[g1xu/(quint32)65536] +  bitcounts[g1xu & (quint32)65535];
    if (t1>maxDiff)
    {
        //breeders get their energy back - this is an 'abort'
        energy+=breedCost;
        partner->energy+=breedCost;
        return 1;
    }

     //work out new genome

     quint64 g1x = genex[nextgenex++]; if (nextgenex >= 65536) nextgenex = 0;
     quint64 g2x = ~g1x; // inverse

     g2x &= genome;
     g1x &= partner->genome;  // cross bred genome
     g2x |= g1x;
/*
     if (qrand()<(mutate*128))
     {
            int w=qrand();
            w &=63;
            g2x ^= tweakers64[w];
     }
*/
     //this is technically not threadsafe, but it doesn't matter - any value for nextrand is fine
     if ((TheSimManager->Rand8())<mutate)
     {
            int w=TheSimManager->Rand8();
            w &=63;
            g2x ^= tweakers64[w];
     }

     //store it all

     newgenomes[*newgenomecount_local]=g2x;
     newgenomeX[*newgenomecount_local]=xpos;
     newgenomeY[*newgenomecount_local]=ypos;
     newgenomeDisp[(*newgenomecount_local)++]=dispersal; //how far to disperse - low is actually far (it's a divider - max is 240, <10% are >30
     return 0;
}
