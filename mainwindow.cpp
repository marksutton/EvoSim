#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "analyser.h"
#include "fossrecwidget.h"
#include "resizecatcher.h"
#include <QTextStream>
#include <QInputDialog>
#include <QGraphicsPixmapItem>
#include <QDockWidget>
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QStringList>
#include <QMessageBox>
#include <QActionGroup>
#include <QDataStream>

//define version for files
#define VERSION 1

SimManager *TheSimManager;
MainWindow *MainWin;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWin=this;

    //Install filter to catch resize events to central widget and deliver to mainwindow (handle dock resizes)
    ResizeCatcher *rescatch = new ResizeCatcher(this);
    ui->centralWidget->installEventFilter(rescatch);

    //---- ARTS: Add Toolbar
    startButton = new QAction(QIcon(QPixmap(":/toolbar/startButton-Enabled.png")), QString("Run"), this);
    runForButton = new QAction(QIcon(QPixmap(":/toolbar/runForButton-Enabled.png")), QString("Run For..."), this);
    pauseButton = new QAction(QIcon(QPixmap(":/toolbar/pauseButton-Enabled.png")), QString("Pause"), this);
    resetButton = new QAction(QIcon(QPixmap(":/toolbar/resetButton-Enabled.png")), QString("Reset"), this);
    startButton->setEnabled(false);
    runForButton->setEnabled(false);
    pauseButton->setEnabled(false);    
    ui->toolBar->addAction(startButton);
    ui->toolBar->addAction(runForButton);
    ui->toolBar->addAction(pauseButton);
    ui->toolBar->addAction(resetButton);
    QObject::connect(startButton, SIGNAL(triggered()), this, SLOT(on_actionStart_Sim_triggered()));
    QObject::connect(runForButton, SIGNAL(triggered()), this, SLOT(on_actionRun_for_triggered()));
    QObject::connect(pauseButton, SIGNAL(triggered()), this, SLOT(on_actionPause_Sim_triggered()));
    QObject::connect(resetButton, SIGNAL(triggered()), this, SLOT(on_actionReseed_triggered()));

    //---- ARTS: Add Genome Comparison UI
    ui->genomeComparisonDock->hide();
    genoneComparison = new GenomeComparison;
    QVBoxLayout *genomeLayout = new QVBoxLayout;
    genomeLayout->addWidget(genoneComparison);
    ui->genomeComparisonContent->setLayout(genomeLayout);

    //----MDS as above for fossil record dock and report dock
    ui->fossRecDock->hide();
    FRW = new FossRecWidget();
    QVBoxLayout *frwLayout = new QVBoxLayout;
    frwLayout->addWidget(FRW);
    ui->fossRecDockContents->setLayout(frwLayout);

    ui->reportViewerDock->hide();

    viewgroup = new QActionGroup(this);
    // These actions were created via qt designer
    viewgroup->addAction(ui->actionPopulation_Count);
    viewgroup->addAction(ui->actionMean_fitness);
    viewgroup->addAction(ui->actionGenome_as_colour);
    viewgroup->addAction(ui->actionNonCoding_genome_as_colour);
    viewgroup->addAction(ui->actionGene_Frequencies_012);
    //viewgroup->addAction(ui->actionBreed_Attempts);
    viewgroup->addAction(ui->actionBreed_Fails_2);
    viewgroup->addAction(ui->actionSettles);
    viewgroup->addAction(ui->actionSettle_Fails);
    QObject::connect(viewgroup, SIGNAL(triggered(QAction *)), this, SLOT(view_mode_changed(QAction *)));

    viewgroup2 = new QActionGroup(this);
    // These actions were created via qt designer
    viewgroup2->addAction(ui->actionNone);
    viewgroup2->addAction(ui->actionSorted_Summary);
    viewgroup2->addAction(ui->actionGroups);
    viewgroup2->addAction(ui->actionGroups2);
    viewgroup2->addAction(ui->actionSimple_List);

    envgroup = new QActionGroup(this);
    envgroup->addAction(ui->actionStatic);
    envgroup->addAction(ui->actionBounce);
    envgroup->addAction(ui->actionOnce);
    envgroup->addAction(ui->actionLoop);
    ui->actionLoop->setChecked(true);


    QObject::connect(viewgroup2, SIGNAL(triggered(QAction *)), this, SLOT(report_mode_changed(QAction *)));

    //create scenes, add to the GVs
    envscene = new EnvironmentScene;
    ui->GV_Environment->setScene(envscene);
    envscene->mw=this;

    popscene = new PopulationScene;
    popscene->mw=this;
    ui->GV_Population->setScene(popscene);

    //add images to the scenes    
    env_item= new QGraphicsPixmapItem();
    envscene->addItem(env_item);
    env_item->setZValue(0);

    pop_item = new QGraphicsPixmapItem();
    popscene->addItem(pop_item);

    pop_image=new QImage(gridX, gridY, QImage::Format_Indexed8);
    QVector <QRgb> clut(256);
    for (int ic=0; ic<256; ic++) clut[ic]=qRgb(ic,ic,ic);
    pop_image->setColorTable(clut);
    pop_image->fill(0);

    env_image=new QImage(gridX, gridY, QImage::Format_RGB32);
    env_image->fill(0);

    pop_image_colour=new QImage(gridX, gridY, QImage::Format_RGB32);
    env_image->fill(0);

    env_item->setPixmap(QPixmap::fromImage(*env_image));
    pop_item->setPixmap(QPixmap::fromImage(*pop_image));

    TheSimManager = new SimManager;

    FinishRun();//sets up enabling
    TheSimManager->SetupRun();
    RefreshRate=50;
    NextRefresh=0;
    Report();

    showMaximized();
    //Now set all the defaults


}

MainWindow::~MainWindow()
{
    delete ui;
    delete TheSimManager;
}

void MainWindow::on_actionReseed_triggered()
{
    TheSimManager->SetupRun();
    NextRefresh=0;
    Report();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::on_actionStart_Sim_triggered()
{
    if (CurrentEnvFile==-1)
    {
        QMessageBox::critical(0,"","Cannot start simulation without environment");
        if (on_actionEnvironment_Files_triggered() == false) {
            return;
        }
    }
    RunSetUp();
    while (pauseflag==false)
    {       
        Report();
        qApp->processEvents();
        int emode=0;
        if (ui->actionOnce->isChecked()) emode=1;
        if (ui->actionBounce->isChecked()) emode=3;
        if (ui->actionLoop->isChecked()) emode=2;
        if (TheSimManager->iterate(emode)) pauseflag=true; //returns true if reached end
        FRW->MakeRecords();
    }
    FinishRun();
}

void MainWindow::on_actionPause_Sim_triggered()
{
    pauseflag=true;
}

void MainWindow::on_actionRun_for_triggered()
{
    if (CurrentEnvFile==-1)
    {
        QMessageBox::critical(0,"","Cannot start simulation without environment");
        if (on_actionEnvironment_Files_triggered() == false) {
            return;
        }
    }

    bool ok;
    int i = QInputDialog::getInt(this, "",
                                 tr("Iterations: "), 1000, 1, 10000000, 1, &ok);
    if (!ok) return;

    RunSetUp();
    while (pauseflag==false && i>0)
    {
        Report();
        qApp->processEvents();
        int emode=0;
        if (ui->actionOnce->isChecked()) emode=1;
        if (ui->actionBounce->isChecked()) emode=3;
        if (ui->actionLoop->isChecked()) emode=2;
        if (TheSimManager->iterate(emode)) pauseflag=true;
        FRW->MakeRecords();
        i--;
    }
    FinishRun();
}

void MainWindow::on_actionRefresh_Rate_triggered()
{
    bool ok;
    int i = QInputDialog::getInt(this, "",
                                 tr("Refresh rate: "), RefreshRate, 1, 10000, 1, &ok);
    if (!ok) return;
    RefreshRate=i;
}

void MainWindow::RunSetUp()
{
    pauseflag=false;
    ui->actionStart_Sim->setEnabled(false);
    startButton->setEnabled(false);
    ui->actionRun_for->setEnabled(false);
    runForButton->setEnabled(false);
    ui->actionPause_Sim->setEnabled(true);
    pauseButton->setEnabled(true);
    ui->actionReseed->setEnabled(false);
    resetButton->setEnabled(false);
    ui->actionSettings->setEnabled(false);
    ui->actionEnvironment_Files->setEnabled(false);
    timer.restart();
    NextRefresh=RefreshRate;
}

void MainWindow::FinishRun()
{
    ui->actionStart_Sim->setEnabled(true);
    startButton->setEnabled(true);
    ui->actionRun_for->setEnabled(true);
    runForButton->setEnabled(true);
    ui->actionReseed->setEnabled(true);
    resetButton->setEnabled(true);
    ui->actionPause_Sim->setEnabled(false);
    pauseButton->setEnabled(false);
    ui->actionSettings->setEnabled(true);
    ui->actionEnvironment_Files->setEnabled(true);
    NextRefresh=0;
    Report();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    exit(0);
}

void MainWindow::Report()
{

    if (--NextRefresh>0) return;

    NextRefresh=RefreshRate;

    QString s;
    QTextStream sout(&s);

    int time=timer.elapsed();
    float atime= (float)time / (float) RefreshRate;
    timer.restart();
    double t=0;
    for (int n2=0; n2<gridX; n2++)
    for (int m2=0; m2<gridY; m2++)
        t+=totalfit[n2][m2];
    t/=(double)AliveCount;
    t/=(double)settleTolerance;
    t*=100; //now %age

    QString out;
    QTextStream o(&out);

    o<<generation; //need to use to avoid int64 issues
    ui->LabelIteration->setText(out);

    out.sprintf("%d",out.toInt()/yearsPerIteration);
    ui->LabelYears->setText(out);

    //now back to sprintf for convenience
    if (CurrentEnvFile>=EnvFiles.count())
    out.sprintf("Finished (%d)",EnvFiles.count());
    else
    out.sprintf("%d/%d",CurrentEnvFile+1,EnvFiles.count());
    ui->LabelEnvironment->setText(out);

    out.sprintf("%.2f%%",t);
    ui->LabelFitness->setText(out);

    out.sprintf("%.2f",atime);
    ui->LabelSpeed->setText(out);

    out.sprintf("%d",AliveCount);
    ui->LabelCritters->setText(out);

    RefreshReport();
    RefreshPopulations();
    RefreshEnvironment();
    FRW->RefreshMe();
    FRW->WriteFiles();

    //reset the breedattempts and breedfails arrays
    for (int n2=0; n2<gridX; n2++)
    for (int m2=0; m2<gridY; m2++)
    {
        //breedattempts[n2][m2]=0;
        breedfails[n2][m2]=0;
        settles[n2][m2]=0;
        settlefails[n2][m2]=0;
    }
}

void MainWindow::RefreshReport()
{
    UpdateTitles();

    QTime refreshtimer;
    refreshtimer.restart();

    if (ui->actionNone->isChecked()) return;

    QString line;
    QTextStream sout(&line);
    int x=popscene->selectedx;
    int y=popscene->selectedy;
    int maxuse=maxused[x][y];
    ui->plainTextEdit->clear();

    if (ui->actionSimple_List->isChecked()) //old crappy code
    {


       for (int i=0; i<=maxuse; i++)
        {
            if (i<10) sout<<" "<<i<<": "; else  sout<<i<<": ";
            if (critters[x][y][i].age > 0 )
            {
                for (int j=0; j<32; j++)
                    if (tweakers64[63-j] & critters[x][y][i].genome) sout<<"1"; else sout<<"0";
                sout<<" ";
                for (int j=32; j<64; j++)
                    if (tweakers64[63-j] & critters[x][y][i].genome) sout<<"1"; else sout<<"0";
                sout<<"  decimal: "<<critters[x][y][i].genome<<"  fitness: "<<critters[x][y][i].fitness;
            }
            else sout<<" EMPTY";
            sout<<"\n";
        }
    }

    if (ui->actionSorted_Summary->isChecked())
    {
        Analyser a;
        for (int i=0; i<=maxuse; i++) if (critters[x][y][i].age > 0) a.AddGenome(critters[x][y][i].genome,critters[x][y][i].fitness);
        sout<<a.SortedSummary();
    }

    if (ui->actionGroups->isChecked())
    {
        Analyser a;
        for (int i=0; i<=maxuse; i++) if (critters[x][y][i].age > 0) a.AddGenome(critters[x][y][i].genome,critters[x][y][i].fitness);
        sout<<a.Groups();
    }

    if (ui->actionGroups2->isChecked())
    {
        //Code to sample all 10000 squares
        Analyser a;

        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            if (totalfit[n][m]==0) continue;
            for (int c=0; c<slotsPerSq; c++)
            {
                if (critters[n][m][c].age>0)
                {
                    a.AddGenome(critters[n][m][c].genome,critters[n][m][c].fitness);
                    break;
                }
            }
        }
        sout<<a.Groups();

    }
    sout<<"\n\nTime for report: "<<refreshtimer.elapsed()<<"ms";

    ui->plainTextEdit->appendPlainText(line);
}

void MainWindow::UpdateTitles()
{
    if (ui->actionPopulation_Count->isChecked())
        ui->LabelVis->setText("Population Count");

    if (ui->actionMean_fitness->isChecked())
        ui->LabelVis->setText("Mean Fitness");


    if (ui->actionGenome_as_colour->isChecked())
        ui->LabelVis->setText("Coding Genome bitcount as colour (modal critter)");

    if (ui->actionNonCoding_genome_as_colour->isChecked())
        ui->LabelVis->setText("Non-Coding Genome bitcount as colour (modal critter)");

    if (ui->actionGene_Frequencies_012->isChecked())
        ui->LabelVis->setText("Frequences genes 0,1,2 (all critters in square)");

    if (ui->actionBreed_Attempts->isChecked())
        ui->LabelVis->setText("Breed Attempts");

    if (ui->actionBreed_Fails->isChecked())
        ui->LabelVis->setText("Breed Fails");

    if (ui->actionSettles->isChecked())
        ui->LabelVis->setText("Successful Settles");

    if (ui->actionSettle_Fails->isChecked())
        ui->LabelVis->setText("Breed Fails (red) and Settle Fails (green)");

    if (ui->actionBreed_Fails_2->isChecked())
        ui->LabelVis->setText("Breed Fails 2 (unused?)");
}

void MainWindow::RefreshPopulations()
{
    //check to see what the mode is

    if (ui->actionPopulation_Count->isChecked())
    {
        //Popcount
        int bigcount=0;
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int count=0;
            for (int c=0; c<slotsPerSq; c++)
            {
                if (critters[n][m][c].age) count++;
            }
            //if (count>0) qDebug()<<count;
            bigcount+=count;
            count*=2;
            if (count>255) count=255;
            pop_image->setPixel(n,m,count);
        }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image));
    }

    if (ui->actionMean_fitness->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int count=0;
            for (int c=0; c<slotsPerSq; c++)
            {
                if (critters[n][m][c].age) count++;
            }
            //if (count>0) qDebug()<<count;
            if (count==0)
            pop_image->setPixel(n,m,0);
                else
            pop_image->setPixel(n,m,(totalfit[n][m] * 10) / count);

        }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image));
    }


    if (ui->actionGenome_as_colour->isChecked())
    {
        //find modal genome in each square, convert to colour
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            //data structure for mode
            quint64 genomes[SLOTS_PER_GRID_SQUARE];
            int counts[SLOTS_PER_GRID_SQUARE];
            int arraypos=0; //pointer

            if (totalfit[n][m]==0) pop_image_colour->setPixel(n,m,0); //black if square is empty
            else
            {
                //for each used slot
                for (int c=0; c<maxused[n][m]; c++)
                {
                    if (critters[n][m][c].age>0)
                    {
                        //If critter is alive

                        quint64 g = critters[n][m][c].genome;

                        //find genome frequencies
                        for (int i=0; i<arraypos; i++)
                        {
                            if (genomes[i]==g) //found it
                            {
                                counts[i]++;
                                goto gotcounts;
                            }
                        }
                        //didn't find it
                        genomes[arraypos]=g;
                        counts[arraypos++]=1;
                    }
                }
                gotcounts:

                //find max frequency
                int max=-1;
                quint64 maxg=0;

                for (int i=0; i<arraypos; i++)
                    if (counts[i]>max)
                    {
                        max=counts[i];
                        maxg=genomes[i];
                    }

                //now convert first 32 bits to a colour
                // r,g,b each counts of 5,5,6 bits
                quint32 genome= (quint32)(maxg & ((quint64)65536*(quint64)65536-(quint64)1));
                quint32 b = bitcounts[genome & 2047] * 23;
                genome /=2048;
                quint32 g = bitcounts[genome & 2047] * 23;
                genome /=2048;
                quint32 r = bitcounts[genome] * 25;
                pop_image_colour->setPixel(n,m,qRgb(r, g, b));
            }

       }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
    }

    if (ui->actionNonCoding_genome_as_colour->isChecked())
    {
        //find modal genome in each square, convert non-coding to colour
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            //data structure for mode
            quint64 genomes[SLOTS_PER_GRID_SQUARE];
            int counts[SLOTS_PER_GRID_SQUARE];
            int arraypos=0; //pointer

            if (totalfit[n][m]==0) pop_image_colour->setPixel(n,m,0); //black if square is empty
            else
            {
                //for each used slot
                for (int c=0; c<maxused[n][m]; c++)
                {
                    if (critters[n][m][c].age>0)
                    {
                        //If critter is alive

                        quint64 g = critters[n][m][c].genome;

                        //find genome frequencies
                        for (int i=0; i<arraypos; i++)
                        {
                            if (genomes[i]==g) //found it
                            {
                                counts[i]++;
                                goto gotcounts2;
                            }
                        }
                        //didn't find it
                        genomes[arraypos]=g;
                        counts[arraypos++]=1;
                    }
                }
                gotcounts2:

                //find max frequency
                int max=-1;
                quint64 maxg=0;

                for (int i=0; i<arraypos; i++)
                    if (counts[i]>max)
                    {
                        max=counts[i];
                        maxg=genomes[i];
                    }

                //now convert first 32 bits to a colour
                // r,g,b each counts of 5,5,6 bits
                quint32 genome= (quint32)(maxg / ((quint64)65536*(quint64)65536));
                quint32 b = bitcounts[genome & 2047] * 23;
                genome /=2048;
                quint32 g = bitcounts[genome & 2047] * 23;
                genome /=2048;
                quint32 r = bitcounts[genome] * 25;
                pop_image_colour->setPixel(n,m,qRgb(r, g, b));
            }
       }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
    }

    if (ui->actionGene_Frequencies_012->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int count=0;
            int gen0tot=0;
            int gen1tot=0;
            int gen2tot=0;
            for (int c=0; c<slotsPerSq; c++)
            {
                if (critters[n][m][c].age)
                {
                    count++;
                    if (critters[n][m][c].genome & 1) gen0tot++;
                    if (critters[n][m][c].genome & 2) gen1tot++;
                    if (critters[n][m][c].genome & 4) gen2tot++;
                }
            }
            if (count==0) pop_image_colour->setPixel(n,m,qRgb(0, 0, 0));
            else
            {
                quint32 r = gen0tot *256 / count;;
                quint32 g = gen1tot *256 / count;;
                quint32 b = gen2tot *256 / count;;
                pop_image_colour->setPixel(n,m,qRgb(r, g, b));
            }
          }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
    }

    if (ui->actionBreed_Attempts->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int value=(breedattempts[n][m]*10)/RefreshRate;
            if (value>255) value=255;
            pop_image->setPixel(n,m,value);
        }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image));
    }

    if (ui->actionBreed_Fails->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            if (breedattempts[n][m]==0) pop_image->setPixel(n,m,0);
            else
            {
                int value=(breedfails[n][m]*255)/breedattempts[n][m];
                pop_image->setPixel(n,m,value);
            }
        }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image));
    }

    if (ui->actionSettles->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int value=(settles[n][m]*10)/RefreshRate;
            if (value>255) value=255;
            pop_image->setPixel(n,m,value);
        }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image));
    }

    if (ui->actionSettle_Fails->isChecked())
    //this now combines breed fails (red) and settle fails (green)
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int r,g;
            r=breedfails[n][m]; if (r>255) r=255;
            int d=settles[n][m] + settlefails[n][m];
            if (d==0) g=0;
            else g=(settlefails[n][m]*2550)/(settles[n][m] + settlefails[n][m]);
            if (g>255) g=255;
            pop_image_colour->setPixel(n,m,qRgb(r, g, 0));
        }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
    }

    if (ui->actionBreed_Fails_2->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            if (breedfails[n][m]==0) pop_image->setPixel(n,m,0);
            else
            {
                int value=(breedfails[n][m]);
                if (value>255) value=255;
                pop_image->setPixel(n,m,value);
            }
        }
        pop_item->setPixmap(QPixmap::fromImage(*pop_image));
    }

}

void MainWindow::RefreshEnvironment()
{


    for (int n=0; n<gridX; n++)
    for (int m=0; m<gridY; m++)
        env_image->setPixel(n,m,qRgb(environment[n][m][0], environment[n][m][1], environment[n][m][2]));

    env_item->setPixmap(QPixmap::fromImage(*env_image));

    //Draw on fossil records
    envscene->DrawLocations(FRW->FossilRecords,ui->actionShow_positions->isChecked());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    //force a rescale of the graphic view
    Resize();
}

void MainWindow::Resize()
{
    ui->GV_Population->fitInView(pop_item,Qt::KeepAspectRatio);
    ui->GV_Environment->fitInView(env_item,Qt::KeepAspectRatio);
}

void MainWindow::view_mode_changed(QAction *temp2)
{
    UpdateTitles();
    RefreshPopulations();
}

void MainWindow::report_mode_changed(QAction *temp2)
{
    RefreshReport();
}

void MainWindow::ResetSquare(int n, int m)
{
    //grid expanded - make sure everything is zeroed in new slots
    for (int c=0; c<slotsPerSq; c++) critters[n][m][c].age=0;

    totalfit[n][m]=0;

    breedattempts[n][m]=0;
    breedfails[n][m]=0;
    settles[n][m]=0;
    settlefails[n][m]=0;

}


void MainWindow::on_actionSettings_triggered()
{
    //AutoMarkers options tab
    //Something like:

    int oldrows, oldcols, oldslots;
    oldrows=gridX; oldcols=gridY; oldslots=slotsPerSq;
    SettingsImpl Dialog;
    Dialog.exec();


    if (Dialog.RedoImages)
    {

        //check that the maxused's are in the new range
         for (int n=0; n<gridX; n++)
         for (int m=0; m<gridY; m++)
             if (maxused[n][m]>=slotsPerSq) maxused[n][m]=slotsPerSq-1;

         //If either rows or cols are bigger - make sure age is set to 0 in all critters in new bit!
        if (gridX>oldrows)
        {
            for (int n=oldrows; n<gridX; n++) for (int m=0; m<gridY; m++)
                ResetSquare(n,m);
        }
        if (gridY>oldcols)
        {
            for (int n=0; n<gridX; n++) for (int m=oldcols; m<gridY; m++)
                ResetSquare(n,m);
        }

        delete pop_image;
        delete env_image;
        delete pop_image_colour;
        pop_image =new QImage(gridX, gridY, QImage::Format_Indexed8);
        QVector <QRgb> clut(256);
        for (int ic=0; ic<256; ic++) clut[ic]=qRgb(ic,ic,ic);
        pop_image->setColorTable(clut);

        env_image=new QImage(gridX, gridY, QImage::Format_RGB32);

        pop_image_colour=new QImage(gridX, gridY, QImage::Format_RGB32);

        RefreshPopulations();
        RefreshEnvironment();
        ui->GV_Population->fitInView(pop_item,Qt::KeepAspectRatio);
        ui->GV_Environment->fitInView(env_item,Qt::KeepAspectRatio);
    }
}


void MainWindow::on_actionMisc_triggered()
{
    TheSimManager->testcode();
}

void MainWindow::on_actionCount_Peaks_triggered()
{
    // for a selection of colours - go through ALL genomes, work out fitness.
    quint8 env[3];
    quint32 fits[96];

    for (int i=0; i<96; i++) fits[i]=0;

    env[0]=100;
    env[1]=100;
    env[2]=100;
    quint64 max = (qint64)65536 * (qint64)65536;

    QList<quint32> fit15;
    QList<quint32> fit14;
    QList<quint32> fit13;
    //quint64 max = 1000000;
    for (quint64 genome=0; genome<max; genome++)
    {
        Critter c;
        c.initialise((quint32)genome, env, 0,0,0);
        fits[c.fitness]++;
        /*if (c.fitness>12)
        {
            if (c.fitness==13) fit13.append((quint32)genome);
            if (c.fitness==14) fit14.append((quint32)genome);
            if (c.fitness==15) fit15.append((quint32)genome);
        }*/
        if (!(genome%6553600))
        {
            QString s;
            QTextStream out(&s);
            ui->plainTextEdit->clear();
            out<<(double)genome*(double)100/(double)max<<"% done...";
            ui->plainTextEdit->appendPlainText(s);
            for (int i=0; i<=settleTolerance; i++)
            {
                QString s2;
                QTextStream out2(&s2);

                out2<<i<<" found: "<<fits[i];
                ui->plainTextEdit->appendPlainText(s2);
                qApp->processEvents();
            }
        }
    }
    //done - write out
    ui->plainTextEdit->clear();
    for (int i=0; i<=settleTolerance; i++)
    {
        QString s;
        QTextStream out(&s);

        out<<i<<" found: "<<fits[i];
        ui->plainTextEdit->appendPlainText(s);
    }

    QString s;
    QTextStream out(&s);
    out<<"\nFit13:\n";
    //now output the winning genomes
    foreach(quint32 gen, fit13)
    {
        for (int j=0; j<32; j++) if (tweakers[j] & gen) out<<"1"; else out<<"0";
        out<<"\n";
    }


    out<<"\nFit14:\n";
    //now output the winning genomes
    foreach(quint32 gen, fit14)
    {
        for (int j=0; j<32; j++) if (tweakers[j] & gen) out<<"1"; else out<<"0";
        out<<"\n";
    }

    out<<"\nFit15:\n";
    //now output the winning genomes
    foreach(quint32 gen, fit15)
    {
        for (int j=0; j<32; j++) if (tweakers[j] & gen) out<<"1"; else out<<"0";
        out<<"\n";
    }
    ui->plainTextEdit->appendPlainText(s);
}

bool  MainWindow::on_actionEnvironment_Files_triggered()
{
    //Select files
    QStringList files = QFileDialog::getOpenFileNames(
                            this,
                            "Select one or more image files to load in simulation environment...",
                            "",
                            "Images (*.png *.bmp)");

    if (files.length()==0) {
        return false;
    } else {
        EnvFiles = files;
        CurrentEnvFile=0;
        TheSimManager->loadEnvironmentFromFile();
        RefreshEnvironment();
        return true;
    }
}

void MainWindow::on_actionChoose_Log_Directory_triggered()
{
    QString dirname = QFileDialog::getExistingDirectory(this,"Select directory to log fossil record to");


    if (dirname.length()==0) return;
    FRW->LogDirectory=dirname;
    FRW->LogDirectoryChosen=true;
    FRW->HideWarnLabel();

}

void MainWindow::on_actionSave_triggered()
{
    QString filename = QFileDialog::getSaveFileName(
                            this,
                            "Save file",
                            "",
                            "EVOSIM files (*.evosim)");

    if (filename.length()==0) return;

    //Otherwise - serialise all my crap
    QFile outfile(filename);
    outfile.open(QIODevice::WriteOnly);

    QDataStream out(&outfile);


    out<<QString("EVOSIM file");
    out<<(int)VERSION;
    out<<gridX;
    out<<gridY;
    out<<slotsPerSq;
    out<<startAge;
    out<<target;
    out<<settleTolerance;
    out<<dispersal;
    out<<food;
    out<<breedThreshold;
    out<<breedCost;
    out<<maxDiff;
    out<<mutate;
    out<<envchangerate;
    out<<CurrentEnvFile;
    out<<EnvChangeCounter;
    out<<EnvChangeForward;
    out<<AliveCount;
    out<<RefreshRate;
    out<<generation;

    //action group settings
    int emode=0;
    if (ui->actionOnce->isChecked()) emode=1;
    if (ui->actionBounce->isChecked()) emode=3;
    if (ui->actionLoop->isChecked()) emode=2;
    out<<emode;

    int vmode=0;
    if (ui->actionPopulation_Count->isChecked()) vmode=0;
    if (ui->actionMean_fitness->isChecked()) vmode=1;
    if (ui->actionGenome_as_colour->isChecked()) vmode=2;
    if (ui->actionNonCoding_genome_as_colour->isChecked()) vmode=3;
    if (ui->actionGene_Frequencies_012->isChecked()) vmode=4;
    if (ui->actionBreed_Attempts->isChecked()) vmode=5;
    if (ui->actionBreed_Fails->isChecked()) vmode=6;
    if (ui->actionSettles->isChecked()) vmode=7;
    if (ui->actionSettle_Fails->isChecked()) vmode=8;
    if (ui->actionBreed_Fails_2->isChecked()) vmode=9;
    out<<vmode;

    int rmode=0;
    if (ui->actionNone->isChecked()) rmode=0;
    if (ui->actionSorted_Summary->isChecked()) rmode=1;
    if (ui->actionGroups->isChecked()) rmode=2;
    if (ui->actionGroups2->isChecked()) rmode=3;
    if (ui->actionSimple_List->isChecked()) rmode=4;
    out<<rmode;

    //file list
    out<<EnvFiles.count();
    for (int i=0; i<EnvFiles.count(); i++)
        out<<EnvFiles[i];

    //now the big arrays

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    for (int k=0; k<slotsPerSq; k++)
    {
        if ((critters[i][j][k]).age==0) //its dead
            out<<(int)0;
        else
        {
            out<<critters[i][j][k].age;
            out<<critters[i][j][k].genome;
            out<<critters[i][j][k].ugenecombo;
            out<<critters[i][j][k].fitness;
            out<<critters[i][j][k].energy;
            out<<critters[i][j][k].xpos;
            out<<critters[i][j][k].ypos;
            out<<critters[i][j][k].zpos;
        }
    }

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        out<<environment[i][j][0];
        out<<environment[i][j][1];
        out<<environment[i][j][2];
    }

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        out<<totalfit[i][j];
    }

    for (int i=0; i<256; i++)
    for (int j=0; j<3; j++)
    {
        out<<xormasks[i][j];
    }

    //---- ARTS Genome Comparison Saving
    out<<genoneComparison->saveComparison();

    //and fossil record stuff
    FRW->WriteFiles(); //make sure all is flushed
    out<<FRW->SaveState();

    //New Year Per Iteration variable
    out<<yearsPerIteration;

    //Some more stuff that was missing from first version
    out<<ui->actionShow_positions->isChecked();

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        out<<breedattempts[i][j];
        out<<breedfails[i][j];
        out<<settles[i][j];
        out<<settlefails[i][j];
        out<<maxused[i][j];
    }

    //And finally some window state stuff
    out<<saveState(); //window state
    out<<ui->actionFossil_Record->isChecked();
    out<<ui->actionReport_Viewer->isChecked();
    out<<ui->actionGenomeComparison->isChecked();

    outfile.close();
}

void MainWindow::on_actionLoad_triggered()
{

    QString filename = QFileDialog::getOpenFileName(
                            this,
                            "Save file",
                            "",
                            "EVOSIM files (*.evosim)");

    if (filename.length()==0) return;

    if (pauseflag==false) pauseflag=true;


    //Otherwise - serialise all my crap
    QFile infile(filename);
    infile.open(QIODevice::ReadOnly);

    QDataStream in(&infile);

    QString temp;
    in>>temp;
    if (temp!="EVOSIM file")
    {QMessageBox::warning(this,"","Not an EVOSIM file"); return;}

    int version;
    in>>version;
    if (version>VERSION)
    {QMessageBox::warning(this,"","Version too high - will try to read, but may go horribly wrong");}

    in>>gridX;
    in>>gridY;
    in>>slotsPerSq;
    in>>startAge;
    in>>target;
    in>>settleTolerance;
    in>>dispersal;
    in>>food;
    in>>breedThreshold;
    in>>breedCost;
    in>>maxDiff;
    in>>mutate;
    in>>envchangerate;
    in>>CurrentEnvFile;
    in>>EnvChangeCounter;
    in>>EnvChangeForward;
    in>>AliveCount;
    in>>RefreshRate;
    in>>generation;

    int emode;
    in>>emode;
    if (emode==0) ui->actionStatic->setChecked(true);
    if (emode==1) ui->actionOnce->setChecked(true);
    if (emode==3) ui->actionBounce->setChecked(true);
    if (emode==2) ui->actionLoop->setChecked(true);

    int vmode;
    in>>vmode;
    if (vmode==0) ui->actionPopulation_Count->setChecked(true);
    if (vmode==1) ui->actionMean_fitness->setChecked(true);
    if (vmode==2) ui->actionGenome_as_colour->setChecked(true);
    if (vmode==3) ui->actionNonCoding_genome_as_colour->setChecked(true);
    if (vmode==4) ui->actionGene_Frequencies_012->setChecked(true);
    if (vmode==5) ui->actionBreed_Attempts->setChecked(true);
    if (vmode==6) ui->actionBreed_Fails->setChecked(true);
    if (vmode==7) ui->actionSettles->setChecked(true);
    if (vmode==8) ui->actionSettle_Fails->setChecked(true);
    if (vmode==9) ui->actionBreed_Fails_2->setChecked(true);

    int rmode;
    in>>rmode;
    if (rmode==0) ui->actionNone->setChecked(true);
    if (rmode==1) ui->actionSorted_Summary->setChecked(true);
    if (rmode==2) ui->actionGroups->setChecked(true);
    if (rmode==3) ui->actionGroups2->setChecked(true);
    if (rmode==4) ui->actionSimple_List->setChecked(true);

    int count;

    //file list
    in>>count;
    EnvFiles.clear();

    for (int i=0; i<count; i++)
    {
        QString t;
        in>>t;
        EnvFiles.append(t);
    }

    //now the big arrays

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    for (int k=0; k<slotsPerSq; k++)
    {
        in>>critters[i][j][k].age;
        if (critters[i][j][k].age>0)
        {
            in>>critters[i][j][k].genome;
            in>>critters[i][j][k].ugenecombo;
            in>>critters[i][j][k].fitness;
            in>>critters[i][j][k].energy;
            in>>critters[i][j][k].xpos;
            in>>critters[i][j][k].ypos;
            in>>critters[i][j][k].zpos;
        }
    }

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        in>>environment[i][j][0];
        in>>environment[i][j][1];
        in>>environment[i][j][2];
    }

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        in>>totalfit[i][j];
    }

    for (int i=0; i<256; i++)
    for (int j=0; j<3; j++)
    {
        in>>xormasks[i][j];
    }

    //---- ARTS Genome Comparison Loading
    QByteArray Temp;
    in>>Temp;
    genoneComparison->loadComparison(Temp);

    //and fossil record stuff
    in>>Temp;
    FRW->RestoreState(Temp);

    //New Year Per Iteration variable
    in>>yearsPerIteration;

    //Some more stuff that was missing from first version
    bool btemp;
    in>>btemp;
    ui->actionShow_positions->setChecked(btemp);

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        in>>breedattempts[i][j];
        in>>breedfails[i][j];
        in>>settles[i][j];
        in>>settlefails[i][j];
        in>>maxused[i][j];
    }

    in>>Temp;
    restoreState(Temp); //window state

    in>>btemp; ui->actionFossil_Record->setChecked(btemp);
    in>>btemp; ui->actionReport_Viewer->setChecked(btemp);
    in>>btemp; ui->actionGenomeComparison->setChecked(btemp);


    infile.close();
    NextRefresh=0;
    Report();
}

//---- ARTS: Genome Comparison UI
bool MainWindow::genomeComparisonAdd()
{
    int x=popscene->selectedx;
    int y=popscene->selectedy;

    //---- Get genome colour
    if (totalfit[x][y]!=0) {
        for (int c=0; c<slotsPerSq; c++)
        {
            if (critters[x][y][c].age>0){
                genoneComparison->addGenomeCritter(critters[x][y][c],environment[x][y]);
                return true;
            }
        }       
    }
    return false;
}

void MainWindow::on_actionAdd_Regular_triggered()
{
    int count;
    bool ok;
    count=QInputDialog::getInteger(this,"","Grid Density?",2,2,10,1,&ok);
    if (!ok) return;

    for (int x=0; x<count; x++)
    for (int y=0; y<count; y++)
    {
        int rx=(int)((((float)gridX/(float)count)/(float)2 + (float)gridX/(float)count * (float)x)+.5);
        int ry=(int)((((float)gridY/(float)count)/(float)2 + (float)gridY/(float)count * (float)y)+.5);
        FRW->NewItem(rx,ry,10);
    }
}

void MainWindow::on_actionAdd_Random_triggered()
{
    int count;
    bool ok;
    count=QInputDialog::getInteger(this,"","Number of records to add?",1,1,100,1,&ok);
    if (!ok) return;
    for (int i=0; i<count; i++)
    {
        int rx=qrand() % gridX;
        int ry=qrand() % gridY;
        FRW->NewItem(rx,ry,10);
    }
}

void MainWindow::on_actionSet_Active_triggered()
{
    FRW->SelectedActive(true);
}

void MainWindow::on_actionSet_Inactive_triggered()
{
    FRW->SelectedActive(false);
}

void MainWindow::on_actionSet_Sparsity_triggered()
{

    bool ok;

    int value=QInputDialog::getInt(this,"","Sparsity",10,1,100000,1,&ok);
    if (!ok) return;

    FRW->SelectedSparse(value);

}

void MainWindow::on_actionShow_positions_triggered()
{
    RefreshEnvironment();
}
