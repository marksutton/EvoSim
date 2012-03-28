#include "analyser.h"
#include <QTextStream>
#include <simmanager.h>
#include <QDebug>
#include <QHash>

Analyser::Analyser()
{
}

void Analyser::AddGenome(quint64 genome, int fitness)
{
    for (int j=0; j<genomes.count(); j++)
    {
        if (genomes[j].genome==genome) {genomes[j].count++; return;}
    }
    genomes.append(sortablegenome(genome, fitness,1));
}

QString Analyser::SortedSummary()
//Pass this a list of genomes pre-grouped
{
    QString line;
    QTextStream sout(&line);

    qSort(genomes.begin(),genomes.end());

    for (int i=0; i<genomes.count(); i++)
    {
        if (genomes[i].count<10) sout<<" "<<genomes[i].count<<": "; else  sout<<genomes[i].count<<": ";
        for (int j=0; j<32; j++)
            if (tweakers64[63-j] & genomes[i].genome) sout<<"1"; else sout<<"0";
        sout<<" ";
        for (int j=32; j<64; j++)
            if (tweakers64[63-j] & genomes[i].genome) sout<<"1"; else sout<<"0";

        sout<<"  fitness: "<<genomes[i].fit;

        sout<<"\n";
    }
    return line;
}
/*
void Analyser::Spread(int position, int group)
{
    //Recursive function
    genomes[position].group=group;
    quint64 mygenome=genomes[position].genome;
    for (int i=0; i<genomes.count(); i++)
    {
        if (genomes[i].group==0)
        {

            quint64 g1x = mygenome ^ genomes[i].genome; //XOR the two to compare
            quint32 g1xl = quint32(g1x & ((quint64)65536*(quint64)65536-(quint64)1)); //lower 32 bits
            int t1 = bitcounts[g1xl/(quint32)65536] +  bitcounts[g1xl & (quint32)65535];
            if (t1<=maxDiff)
            {
                quint32 g1xu = quint32(g1x / ((quint64)65536*(quint64)65536)); //upper 32 bits
                t1+= bitcounts[g1xu/(quint32)65536] +  bitcounts[g1xu & (quint32)65535];
                if (t1<=maxDiff) Spread(i,group);
                    else continue;
            }
            else continue;
        }
    }
    return;
}
*/

int Analyser::Spread(int position, int group)
//return value for next group!
{
//    QList <bool>joingroups; // so if this is group 3, 2 other groups exist
    QList <int> joingroups;

    genomes[position].group=group;
    quint64 mygenome=genomes[position].genome;

    for (int i=0; i<genomes.count(); i++)
    {
        if (joingroups.indexOf(genomes[i].group)==-1)
        {
            quint64 g1x = mygenome ^ genomes[i].genome; //XOR the two to compare
            quint32 g1xl = quint32(g1x & ((quint64)65536*(quint64)65536-(quint64)1)); //lower 32 bits
            int t1 = bitcounts[g1xl/(quint32)65536] +  bitcounts[g1xl & (quint32)65535];
            if (t1<=maxDiff)
            {
                quint32 g1xu = quint32(g1x / ((quint64)65536*(quint64)65536)); //upper 32 bits
                t1+= bitcounts[g1xu/(quint32)65536] +  bitcounts[g1xu & (quint32)65535];
                if (t1<=maxDiff)
                {
                    if (genomes[i].group==0) genomes[i].group=group;
                    else
                    {
                        if (i!=position)  {joingroups.append(genomes[i].group); unusedgroups.append(genomes[i].group);}
                    }
                }
            }
        }
    }

    for (int i=0; i<genomes.count(); i++) if (joingroups.indexOf(genomes[i].group)!=-1) genomes[i].group=group;
    //OK, should now
    //newnum should be lowest group to use next time
    return group+1;
}


QString Analyser::Groups()
//Works out species (hopefully)
//Pass this a list of genomes pre-grouped
{
    if (genomes.count()==0) return "Nothing to analyse";

    qSort(genomes.begin(),genomes.end());
    unusedgroups.clear();
    //start flood-fill style spread algorithm on position 0, for group 1
    int group=1;
    bool done=false;
    while (done==false)
    {
        done=true;
        for (int i=0; i<genomes.count(); i++)
            if (genomes[i].group==0) {group=Spread(i,group); done=false; break;}
    }

    qSort(unusedgroups.begin(), unusedgroups.end());
    unusedgroups.append(-1); //to avoid check failing
    //Unused groups now holds
    QList <int> GroupTranslate;
    int unusedpos=0;
    int realpos=1;
    GroupTranslate.append(-1);
    for (int i=1; i<=group; i++)
    {
        if (unusedgroups[unusedpos]==i)
        {
            GroupTranslate.append(-1);
            unusedpos++;
        }
        else
            GroupTranslate.append(realpos++);
    }


    QString line;
    QTextStream sout(&line);

/*
    for (int i=0; i<genomes.count(); i++)
    {
        if (genomes[i].count<10) sout<<" "<<genomes[i].count<<": "; else  sout<<genomes[i].count<<": ";
        for (int j=0; j<32; j++)
            if (tweakers64[63-j] & genomes[i].genome) sout<<"1"; else sout<<"0";
        sout<<" ";
        for (int j=32; j<64; j++)
            if (tweakers64[63-j] & genomes[i].genome) sout<<"1"; else sout<<"0";

        sout<<"  fitness: "<<genomes[i].fit<< "  Group: "<<GroupTranslate[genomes[i].group];

        sout<<"\n";
    }
    sout<<"\n";

    sout<<"Group count"<<group;
*/

    //OK, now we want to find the modal occurrence for each group - as they are sorted this is FIRST occurence
    QVector <int> modal(group+1);
    for (int i=1; i<=group; i++)
        if (GroupTranslate[i]!=-1)
        for (int j=0; j<genomes.count(); j++)
              if (genomes[j].group==i) {modal[i]=j; break;}


    //For each group work out and output some stats
    for (int i=group-1; i>=1; i--)
    {
        if (GroupTranslate[i]!=-1)
        {
            int minfit=999999, maxfit=-1, maxspread=-1;
            double meanfit=0, meanspread=0;
            int count=0;
            //work out summary stats

            int changes[64];
            for (int j=0; j<64; j++) changes[j]=0;
            quint64 refgenome=genomes[modal[i]].genome;
            for (int j=0; j<genomes.count(); j++)
            {
                if (genomes[j].group==i)
                {
                    count+=genomes[j].count;
                    meanfit+=genomes[j].fit*genomes[j].count;

                    if (genomes[j].fit<minfit) minfit=genomes[j].fit;
                    if (genomes[j].fit>maxfit) maxfit=genomes[j].fit;

                    quint64 g1x=refgenome ^ genomes[j].genome;
                    quint32 g1xl = quint32(g1x & ((quint64)65536*(quint64)65536-(quint64)1)); //lower 32 bits
                    int t1 = bitcounts[g1xl/(quint32)65536] +  bitcounts[g1xl & (quint32)65535];

                    quint32 g1xu = quint32(g1x / ((quint64)65536*(quint64)65536)); //upper 32 bits
                    t1+= bitcounts[g1xu/(quint32)65536] +  bitcounts[g1xu & (quint32)65535];
                    meanspread+=t1*genomes[j].count;
                    if (t1>maxspread) maxspread=t1;

                    for (int k=0; k<64; k++) if (tweakers64[k] & g1x) changes[k]+=genomes[j].count;
                }

            }
            meanfit/=count;
            meanspread/=count;

            if (count<((genomes.count()/100))) continue;

            sout<<"Group: "<<GroupTranslate[i]<<"\nGenome:   ";

            for (int k=0; k<4; k++)
            {
                for (int j=k*16; j<(k+1)*16; j++) if (tweakers64[63-j] & genomes[modal[i]].genome) sout<<"1"; else sout<<"0";
                sout<<" ";
            }
            sout<<" Dec: "<<genomes[modal[i]].genome;
            /*
            sout<<" Slots: ";
            //breed slots used
            quint32 gen2 = genomes[modal[i]].genome>>32;
            quint32 ugenecombo = (gen2>>16) ^ (gen2 & 65535);

            for (int j=0; j<16; j++) if (tweakers[j] & ugenecombo) sout<<"1"; else sout<<"0";
            */
            sout<<"\n";

            sout<<"Changes:  ";

            for (int j=0; j<64; j++)
            {
                if (changes[j]==0) changes[j]=-1;
                else
                {
                    changes[j]*=10;
                    changes[j]/=count;
                }
            }
            for (int k=0; k<4; k++)
            {
                for (int j=k*16; j<(k+1)*16; j++) if (changes[63-j]<0) sout<<" "; else sout<<changes[63-j];
                sout<<" ";
            }
            sout<<"\n";

            sout<<"Total: "<<count<<"  MinFit: "<<minfit<<"  MaxFit: "<<maxfit<<"  MeanFit: "<<meanfit<<"  MaxSpread: "<<maxspread<<"  MeanSpread: "<<meanspread<<"\n\n";
        }
    }

/*
    //Finally work out distances to each other center - and breed time overlaps

    for (int i=1; i<=group; i++)
    for (int j=i+1; j<=group; j++)
    {

        quint64 g1x = genomes[modal[i]].genome ^ genomes[modal[j]].genome; //XOR the two to compare
        quint32 g1xl = quint32(g1x & ((quint64)65536*(quint64)65536-(quint64)1)); //lower 32 bits
        int t1 = bitcounts[g1xl/(quint32)65536] +  bitcounts[g1xl & (quint32)65535];

        quint32 g1xu = quint32(g1x / ((quint64)65536*(quint64)65536)); //upper 32 bits
        t1+= bitcounts[g1xu/(quint32)65536] +  bitcounts[g1xu & (quint32)65535];

        quint32 gen2 = genomes[modal[i]].genome>>32;
        quint32 ugenecombo = (gen2>>16) ^ (gen2 & 65535);
        quint32 gen3=genomes[modal[j]].genome>>32;
        sout<<"Distance between groups "<<i<<"->"<<j<<": "<<t1<<"  Breed time overlaps: "<<bitcounts[ugenecombo & ((gen3>>16) ^ (gen3 & 65535))]<<"\n";
    }
*/


    return line;
}
