#include "fossilrecord.h"
#include "simmanager.h"
#include <QString>
#include <QFile>
#include <QTextStream>

FossilRecord::FossilRecord(int x, int y, int sparse, QString Name)
{
    fossils.clear();

    xpos=x; //grid pos
    ypos=y;
    sparsity=sparse; //quality of the record - iterations between records of fossil/env
    startage=generation; //iteration count when record started
    name=Name;
}

void FossilRecord::MakeFossil()
{
    if (generation % sparsity ==0)
    {
        if (critters[xpos][ypos][0].age==0) return;

        Fossil *newfossil = new Fossil;

        newfossil->env[0]=environment[xpos][ypos][0];
        newfossil->env[1]=environment[xpos][ypos][1];
        newfossil->env[2]=environment[xpos][ypos][2];

        newfossil->genome=critters[xpos][ypos][0].genome;
        newfossil->timestamp=generation;

        fossils.append(newfossil);
    }
}

void FossilRecord::WriteRecord(QString fname)
{
    QFile outputfile(fname);
    outputfile.open(QIODevice::WriteOnly);
    QTextStream out(&outputfile);

    out<<"'"<<name<<"'\n";
    out<<"'X',"<<xpos<<",'Y'',"<<ypos<<",'start',"<<startage<<",'records',"<<fossils.length()<<"\n";
    out<<"'Time','Red','Green','Blue','Genome',";

    for (int j=0; j<63; j++) out<<j<<",";
    out<<"63\n";

    for (int i=0; i<fossils.length(); i++)
    {
        out<<fossils[i]->timestamp<<","<<fossils[i]->env[0]<<","<<fossils[i]->env[1]<<","<<fossils[i]->env[2]<<","<<fossils[i]->genome<<",";
        for (int j=0; j<63; j++)
           if (tweakers64[63-j] & fossils[i]->genome) out<<"1,"; else out<<"0,";

        if (tweakers64[0] & fossils[i]->genome) out<<"1\n"; else out<<"0\n";
    }
    outputfile.close();
}
