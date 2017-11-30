
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "reseed.h"
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
#include <QStringList>
#include <QFile>
#include "analysistools.h"
#include "version.h"
#include "math.h"

SimManager *TheSimManager;
MainWindow *MainWin;


#include <QThread>

class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep(unsigned long secs){QThread::sleep(secs);}
};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    a = new Analyser; // so can delete next time!
    ui->setupUi(this);
    MainWin=this;

    //Install filter to catch resize events to central widget and deliver to mainwindow (handle dock resizes)
    ResizeCatcher *rescatch = new ResizeCatcher(this);
    ui->centralWidget->installEventFilter(rescatch);

    //---- ARTS: Add Toolbar
    startButton = new QAction(QIcon(QPixmap(":/toolbar/startButton-Enabled.png")), QString("Run"), this);
    runForButton = new QAction(QIcon(QPixmap(":/toolbar/runForButton-Enabled.png")), QString("Run for..."), this);
    pauseButton = new QAction(QIcon(QPixmap(":/toolbar/pauseButton-Enabled.png")), QString("Pause"), this);
    resetButton = new QAction(QIcon(QPixmap(":/toolbar/resetButton-Enabled.png")), QString("Reset"), this);
    //---- RJG add further Toolbar options - May 17.
    reseedButton = new QAction(QIcon(QPixmap(":/toolbar/resetButton_knowngenome-Enabled.png")), QString("Reseed"), this);
    runForBatchButton = new QAction(QIcon(QPixmap(":/toolbar/runForBatchButton-Enabled.png")), QString("Batch..."), this);
    settingsButton = new QAction(QIcon(QPixmap(":/toolbar/settingsButton-Enabled.png")), QString("Settings"), this);

    startButton->setEnabled(false);
    runForButton->setEnabled(false);
    pauseButton->setEnabled(false);
    reseedButton->setEnabled(false);
    runForBatchButton->setEnabled(false);
    settingsButton ->setEnabled(false);

    ui->toolBar->addAction(startButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(runForButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(runForBatchButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(pauseButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(resetButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(reseedButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(settingsButton);

    QObject::connect(startButton, SIGNAL(triggered()), this, SLOT(on_actionStart_Sim_triggered()));
    QObject::connect(runForButton, SIGNAL(triggered()), this, SLOT(on_actionRun_for_triggered()));
    QObject::connect(pauseButton, SIGNAL(triggered()), this, SLOT(on_actionPause_Sim_triggered()));
    //----RJG - note for clarity. Reset = start again with random individual. Reseed = start again with user defined genome
    QObject::connect(resetButton, SIGNAL(triggered()), this, SLOT(on_actionReset_triggered()));
    QObject::connect(reseedButton, SIGNAL(triggered()), this, SLOT(on_actionReseed_triggered()));
    QObject::connect(runForBatchButton, SIGNAL(triggered()), this, SLOT(on_actionBatch_triggered()));
    //--- RJG - Also connect the save all menu option.
    QObject::connect(settingsButton, SIGNAL(triggered()), this, SLOT(on_actionSettings_triggered()));

    QObject::connect(ui->save_all, SIGNAL(toggled(bool)), this, SLOT(save_all(bool)));

    //---- RJG - add savepath for all functions, and allow this to be changed. Also add about. Spt 17.
    ui->toolBar->addSeparator();
    QLabel *spath = new QLabel("Save path: ", this);
    ui->toolBar->addWidget(spath);
    QString program_path(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    program_path.append("/");
    path = new QLineEdit(program_path,this);
    ui->toolBar->addWidget(path);
    //Spacer
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    empty->setMaximumWidth(10);
    empty->setMaximumHeight(5);
    ui->toolBar->addWidget(empty);
    QPushButton *cpath = new QPushButton("&Change", this);
    ui->toolBar->addWidget(cpath);
    connect(cpath, SIGNAL (clicked()), this, SLOT(changepath_triggered()));

    ui->toolBar->addSeparator();
    aboutButton = new QAction(QIcon(QPixmap(":/toolbar/aboutButton-Enabled.png")), QString("About"), this);
    ui->toolBar->addAction(aboutButton);
    QObject::connect(aboutButton, SIGNAL (triggered()), this, SLOT (about_triggered()));

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

    speciesgroup = new QActionGroup(this);
    speciesgroup->addAction(ui->actionBasic);
    speciesgroup->addAction(ui->actionOff);
    speciesgroup->addAction(ui->actionPhylogeny);
    speciesgroup->addAction(ui->actionPhylogeny_metrics);
    QObject::connect(speciesgroup, SIGNAL(triggered(QAction *)), this, SLOT(species_mode_changed(QAction *)));

    viewgroup = new QActionGroup(this);
    // These actions were created via qt designer
    viewgroup->addAction(ui->actionPopulation_Count);
    viewgroup->addAction(ui->actionMean_fitness);
    viewgroup->addAction(ui->actionGenome_as_colour);
    viewgroup->addAction(ui->actionNonCoding_genome_as_colour);
    viewgroup->addAction(ui->actionGene_Frequencies_012);
    viewgroup->addAction(ui->actionSpecies);
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

    //RJG - load default environment image to allow program to run out of box (quicker for testing)
    EnvFiles.append(":/EvoSim_default_env.png");
    CurrentEnvFile=0;
    TheSimManager->loadEnvironmentFromFile(1);

    FinishRun();//sets up enabling
    TheSimManager->SetupRun();
    RefreshRate=50;
    NextRefresh=0;
    Report();

    //RJG - Set batch variables
    batch_running=false;
    runs=-1;
    batch_iterations=-1;
    batch_target_runs=-1;

    showMaximized();

    //RJG - Output version, but also date compiled for clarity
    QString vstring;
    vstring.sprintf("%d.%03d",MAJORVERSION,MINORVERSION);
    this->setWindowTitle("EVOSIM v"+vstring+" - compiled - "+__DATE__);

    //RJG - seed pseudorandom numbers
    qsrand(QTime::currentTime().msec());
    //RJG - Now load randoms into program - portable rand is just plain pseudorandom number - initially used in makelookups (called from simmanager contructor) to write to randoms array
    int seedoffset = TheSimManager->portable_rand();
    QFile rfile(":/randoms.dat");
    if (!rfile.exists()) QMessageBox::warning(this,"Oops","Error loading randoms. Please do so manually.");
    rfile.open(QIODevice::ReadOnly);

    rfile.seek(seedoffset);

    //RJG - overwrite pseudorandoms with genuine randoms
    int i=rfile.read((char *)randoms,65536);
    if (i!=65536) QMessageBox::warning(this,"Oops","Failed to read 65536 bytes from file - random numbers may be compromised - try again or restart program");

    //RJG - fill cumulative_normal_distribution with numbers for variable breeding
    //These are a cumulative standard normal distribution from -3 to 3, created using the math.h complementary error function
    //Then scaled to zero to 32 bit rand max, to allow for probabilities within each iteration through a random number
    float x=-3., inc=(6./33);
    int cnt=0;
    do{
        float NSDF=(0.5 * erfc(-(x) * M_SQRT1_2));
        cumulative_normal_distribution[cnt]=4294967296*NSDF;
        x+=inc;
        cnt++;
    }while(cnt<33);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete TheSimManager;
}

// ---- RJG: Change the save path for various stuff.
void MainWindow::changepath_triggered()
{
    QString dirname = QFileDialog::getExistingDirectory(this,"Select directory in which files should be saved.");
    if (dirname.length()!=0)
    {
        dirname.append("/");
        path->setText(dirname);
    }

}

void MainWindow::about_triggered()
{
    About adialogue;
    adialogue.exec();
}

// ---- RJG: Reset simulation (i.e. fill the centre pixel with a genome, then set up a run).
void MainWindow::on_actionReset_triggered()
{
    if ((speciesLoggingToFile==true || fitnessLoggingToFile==true)&&!batch_running)
        {
            QFile outputfile(QString(path->text()+"EvoSim_fitness_log.txt"));

            if (outputfile.exists())
                QMessageBox::warning(this,"Logging","This will append the log of the new run onto your last one, unless you change directories or move the odl log file");
        }

    //--- RJG: This resets all the species logging stuff as well as setting up the run
    TheSimManager->SetupRun();
    NextRefresh=0;

    //Update views...
    RefreshReport();
    UpdateTitles();
    RefreshPopulations();

}

//RJG - Reseed provides options to either reset using a random genome, or a user defined one - drawn from the genome comparison docker.
void MainWindow::on_actionReseed_triggered()
{
    reseed reseed_dialogue;
    reseed_dialogue.exec();

    ui->actionReseed->setChecked(reseedKnown);

    on_actionReset_triggered();

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
        if (ui->actionGo_Slow->isChecked()) Sleeper::msleep(30);
        int emode=0;
        if (ui->actionOnce->isChecked()) emode=1;
        if (ui->actionBounce->isChecked()) emode=3;
        if (ui->actionLoop->isChecked()) emode=2;
        if (TheSimManager->iterate(emode,ui->actionInterpolate->isChecked())) pauseflag=true; //returns true if reached end
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
    int i;
    if(batch_running)
                {
                i=batch_iterations;
                if(i>2)ok=true;
                }
    else i= QInputDialog::getInt(this, "",tr("Iterations: "), 1000, 1, 10000000, 1, &ok);
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
        if (TheSimManager->iterate(emode,ui->actionInterpolate->isChecked())) pauseflag=true;
        FRW->MakeRecords();
        i--;
    }
    FinishRun();
}

//RJG - Batch - primarily intended to allow repeats of runs with the same settings, rather than allowing these to be changed between runs
void MainWindow::on_actionBatch_triggered()
{
    batch_running=true;
    runs=0;

    int environment_start = CurrentEnvFile;

    bool ok;
    batch_iterations=QInputDialog::getInt(this, "",tr("How many iterations would you like each run to go for?"), 1000, 1, 10000000, 1, &ok);
    batch_target_runs=QInputDialog::getInt(this, "",tr("And how many runs?"), 1000, 1, 10000000, 1, &ok);

    QStringList options;
    options << tr("Yes") << tr("No");

    QString environment = QInputDialog::getItem(this, tr("Environment"),
                                            tr("Would you like the environment to repeat with each bacth?"), options, 0, false, &ok);

    if (!ok) {QMessageBox::warning(this,"Woah...","Looks like you cancelled. Batch won't run.");return;}

    bool repeat_environment;
    if (environment=="Yes")repeat_environment=true;
    else repeat_environment=false;

    do{

        //RJG - Sort environment so it repeats
        if(repeat_environment)
                {
                CurrentEnvFile=environment_start;
                int emode=0;
                if (ui->actionOnce->isChecked()) emode=1;
                if (ui->actionBounce->isChecked()) emode=3;
                if (ui->actionLoop->isChecked()) emode=2;
                TheSimManager->loadEnvironmentFromFile(emode);
                }

        //And run...
        on_actionRun_for_triggered();

        if(ui->actionSpecies_logging->isChecked())HandleAnalysisTool(ANALYSIS_TOOL_CODE_MAKE_NEWICK);
        if(ui->actionWrite_phylogeny->isChecked())HandleAnalysisTool(ANALYSIS_TOOL_CODE_DUMP_DATA);

        on_actionReset_triggered();
        runs++;
       }while(runs<batch_target_runs);

    batch_running=false;
    runs=0;
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
    //RJG - Sort out GUI at start of run
    pauseflag=false;
    ui->actionStart_Sim->setEnabled(false);
    startButton->setEnabled(false);
    ui->actionRun_for->setEnabled(false);
    runForButton->setEnabled(false);
    ui->actionPause_Sim->setEnabled(true);
    pauseButton->setEnabled(true);

    if(ui->actionWrite_phylogeny->isChecked()||ui->actionSpecies_logging->isChecked())ui->actionPhylogeny_metrics->setChecked(true);

    //Reseed or reset
    ui->actionReset->setEnabled(false);
    resetButton->setEnabled(false);
    ui->actionSettings->setEnabled(false);
    ui->actionEnvironment_Files->setEnabled(false);

    reseedButton->setEnabled(false);
    runForBatchButton->setEnabled(false);
    settingsButton->setEnabled(false);


    timer.restart();
    NextRefresh=RefreshRate;
}

void MainWindow::FinishRun()
{
    ui->actionStart_Sim->setEnabled(true);
    startButton->setEnabled(true);
    ui->actionRun_for->setEnabled(true);
    runForButton->setEnabled(true);
    //Reseed or reset
    ui->actionReset->setEnabled(true);
    resetButton->setEnabled(true);
    ui->actionPause_Sim->setEnabled(false);
    pauseButton->setEnabled(false);
    ui->actionSettings->setEnabled(true);
    ui->actionEnvironment_Files->setEnabled(true);


    reseedButton->setEnabled(true);
    runForBatchButton->setEnabled(true);
    settingsButton->setEnabled(true);

    //----RJG disabled this to stop getting automatic logging at end of run, thus removing variability making analysis harder.
    //NextRefresh=0;
    //Report();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    exit(0);
}

// ---- RJG: Updates reports, and does logging
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

    out.sprintf("%.3f",(3600000/(atime*yearsPerIteration))/1000000);
    ui->LabelMyPerHour->setText(out);

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

    CalcSpecies();
    out="-";
    if (speciesLogging || ui->actionSpecies->isChecked())
    {
        int g5=0, g50=0;
        for (int i=0; i<oldspecieslist.count(); i++)
        {
            if (oldspecieslist[i].size>5) g5++;
            if (oldspecieslist[i].size>50) g50++;
        }
        out.sprintf("%d (>5:%d >50:%d)",oldspecieslist.count(), g5, g50);
    }
    ui->LabelSpecies->setText(out);

    RefreshReport();

    //do species stuff
    if(!ui->actionDon_t_update_gui->isChecked())RefreshPopulations();
    if(!ui->actionDon_t_update_gui->isChecked())RefreshEnvironment();
    FRW->RefreshMe();
    FRW->WriteFiles();

    WriteLog();


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
    Analyser a;

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

        for (int i=0; i<=maxuse; i++) if (critters[x][y][i].age > 0) a.AddGenome(critters[x][y][i].genome,critters[x][y][i].fitness);
        sout<<a.SortedSummary();
    }

    if (ui->actionGroups->isChecked())
    {
        for (int i=0; i<=maxuse; i++) if (critters[x][y][i].age > 0) a.AddGenome(critters[x][y][i].genome,critters[x][y][i].fitness);
        sout<<a.Groups();
    }

    if (ui->actionGroups2->isChecked())
    {
        //Code to sample all 10000 squares

        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            if (totalfit[n][m]==0) continue;
            for (int c=0; c<slotsPerSq; c++)
            {
                if (critters[n][m][c].age>0)
                {
                    a.AddGenome_Fast(critters[n][m][c].genome);
                    break;
                }
            }
        }
        //TO DO - new report
        //sout<<a.Groups_Report();
        sout<<"Currently no code here!";

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

    if (ui->actionSpecies->isChecked())
        ui->LabelVis->setText("Species");

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

int MainWindow::ScaleFails(int fails, float gens)
{
    //scale colour of fail count, correcting for generations, and scaling high values to something saner
    float ffails=((float)fails)/gens;

    ffails*=100.0; // a fudge factor no less! Well it's only visualization...
    ffails=pow(ffails,0.8);

    if (ffails>255.0) ffails=255.0;
    return (int)ffails;
}

void MainWindow::RefreshPopulations()
//Refreshes of left window - also run species ident
{

    QDir save_dir(path->text());

    //check to see what the mode is
    if (ui->actionPopulation_Count->isChecked()||ui->save_population_count->isChecked())
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
            bigcount+=count;
            count*=2;
            if (count>255) count=255;
            pop_image->setPixel(n,m,count);
        }
        if (ui->actionPopulation_Count->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image));
        if (ui->save_population_count->isChecked())
                 if(save_dir.mkpath("population/"))
                             pop_image_colour->save(QString(save_dir.path()+"/population/EvoSim_population_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    if (ui->actionMean_fitness->isChecked()||ui->save_mean_fitness->isChecked())
    {
        //Popcount
        int multiplier=255/settleTolerance;
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int count=0;
            for (int c=0; c<slotsPerSq; c++)
            {
                if (critters[n][m][c].age) count++;
            }
            if (count==0)
            pop_image->setPixel(n,m,0);
                else
            pop_image->setPixel(n,m,(totalfit[n][m] * multiplier) / count);

        }
        if (ui->actionMean_fitness->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image));
        if (ui->save_mean_fitness->isChecked())
                 if(save_dir.mkpath("fitness/"))
                             pop_image_colour->save(QString(save_dir.path()+"/fitness/EvoSim_mean_fitness_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }


    if (ui->actionGenome_as_colour->isChecked()||ui->save_coding_genome_as_colour->isChecked())
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
                gotcounts:;
                }

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
                // r,g,b each counts of 11,11,10 bits
                quint32 genome= (quint32)(maxg & ((quint64)65536*(quint64)65536-(quint64)1));
                quint32 b = bitcounts[genome & 2047] * 23;
                genome /=2048;
                quint32 g = bitcounts[genome & 2047] * 23;
                genome /=2048;
                quint32 r = bitcounts[genome] * 25;
                pop_image_colour->setPixel(n,m,qRgb(r, g, b));
            }

       }

       if (ui->actionGenome_as_colour->isChecked()) pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
       if (ui->save_coding_genome_as_colour->isChecked())
                if(save_dir.mkpath("coding/"))
                            pop_image_colour->save(QString(save_dir.path()+"/coding/EvoSim_coding_genome_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }


    if (ui->actionSpecies->isChecked()||ui->save_species->isChecked()) //do visualisation if necessary
    {
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {

            if (totalfit[n][m]==0) pop_image_colour->setPixel(n,m,0); //black if square is empty
            else
            {
                quint64 thisspecies=0;
                for (int c=0; c<slotsPerSq; c++)
                {
                    if (critters[n][m][c].age>0)
                    {
                        thisspecies=critters[n][m][c].speciesid;
                        break;
                    }
                }

                pop_image_colour->setPixel(n,m,species_colours[thisspecies % 65536]);
            }
        }
         if (ui->actionSpecies->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
         if (ui->save_species->isChecked())
                  if(save_dir.mkpath("species/"))
                              pop_image_colour->save(QString(save_dir.path()+"/species/EvoSim_species_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    if (ui->actionNonCoding_genome_as_colour->isChecked()||ui->save_non_coding_genome_as_colour->isChecked())
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
                gotcounts2:;
                }


                //find max frequency
                int max=-1;
                quint64 maxg=0;

                for (int i=0; i<arraypos; i++)
                    if (counts[i]>max)
                    {
                        max=counts[i];
                        maxg=genomes[i];
                    }

                //now convert second 32 bits to a colour
                // r,g,b each counts of 11,11,10 bits
                quint32 genome= (quint32)(maxg / ((quint64)65536*(quint64)65536));
                quint32 b = bitcounts[genome & 2047] * 23;
                genome /=2048;
                quint32 g = bitcounts[genome & 2047] * 23;
                genome /=2048;
                quint32 r = bitcounts[genome] * 25;
                pop_image_colour->setPixel(n,m,qRgb(r, g, b));
            }
       }
        if(ui->actionNonCoding_genome_as_colour->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
        if(ui->save_non_coding_genome_as_colour->isChecked())
                 if(save_dir.mkpath("non_coding/"))
                             pop_image_colour->save(QString(save_dir.path()+"/non_coding/EvoSim_non_coding_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    if (ui->actionGene_Frequencies_012->isChecked()||ui->save_gene_frequencies->isChecked())
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
                quint32 r = gen0tot *256 / count;
                quint32 g = gen1tot *256 / count;
                quint32 b = gen2tot *256 / count;
                pop_image_colour->setPixel(n,m,qRgb(r, g, b));
            }
          }
          if (ui->actionGene_Frequencies_012->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
          if(ui->save_gene_frequencies->isChecked())
                 if(save_dir.mkpath("gene_freq/"))
                             pop_image_colour->save(QString(save_dir.path()+"/gene_freq/EvoSim_gene_freq_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    //RJG - No save option as no longer in the menu as an option.
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

    //RJG - No save option as no longer in the menu as an option.
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


    if (ui->actionSettles->isChecked()||ui->save_settles->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int value=(settles[n][m]*10)/RefreshRate;
            if (value>255) value=255;
            pop_image->setPixel(n,m,value);
        }

       if (ui->actionSettles->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image));
       if(ui->save_settles->isChecked())
               if(save_dir.mkpath("settles/"))
                           pop_image_colour->save(QString(save_dir.path()+"/settles/EvoSim_settles_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    if (ui->actionSettle_Fails->isChecked()||ui->save_fails_settles->isChecked())
    //this now combines breed fails (red) and settle fails (green)
    {
        //work out max and ratios
        /*int maxbf=1;
        int maxsf=1;
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            if (settlefails[n][m]>maxsf) maxsf=settlefails[n][m];
            if (breedfails[n][m]>maxsf) maxbf=breedfails[n][m];
        }
        float bf_mult=255.0 / (float)maxbf;
        float sf_mult=255.0 / (float)maxsf;
        qDebug()<<bf_mult<<sf_mult;
        bf_mult=1.0;
        sf_mult=1.0;
        */

        //work out average per generation
        float gens=generation-lastReport;


        //Make image
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int r=ScaleFails(breedfails[n][m],gens);
            int g=ScaleFails(settlefails[n][m],gens);
            pop_image_colour->setPixel(n,m,qRgb(r, g, 0));
        }
        if (ui->actionSettle_Fails->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
        if(ui->save_fails_settles->isChecked())
                if(save_dir.mkpath("settles_fails/"))
                            pop_image_colour->save(QString(save_dir.path()+"/settles_fails/EvoSim_settles_fails_it_%1.png").arg(generation, 7, 10, QChar('0')));

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

    lastReport=generation;
}

void MainWindow::RefreshEnvironment()
{

    QDir save_dir(path->text());

    for (int n=0; n<gridX; n++)
    for (int m=0; m<gridY; m++)
        env_image->setPixel(n,m,qRgb(environment[n][m][0], environment[n][m][1], environment[n][m][2]));

    env_item->setPixmap(QPixmap::fromImage(*env_image));
    if(ui->save_environment->isChecked())
        if(save_dir.mkpath("environment/"))
                    env_image->save(QString(save_dir.path()+"/environment/EvoSim_environment_it_%1.png").arg(generation, 7, 10, QChar('0')));

    //Draw on fossil records
    envscene->DrawLocations(FRW->FossilRecords,ui->actionShow_positions->isChecked());
}

void MainWindow::save_all(bool toggled)
{
    if (toggled)
    {
        ui->save_coding_genome_as_colour->setChecked(true);
        ui->save_environment->setChecked(true);
        ui->save_fails_settles->setChecked(true);
        ui->save_gene_frequencies->setChecked(true);
        ui->save_mean_fitness->setChecked(true);
        ui->save_non_coding_genome_as_colour->setChecked(true);
        ui->save_population_count->setChecked(true);
        ui->save_settles->setChecked(true);
        ui->save_species->setChecked(true);
    }
    else
    {
        ui->save_coding_genome_as_colour->setChecked(false);
        ui->save_environment->setChecked(false);
        ui->save_fails_settles->setChecked(false);
        ui->save_gene_frequencies->setChecked(false);
        ui->save_mean_fitness->setChecked(false);
        ui->save_non_coding_genome_as_colour->setChecked(false);
        ui->save_population_count->setChecked(false);
        ui->save_settles->setChecked(false);
        ui->save_species->setChecked(false);
    }
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

void MainWindow::species_mode_changed(QAction *temp2)
{
    int new_species_mode=SPECIES_MODE_NONE;
    if (ui->actionPhylogeny->isChecked()) new_species_mode=SPECIES_MODE_PHYLOGENY;
    if (ui->actionPhylogeny_metrics->isChecked()) new_species_mode=SPECIES_MODE_PHYLOGENY_AND_METRICS;
    if (ui->actionBasic->isChecked()) new_species_mode=SPECIES_MODE_BASIC;

    //some changes not allowed
    if (generation!=0)
    {
        //already running. Can switch tracking off - but not on
        //detailed tracking can be switched on/off at any point
        if (species_mode==SPECIES_MODE_NONE)
        {
            if (new_species_mode!=SPECIES_MODE_NONE)
            {
                QMessageBox::warning(this,"Error","Turning on species logging is not allowed mid-simulation");
                ui->actionOff->setChecked(true);
                return;
            }
        }

        if (species_mode==SPECIES_MODE_BASIC)
        {
            if (new_species_mode==SPECIES_MODE_PHYLOGENY || new_species_mode==SPECIES_MODE_PHYLOGENY_AND_METRICS)
            {
                QMessageBox::warning(this,"Error","Turning on phylogeny tracking is not allowed mid-simulation");
                ui->actionBasic->setChecked(true);
                return;
            }
        }
        //all other combinations are OK - hopefully
    }

    //uncheck species visualisation if needbe
    if (ui->actionOff->isChecked())
    {
        if (ui->actionSpecies->isChecked()) ui->actionGenome_as_colour->setChecked(true);
        ui->actionSpecies->setEnabled(false);
    }
    else ui->actionSpecies->setEnabled(true);

    species_mode=new_species_mode;
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


void MainWindow::ResizeImageObjects()
{
    delete pop_image;
    delete env_image;
    delete pop_image_colour;
    pop_image =new QImage(gridX, gridY, QImage::Format_Indexed8);
    QVector <QRgb> clut(256);
    for (int ic=0; ic<256; ic++) clut[ic]=qRgb(ic,ic,ic);
    pop_image->setColorTable(clut);

    env_image=new QImage(gridX, gridY, QImage::Format_RGB32);

    pop_image_colour=new QImage(gridX, gridY, QImage::Format_RGB32);
}

void MainWindow::on_actionSettings_triggered()
{
    //AutoMarkers options tab
    //Something like:

    int oldrows, oldcols;
    oldrows=gridX; oldcols=gridY;
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

        ResizeImageObjects();

        RefreshPopulations();
        RefreshEnvironment();
        Resize();
    }
}


void MainWindow::on_actionMisc_triggered()
{
    TheSimManager->testcode();
}

void MainWindow::on_actionCount_Peaks_triggered()
{
    HandleAnalysisTool(ANALYSIS_TOOL_CODE_COUNT_PEAKS);
}

bool  MainWindow::on_actionEnvironment_Files_triggered()
{
    //Select files
    QStringList files = QFileDialog::getOpenFileNames(
                            this,
                            "Select one or more image files to load in simulation environment...",
                            "",
                            "Images (*.png *.bmp)");

    if (files.length()==0) return false;

    bool notsquare=false, different_size=false;
    for(int i=0;i<files.length();i++)
        {
        QImage LoadImage(files[i]);
        int x=LoadImage.width();
        int y=LoadImage.height();
        if(x!=y)notsquare=true;
        if(x!=100||y!=100)different_size=true;
        }
        if(notsquare||different_size)QMessageBox::warning(this,"FYI","For speed EvoSim currently has static arrays for the environment, which limits out of the box functionality to 100 x 100 square environments. "
        "It looks like some of your Environment images don't meet this requirement. Anything smaller than 100 x 100 will be stretched (irrespective of aspect ratio) to 100x100. Anything bigger, and we'll use the top left corner. Should you wish to use a different size environment, please email RJG or MDS.");

    EnvFiles = files;
    CurrentEnvFile=0;
    int emode=0;
    if (ui->actionOnce->isChecked()) emode=1;
    if (ui->actionBounce->isChecked()) emode=3;
    if (ui->actionLoop->isChecked()) emode=2;
    TheSimManager->loadEnvironmentFromFile(emode);
    RefreshEnvironment();

    //---- RJG - Reset for this new environment
    TheSimManager->SetupRun();

    return true;
}

void MainWindow::on_actionChoose_Log_Directory_triggered()
{
    QString dirname = QFileDialog::getExistingDirectory(this,"Select directory to log fossil record to");


    if (dirname.length()==0) return;
    FRW->LogDirectory=dirname;
    FRW->LogDirectoryChosen=true;
    FRW->HideWarnLabel();

}

// ----RJG: Fitness logging to file not sorted on save as yet.
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
    outfile.open(QIODevice::WriteOnly|QIODevice::Text);

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
    if (ui->actionSpecies->isChecked()) vmode=10;

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

    //And some window state stuff
    out<<saveState(); //window state
    out<<ui->actionFossil_Record->isChecked();
    out<<ui->actionReport_Viewer->isChecked();
    out<<ui->actionGenomeComparison->isChecked();

    //interpolate environment stuff
    out<<ui->actionInterpolate->isChecked();
    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        out<<environmentlast[i][j][0];
        out<<environmentlast[i][j][1];
        out<<environmentlast[i][j][2];
    }
    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        out<<environmentnext[i][j][0];
        out<<environmentnext[i][j][1];
        out<<environmentnext[i][j][2];
    }

    out<<speciesSamples;
    out<<speciesSensitivity;
    out<<timeSliceConnect;
    out<<speciesLogging; //no longer used - compatibility only
    out<<speciesLoggingToFile; //no longer used - compatibility only
    out<<SpeciesLoggingFile;


    //now the species archive
    out<<oldspecieslist.count();
    for (int j=0; j<oldspecieslist.count(); j++)
    {
         out<<oldspecieslist[j].ID;
         out<<oldspecieslist[j].type;
         out<<oldspecieslist[j].origintime;
         out<<oldspecieslist[j].parent;
         out<<oldspecieslist[j].size;
         out<<oldspecieslist[j].internalID;
    }

    out<<archivedspecieslists.count();
    for (int i=0; i<archivedspecieslists.count(); i++)
    {
        out<<archivedspecieslists[i].count();
        for (int j=0; j<archivedspecieslists[i].count(); j++)
        {
             out<<archivedspecieslists[i][j].ID;
             out<<archivedspecieslists[i][j].type;
             out<<archivedspecieslists[i][j].origintime;
             out<<archivedspecieslists[i][j].parent;
             out<<archivedspecieslists[i][j].size;
             out<<archivedspecieslists[i][j].internalID;
        }
    }
    out<<nextspeciesid;
    out<<lastSpeciesCalc;

    //now random number array
    for (int i=0; i<65536; i++)
        out<<randoms[i];

    out<<recalcFitness; //extra new parameter

    out<<breeddiff;
    out<<breedspecies;

    out<<species_mode;

    outfile.close();
}

// ----RJG: Fitness logging to file not sorted on load as yet.
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
    if (vmode==10) ui->actionSpecies->setChecked(true);

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

    //interpolate environment stuff
    in>>btemp;
    ui->actionInterpolate->setChecked(btemp);

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        in>>environmentlast[i][j][0];
        in>>environmentlast[i][j][1];
        in>>environmentlast[i][j][2];
    }

    for (int i=0; i<gridX; i++)
    for (int j=0; j<gridY; j++)
    {
        in>>environmentnext[i][j][0];
        in>>environmentnext[i][j][1];
        in>>environmentnext[i][j][2];
    }

    if (!(in.atEnd())) in>>speciesSamples;
    if (!(in.atEnd())) in>>speciesSensitivity;
    if (!(in.atEnd())) in>>timeSliceConnect;
    if (!(in.atEnd())) in>>speciesLogging; //no longer used - keep for file compatibility
    if (!(in.atEnd())) in>>speciesLoggingToFile; //no longer used - keep for file compatibility
    if (!(in.atEnd())) in>>SpeciesLoggingFile;

    //if (speciesLogging) ui->actionTracking->setChecked(true); else ui->actionTracking->setChecked(false);
    //if (speciesLoggingToFile)  {ui->actionLogging->setChecked(true); ui->actionTracking->setEnabled(false);}  else {ui->actionLogging->setChecked(false); ui->actionTracking->setEnabled(true);}
    if (SpeciesLoggingFile!="") ui->actionLogging->setEnabled(true); else ui->actionLogging->setEnabled(false);


    //now the species archive
    archivedspecieslists.clear();
    oldspecieslist.clear();

    if (!(in.atEnd()))
    {
        int temp;
        in>>temp; //oldspecieslist.count();
        for (int j=0; j<temp; j++)
        {
             species s;
             in>>s.ID;
             in>>s.type;
             in>>s.origintime;
             in>>s.parent;
             in>>s.size;
             in>>s.internalID;
             oldspecieslist.append(s);
        }

        in>>temp; //archivedspecieslists.count();

        for (int i=0; i<temp; i++)
        {
            int temp2;
            in>>temp2; //archivedspecieslists.count();
            QList<species> ql;
            for (int j=0; j<temp2; j++)
            {
                species s;
                in>>s.ID;
                in>>s.type;
                in>>s.origintime;
                in>>s.parent;
                in>>s.size;
                in>>s.internalID;
                ql.append(s);
            }
            archivedspecieslists.append(ql);
        }
        in>>nextspeciesid;
        in>>lastSpeciesCalc; //actually no - if we import this it will assume an 'a' object exists.
        //bodge
        lastSpeciesCalc--;
    }

    //now random array
    if (!(in.atEnd()))
        for (int i=0; i<65536; i++)
            in>>randoms[i];

    if (!(in.atEnd()))
        in>>recalcFitness;

    if (!(in.atEnd()))
        in>>breeddiff;
    if (!(in.atEnd()))
        in>>breedspecies;

    species_mode=SPECIES_MODE_BASIC;
    if (!(in.atEnd()))
        in>>species_mode;
    if (rmode==0) ui->actionOff->setChecked(true);
    if (rmode==1) ui->actionSorted_Summary->setChecked(true);
    if (rmode==2) ui->actionGroups->setChecked(true);
    if (rmode==3) ui->actionGroups2->setChecked(true);
    if (rmode==4) ui->actionSimple_List->setChecked(true);


    infile.close();
    NextRefresh=0;
    ResizeImageObjects();
    Report();
    Resize();
}

//---- ARTS: Genome Comparison UI ----
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
    count=QInputDialog::getInt(this,"","Grid Density?",2,2,10,1,&ok);
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
    count=QInputDialog::getInt(this,"","Number of records to add?",1,1,100,1,&ok);
    if (!ok) return;
    for (int i=0; i<count; i++)
    {
        int rx=TheSimManager->portable_rand() % gridX;
        int ry=TheSimManager->portable_rand() % gridY;
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

void MainWindow::on_actionFitness_logging_to_File_triggered()
{
    fitnessLoggingToFile=ui->actionFitness_logging_to_File->isChecked();
}

/*
This is now obsolete, but retained in case we return to this approach
void MainWindow::on_actionSet_Logging_File_triggered()
{

    // ----RJG: set logging to a text file for greater versatility across operating systems and analysis programs (R, Excel, etc.)
    QString filename = QFileDialog::getSaveFileName(this,"Select file to log fossil record to","",".txt");
    if (filename.length()==0) return;
    QString filenamefitness(filename);

    // ----RJG: Add extension as Linux does not automatically
    if(filename.contains(".txt"))filename.insert(filename.length()-4,"_species");
    else filename.append("_species.txt");

    // ----RJG: Fitness logging
    if(filenamefitness.contains(".txt"))filenamefitness.insert(filenamefitness.length()-4,"_fitness");
    else filenamefitness.append("_fitness.txt");

    SpeciesLoggingFile=filename;
    FitnessLoggingFile=filenamefitness;

    ui->actionLogging->setEnabled(true);
    //ui->actionLogging->trigger();
    ui->actionFitness_logging_to_File->setEnabled(true);
    //ui->actionFitness_logging_to_File->trigger();


}*/


void MainWindow::on_actionGenerate_Tree_from_Log_File_triggered()
{
    HandleAnalysisTool(ANALYSIS_TOOL_CODE_GENERATE_TREE);
}


void MainWindow::on_actionRates_of_Change_triggered()
{
    HandleAnalysisTool(ANALYSIS_TOOL_CODE_RATES_OF_CHANGE);
}

void MainWindow::on_actionStasis_triggered()
{
    HandleAnalysisTool(ANALYSIS_TOOL_CODE_STASIS);
}

void MainWindow::on_actionExtinction_and_Origination_Data_triggered()
{
    HandleAnalysisTool(ANALYSIS_TOOL_CODE_EXTINCT_ORIGIN);
}

void MainWindow::CalcSpecies()
{
    if (species_mode==SPECIES_MODE_NONE) return; //do nothing!
    if (generation!=lastSpeciesCalc)
    {
        delete a;  //replace old analyser object with new
        a=new Analyser;

        //New species analyser
        a->Groups_2017();

        //OLDER CODE
        /*
        if (ui->actionSpecies->isChecked() || speciesLogging) //do species calcs here even if not showing species - unless turned off in settings
        {
            //set up species ID

            for (int n=0; n<gridX; n++)
            for (int m=0; m<gridY; m++)
            {
                if (totalfit[n][m]==0) continue;
                int found=0;
                for (int c=0; c<slotsPerSq; c++)
                {
                    if (critters[n][m][c].age>0)
                    {
                        a->AddGenome_Fast(critters[n][m][c].genome);
                        if ((++found)>=speciesSamples) break; //limit number sampled
                    }
                }
            }

            a->Groups_With_History_Modal();
            lastSpeciesCalc=generation;
        }
        */

    }
}


void MainWindow::WriteLog()
{
    if (speciesLoggingToFile==false && fitnessLoggingToFile==false) return;


    // ----RJG separated species logging from fitness logging
    //Now obsolete with new species system? Left here in case this is required again
    /*if (speciesLoggingToFile==true)
    {
        //log em!
        QFile outputfile(SpeciesLoggingFile);

            if (!(outputfile.exists()))
            {
                outputfile.open(QIODevice::WriteOnly|QIODevice::Text);
                QTextStream out(&outputfile);

                out<<"Time,Species_ID,Species_origin_time,Species_parent_ID,Species_current_size,Species_current_genome\n";
                outputfile.close();
            }

        outputfile.open(QIODevice::Append|QIODevice::Text);
        QTextStream out(&outputfile);

        for (int i=0; i<oldspecieslist.count(); i++)
        {
            out<<generation;
            out<<","<<(oldspecieslist[i].ID);
            out<<","<<oldspecieslist[i].origintime;
            out<<","<<oldspecieslist[i].parent;
            out<<","<<oldspecieslist[i].size;
            //out<<","<<oldspecieslist[i].type;
            //---- RJG - output binary genome if needed
            out<<",";
            for (int j=0; j<63; j++)
            if (tweakers64[63-j] & oldspecieslist[i].type) out<<"1"; else out<<"0";
            if (tweakers64[0] & oldspecieslist[i].type) out<<"1"; else out<<"0";
            out<<"\n";
        }

        outputfile.close();
      }*/

    // ----RJG log fitness to separate file
    if (fitnessLoggingToFile==true)
    {

        // else FitnessLoggingFile.replace(QString("_run_%1").arg(runs-1),QString("_run_%1").arg(runs));

        QString File(path->text()+"EvoSim_fitness");
        if(batch_running)
            File.append(QString("_run_%1").arg(runs, 4, 10, QChar('0')));
        File.append(".txt");
        QFile outputfile(File);

        if (!(outputfile.exists()))
        {
            outputfile.open(QIODevice::WriteOnly|QIODevice::Text);
            QTextStream out(&outputfile);

            // Info on simulation setup
            out<<"Slots Per square = "<<slotsPerSq;
            out<<"\n";

            //Different versions of output, for reuse as needed
            //out<<"Each generation lists, for each pixel: mean fitness, entries on breed list";
            //out<<"Each line lists generation, then the grid's: total critter number, total fitness, total entries on breed list";
            out<<"Each generation lists, for each pixel (top left to bottom right): total fitness, number of critters,entries on breed list\n\n";

           out<<"\n";
           outputfile.close();
        }

        outputfile.open(QIODevice::Append|QIODevice::Text);
        QTextStream out(&outputfile);

        // ----RJG: Breedattempts was no longer in use - but seems accurate, so can be co-opted for this.
        out<<"Iteration: "<<generation<<"\n";

        //int gridNumberAlive=0, gridTotalFitness=0, gridBreedEntries=0;

        for (int i=0; i<gridX; i++)
            {
                for (int j=0; j<gridY; j++)
                    {
                     //----RJG: Total fitness per grid square.
                     //out<<totalfit[i][j];

                    //----RJG: Number alive per square - output with +1 due to c numbering, zero is one critter, etc.
                    //out<<maxused[i][j]+1;
                    // ---- RJG: Note, however, there is an issue that when critters die they remain in cell list for iteration
                    // ---- RJG: Easiest to account for this by removing those which are dead from alive count, or recounting - rather than dealing with death system
                    // int numberalive=0;

                    //----RJG: In case mean is ever required:
                    //float mean=0;
                    // mean = (float)totalfit[i][j]/(float)maxused[i][j]+1;

                    //----RJG: Manually calculate total fitness for grid
                    //gridTotalFitness+=totalfit[i][j];

                    int critters_alive=0;

                     //----RJG: Manually count number alive thanks to maxused issue
                    for  (int k=0; k<slotsPerSq; k++)if(critters[i][j][k].fitness)
                                    {
                                    //numberalive++;
                                    //gridNumberAlive++;
                                    critters_alive++;
                                    }

                    //total fitness, number of critters,entries on breed list";
                    out<<totalfit[i][j]<<" "<<critters_alive<<" "<<breedattempts[i][j];

                    //----RJG: Manually count breed attempts for grid
                    //gridBreedEntries+=breedattempts[i][j];

                    out<<"\n";

                    }
            }

       out<<"\n\n";

        //---- RJG: If outputting averages to log.
        //float avFit=(float)gridTotalFitness/(float)gridNumberAlive;
        //float avBreed=(float)gridBreedEntries/(float)gridNumberAlive;
        //out<<avFit<<","<<avBreed;

        //---- RJG: If outputting totals
        //critter - fitness - breeds
        //out<<gridNumberAlive<<"\t"<<gridTotalFitness<<"\t"<<gridBreedEntries<<"\n";

        outputfile.close();
      }
}
void MainWindow::setStatusBarText(QString text)
{
    ui->statusBar->showMessage(text);
}

void MainWindow::on_actionLoad_Random_Numbers_triggered()
{
    // ---- RJG - have added randoms to resources and into constructor, load on launch to ensure true randoms are loaded by default.
    //Select files
    QString file = QFileDialog::getOpenFileName(
                            this,
                            "Select random number file",
                            "",
                            "*.*");

    if (file.length()==0) return;

    int seedoffset;
    seedoffset=QInputDialog::getInt(this,"Seed value","Byte offset to start reading from (will read 65536 bytes)");

    //now read in values to array
    QFile rfile(file);
    rfile.open(QIODevice::ReadOnly);

    rfile.seek(seedoffset);

    int i=rfile.read((char *)randoms,65536);
    if (i!=65536) QMessageBox::warning(this,"Oops","Failed to read 65536 bytes from file - random numbers may be compromised - try again or restart program");
    else QMessageBox::information(this,"Success","New random numbers read successfully");
}

void MainWindow::on_SelectLogFile_pressed()
{
    QString filename = QFileDialog::getOpenFileName(this,"Select log file","","*.csv");
    if (filename.length()==0) return;

    ui->LogFile->setText(filename);
}

void MainWindow::HandleAnalysisTool(int code)
{
    //Tidied up a bit - MDS 14/9/2017
    //do filenames
    //Is there a valid input file?

    AnalysisTools a;
    QString OutputString, FilenameString;

    if (a.doesthiscodeneedafile(code))
    {
        QFile f(ui->LogFile->text());
        if (!(f.exists()))
        {
            QMessageBox::warning(this,"Error","No valid input file set");
            return;
        }
    }

    switch (code)
    {
        //sort filenames here
        case ANALYSIS_TOOL_CODE_GENERATE_TREE:
            OutputString = a.GenerateTree(ui->LogFile->text()); //deprecated - log file generator now removed
            break;

        case ANALYSIS_TOOL_CODE_RATES_OF_CHANGE:
            OutputString = a.SpeciesRatesOfChange(ui->LogFile->text());
            break;

        case ANALYSIS_TOOL_CODE_EXTINCT_ORIGIN:
            OutputString = a.ExtinctOrigin(ui->LogFile->text());
            break;

        case ANALYSIS_TOOL_CODE_STASIS:
            OutputString = a.Stasis(ui->LogFile->text(),ui->StasisBins->value()
                                ,((float)ui->StasisPercentile->value())/100.0,ui->StasisQualify->value());
            break;

        case ANALYSIS_TOOL_CODE_COUNT_PEAKS:
            OutputString = a.CountPeaks(ui->PeaksRed->value(),ui->PeaksGreen->value(),ui->PeaksBlue->value());
            break;

        case ANALYSIS_TOOL_CODE_MAKE_NEWICK:
            if (ui->actionPhylogeny_metrics->isChecked()||ui->actionPhylogeny->isChecked())OutputString = a.MakeNewick(rootspecies, ui->minSpeciesSize->value(), ui->chkExcludeWithChildren->isChecked());
            else OutputString = "Species tracking is not enabled.";
            FilenameString = "_newick";
            break;

        case ANALYSIS_TOOL_CODE_DUMP_DATA:
            if (ui->actionPhylogeny_metrics->isChecked())OutputString = a.DumpData(rootspecies, ui->minSpeciesSize->value(), ui->chkExcludeWithChildren->isChecked());
            else OutputString = "Species tracking is not enabled.";
            FilenameString = "_specieslog";
            break;

        default:
            QMessageBox::warning(this,"Error","No handler for analysis tool");
            return;

    }

    //write result to screen
    ui->plainTextEdit->clear();
    ui->plainTextEdit->appendPlainText(OutputString);

    //RJG - Make file here
    //and attempt to write to file
    if (FilenameString.length()>1) //i.e. if not blank
    {

        QString File(path->text()+"EvoSim"+FilenameString);
        if(batch_running)
            File.append(QString("_run_%1").arg(runs, 4, 10, QChar('0')));
        File.append(".txt");

        QFile o(File);

        if (!o.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::warning(this,"Error","Could not open output file for writing");
            return;
        }

        QTextStream out(&o);
        out<<OutputString;
        o.close();
    }
}

void MainWindow::on_actionGenerate_NWK_tree_file_triggered()
{
    HandleAnalysisTool(ANALYSIS_TOOL_CODE_MAKE_NEWICK);
}

void MainWindow::on_actionSpecies_sizes_triggered()
{
    HandleAnalysisTool(ANALYSIS_TOOL_CODE_DUMP_DATA);
}
