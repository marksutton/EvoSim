#include <QTextStream>
#include <QDebug>
#include <QStringList>
#include <QDataStream>
#include <QMap>
#include <QFile>
#include <QTime>
#include <QList>
#include <QApplication>
#include "simmanager.h"
#include "analysistools.h"
#include "mainwindow.h"


logged_species::logged_species()
{
    lastgenome=0;
    for (int i=0; i<SCALE; i++) sizes[i]=0; //means no data
}

/* Text-generating tools to analyse files */

AnalysisTools::AnalysisTools()
{
    return;
}

QString AnalysisTools::SpeciesRatesOfChange(QString filename)
{
    //Modified version of tree maker
    QMap <quint64, logged_species> species_list;  //main list of species list, filed by Species ID

    QFile f(filename);
    if (f.open(QIODevice::ReadOnly))
    {
     //file opened successfully

     quint64 lasttime=0;

     //Need finish time to get scaling recorded correctly

     //read last line to find last time
        qint64 size=f.size(); //get file length
        f.seek(size-500); // start 500 from end
        QTextStream in1(&f);
        QString s1;
        s1=in1.readLine(); //header line
        while (!(s1.isNull()))
        {
             QStringList split_up;
             split_up = s1.split(',');
             lasttime=(quint64) (split_up[0].toULongLong());
             s1=in1.readLine();
        }

        //OK, lasttime should be correct
        float timescale = (float)lasttime/(float)SCALE;  //scale factor for working out the timeslice for diagram


    f.seek(0); //reset to start of file
    QTextStream in(&f);
    QString s;
    s=in.readLine(); //header
    s=in.readLine(); //first data line

    quint64 count=0;
     while (!(s.isNull())) //reads from first to last - which will be in date order
     {
         QStringList split_up;
         split_up = s.split(',');
         //0 Time,1 Species_ID,2 Species_origin_time,3 Species_parent_ID,4 Species_current_size,5 Species_current_genome
         quint64 species_ID = (quint64) (split_up[1].toULongLong());

         //work out slot in 0-(SCALE-1)
         int xpos=(int)(((float)(split_up[0].toInt()))/timescale);  if (xpos>(SCALE-1)) xpos=SCALE-1;


         if (species_list.contains(species_ID)) //if ID species already recorded, update details
         {
             species_list[species_ID].end=(split_up[0].toULongLong());
             int ssize = split_up[4].toInt();
             species_list[species_ID].sizes[xpos]=ssize; //record last size in each slot, will be fine
             if (species_list[species_ID].maxsize<ssize) species_list[species_ID].maxsize=ssize;
             species_list[species_ID].totalsize+=(quint64) (split_up[4].toULongLong());
             species_list[species_ID].occurrences++;
             species_list[species_ID].genomes[xpos]=species_list[species_ID].lastgenome=(quint64) (split_up[5].toULongLong());  //record all genomes - as yet do nothing with them except last
         }
         else  //not yet recorded
         {
             logged_species spe;
             spe.start=(quint64) (split_up[2].toULongLong());
             species_list[species_ID].sizes[xpos]=spe.end=(quint64) (split_up[0].toULongLong());
             spe.parent=(quint64) (split_up[3].toULongLong());
             spe.maxsize=split_up[4].toInt();
             spe.totalsize=(quint64) (split_up[4].toULongLong());
             spe.occurrences=1;
             species_list[species_ID].genomes[xpos]=spe.lastgenome=(quint64) (split_up[5].toULongLong());
             species_list.insert(species_ID,spe);
         }

         count++;
         if (count%1000==0)
         //do some display in status bar every 1000 iterations to show user that algorithm didn't die
         {
             quint64 thistime=(quint64) (split_up[0].toULongLong()); //record latest timestamp
             QString outstring;
             QTextStream out(&outstring);
             out<<"Read to iteration "<<thistime<<" ("<<((thistime*100)/lasttime)<<"%)";
             MainWin->setStatusBarText(outstring);
             qApp->processEvents();
         }
         s=in.readLine(); //next line
     }
     f.close();

    //Write full species data out to report window
     QString OutputString;
     QTextStream out(&OutputString);
     out<<endl<<"============================================================="<<endl;
     out<<endl<<"Full species data "<<endl;
     out<<endl<<"============================================================="<<endl;

     QMutableMapIterator<quint64, logged_species> i(species_list);  //doesn't need to be mutable here, but reused later
      while (i.hasNext())
      {
          i.next();
          quint64 ID=i.key();
          logged_species spe = i.value();
          int pval;
          if (spe.end!=spe.start) pval=(100*((spe.end-spe.start)-spe.occurrences))/(spe.end-spe.start);
          else pval=100;
          out << "Species: "<<ID << ": " << spe.start << "-"<<spe.end<<" Parent "<<spe.parent<<"  maxsize "<<spe.maxsize<<"  Av size "<<(spe.totalsize/spe.occurrences)<< "  %missing "<<100-pval<< endl;
      }

      //Now cull  extinct species without issue

      count=0;
      int speccount=species_list.count();

      QSet <quint64> parents; //make a parents set - timesaving
      i.toFront();
      while (i.hasNext())
      {
         i.next();
         parents.insert(i.value().parent);
      }

      i.toFront();
     while (i.hasNext())
     {
         i.next();
         bool issue=false; if (parents.contains(i.key())) issue=true;  //if it is in parents list it should survive cull
         //does it have issue?

         if (i.value().end != lasttime  && issue==false)  //used to also have a term here to exlude short-lived species: && (i.value().end - i.value().start) < 400
            i.remove();

         count++;
         if (count%100==0)
         //reporting to status bar
         {
             QString outstring;
             QTextStream out(&outstring);
             out<<"Doing cull: done "<<count<<" species of "<<speccount;
             MainWin->setStatusBarText(outstring);
             qApp->processEvents();
         }
     }

     //Output it
     out<<endl<<"============================================================="<<endl;
     out<<endl<<"Culled data (extinct species with no issue removed)"<<endl;
     out<<endl<<"============================================================="<<endl;
    i.toFront();
     while (i.hasNext())
     {
         i.next();
         quint64 ID=i.key();
         logged_species spe = i.value();
         int pval;
         if (spe.end!=spe.start) pval=(100*((spe.end-spe.start)-spe.occurrences))/(spe.end-spe.start);
         else pval=100;
         out << "Species: "<<ID << ": " << spe.start << "-"<<spe.end<<" Parent "<<spe.parent<<"  maxsize "<<spe.maxsize<<"  Av size "<<(spe.totalsize/spe.occurrences)<< "  %missing "<<100-pval<<endl;
     }

     //Tree version reordered here, I just create magiclist as a copy of culled list
     QList<quint64> magiclist;

     i.toFront();
      while (i.hasNext())
      {
          i.next();
          quint64 ID=i.key();
          magiclist.append(ID);
      }

     //output the tree
     out<<endl<<"============================================================="<<endl;
     out<<endl<<"Species with change metrics (as csv) "<<endl;
     out<<endl<<"============================================================="<<endl;

     //csv headers
     out<<endl<<"ID,cum_change,end_to_end_change,steps,";

     for (int k=0; k<20; k++)
         out<<"size"<<k<<",";

     for (int k=0; k<19; k++)
         out<<"change"<<k<<",";

     out<<"change19"<<endl;

     //work out averages etc
     for (int j=0; j<magiclist.count(); j++)
     {

             //work out average genome change

             int change=0;
             int steps=0;
             quint64 firstgenome=0;
             bool flag=false;
             int start=-1;
             int tonextav=-1;
             int act_av_count=0;
             int local_tot_size=0;
             int local_tot_change=0;

             if (magiclist[j]==1969)
                 qDebug()<<"Here";

             for (int k=1; k<SCALE; k++)
             {
                if (start!=-1) tonextav--;
                if (species_list[magiclist[j]].sizes[k]) //if this exists
                 {
                     if (flag==false)
                     {
                         firstgenome=species_list[magiclist[j]].genomes[k];
                         flag=true;
                     }
                     if (species_list[magiclist[j]].sizes[k-1]) //if previous exists
                    {   //have a this and last to compare

                         if (start==-1) {start=k; tonextav=5; local_tot_size=0; local_tot_change=0; act_av_count=0;}
                         if (start!=-1 && tonextav<=0)
                         {
                            tonextav=5;
                            species_list[magiclist[j]].avsizes.append((float)local_tot_size/(float)act_av_count);
                            species_list[magiclist[j]].avchanges.append((float)local_tot_change/(float)act_av_count);
                            local_tot_size=0; local_tot_change=0; act_av_count=0;tonextav=5;
                         }
                         quint64 thisgenome=species_list[magiclist[j]].genomes[k];
                         quint64 lastgenome=species_list[magiclist[j]].genomes[k-1];
                         //work out difference

                         quint64 cg1x = thisgenome ^ lastgenome; //XOR the two to compare

                        //Coding half
                         quint32 g1xl = quint32(cg1x & ((quint64)65536*(quint64)65536-(quint64)1)); //lower 32 bits
                         int t1 = bitcounts[g1xl/(quint32)65536] +  bitcounts[g1xl & (quint32)65535];

                         //non-Coding half
                         quint32 g1xu = quint32(cg1x / ((quint64)65536*(quint64)65536)); //upper 32 bits
                         t1 += bitcounts[g1xu/(quint32)65536] +  bitcounts[g1xu & (quint32)65535];

                         //if (t1>0) qDebug()<<"T1 not 0!"<<t1;
                         steps++;
                         change+=t1;
                         local_tot_size+=species_list[magiclist[j]].sizes[k];
                         local_tot_change+=t1;
                         act_av_count++;
                    }
                 }

             }

             quint64 cg1x = species_list[magiclist[j]].lastgenome ^ firstgenome; //XOR the two to compare

            //Coding half
             quint32 g1xl = quint32(cg1x & ((quint64)65536*(quint64)65536-(quint64)1)); //lower 32 bits
             int t1 = bitcounts[g1xl/(quint32)65536] +  bitcounts[g1xl & (quint32)65535];

             //non-Coding half
             quint32 g1xu = quint32(cg1x / ((quint64)65536*(quint64)65536)); //upper 32 bits
             t1 += bitcounts[g1xu/(quint32)65536] +  bitcounts[g1xu & (quint32)65535];

             QString changestring1="'NA'";
             if (steps>0) changestring1.sprintf("%0.5f",((float)t1)/((float)(steps+1)));

             QString changestring="'NA'";
             if (steps>0)  changestring.sprintf("%0.5f",((float)change)/((float)steps));
             out<<magiclist[j]<<","<<changestring<<","<<changestring1<<","<<steps<<",";
            // qDebug()<<magiclist[j]<<","<<changestring<<","<<changestring1<<","<<steps,<",";
             int countsizes=species_list[magiclist[j]].avsizes.count();

             for (int k=0; k<20; k++)
             {
                 QString outstringtemp;

                 if (k>=countsizes) out<<"0,";
                 else
                 {
                     outstringtemp.sprintf("%0.5f",species_list[magiclist[j]].avsizes[k]);
                     out<<outstringtemp<<",";
                 }
              }
             for (int k=0; k<20; k++)
             {
                 QString outstringtemp, commastring;
                 if (k==19) commastring=""; else commastring=",";

                 if (k>=countsizes) out<<"0"<<commastring;
                 else
                 {
                     outstringtemp.sprintf("%0.5f",species_list[magiclist[j]].avchanges[k]);
                     out<<outstringtemp<<commastring;
                 }
              }


             out<<endl;

     }

     MainWin->setStatusBarText("Done");
     qApp->processEvents();

      return OutputString;
    }
    else
    {
        return "Can't open file";
    }

}

QString AnalysisTools::ExtinctOrigin(QString filename)
{
    QMap <quint64, logged_species> species_list;  //main list of species list, filed by Species ID

    QFile f(filename);
    if (f.open(QIODevice::ReadOnly))
    {
        quint64 lasttime=0;

        //Need finish time to get scaling recorded correctly

        //read last line to find last time
           qint64 size=f.size(); //get file length
           f.seek(size-500); // start 500 from end
           QTextStream in1(&f);
           QString s1;
           s1=in1.readLine(); //header line
           while (!(s1.isNull()))
           {
                QStringList split_up;
                split_up = s1.split(',');
                lasttime=(quint64) (split_up[0].toULongLong());
                s1=in1.readLine();
           }

           //OK, lasttime should be correct
           float timescale = (float)lasttime/(float)SCALE;  //scale factor for working out the timeslice for diagram


       f.seek(0); //reset to start of file
       QTextStream in(&f);
       QString s;
       s=in.readLine(); //header
       s=in.readLine(); //first data line

       quint64 count=0;
        while (!(s.isNull())) //reads from first to last - which will be in date order
        {
            QStringList split_up;
            split_up = s.split(',');
            //0 Time,1 Species_ID,2 Species_origin_time,3 Species_parent_ID,4 Species_current_size,5 Species_current_genome
            quint64 species_ID = (quint64) (split_up[1].toULongLong());

            //work out slot in 0-(SCALE-1)
            int xpos=(int)(((float)(split_up[0].toInt()))/timescale);  if (xpos>(SCALE-1)) xpos=SCALE-1;


            if (species_list.contains(species_ID)) //if ID species already recorded, update details
            {
                species_list[species_ID].end=(split_up[0].toULongLong());
                int ssize = split_up[4].toInt();
                species_list[species_ID].sizes[xpos]=ssize; //record last size in each slot, will be fine
                if (species_list[species_ID].maxsize<ssize) species_list[species_ID].maxsize=ssize;
                species_list[species_ID].totalsize+=(quint64) (split_up[4].toULongLong());
                species_list[species_ID].occurrences++;
                species_list[species_ID].genomes[xpos]=species_list[species_ID].lastgenome=(quint64) (split_up[5].toULongLong());  //record all genomes - as yet do nothing with them except last
            }
            else  //not yet recorded
            {
                logged_species spe;
                spe.start=(quint64) (split_up[2].toULongLong());
                species_list[species_ID].sizes[xpos]=spe.end=(quint64) (split_up[0].toULongLong());
                spe.parent=(quint64) (split_up[3].toULongLong());
                spe.maxsize=split_up[4].toInt();
                spe.totalsize=(quint64) (split_up[4].toULongLong());
                spe.occurrences=1;
                species_list[species_ID].genomes[xpos]=spe.lastgenome=(quint64) (split_up[5].toULongLong());
                species_list.insert(species_ID,spe);
            }

            count++;
            if (count%1000==0)
            //do some display in status bar every 1000 iterations to show user that algorithm didn't die
            {
                quint64 thistime=(quint64) (split_up[0].toULongLong()); //record latest timestamp
                QString outstring;
                QTextStream out(&outstring);
                out<<"Read to iteration "<<thistime<<" ("<<((thistime*100)/lasttime)<<"%)";
                MainWin->setStatusBarText(outstring);
                qApp->processEvents();
            }
            s=in.readLine(); //next line
        }
        f.close();

        //now work out origins etc

        QList<int> origincounts;
        QList<int> extinctcounts;
        QList<int> alivecounts;
        QList<int> alive_big_counts;

        for (int i=0; i<SCALE; i++)
        {
            int count=0;
            int ecount=0;
            int acount=0;
            int bcount=0;
            foreach (logged_species spec, species_list)
            {
                int xpos=(int)(((float)(spec.start))/timescale);
                if (xpos>(SCALE-1)) xpos=SCALE-1;
                if (xpos==i) count++;

                int xpos2=(int)(((float)(spec.end))/timescale);
                if (xpos2>(SCALE-1)) xpos2=SCALE-1;
                if (xpos2==i) ecount++;

                if (xpos<=i && xpos2>=i) acount++;
                if (xpos<=i && xpos2>=i && spec.maxsize>20) bcount++;
            }
            extinctcounts.append(ecount);
            origincounts.append(count);
            alivecounts.append(acount);
            alive_big_counts.append(bcount);
        }

        QString outstring;
        QTextStream out(&outstring);

        out<<"Extinctions:"<<endl;
        //output
        for (int i=0; i<SCALE; i++)
        {
            out<<extinctcounts[i]<<endl;
        }
        out<<"Originations:"<<endl;
        //output
        for (int i=0; i<SCALE; i++)
        {
            out<<origincounts[i]<<endl;
        }
        out<<"Species Count:"<<endl;
        //output
        for (int i=0; i<SCALE; i++)
        {
            out<<alivecounts[i]<<endl;
        }
        out<<"Big (>20) Species Count:"<<endl;
        //output
        for (int i=0; i<SCALE; i++)
        {
            out<<alive_big_counts[i]<<endl;
        }

        return outstring;
    }
    else return "Could not open file";
}

QString AnalysisTools::GenerateTree(QString filename)
//Takes a log file generated by the species logging system
//Generates output to the report dock
//Currently assumes start at time 0, with species 1
//1. Generates list of all species to have lived, together with time-range, parent, etc
//2. Culls this list to exclude all species than went extinct without issue
//3. Generates a phylogram showing ranges of species and their parent
{
    QMap <quint64, logged_species> species_list;  //main list of species list, filed by Species ID

    QFile f(filename);
    if (f.open(QIODevice::ReadOnly))
    {
     //file opened successfully

     quint64 lasttime=0;

     //Need finish time to get scaling recorded correctly

     //read last line to find last time
        qint64 size=f.size(); //get file length
        f.seek(size-500); // start 500 from end
        QTextStream in1(&f);
        QString s1;
        s1=in1.readLine(); //header line
        while (!(s1.isNull()))
        {
             QStringList split_up;
             split_up = s1.split(',');
             lasttime=(quint64) (split_up[0].toULongLong());
             s1=in1.readLine();
        }

        //OK, lasttime should be correct
        float timescale = (float)lasttime/(float)SCALE;  //scale factor for working out the timeslice for diagram


    f.seek(0); //reset to start of file
    QTextStream in(&f);
    QString s;
    s=in.readLine(); //header
    s=in.readLine(); //first data line

    quint64 count=0;
     while (!(s.isNull())) //reads from first to last - which will be in date order
     {
         QStringList split_up;
         split_up = s.split(',');
         //0 Time,1 Species_ID,2 Species_origin_time,3 Species_parent_ID,4 Species_current_size,5 Species_current_genome
         quint64 species_ID = (quint64) (split_up[1].toULongLong());

         //work out slot in 0-(SCALE-1)
         int xpos=(int)(((float)(split_up[0].toInt()))/timescale);  if (xpos>(SCALE-1)) xpos=SCALE-1;


         if (species_list.contains(species_ID)) //if ID species already recorded, update details
         {
             species_list[species_ID].end=(split_up[0].toULongLong());
             int ssize = split_up[4].toInt();
             species_list[species_ID].sizes[xpos]=ssize; //record last size in each slot, will be fine
             if (species_list[species_ID].maxsize<ssize) species_list[species_ID].maxsize=ssize;
             species_list[species_ID].totalsize+=(quint64) (split_up[4].toULongLong());
             species_list[species_ID].occurrences++;
             species_list[species_ID].genomes[xpos]=species_list[species_ID].lastgenome=(quint64) (split_up[5].toULongLong());  //record all genomes - as yet do nothing with them except last
         }
         else  //not yet recorded
         {
             logged_species spe;
             spe.start=(quint64) (split_up[2].toULongLong());
             species_list[species_ID].sizes[xpos]=spe.end=(quint64) (split_up[0].toULongLong());
             spe.parent=(quint64) (split_up[3].toULongLong());
             spe.maxsize=split_up[4].toInt();
             spe.totalsize=(quint64) (split_up[4].toULongLong());
             spe.occurrences=1;
             species_list[species_ID].genomes[xpos]=spe.lastgenome=(quint64) (split_up[5].toULongLong());
             species_list.insert(species_ID,spe);
         }

         count++;
         if (count%1000==0)
         //do some display in status bar every 1000 iterations to show user that algorithm didn't die
         {
             quint64 thistime=(quint64) (split_up[0].toULongLong()); //record latest timestamp
             QString outstring;
             QTextStream out(&outstring);
             out<<"Read to iteration "<<thistime<<" ("<<((thistime*100)/lasttime)<<"%)";
             MainWin->setStatusBarText(outstring);
             qApp->processEvents();
         }
         s=in.readLine(); //next line
     }
     f.close();

    //Write full species data out to report window
     QString OutputString;
     QTextStream out(&OutputString);
     out<<endl<<"============================================================="<<endl;
     out<<endl<<"Full species data "<<endl;
     out<<endl<<"============================================================="<<endl;

     QMutableMapIterator<quint64, logged_species> i(species_list);  //doesn't need to be mutable here, but reused later
      while (i.hasNext())
      {
          i.next();
          quint64 ID=i.key();
          logged_species spe = i.value();
          int pval;
          if (spe.end!=spe.start) pval=(100*((spe.end-spe.start)-spe.occurrences))/(spe.end-spe.start);
          else pval=100;
          out << "Species: "<<ID << ": " << spe.start << "-"<<spe.end<<" Parent "<<spe.parent<<"  maxsize "<<spe.maxsize<<"  Av size "<<(spe.totalsize/spe.occurrences)<< "  %missing "<<100-pval<< endl;
      }

      //Now cull  extinct species without issue

      count=0;
      int speccount=species_list.count();

      QSet <quint64> parents; //make a parents set - timesaving
      i.toFront();
      while (i.hasNext())
      {
         i.next();
         parents.insert(i.value().parent);
      }

      i.toFront();
     while (i.hasNext())
     {
         i.next();
         bool issue=false; if (parents.contains(i.key())) issue=true;  //if it is in parents list it should survive cull
         //does it have issue?

         if (i.value().end != lasttime  && issue==false)  //used to also have a term here to exlude short-lived species: && (i.value().end - i.value().start) < 400
            i.remove();

         count++;
         if (count%100==0)
         //reporting to status bar
         {
             QString outstring;
             QTextStream out(&outstring);
             out<<"Doing cull: done "<<count<<" species of "<<speccount;
             MainWin->setStatusBarText(outstring);
             qApp->processEvents();
         }
     }


     //Output it
     out<<endl<<"============================================================="<<endl;
     out<<endl<<"Culled data (extinct species with no issue removed)"<<endl;
     out<<endl<<"============================================================="<<endl;
    i.toFront();
     while (i.hasNext())
     {
         i.next();
         quint64 ID=i.key();
         logged_species spe = i.value();
         int pval;
         if (spe.end!=spe.start) pval=(100*((spe.end-spe.start)-spe.occurrences))/(spe.end-spe.start);
         else pval=100;
         out << "Species: "<<ID << ": " << spe.start << "-"<<spe.end<<" Parent "<<spe.parent<<"  maxsize "<<spe.maxsize<<"  Av size "<<(spe.totalsize/spe.occurrences)<< "  %missing "<<100-pval<<endl;
     }

     //now do my reordering magic - magiclist ends up as a list of species IDs in culled list, but in a sensible order for tree output
     QList<quint64> magiclist;

     MainWin->setStatusBarText("Starting list reordering");
     qApp->processEvents();

     MakeListRecursive(&magiclist,&species_list, 1, 0); //Recursively sorts culled species_list into magiclist. args are list pointer, ID

     //Display code for this list, now removed
     /*
     out<<endl<<"============================================================="<<endl;
     out<<endl<<"Sorted by parents"<<endl;
     out<<endl<<"============================================================="<<endl;

     foreach (quint64 ID, magiclist)
     {
         out << "ID: "<<ID<<" Parent "<<species_list[ID].parent<<" Genome: "<<species_list[ID].lastgenome<<endl;
     }

     */

     //output the tree
     out<<endl<<"============================================================="<<endl;
     out<<endl<<"Tree"<<endl;
     out<<endl<<"============================================================="<<endl;


     MainWin->setStatusBarText("Calculating Tree");
     qApp->processEvents();

     /*
        grid for output - holds output codes for each point in grid, which is SCALE wide and species_list.count()*2 high (includes blank lines, one per species).
        Codes are:  0: space  1: -  2: +  3: |  4: /  5: \
     */
     QVector <QVector <int> > output_grid;

     QVector <int> blankline; //useful later
     for (int j=0; j<SCALE; j++)
         blankline.append(0);

         //put in the horizontals
     foreach (quint64 ID, magiclist)
     {
         //do horizontal lines
         QVector <int> line;
         for (int j=0; j<SCALE; j++)  //+1 in case of rounding cockups
         {
             //work out what start and end time this means
             quint64 starttime=(quint64)((float)j*timescale);
             quint64 endtime=(quint64)((float)(j+1)*timescale);

             if (species_list[ID].start<=endtime && species_list[ID].end>=starttime)
             {
                 //do by size
                 line.append(1);
             }
             else line.append(0);
         }

         output_grid.append(line);
         //append a blank line
         output_grid.append(blankline);
     }

     //now the connectors, i.e. the verticals
     int myline=0;
     foreach (quint64 ID, magiclist)
     {
         //find parent
         int parent=species_list[ID].parent;

         if (parent>0)
         {
             //find parent's line number
             int pline=magiclist.indexOf(parent)*2;
             int xpos=(int)(((float)species_list[ID].start)/timescale);
             if (xpos>(SCALE-1)) xpos=SCALE-1;
             output_grid[pline][xpos]=2;
             if (pline>myline) // below
             {
                 for (int j=myline+1; j<pline; j++)
                 {
                     output_grid[j][xpos]=3;
                 }
                 output_grid[myline][xpos]=4;
             }
             else //above
             {
                 for (int j=pline+1; j<myline; j++)
                 {
                     output_grid[j][xpos]=3;
                 }
                 output_grid[myline][xpos]=5;
             }
         }
         myline++;
         myline++;
     }

     //Convert grid to ASCII output
     for (int j=0; j<output_grid.count(); j++)
     {
         for (int k=0; k<SCALE; k++)
         {
             if (output_grid[j][k]==0) out<<" ";
             if (output_grid[j][k]==1) out<<"-"; //Possibly size characters: from small to big: . ~ = % @
             if (output_grid[j][k]==2) out<<"+";
             if (output_grid[j][k]==3) out<<"|";
             if (output_grid[j][k]==4) out<<"/";
             if (output_grid[j][k]==5) out<<"\\";
         }
         QString extra;

         if (j%2 == 0)
         {
             out<< "ID:"<<magiclist[j/2]<<endl;
         }
         else out<<endl;
     }

     MainWin->setStatusBarText("Done tree");
     qApp->processEvents();

     //Finally output genomes of all extant taxa - for cladistic comparison originally
     out<<endl<<"============================================================="<<endl;
     out<<endl<<"Genomes for extant taxa"<<endl;
     out<<endl<<"============================================================="<<endl;

     int tcount=0;
     i.toFront();
      while (i.hasNext())
      {
          i.next();
          quint64 ID=i.key();
          logged_species spe = i.value();
          if (spe.end==lasttime)
           {
              tcount++;
              out<<"Genome: "<<ReturnBinary(spe.lastgenome)<<"  ID: "<<ID<<endl;
          }
      }

      out<<endl;
      out<<"Taxa: "<<tcount<<endl;

      return OutputString;
    }
    else
    {
        return "Can't open file";
    }
}

QString AnalysisTools::ReturnBinary(quint64 genome)
{
    QString s;
    QTextStream sout(&s);

    for (int j=0; j<64; j++)
        if (tweakers64[63-j] & genome) sout<<"1"; else sout<<"0";

    return s;
}

void AnalysisTools::MakeListRecursive(QList<quint64> *magiclist, QMap <quint64, logged_species> *species_list, quint64 ID, int insertpos)
//This does the re-ordering for graph
//Trick is to insert children of each taxon alternatively on either side of it - but do that recursively so each
//child turns into an entire clade, if appropriate
{
    //insert this item into the magiclist at requested pos
    magiclist->insert(insertpos,ID);
    //find children
    QMapIterator <quint64, logged_species> i(*species_list);

    bool before=false; //alternate before/after
    i.toFront();
    while (i.hasNext())
    {
        i.next();
        if (i.value().parent==ID)
        {
            //find insert position
            int pos=magiclist->indexOf(ID);
            if (before)
                MakeListRecursive(magiclist,species_list,i.key(),pos);
            else
                MakeListRecursive(magiclist,species_list,i.key(),pos+1);

            before=!before;
        }
    }
}
