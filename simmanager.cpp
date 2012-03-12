#include "simmanager.h"
#include <QDebug>
#include <stdlib.h>
#include <math.h>
#include <QThread>
#include <QImage>
#include <QMessageBox>


quint32 tweakers[32]; // the 32 single bit XOR values (many uses!)
quint64 tweakers64[64]; // the 64 bit version
quint32 bitcounts[65536]; // the bytes representing bit count of each number 0-635535
quint32 xormasks[256][3]; //determine fitness
int xdisp[256][256];
int ydisp[256][256];
quint64 genex[65536];
int nextgenex;

//Settable ints
int gridX = 100;        //Can't be used to define arrays - hence both ATM
int gridY = 100;
int slotsPerSq = 100;
int startAge = 15;
int target = 66;
int settleTolerance = 15;
int dispersal = 15;
int food = 3000;
int breedThreshold = 500;
int breedCost = 500;
int maxDiff = 2;
int mutate = 10;
int envchangerate=100;
int yearsPerIteration=15;
QStringList EnvFiles;
int CurrentEnvFile;
int EnvChangeCounter;
bool EnvChangeForward;

//Globabl data
Critter critters[GRID_X][GRID_Y][SLOTS_PER_GRID_SQUARE]; //main array - static for speed
quint8 environment[GRID_X][GRID_Y][3];  //0 = red, 1 = green, 2 = blue
quint32 totalfit[GRID_X][GRID_Y];
quint64 generation;

//These next to hold the babies... old style arrays for max speed
quint64 newgenomes[GRID_X*GRID_Y*SLOTS_PER_GRID_SQUARE*2];
quint32 newgenomeX[GRID_X*GRID_Y*SLOTS_PER_GRID_SQUARE*2];
quint32 newgenomeY[GRID_X*GRID_Y*SLOTS_PER_GRID_SQUARE*2];
int newgenomeDisp[GRID_X*GRID_Y*SLOTS_PER_GRID_SQUARE*2];
int newgenomecount;
quint8 randoms[65536];
quint16 nextrandom=0;

int breedattempts[GRID_X][GRID_Y]; //for analysis purposes
int breedfails[GRID_X][GRID_Y]; //for analysis purposes
int settles[GRID_X][GRID_Y]; //for analysis purposes
int settlefails[GRID_X][GRID_Y]; //for analysis purposes
int maxused[GRID_X][GRID_Y];
int AliveCount;




QMutex *mutexes[GRID_X][GRID_Y]; //set up array of mutexes


SimManager::SimManager()
{
    //Constructor - set up all the data!
    MakeLookups();
    AliveCount=0;
    ProcessorCount=QThread::idealThreadCount();
    if (ProcessorCount==-1) ProcessorCount=1;
     if (ProcessorCount>256) ProcessorCount=256;  //a sanity check
    //ProcessorCount=1;
     for (int i=0; i<GRID_X; i++)
     for (int j=0; j<GRID_X; j++)
         mutexes[i][j]= new QMutex();

     for (int i=0; i<ProcessorCount; i++)
         FuturesList.append(new QFuture<int>);


     EnvFiles.clear();
     CurrentEnvFile=-1;
     EnvChangeCounter=0;
     EnvChangeForward=true;
}


void SimManager::MakeLookups()
{
        //These are 00000001, 000000010, 0000000100 etc
        tweakers[0]=1;
        for (int n=1; n<32; n++) tweakers[n]=tweakers[n-1]*2;

        tweakers64[0]=1;
        for (int n=1; n<64; n++) tweakers64[n]=tweakers64[n-1]*2;

        //and now the bitcounting...
        // set up lookup 0 to 65535 to enable bits to be counted for each
        for (qint32 n=0; n<65536; n++)
        {
                qint32 count=0;
                for (int m=0; m<16; m++) if ((n & tweakers[m])!=0) ++count;  // count the bits
                bitcounts[n]=count;
        }
        qsrand(RAND_SEED);

        //now set up xor masks for 3 variables - these are used for each of R G and B to work out fitness

        //Start - random bit pattern for each
        //Assumed RAND_MAX is 32767
        if (RAND_MAX!=32767) qDebug()<<"Error - rand max not 32767!";

        xormasks[0][0]=qrand() * qrand() *2;
        xormasks[0][1]=qrand() * qrand() *2;
        xormasks[0][2]=qrand() * qrand() *2;

        for (int n=1; n<256; n++) //for all the others - flip a random bit each time (^ is xor) - will slowly modify from 0 to 255
        {
                xormasks[n][0] = xormasks[n-1][0] ^ tweakers[qrand()/(RAND_MAX/32)];
                xormasks[n][1] = xormasks[n-1][1] ^ tweakers[qrand()/(RAND_MAX/32)];
                xormasks[n][2] = xormasks[n-1][2] ^ tweakers[qrand()/(RAND_MAX/32)];
        }


        //now the randoms - pre_rolled random numbers 0-255
        for (int n=0; n<65536; n++) randoms[n] = (quint8)((qrand() & 255));
        nextrandom=0;

        // gene exchange lookup
        for (int n=0; n<65536; n++)  //random bit combs, averaging every other bit on
        {
                quint64 value = 0;
                for (int m=0; m<64; m++) if (qrand()>(RAND_MAX/2)) value += tweakers64[m];
                genex[n]=value;
        }
        nextgenex=0;

        //dispersal table - lookups for dispersal amount
        //n is the distance to be dispersed - biased locally (the sqrt)
        //m is angle
        for (int n=0; n<256; n++)
        {
                double d=sqrt(65536/(double)(n+1))-16;
                for (int m=0; m<256; m++)
                {
                    xdisp[n][m]=(int)(d * sin((double)(m)/40.5845));
                    ydisp[n][m]=(int)(d * cos((double)(m)/40.5845));
                   //qDebug()<<"pos:"<<n<<m<<"y"<<ydisp[n][m]<<"x"<<xdisp[n][m];
                }
        }

}


void SimManager::loadEnvironmentFromFile()
// Load current envirnonment from file
{
    //Use make qimage from file method

    //qDebug()<<"LoadEnvFiles - CurrentEnvFile is "<<CurrentEnvFile<<" Gen:"<<generation;

    //Load the image
    if (CurrentEnvFile>=EnvFiles.count())
    {
        return;
    }
    QImage LoadImage(EnvFiles[CurrentEnvFile]);

    if (LoadImage.isNull())
    {
        QMessageBox::critical(0,"Error","Fatal - can't open image " + EnvFiles[CurrentEnvFile]);
        exit(1);
    }
    //check size works
    int xsize=LoadImage.width();
    int ysize=LoadImage.height();

    if (xsize<gridX || ysize<gridY) //rescale if necessary - only if too small
        LoadImage = LoadImage.scaled(QSize(gridX,gridY),Qt::IgnoreAspectRatio);

    //turn into environment array
    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        QRgb colour = LoadImage.pixel(i,j);
        environment[i][j][0]=qRed(colour);
        environment[i][j][1]=qGreen(colour);
        environment[i][j][2]=qBlue(colour);
    }
}

bool SimManager::regenerateEnvironment(int emode)
//returns true if finished sim
{
    if (envchangerate==0 || emode==0) return false; //constant environment - either static in menu, or 0 envchangerate

    --EnvChangeCounter;
    //qDebug()<<"RegEnv: "<<EnvChangeCounter<<generation;
    if (EnvChangeCounter<=0)
    //is it time to change?
    {
        if (emode!=3 && EnvChangeForward==false) //should not be going backwards!
            EnvChangeForward=true;
        if (EnvChangeForward)
        {
            CurrentEnvFile++; //next image
            if (CurrentEnvFile>=EnvFiles.count())
            {
                if (emode==1) return true;   //no more files and we are in 'once' mode - stop the sim
                if (emode==2) CurrentEnvFile=0; //loop mode
                if (emode==3)
                {
                    CurrentEnvFile-=2; //bounce mode - back two to undo the extra ++
                    EnvChangeForward=false;
                }
            }
        }
        else //going backwards - must be in emode 3 (bounce)
        {
            CurrentEnvFile--; //next image
            if (CurrentEnvFile<0)
            {
                CurrentEnvFile=1; //bounce mode - one to one again, must have just been 0
                EnvChangeForward=true;
            }
        }
        EnvChangeCounter=envchangerate; //reset counter
        loadEnvironmentFromFile(); //and load it from the file

    }
    return false;
}

quint32 SimManager::Rand32()
{
    //4 lots of RAND8
    quint32 rand1=qrand() & 255;
    quint32 rand2=(qrand() & 255) * 256;
    quint32 rand3=(qrand() & 255) * 256 * 256;
    quint32 rand4=(qrand() & 255) * 256 * 256 * 256;

    return rand1 + rand2 + rand3 + rand4;
}

quint8 SimManager::Rand8()
{
    return randoms[nextrandom++];
}

void SimManager::SetupRun()
{

    //new simpler version - find middle square, try creatures till something lives, duplicate it [slots] times

    //Kill em all
    for (int n=0; n<gridX; n++)
    for (int m=0; m<gridY; m++)
    {
        for (int c=0; c<slotsPerSq; c++) {critters[n][m][c].age=0; critters[n][m][c].fitness=0;}
        totalfit[n][m]=0;
        maxused[n][m]=-1;
    }

    int n=gridX/2, m=gridY/2;

    while (critters[n][m][0].fitness<7) //try till one lives
    {
        critters[n][m][0].initialise((quint64)Rand32()+(quint64)(65536)*(quint64)(65536)*(quint64)Rand32(), environment[n][m], n,m,0);
    }

    totalfit[n][m]=critters[n][m][0].fitness; //may have gone wrong from above

    AliveCount=1;
    quint64 gen=critters[n][m][0].genome;
    for (int c=1; c<slotsPerSq; c++)
    {
        critters[n][m][c].initialise(gen, environment[n][m], n,m,c);

        if (critters[n][m][c].age>0)
        {
            critters[n][m][c].age/=((Rand8()/10)+1);
            critters[n][m][c].age +=10;
            AliveCount++;
            maxused[n][m]=c;
            totalfit[n][m]+=critters[n][m][c].fitness;
        }
    }

    generation=0;

    EnvChangeCounter=envchangerate;
    EnvChangeForward=true;
}

int SimManager::iterate_parallel(int firstx, int lastx, int newgenomecount_local, int *KillCount_local)
//parallel version - takes newgenomes_local as the start point it can write to in main genomes array
//returns number of new genomes
{
    int breedlist[SLOTS_PER_GRID_SQUARE];
    for (int n=firstx; n<=lastx; n++)
    for (int m=0; m<gridY; m++)
    {
        if (totalfit[n][m]) //skip whole square if needbe
        {
            int addfood = 1+(food / totalfit[n][m]);
            int maxv=maxused[n][m];
            Critter *crit = critters[n][m];

            int breedlistentries=0;

            for (int c=0; c<=maxv; c++)
            if (crit[c].iterate_parallel(KillCount_local,addfood)) breedlist[breedlistentries++]=c;

            if (breedlistentries>0)
            {
                quint8 divider=255/(breedlistentries+5);
                for (int c=0; c<breedlistentries; c++)
                {

                    int partner=Rand8()/divider;  //divide needs to be settable

                    if (partner<breedlistentries)
                    {
                        if (crit[breedlist[c]].breed_with_parallel(n,m,&(crit[breedlist[partner]]),&newgenomecount_local))
                            breedfails[n][m]++; //for analysis purposes
                    }
                }
            }
        }
    }
    return newgenomecount_local;
}

int SimManager::settle_parallel(int newgenomecounts_start, int newgenomecounts_end,int *trycount_local, int *settlecount_local, int *birthcounts_local)
{
    for (int n=newgenomecounts_start; n<newgenomecounts_end; n++)
    {
        //first handle dispersal

        quint8 t1=Rand8();
        quint8 t2=Rand8();

        int xpos=(xdisp[t1][t2])/newgenomeDisp[n];
        int ypos=(ydisp[t1][t2])/newgenomeDisp[n];
        xpos+=newgenomeX[n];
        ypos+=newgenomeY[n];

        if (xpos>=0)
        if (xpos<gridX)
        if (ypos>=0)
        if (ypos<gridY)
        {
            mutexes[xpos][ypos]->lock(); //ensure no-one else buggers with this square
            (*trycount_local)++;
            Critter *crit=critters[xpos][ypos];
            //Now put the baby into any free slot here
            for (int m=0; m<slotsPerSq; m++)
            {
                Critter *crit2=&(crit[m]);
                if (crit2->age==0)
                {
                    //place it

                    crit2->initialise(newgenomes[n],environment[xpos][ypos], xpos, ypos,m);
                    if (crit2->age)
                    {
                        int fit=crit2->fitness;
                        totalfit[xpos][ypos]+=fit;
                        (*birthcounts_local)++;
                        if (m>maxused[xpos][ypos]) maxused[xpos][ypos]=m;
                        settles[xpos][ypos]++;
                        (*settlecount_local)++;
                    }
                    else settlefails[xpos][ypos]++;
                    goto done;
                }
            }
        done: //break point out of loop
            mutexes[xpos][ypos]->unlock()
                    ;
        }
    }
    return 0;
}


bool SimManager::iterate(int emode)
{
    generation++;

    //qDebug()<<"Iterate"<<generation;
    if (regenerateEnvironment(emode)==true) return true;
    //New parallelised version

    int newgenomecounts_starts[256]; //allow for up to 256 threads
    int newgenomecounts_ends[256]; //allow for up to 256 threads

    //work out positions in genome array that each thread can write to to guarantee no overlap
    int positionadd=(GRID_X*GRID_Y*SLOTS_PER_GRID_SQUARE*2)/ProcessorCount;
    for (int i=0; i<ProcessorCount; i++)
        newgenomecounts_starts[i]=i*positionadd;

    int KillCounts[256];
    for (int i=0; i<ProcessorCount; i++) KillCounts[i]=0;


    //do the magic! Set up futures objects, call the functions, wait till done, retrieve values

    for (int i=0; i<ProcessorCount; i++)
        *(FuturesList[i]) = QtConcurrent::run(this, &SimManager::iterate_parallel, (i*gridX)/ProcessorCount, (((i+1)*gridX)/ProcessorCount)-1,newgenomecounts_starts[i], &(KillCounts[i]));

    for (int i=0; i<ProcessorCount; i++)
        FuturesList[i]->waitForFinished();

    for (int i=0; i<ProcessorCount; i++)
         newgenomecounts_ends[i]=FuturesList[i]->result();

    //Testbed - call parallel functions, but in series
/*    for (int i=0; i<ProcessorCount; i++)
        newgenomecounts_ends[i]=SimManager::iterate_parallel((i*gridX)/ProcessorCount, (((i+1)*gridX)/ProcessorCount)-1,newgenomecounts_starts[i], &(KillCounts[i]));
*/

    //apply all the kills to the global count
    for (int i=0; i<ProcessorCount; i++)
            AliveCount-=KillCounts[i];


    //Now handle spat settling

    int trycount=0;
    int settlecount=0;

    int trycounts[256]; for (int i=0; i<ProcessorCount; i++) trycounts[i]=0;
    int settlecounts[256]; for (int i=0; i<ProcessorCount; i++) settlecounts[i]=0;
    int birthcounts[256]; for (int i=0; i<ProcessorCount; i++) birthcounts[i]=0;

    //call the parallel settling function - in series for now
/*    for (int i=0; i<ProcessorCount; i++)
        settle_parallel(newgenomecounts_starts[i],newgenomecounts_ends[i],&(trycounts[i]), &(settlecounts[i]), &(birthcounts[i]));
*/

    //Parallel version of settle functions
    for (int i=0; i<ProcessorCount; i++)
        *(FuturesList[i]) = QtConcurrent::run(this, &SimManager::settle_parallel, newgenomecounts_starts[i],newgenomecounts_ends[i],&(trycounts[i]), &(settlecounts[i]), &(birthcounts[i]));

    for (int i=0; i<ProcessorCount; i++)
        FuturesList[i]->waitForFinished();

    //sort out all the counts
    for (int i=0; i<ProcessorCount; i++)
    {
         AliveCount+=birthcounts[i];
         trycount+=trycounts[i];
         settlecount+=settlecounts[i];
    }

    return false;
}

void SimManager::testcode()
//Use for any test with debugger, triggers from menu item
{
    qDebug()<<"Test code";

}
