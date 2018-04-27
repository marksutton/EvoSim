/**
 * @file
 * Main Window
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "reseed.h"
#include "analyser.h"
#include "fossrecwidget.h"
#include "resizecatcher.h"

#include <QTextStream>
#include <QInputDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGraphicsPixmapItem>
#include <QDockWidget>
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QFormLayout>
#include <QStringList>
#include <QMessageBox>
#include <QActionGroup>
#include <QDataStream>
#include <QStringList>
#include <QFile>
#include <QXmlStreamReader>
#include <QDesktopServices>

#include "analysistools.h"
#include "version.h"
#include "math.h"

#ifndef M_SQRT1_2 //not defined in all versions
#define M_SQRT1_2 0.7071067811865475
#endif

SimManager *TheSimManager;
MainWindow *MainWin;

#include <QThread>

/*

To do for paper:

Coding:
-- Load and Save don't include everything - they need to!
-- Timer on calculting species - add progress bar and escape warning if needed to prevent crash
-- Add Keyboard shortcuts where required

Visualisation:
-- Settles - does this work at all?
-- Fails - check green scaling

To remove prior to release, but after all changes are complete:
-- Dual reseed
-- Remove all custom logging options (RJG is using these for research and projects, so needs to keep them in master fork).
-- Fossil record - down the line we need to work out that this actually does check it actually works. For release, just remove option
-- Entirely lose the Analysis Tools menu, and analysis docker. Phylogeny settings will become part of settings.


To do in revisions:
-- Further comment code
-- Standardise case throughout the code, and also variable names in a sensible fashion
-- Rename "generations" variable "iterations", which is more correct

To do long term:
-- Add variable mutation rate depent on population density:
---- Count number of filled slots (do as percentage of filled slots)
---- Use percentage to dictate probability of mutation (between 0 and 1), following standard normal distribution
---- But do this both ways around so really full mutation rate can be either very high, or very low

*/


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

    //RJG - Output version, but also date compiled for clarity
    QString version;
    version.sprintf("%d.%d.%d",MAJORVERSION,MINORVERSION,PATCHVERSION);
    setWindowTitle(QString(PRODUCTNAME)+" v"+version+" - compiled - "+__DATE__);
    setWindowIcon(QIcon (":/icon.png"));

    //Install filter to catch resize events to central widget and deliver to mainwindow (handle dock resizes)
    ResizeCatcher *rescatch = new ResizeCatcher(this);
    ui->centralWidget->installEventFilter(rescatch);

    //ARTS - Toolbar buttons
    //RJG - docker toggles
    startButton = new QAction(QIcon(QPixmap(":/toolbar/startButton-Enabled.png")), QString("Run"), this);
    runForButton = new QAction(QIcon(QPixmap(":/toolbar/runForButton-Enabled.png")), QString("Run for..."), this);
    runForBatchButton = new QAction(QIcon(QPixmap(":/toolbar/runForBatchButton-Enabled.png")), QString("Batch..."), this);
    pauseButton = new QAction(QIcon(QPixmap(":/toolbar/pauseButton-Enabled.png")), QString("Pause"), this);
    stopButton = new QAction(QIcon(QPixmap(":/toolbar/stopButton-Enabled.png")), QString("Stop"), this);
    resetButton = new QAction(QIcon(QPixmap(":/toolbar/resetButton-Enabled.png")), QString("Reset"), this);
    reseedButton = new QAction(QIcon(QPixmap(":/toolbar/resetButton_knowngenome-Enabled.png")), QString("Reseed"), this);
    settingsButton = new QAction(QIcon(QPixmap(":/toolbar/globesettingsButton-Enabled-white.png")), QString("Settings"), this);
    aboutButton = new QAction(QIcon(QPixmap(":/toolbar/aboutButton-Enabled-white.png")), QString("About"), this);

    //ARTS - Toolbar default settings
    //RJG - docker toggles defaults
    startButton->setEnabled(false);
    runForButton->setEnabled(false);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
    reseedButton->setEnabled(false);
    runForBatchButton->setEnabled(false);
    settingsButton->setCheckable(true);

    //ARTS - Toolbar layout
    ui->toolBar->addAction(startButton);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(runForButton);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(runForBatchButton);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(pauseButton);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(stopButton);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(resetButton);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(reseedButton);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(settingsButton);

    //Spacer
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    empty->setMaximumWidth(10);
    empty->setMaximumHeight(5);
    ui->toolBar->addWidget(empty);
    ui->toolBar->addSeparator();

    ui->toolBar->addAction(aboutButton);

    //RJG - Connect button signals to slot.
    //Note for clarity:
    //Reset = start again with random individual.
    //Reseed = start again with user defined genome
    QObject::connect(startButton, SIGNAL(triggered()), this, SLOT(on_actionStart_Sim_triggered()));
    QObject::connect(runForButton, SIGNAL(triggered()), this, SLOT(on_actionRun_for_triggered()));
    QObject::connect(pauseButton, SIGNAL(triggered()), this, SLOT(on_actionPause_Sim_triggered()));
    QObject::connect(stopButton, SIGNAL(triggered()), this, SLOT(on_actionStop_Sim_triggered()));
    QObject::connect(resetButton, SIGNAL(triggered()), this, SLOT(on_actionReset_triggered()));
    QObject::connect(reseedButton, SIGNAL(triggered()), this, SLOT(on_actionReseed_triggered()));
    QObject::connect(runForBatchButton, SIGNAL(triggered()), this, SLOT(on_actionBatch_triggered()));
    QObject::connect(settingsButton, SIGNAL(triggered()), this, SLOT(on_actionSettings_triggered()));
    QObject::connect(aboutButton, SIGNAL (triggered()), this, SLOT (on_actionAbout_triggered()));

    QObject::connect(ui->actionSave_settings, SIGNAL (triggered()), this, SLOT (save_settings()));
    QObject::connect(ui->actionLoad_settings, SIGNAL (triggered()), this, SLOT (load_settings()));
    QObject::connect(ui->actionCount_peaks, SIGNAL(triggered()), this, SLOT(on_actionCount_Peaks_triggered()));

    //RJG - set up settings docker.
    simulationSettingsDock = createSimulationSettingsDock();
    //----RJG - second settings docker.
    organismSettingsDock = createOrganismSettingsDock();
    //RJG - Third settings docker
    outputSettingsDock = createOutputSettingsDock();

    simulationSettingsDock->setObjectName("simulationSettingsDock");
    organismSettingsDock->setObjectName("organismSettingsDock");
    outputSettingsDock->setObjectName("outputSettingsDock");

    //RJG - Make docks tabbed
    tabifyDockWidget(organismSettingsDock,simulationSettingsDock);
    tabifyDockWidget(simulationSettingsDock,outputSettingsDock);

    //ARST - Hide docks by default
    organismSettingsDock->hide();
    simulationSettingsDock->hide();
    outputSettingsDock->hide();

    //ARTS - Add Genome Comparison UI
    ui->genomeComparisonDock->hide();
    genoneComparison = new GenomeComparison;
    QVBoxLayout *genomeLayout = new QVBoxLayout;
    genomeLayout->addWidget(genoneComparison);
    ui->genomeComparisonContent->setLayout(genomeLayout);

    //MDS - as above for fossil record dock and report dock
    ui->fossRecDock->hide();
    FRW = new FossRecWidget();
    QVBoxLayout *frwLayout = new QVBoxLayout;
    frwLayout->addWidget(FRW);
    ui->fossRecDockContents->setLayout(frwLayout);
    ui->reportViewerDock->hide();

    viewgroup2 = new QActionGroup(this);
    // These actions were created via qt designer
    viewgroup2->addAction(ui->actionNone);
    viewgroup2->addAction(ui->actionSorted_Summary);
    viewgroup2->addAction(ui->actionGroups);
    viewgroup2->addAction(ui->actionGroups2);
    viewgroup2->addAction(ui->actionSimple_List);

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

    //ARTS - Population Window dropdown must be after settings dock setup
    // 0 = Population count
    // 1 = Mean fitnessFails (R-Breed, G=Settle)
    // 2 = Coding genome as colour
    // 3 = NonCoding genome as colour
    // 4 = Gene Frequencies
    // 5 = Breed Attempts
    // 6 = Breed Fails
    // 7 = Settles
    // 8 = Settle Fails
    // 9 = Breed Fails 2
    // 10 = Species
    ui->populationWindowComboBox->addItem("Population count",QVariant(0));
    ui->populationWindowComboBox->addItem("Mean fitnessFails (R-Breed, G=Settle)",QVariant(1));
    ui->populationWindowComboBox->addItem("Coding genome as colour",QVariant(2));
    ui->populationWindowComboBox->addItem("NonCoding genome as colour",QVariant(3));
    ui->populationWindowComboBox->addItem("Gene Frequencies",QVariant(4));
    //ui->populationWindowComboBox->addItem("Breed Attempts",QVariant(5));
    //ui->populationWindowComboBox->addItem("Breed Fails",QVariant(6));
    ui->populationWindowComboBox->addItem("Settles",QVariant(7));
    ui->populationWindowComboBox->addItem("Settle Fails",QVariant(8));
    //ui->populationWindowComboBox->addItem("Breed Fails 2",QVariant(9));
    ui->populationWindowComboBox->addItem("Species",QVariant(10));

    //ARTS -Population Window dropdown set current index. Note this value is the index not the data value.
    ui->populationWindowComboBox->setCurrentIndex(2);

    TheSimManager = new SimManager;

    pauseflag = false;

    //RJG - load default environment image to allow program to run out of box (quicker for testing)
    EnvFiles.append(":/EvoSim_default_env.png");
    CurrentEnvFile=0;
    TheSimManager->loadEnvironmentFromFile(1);

    FinishRun();//sets up enabling
    TheSimManager->SetupRun();
    NextRefresh=0;
    Report();

    //RJG - Set batch variables
    batch_running=false;
    runs=-1;
    batch_iterations=-1;
    batch_target_runs=-1;

    showMaximized();

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
    float x=-3., inc=(6./32.);
    for(int cnt=0;cnt<33;cnt++)
    {
        double NSDF=(0.5 * erfc(-(x) * M_SQRT1_2));
        cumulative_normal_distribution[cnt]=4294967296*NSDF;
        x+=inc;
    }

    //RJG - fill pathogen probability distribution as required so pathogens can kill critters
    //Start with linear, may want to change down the line.
    for(int cnt=0;cnt<65;cnt++)
        pathogen_prob_distribution[cnt]=(4294967296/2)+(cnt*(4294967295/128));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete TheSimManager;
}

QDockWidget *MainWindow::createSimulationSettingsDock()
{
    simulationSettingsDock = new QDockWidget("Simulation", this);
    simulationSettingsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    simulationSettingsDock->setFeatures(QDockWidget::DockWidgetMovable);
    simulationSettingsDock->setFeatures(QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, simulationSettingsDock);

    QGridLayout *settings_grid = new QGridLayout;
    settings_grid->setAlignment(Qt::AlignTop);

    // Environment Settings
    QGridLayout *environmentSettingsGrid = new QGridLayout;

    QLabel *environment_label= new QLabel("Environmental Settings");
    environment_label->setStyleSheet("font-weight: bold");
    environmentSettingsGrid->addWidget(environment_label,0,1,1,2);

    QPushButton *changeEnvironmentFilesButton = new QPushButton("&Change Enviroment Files");
    changeEnvironmentFilesButton->setObjectName("changeEnvironmentFilesButton");
    changeEnvironmentFilesButton->setToolTip("<font>REvoSim allow you to customise the enviroment by loading one or more image files.</font>");
    environmentSettingsGrid->addWidget(changeEnvironmentFilesButton,1,1,1,2);
    connect(changeEnvironmentFilesButton, SIGNAL (clicked()), this, SLOT(on_actionEnvironment_Files_triggered()));

    QLabel *environment_rate_label = new QLabel("Environment refresh rate:");
    environment_rate_label->setToolTip("<font>This is the rate of change for the selected enviromental images.</font>");
    environment_rate_spin = new QSpinBox;
    environment_rate_spin->setToolTip("<font>This is the rate of change for the selected enviromental images.</font>");
    environment_rate_spin->setMinimum(0);
    environment_rate_spin->setMaximum(100000);
    environment_rate_spin->setValue(envchangerate);
    environmentSettingsGrid->addWidget(environment_rate_label,2,1);
    environmentSettingsGrid->addWidget(environment_rate_spin,2,2);
    //RJG - Note in order to use a lamda not only do you need to use C++11, but there are two valueChanged signals for spinbox - and int and a string. Need to cast it to an int
    connect(environment_rate_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i ) { envchangerate=i; });

    QLabel *environment_mode_label = new QLabel("Environment mode:");
    environmentSettingsGrid->addWidget(environment_mode_label,3,1,1,2);

    QGridLayout *environmentModeGrid = new QGridLayout;
    environmentModeStaticButton = new QRadioButton("Static");
    environmentModeStaticButton->setToolTip("<font>'Static' uses a single environment image.</font>");
    environmentModeOnceButton = new QRadioButton("Once");
    environmentModeOnceButton->setToolTip("<font>'Once' uses each image only once, simulation stops after last image.</font>");
    environmentModeLoopButton = new QRadioButton("Loop");
    environmentModeLoopButton->setToolTip("<font>'Loop' uses each image in order in a loop</font>");
    environmentModeBounceButton = new QRadioButton("Bounce");
    environmentModeBounceButton->setToolTip("<font>'Bounce' rebounds between the first and last image in a loop.</font>");
    QButtonGroup* environmentModeButtonGroup = new QButtonGroup;
    environmentModeButtonGroup->addButton(environmentModeStaticButton,ENV_MODE_STATIC);
    environmentModeButtonGroup->addButton(environmentModeOnceButton,ENV_MODE_ONCE);
    environmentModeButtonGroup->addButton(environmentModeLoopButton,ENV_MODE_LOOP);
    environmentModeButtonGroup->addButton(environmentModeBounceButton,ENV_MODE_BOUNCE);
    environmentModeLoopButton->setChecked(true);
    environmentModeGrid->addWidget(environmentModeStaticButton,1,1,1,2);
    environmentModeGrid->addWidget(environmentModeOnceButton,1,2,1,2);
    environmentModeGrid->addWidget(environmentModeLoopButton,2,1,1,2);
    environmentModeGrid->addWidget(environmentModeBounceButton,2,2,1,2);
    connect(environmentModeButtonGroup, (void(QButtonGroup::*)(int))&QButtonGroup::buttonClicked,[=](const int &i) { environment_mode_changed(i,false); });
    environmentSettingsGrid->addLayout(environmentModeGrid,4,1,1,2);

    interpolateCheckbox = new QCheckBox("Interpolate between images");
    interpolateCheckbox->setChecked(enviroment_interpolate);
    interpolateCheckbox->setToolTip("<font>Turning this ON will iterpolate the environment between individual images.</font>");
    environmentSettingsGrid->addWidget(interpolateCheckbox,5,1,1,2);
    connect(interpolateCheckbox,&QCheckBox::stateChanged,[=](const bool &i) { enviroment_interpolate=i; });

    toroidal_checkbox = new QCheckBox("Toroidal enviroment");
    toroidal_checkbox->setChecked(toroidal);
    toroidal_checkbox->setToolTip("<font>Turning this ON will allow dispersal of progeny in an unbounded warparound enviroment. Progeny leaving one side of the population window will immediately reappear on the opposite side.</font>");
    environmentSettingsGrid->addWidget(toroidal_checkbox,6,1,1,2);
    connect(toroidal_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { toroidal=i; });

    // Simulation Size Settings
    QGridLayout *simulationSizeSettingsGrid = new QGridLayout;

    QLabel *simulation_size_label= new QLabel("Simulation size");
    simulation_size_label->setStyleSheet("font-weight: bold");
    simulationSizeSettingsGrid->addWidget(simulation_size_label,0,1,1,2);

    QLabel *gridX_label = new QLabel("Grid X:");
    gridX_spin = new QSpinBox;
    gridX_spin->setMinimum(1);
    gridX_spin->setMaximum(256);
    gridX_spin->setValue(gridX);
    simulationSizeSettingsGrid->addWidget(gridX_label,2,1);
    simulationSizeSettingsGrid->addWidget(gridX_spin,2,2);
    connect(gridX_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { int oldrows=gridX; gridX=i;redoImages(oldrows,gridY);});

    QLabel *gridY_label = new QLabel("Grid Y:");
    gridY_spin = new QSpinBox;
    gridY_spin->setMinimum(1);
    gridY_spin->setMaximum(256);
    gridY_spin->setValue(gridY);
    simulationSizeSettingsGrid->addWidget(gridY_label,3,1);
    simulationSizeSettingsGrid->addWidget(gridY_spin,3,2);
    connect(gridY_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {int oldcols=gridY; gridY=i;redoImages(gridX,oldcols);});

    QLabel *slots_label = new QLabel("Slots:");
    slots_spin = new QSpinBox;
    slots_spin->setMinimum(1);
    slots_spin->setMaximum(256);
    slots_spin->setValue(slotsPerSq);
    simulationSizeSettingsGrid->addWidget(slots_label,4,1);
    simulationSizeSettingsGrid->addWidget(slots_spin,4,2);
    connect(slots_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { slotsPerSq=i;redoImages(gridX,gridY); });

    // Simulation Settings
    QGridLayout *simulationSettingsGrid = new QGridLayout;

    QLabel *simulation_settings_label= new QLabel("Simulation settings");
    simulation_settings_label->setStyleSheet("font-weight: bold");
    simulationSettingsGrid->addWidget(simulation_settings_label,0,1,1,2);

    QLabel *target_label = new QLabel("Fitness target:");
    target_spin = new QSpinBox;
    target_spin->setMinimum(1);
    target_spin->setMaximum(96);
    target_spin->setValue(target);
    simulationSettingsGrid->addWidget(target_label,1,1);
    simulationSettingsGrid->addWidget(target_spin,1,2);
    connect(target_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { target=i; });

    QLabel *energy_label = new QLabel("Energy input:");
    energy_spin = new QSpinBox;
    energy_spin->setMinimum(1);
    energy_spin->setMaximum(20000);
    energy_spin->setValue(food);
    simulationSettingsGrid->addWidget(energy_label,2,1);
    simulationSettingsGrid->addWidget(energy_spin,2,2);
    connect(energy_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { food=i; });

    QLabel *settleTolerance_label = new QLabel("Settle tolerance:");
    settleTolerance_spin = new QSpinBox;
    settleTolerance_spin->setMinimum(1);
    settleTolerance_spin->setMaximum(30);
    settleTolerance_spin->setValue(settleTolerance);
    simulationSettingsGrid->addWidget(settleTolerance_label,3,1);
    simulationSettingsGrid->addWidget(settleTolerance_spin,3,2);
    connect(settleTolerance_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { settleTolerance=i; });

    recalcFitness_checkbox = new QCheckBox("Recalculate fitness");
    recalcFitness_checkbox->setChecked(toroidal);
    simulationSettingsGrid->addWidget(recalcFitness_checkbox,4,1,1,2);
    connect(recalcFitness_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { recalcFitness=i; });

    //Phylogeny Settings
    QGridLayout *phylogenySettingsGrid = new QGridLayout;

    QLabel *phylogeny_settings_label= new QLabel("Phylogeny settings");
    phylogeny_settings_label->setStyleSheet("font-weight: bold");
    phylogenySettingsGrid->addWidget(phylogeny_settings_label,0,1,1,1);

    QGridLayout *phylogeny_grid = new QGridLayout;
    phylogeny_off_button = new QRadioButton("Off");
    basic_phylogeny_button = new QRadioButton("Basic");
    phylogeny_button = new QRadioButton("Phylogeny");
    phylogeny_and_metrics_button = new QRadioButton("Phylogeny and metrics");
    QButtonGroup* phylogeny_button_group = new QButtonGroup;
    phylogeny_button_group->addButton(phylogeny_off_button,SPECIES_MODE_NONE);
    phylogeny_button_group->addButton(basic_phylogeny_button,SPECIES_MODE_BASIC);
    phylogeny_button_group->addButton(phylogeny_button,SPECIES_MODE_PHYLOGENY);
    phylogeny_button_group->addButton(phylogeny_and_metrics_button,SPECIES_MODE_PHYLOGENY_AND_METRICS);
    basic_phylogeny_button->setChecked(true);
    phylogeny_grid->addWidget(phylogeny_off_button,1,1,1,2);
    phylogeny_grid->addWidget(basic_phylogeny_button,1,2,1,2);
    phylogeny_grid->addWidget(phylogeny_button,2,1,1,2);
    phylogeny_grid->addWidget(phylogeny_and_metrics_button,2,2,1,2);
    connect(phylogeny_button_group, (void(QButtonGroup::*)(int))&QButtonGroup::buttonClicked,[=](const int &i) { species_mode_changed(i); });
    phylogenySettingsGrid->addLayout(phylogeny_grid,1,1,1,2);

    //ARTS - Dock Grid Layout
    settings_grid->addLayout(environmentSettingsGrid,0,1);
    settings_grid->addLayout(simulationSizeSettingsGrid,1,1);
    settings_grid->addLayout(simulationSettingsGrid,2,1);
    settings_grid->addLayout(phylogenySettingsGrid,3,1);

    QWidget *settings_layout_widget = new QWidget;
    settings_layout_widget->setLayout(settings_grid);
    settings_layout_widget->setMinimumWidth(300);
    simulationSettingsDock->setWidget(settings_layout_widget);
    simulationSettingsDock->adjustSize();

    return simulationSettingsDock;
}

QDockWidget *MainWindow::createOutputSettingsDock()
{
    outputSettingsDock = new QDockWidget("Output", this);
    outputSettingsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    outputSettingsDock->setFeatures(QDockWidget::DockWidgetMovable);
    outputSettingsDock->setFeatures(QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, outputSettingsDock);

    QGridLayout *output_settings_grid = new QGridLayout;
    output_settings_grid->setAlignment(Qt::AlignTop);

    //ARTS - Output Save Path
    QGridLayout *savePathGrid = new QGridLayout;
    QLabel *savePathLabel = new QLabel("Output save path");
    savePathLabel->setObjectName("savePathLabel");
    savePathGrid->addWidget(savePathLabel,1,1,1,2);
    QString program_path(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    program_path.append("/");
    path = new QLineEdit(program_path);
    savePathGrid->addWidget(path,2,1,1,2);
    QPushButton *changePathButton = new QPushButton("&Change");
    changePathButton->setObjectName("changePathButton");
    savePathGrid->addWidget(changePathButton,3,1,1,2);
    connect(changePathButton, SIGNAL (clicked()), this, SLOT(changepath_triggered()));

    //ARTS - Refresh/Polling Rate
    QGridLayout *pollingRateGrid = new QGridLayout;
    QLabel *pollingRateLabel = new QLabel("Refresh/Polling Rate");
    pollingRateLabel->setObjectName("pollingRateLabel");
    pollingRateGrid->addWidget(pollingRateLabel,1,1,1,2);

    RefreshRate=50;
    QLabel *refreshRateLabel = new QLabel("Refresh/polling rate:");
    refreshRateLabel->setObjectName("refreshRateLabel");
    refreshRateSpin = new QSpinBox;
    refreshRateSpin->setMinimum(1);
    refreshRateSpin->setMaximum(10000);
    refreshRateSpin->setValue(RefreshRate);
    pollingRateGrid->addWidget(refreshRateLabel,2,1);
    pollingRateGrid->addWidget(refreshRateSpin,2,2);
    connect(refreshRateSpin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { RefreshRate=i; });

    //ARTS - Logging: Population & Environment
    QGridLayout *images_grid = new QGridLayout;

    QLabel *imagesLabel= new QLabel("Logging: Population/Enivronment");
    imagesLabel->setObjectName("imagesLabel");
    images_grid->addWidget(imagesLabel,1,1,1,2);

    QLabel *imagesInfoLabel= new QLabel("Turn on/off these logging options to save images of the population/environment windows every refresh/poll.");
    imagesInfoLabel->setObjectName("imagesInfoLabel");
    imagesInfoLabel->setWordWrap(true);
    images_grid->addWidget(imagesInfoLabel,2,1,1,2);

    save_population_count = new QCheckBox("Population count");
    images_grid->addWidget(save_population_count,3,1,1,1);
    save_mean_fitness = new QCheckBox("Mean fitness");
    images_grid->addWidget(save_mean_fitness,3,2,1,1);
    save_coding_genome_as_colour = new QCheckBox("Coding genome");
    images_grid->addWidget(save_coding_genome_as_colour,4,1,1,1);
    save_non_coding_genome_as_colour = new QCheckBox("Noncoding genome");
    images_grid->addWidget(save_non_coding_genome_as_colour,4,2,1,1);
    save_species = new QCheckBox("Species");
    images_grid->addWidget(save_species,5,1,1,1);
    save_gene_frequencies = new QCheckBox("Gene frequencies");
    images_grid->addWidget(save_gene_frequencies,5,2,1,1);
    save_settles = new QCheckBox("Settles");
    images_grid->addWidget(save_settles,6,1,1,1);
    save_fails_settles = new QCheckBox("Fails + settles");
    images_grid->addWidget(save_fails_settles,6,2,1,1);
    save_environment = new QCheckBox("Environment");
    images_grid->addWidget(save_environment,7,1,1,1);

    QCheckBox *saveAllImagesCheckbox = new QCheckBox("All");
    saveAllImagesCheckbox->setObjectName("saveAllImagesCheckbox");
    images_grid->addWidget(saveAllImagesCheckbox,7,2,1,1);
    QObject::connect(saveAllImagesCheckbox, SIGNAL (toggled(bool)), this, SLOT(save_all_checkbox_state_changed(bool)));


    //ARTS - Logging to text file
    QGridLayout *fileLoggingGrid = new QGridLayout;

    QLabel *outputSettingsLabel= new QLabel("Logging: To Text File(s)");
    outputSettingsLabel->setObjectName("outputSettingsLabel");
    fileLoggingGrid->addWidget(outputSettingsLabel,1,1,1,2);

    QLabel *textLogInfoLabel= new QLabel("Turn on/off this option to write to a text log file every refresh/poll.");
    textLogInfoLabel->setObjectName("textLogInfoLabel");
    textLogInfoLabel->setWordWrap(true);
    fileLoggingGrid->addWidget(textLogInfoLabel,2,1,1,2);

    logging_checkbox = new QCheckBox("Write Text Log Files");
    logging_checkbox->setChecked(logging);
    fileLoggingGrid->addWidget(logging_checkbox,3,1,1,2);
    connect(logging_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { logging=i; });

    QLabel *textLogInfo1Label= new QLabel("After a batched run has finished a more detailed log file (includding trees) can be automatically created.");
    textLogInfo1Label->setObjectName("textLogInfo1Label");
    textLogInfo1Label->setWordWrap(true);
    fileLoggingGrid->addWidget(textLogInfo1Label,4,1,1,2);

    autodump_checkbox= new QCheckBox("Create automatically detailed log on batch runs");
    autodump_checkbox->setChecked(true);
    fileLoggingGrid->addWidget(autodump_checkbox,5,1,1,2);

    QLabel *textLogInfo2Label= new QLabel("...you can also manually create this detailed log file after any run.");
    textLogInfo2Label->setObjectName("textLogInfo2Label");
    textLogInfo2Label->setWordWrap(true);
    fileLoggingGrid->addWidget(textLogInfo2Label,6,1,1,2);

    QPushButton *dump_nwk = new QPushButton("Write data (including tree) for current run");
    fileLoggingGrid->addWidget(dump_nwk,7,1,1,2);
    connect(dump_nwk , SIGNAL (clicked()), this, SLOT(dump_run_data()));

    QLabel *textLogInfo3Label= new QLabel("More advanced options on what is included in the log files:");
    textLogInfo3Label->setObjectName("textLogInfo3Label");
    textLogInfo3Label->setWordWrap(true);
    fileLoggingGrid->addWidget(textLogInfo3Label,8,1,1,2);

    exclude_without_issue_checkbox = new QCheckBox("Exclude species without issue");
    exclude_without_issue_checkbox->setChecked(allowexcludewithissue);
    fileLoggingGrid->addWidget(exclude_without_issue_checkbox,9,1,1,1);
    connect(exclude_without_issue_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { allowexcludewithissue=i; });

    QLabel *Min_species_size_label = new QLabel("Minimum species size:");
    QSpinBox *Min_species_size_spin = new QSpinBox;
    Min_species_size_spin->setMinimum(0);
    Min_species_size_spin->setMaximum(10000);
    Min_species_size_spin->setValue(minspeciessize);
    fileLoggingGrid->addWidget(Min_species_size_label,10,1);
    fileLoggingGrid->addWidget(Min_species_size_spin,10,2);
    connect(Min_species_size_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { minspeciessize=i; });

    //ARTS - Advanced
    QGridLayout *advancedLoggingGrid = new QGridLayout;

    QLabel *advancedSettingsLabel= new QLabel("Advanced");
    advancedSettingsLabel->setObjectName("advancedSettingsLabel");
    advancedLoggingGrid->addWidget(advancedSettingsLabel,1,1,1,2);

    QLabel *guiInfoLabel= new QLabel("If you turn off GUI update you cannot log the population/environment windows using saved images.");
    guiInfoLabel->setObjectName("guiInfoLabel");
    guiInfoLabel->setWordWrap(true);
    advancedLoggingGrid->addWidget(guiInfoLabel,2,1,1,2);

    gui_checkbox = new QCheckBox("Don't update GUI on refresh/poll");
    gui_checkbox->setChecked(gui);
    advancedLoggingGrid->addWidget(gui_checkbox,3,1,1,2);
    QObject::connect(gui_checkbox ,SIGNAL (toggled(bool)), this, SLOT(gui_checkbox_state_changed(bool)));

    //ARTS - Dock Grid Layout
    output_settings_grid->addLayout(savePathGrid,1,1,1,2);
    output_settings_grid->addLayout(pollingRateGrid,2,1,1,2);
    output_settings_grid->addLayout(images_grid,3,1,1,2);
    output_settings_grid->addLayout(fileLoggingGrid,4,1,1,2);
    output_settings_grid->addLayout(advancedLoggingGrid,5,1,1,2);

    QWidget *output_settings_layout_widget = new QWidget;
    output_settings_layout_widget->setLayout(output_settings_grid);
    outputSettingsDock->setWidget(output_settings_layout_widget);

    return outputSettingsDock;
}

QDockWidget *MainWindow::createOrganismSettingsDock() {
    organismSettingsDock = new QDockWidget("Organism", this);
    organismSettingsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    organismSettingsDock->setFeatures(QDockWidget::DockWidgetMovable);
    organismSettingsDock->setFeatures(QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, organismSettingsDock);

    QGridLayout *org_settings_grid = new QGridLayout;
    org_settings_grid->setAlignment(Qt::AlignTop);

    QLabel *org_settings_label= new QLabel("Organism settings");
    org_settings_label->setStyleSheet("font-weight: bold");
    org_settings_grid->addWidget(org_settings_label,1,1,1,2);

    QLabel *mutate_label = new QLabel("Chance of mutation:");
    mutate_spin = new QSpinBox;
    mutate_spin->setMinimum(0);
    mutate_spin->setMaximum(255);
    mutate_spin->setValue(mutate);
    org_settings_grid->addWidget(mutate_label,2,1);
    org_settings_grid->addWidget(mutate_spin,2,2);
    connect(mutate_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {mutate=i;});

    variable_mutation_checkbox = new QCheckBox("Variable mutation");
    org_settings_grid->addWidget(variable_mutation_checkbox,3,1,1,1);
    variable_mutation_checkbox->setChecked(variableMutate);
    connect(variable_mutation_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { variableMutate=i; mutate_spin->setEnabled(!i); });

    QLabel *startAge_label = new QLabel("Start age:");
    startAge_spin = new QSpinBox;
    startAge_spin->setMinimum(1);
    startAge_spin->setMaximum(1000);
    startAge_spin->setValue(startAge);
    org_settings_grid->addWidget(startAge_label,4,1);
    org_settings_grid->addWidget(startAge_spin,4,2);
    connect(startAge_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {startAge=i;});

    QLabel *breed_settings_label= new QLabel("Breed settings");
    breed_settings_label->setStyleSheet("font-weight: bold");
    org_settings_grid->addWidget(breed_settings_label,5,1,1,2);

    QLabel *breedThreshold_label = new QLabel("Breed threshold:");
    breedThreshold_spin = new QSpinBox;
    breedThreshold_spin->setMinimum(1);
    breedThreshold_spin->setMaximum(5000);
    breedThreshold_spin->setValue(breedThreshold);
    org_settings_grid->addWidget(breedThreshold_label,6,1);
    org_settings_grid->addWidget(breedThreshold_spin,6,2);
    connect(breedThreshold_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {breedThreshold=i;});

    QLabel *breedCost_label = new QLabel("Breed cost:");
    breedCost_spin = new QSpinBox;
    breedCost_spin->setMinimum(1);
    breedCost_spin->setMaximum(10000);
    breedCost_spin->setValue(breedCost);
    org_settings_grid->addWidget(breedCost_label,7,1);
    org_settings_grid->addWidget(breedCost_spin,7,2);
    connect(breedCost_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {breedCost=i;});

    QLabel *maxDiff_label = new QLabel("Max difference to breed:");
    maxDiff_spin = new QSpinBox;
    maxDiff_spin->setMinimum(1);
    maxDiff_spin->setMaximum(31);
    maxDiff_spin->setValue(maxDiff);
    org_settings_grid->addWidget(maxDiff_label,8,1);
    org_settings_grid->addWidget(maxDiff_spin,8,2);
    connect(maxDiff_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {maxDiff=i;});

    breeddiff_checkbox = new QCheckBox("Use max diff to breed");
    org_settings_grid->addWidget(breeddiff_checkbox,9,1,1,1);
    breeddiff_checkbox->setChecked(breeddiff);
    connect(breeddiff_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { breeddiff=i;});

    breedspecies_checkbox = new QCheckBox("Breed only within species");
    org_settings_grid->addWidget(breedspecies_checkbox,10,1,1,1);
    breedspecies_checkbox->setChecked(breedspecies);
    connect(breedspecies_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { breedspecies=i;});

    QLabel *breed_mode_label= new QLabel("Breed mode:");
    org_settings_grid->addWidget(breed_mode_label,11,1,1,2);
    sexual_radio = new QRadioButton("Sexual");
    asexual_radio = new QRadioButton("Asexual");
    variableBreed_radio = new QRadioButton("Variable");
    QButtonGroup *breeding_button_group = new QButtonGroup;
    breeding_button_group->addButton(sexual_radio,0);
    breeding_button_group->addButton(asexual_radio,1);
    breeding_button_group->addButton(variableBreed_radio,2);
    sexual_radio->setChecked(sexual);
    asexual_radio->setChecked(asexual);
    variableBreed_radio->setChecked(variableBreed);
    org_settings_grid->addWidget(sexual_radio,12,1,1,2);
    org_settings_grid->addWidget(asexual_radio,13,1,1,2);
    org_settings_grid->addWidget(variableBreed_radio,14,1,1,2);
    connect(breeding_button_group, (void(QButtonGroup::*)(int))&QButtonGroup::buttonClicked,[=](const int &i)
        {
        if(i==0){sexual=true;asexual=false;variableBreed=false;}
        if(i==1){sexual=false;asexual=true;variableBreed=false;}
        if(i==2){sexual=false;asexual=false;variableBreed=true;}
        });

    QLabel *settle_settings_label= new QLabel("Settle settings");
    settle_settings_label->setStyleSheet("font-weight: bold");
    org_settings_grid->addWidget(settle_settings_label,15,1,1,2);

    QLabel *dispersal_label = new QLabel("Dispersal:");
    dispersal_spin = new QSpinBox;
    dispersal_spin->setMinimum(1);
    dispersal_spin->setMaximum(200);
    dispersal_spin->setValue(dispersal);
    org_settings_grid->addWidget(dispersal_label,16,1);
    org_settings_grid->addWidget(dispersal_spin,16,2);
    connect(dispersal_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {dispersal=i;});

    nonspatial_checkbox = new QCheckBox("Nonspatial settling");
    org_settings_grid->addWidget(nonspatial_checkbox,17,1,1,2);
    nonspatial_checkbox->setChecked(nonspatial);
    connect(nonspatial_checkbox,&QCheckBox::stateChanged,[=](const bool &i) {nonspatial=i;});

    QLabel *pathogen_settings_label= new QLabel("Pathogen settings");
    pathogen_settings_label->setStyleSheet("font-weight: bold");
    org_settings_grid->addWidget(pathogen_settings_label,18,1,1,2);

    pathogens_checkbox = new QCheckBox("Pathogens layer");
    org_settings_grid->addWidget(pathogens_checkbox,19,1,1,2);
    pathogens_checkbox->setChecked(path_on);
    connect(pathogens_checkbox,&QCheckBox::stateChanged,[=](const bool &i) {path_on=i;});

    QLabel *pathogen_mutate_label = new QLabel("Pathogen mutation:");
    pathogen_mutate_spin = new QSpinBox;
    pathogen_mutate_spin->setMinimum(1);
    pathogen_mutate_spin->setMaximum(255);
    pathogen_mutate_spin->setValue(pathogen_mutate);
    org_settings_grid->addWidget(pathogen_mutate_label,20,1);
    org_settings_grid->addWidget(pathogen_mutate_spin,20,2);
    connect(pathogen_mutate_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {pathogen_mutate=i;});

    QLabel *pathogen_frequency_label = new QLabel("Pathogen frequency:");
    pathogen_frequency_spin = new QSpinBox;
    pathogen_frequency_spin->setMinimum(1);
    pathogen_frequency_spin->setMaximum(1000);
    pathogen_frequency_spin->setValue(pathogen_frequency);
    org_settings_grid->addWidget(pathogen_frequency_label,21,1);
    org_settings_grid->addWidget(pathogen_frequency_spin,21,2);
    connect(pathogen_frequency_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {pathogen_frequency=i;});

    QWidget *org_settings_layout_widget = new QWidget;
    org_settings_layout_widget->setLayout(org_settings_grid);
    organismSettingsDock->setWidget(org_settings_layout_widget);

    return organismSettingsDock;
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

//ARTS - adds a null loop to the simulation iteration run when pause button/command is issued
// this loop is removed on the next tiggered() signal send from either one.
int MainWindow::waitUntilPauseSignalIsEmitted() {
    QEventLoop loop;
    QObject::connect(pauseButton, SIGNAL(triggered()),&loop, SLOT(quit()));
    QObject::connect(ui->actionPause_Sim, SIGNAL(triggered()),&loop, SLOT(quit()));
    return loop.exec();
}

void MainWindow::on_actionAbout_triggered()
{
    About adialogue;
    adialogue.exec();
}

//RJG - Reset simulation (i.e. fill the centre pixel with a genome (unless dual seed is selected), then set up a run).
void MainWindow::on_actionReset_triggered()
{
    // Reset the information bar
    resetInformationBar();

    //RJG - This resets all the species logging stuff as well as setting up the run
    TheSimManager->SetupRun();
    NextRefresh=0;

    //ARTS - Update views based on the new reset simulation
    RefreshReport();
    //UpdateTitles();
    RefreshPopulations();
}

void MainWindow::resetInformationBar()
{
    //ARTS - reset the bottom information bar
    ui->LabelBatch->setText(tr("1/1"));
    ui->LabelIteration->setText(tr("0"));
    ui->LabelIterationsPerHour->setText(tr("0.00"));
    ui->LabelCritters->setText(tr("0"));
    ui->LabelSpeed->setText(tr("0.00"));
    ui->LabelFitness->setText(tr("0.00%"));
    ui->LabelSpecies->setText(tr("-"));

    QString environment_scene_value;
    environment_scene_value.sprintf("%d/%d",CurrentEnvFile+1,EnvFiles.count());
    ui->LabelEnvironment->setText(environment_scene_value);
}

//RJG - Reseed provides options to either reset using a random genome, or a user defined one - drawn from the genome comparison docker.
void MainWindow::on_actionReseed_triggered()
{
    reseed reseed_dialogue;
    reseed_dialogue.exec();
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

//ARTS - "Run" action
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

    ui->LabelBatch->setText(tr("1/1"));

    while (stopflag==false)
    {
        while(pauseflag == true){
            waitUntilPauseSignalIsEmitted();
            pauseflag = false;
        }

        Report();
        qApp->processEvents();
        if (ui->actionGo_Slow->isChecked()) Sleeper::msleep(30);

        //ARTS - set Stop flag to returns true if reached end... but why? It will fire the FinishRun() function at the end.
        if (TheSimManager->iterate(environment_mode,enviroment_interpolate)) stopflag=true;
        FRW->MakeRecords();
    }

    FinishRun();
}

//ARTS - "Run For..." action
void MainWindow::on_actionRun_for_triggered()
{
    if (CurrentEnvFile==-1)
    {
        QMessageBox::critical(0,"","Cannot start simulation without environment");
        if (on_actionEnvironment_Files_triggered() == false) {
            return;
        }
    }

    bool ok = false;
    int i, num_iterations;
    num_iterations = QInputDialog::getInt(this, "",tr("Iterations: "), 1000, 1, 10000000, 1, &ok);
    i = num_iterations;
    if (!ok) return;

    ui->LabelBatch->setText(tr("1/1"));

    RunSetUp();

    while (stopflag==false && i>0)
    {
        while(pauseflag == true){
            waitUntilPauseSignalIsEmitted();
            pauseflag = false;
        }

        Report();
        qApp->processEvents();

        if (TheSimManager->iterate(environment_mode,enviroment_interpolate)) stopflag=true;
        FRW->MakeRecords();
        i--;
    }

    if(autodump_checkbox->isChecked())dump_run_data();

    //ARTS Show finish message and run FinshRun()
    if (stopflag==false) {
        QMessageBox::information(0,tr("Run For... Finished"),tr("The run for %1 iterations has finished.").arg(num_iterations));
        FinishRun();
    } else {
        QMessageBox::information(0,tr("Run For... Stopped"),tr("The run for %1 iterations has been stopped at iteration %2.").arg(num_iterations).arg(i));
        FinishRun();
    }
}

//RJG - Batch - primarily intended to allow repeats of runs with the same settings, rather than allowing these to be changed between runs
//ARTS - Condensed the Batch Setup multiple dialogs into one popup dialog to make it easier to explain in the User Manual.
void MainWindow::on_actionBatch_triggered()
{
    //ARTS - set default vaules
    batch_running=true;
    runs=0;
    int environment_start = CurrentEnvFile;

    bool repeat_environment;
    QString save_path(path->text());

    //ARTS - batch setup default and maxium values
    int maxIterations = 10000000;
    int defaultIterations = 1000;
    int maxRuns = 10000000;
    int defaultRuns = 1000;

    //ARTS - start of batch setup dialog form
    QDialog dialog(this);
    dialog.setMinimumSize(480,150);
    dialog.setWindowTitle(tr("Batch Run Setup"));

    QFormLayout form(&dialog);
    // Add some text above the fields
    form.addRow(new QLabel("You may: 1) set the number of runs you require; 2) set the number of iterations per run; and 3) choose to repeat or not to repeat the environment each run."));

    QSpinBox *iterationsSpinBox = new QSpinBox(&dialog);
    iterationsSpinBox->setRange(2, maxIterations);
    iterationsSpinBox->setSingleStep(1);
    iterationsSpinBox->setValue(defaultIterations);
    QString iterationsLabel = QString(tr("How many iterations would you like each run to go for (max = %1)?")).arg(maxIterations);
    form.addRow(iterationsLabel, iterationsSpinBox);

    QSpinBox *runsSpinBox = new QSpinBox(&dialog);
    runsSpinBox->setRange(1, maxRuns);
    runsSpinBox->setSingleStep(2);
    runsSpinBox->setValue(defaultRuns);
    QString runsLabel = QString(tr("And how many runs (max = %1)?")).arg(maxRuns);
    form.addRow(runsLabel, runsSpinBox);

    QComboBox *environmentComboBox = new QComboBox(&dialog);
    environmentComboBox->addItem("Yes", 1);
    environmentComboBox->addItem("No", 0);
    int index = environmentComboBox->findData(1);
    if ( index != -1 ) { // -1 for not found
       environmentComboBox->setCurrentIndex(index);
    }
    QString environmentLabel = QString(tr("Would you like the environment to repeat with each batch?"));
    form.addRow(environmentLabel, environmentComboBox);

    // Add some standard buttons (Cancel/Ok) at the bottom of the dialog
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    //ARTS - end of batch setup dialog form

    //ARTS - if OK has been pressed take values and update the defaults, else return false without running batch
    if (dialog.exec() == QDialog::Accepted) {
        batch_iterations=iterationsSpinBox->value();
        batch_target_runs=runsSpinBox->value();
        if(environmentComboBox->itemData(environmentComboBox->currentIndex()) == 1) repeat_environment = true;
            else repeat_environment = false;

        // Reset before starting batch run
        on_actionReset_triggered();

        ui->LabelBatch->setText(tr("%1/%2").arg(1).arg(batch_target_runs));
    } else {
        return;
    }

    //ARTS - run the batch
    do {

        //RJG - Sort environment so it repeats
        if(repeat_environment)
        {
            CurrentEnvFile=environment_start;
            TheSimManager->loadEnvironmentFromFile(environment_mode);
        }

        //And run...
        ui->LabelBatch->setText(tr("%1/%2").arg((runs+1)).arg(batch_target_runs));

        if (CurrentEnvFile==-1)
        {
            QMessageBox::critical(0,"","Cannot start simulation without environment");
            if (on_actionEnvironment_Files_triggered() == false) {
                return;
            }
        }

        RunSetUp();
        int i = batch_iterations;
        while (stopflag==false && i>0)
        {
            while(pauseflag == true){
                waitUntilPauseSignalIsEmitted();
                pauseflag = false;
            }

            Report();
            qApp->processEvents();

            TheSimManager->iterate(environment_mode,enviroment_interpolate);
            FRW->MakeRecords();
            i--;
        }

       if(autodump_checkbox->isChecked())dump_run_data();

        runs++;

        if(stopflag==false && runs<batch_target_runs) {
            on_actionReset_triggered();
        }

    } while(runs<batch_target_runs && stopflag==false);

    //ARTS Show finish message and reset batch counters
    if ((runs)==batch_target_runs) {
        QMessageBox::information(0,tr("Batch Finished"),tr("The batch of %1 runs with %2 iterations has finished.").arg(batch_target_runs).arg(batch_iterations));
    } else {
        QMessageBox::information(0,tr("Batch Stopped"),tr("The batch of %1 runs with %2 iterations has been stopped at batch run number %3.").arg(batch_target_runs).arg(batch_iterations).arg(runs));
    }

    path->setText(save_path);
    runs=0;
    batch_running=false;
    FinishRun();
}

//ARTS - pause function to halt the simulation mid run and allow restart at same point
//Note this disables the Stop button as the Stop function runs outside the iteration loop,
//so can not be triggered while paused.
void MainWindow::on_actionPause_Sim_triggered()
{
    if (pauseflag == true)
    {
        pauseflag = false;
        ui->actionStop_Sim->setEnabled(true);
        ui->actionPause_Sim->setText(tr("Pause"));
        ui->actionPause_Sim->setToolTip(tr("Pause"));
        stopButton->setEnabled(true);
        pauseButton->setText(tr("Pause"));
    }
    else {
        pauseflag = true;
        ui->actionStop_Sim->setEnabled(false);
        ui->actionPause_Sim->setText(tr("Resume"));
        ui->actionPause_Sim->setToolTip(tr("Resume"));
        stopButton->setEnabled(false);
        pauseButton->setText(tr("Resume"));
    }
}

//ART - Sets the stopflag to be true on Stop button/command trigger.
void MainWindow::on_actionStop_Sim_triggered()
{
    stopflag=true;
}

//ARTS - Sets up the defaults for a simulation run
void MainWindow::RunSetUp()
{
    stopflag=false;

    // Run start action
    ui->actionStart_Sim->setEnabled(false);
    startButton->setEnabled(false);

    // Run for... start action
    ui->actionRun_for->setEnabled(false);
    runForButton->setEnabled(false);

    // Batched Run start action
    ui->actionBatch->setEnabled(false);
    runForBatchButton->setEnabled(false);

    // Pause action
    ui->actionPause_Sim->setEnabled(true);
    pauseButton->setEnabled(true);

    // Stop action
    ui->actionStop_Sim->setEnabled(true);
    stopButton->setEnabled(true);

    // Reset action
    ui->actionReset->setEnabled(false);
    resetButton->setEnabled(false);

    // Reseed action
    ui->actionReseed->setEnabled(false);
    reseedButton->setEnabled(false);

    if(logging && species_mode==SPECIES_MODE_NONE)QMessageBox::warning(this,"Be aware","Species tracking is off, so the log files won't show species information");

    ui->actionSettings->setEnabled(false); /* unused */
    ui->actionEnvironment_Files->setEnabled(false);

    timer.restart();
    NextRefresh=RefreshRate;

    if (logging_checkbox->isChecked())WriteLog();
}

//ARTS - resets the buttons/commands back to a pre-run state
void MainWindow::FinishRun()
{
    // Run start action
    ui->actionStart_Sim->setEnabled(true);
    startButton->setEnabled(true);

    // Run for... start action
    ui->actionRun_for->setEnabled(true);
    runForButton->setEnabled(true);

    // Batched Run start action
    ui->actionBatch->setEnabled(true);
    runForBatchButton->setEnabled(true);

    // Pause action
    ui->actionPause_Sim->setEnabled(false);
    pauseButton->setEnabled(false);

    // Stop action
    ui->actionStop_Sim->setEnabled(false);
    stopButton->setEnabled(false);

    // Reset action
    ui->actionReset->setEnabled(true);
    resetButton->setEnabled(true);

    // Reseed action
    ui->actionReseed->setEnabled(true);
    reseedButton->setEnabled(true);

    ui->actionSettings->setEnabled(true); /* unused */
    ui->actionEnvironment_Files->setEnabled(true);
}

//ARTS - main close action
void MainWindow::closeEvent(QCloseEvent * /* unused */)
{
    exit(0);
}

//RJG - Updates reports, and does logging
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

    out.sprintf("%.3f",(3600000/atime)/1000000);
    ui->LabelIterationsPerHour->setText(out);

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
    if (species_mode!=SPECIES_MODE_NONE)
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
    if(!gui)RefreshPopulations();
    if(!gui)RefreshEnvironment();
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

int MainWindow::ScaleFails(int fails, float gens)
{
    //scale colour of fail count, correcting for generations, and scaling high values to something saner
    float ffails=((float)fails)/gens;

    ffails*=100.0; // a fudge factor no less! Well it's only visualization...
    ffails=pow(ffails,0.8);

    if (ffails>255.0) ffails=255.0;
    return (int)ffails;
}


void MainWindow::on_populationWindowComboBox_currentIndexChanged(int index)
{
    // 0 = Population count
    // 1 = Mean fitnessFails (R-Breed, G=Settle)
    // 2 = Coding genome as colour
    // 3 = NonCoding genome as colour
    // 4 = Gene Frequencies
    // 5 = Breed Attempts
    // 6 = Breed Fails
    // 7 = Settles
    // 8 = Settle Fails
    // 9 = Breed Fails 2
    // 10 = Species
    int currentSelectedMode = ui->populationWindowComboBox->itemData(index).toInt();
    Q_UNUSED(currentSelectedMode);

    //view_mode_changed();
    RefreshPopulations();
}

void MainWindow::RefreshPopulations()
//Refreshes of left window - also run species ident
{
    //RJG - make path if required - this way as if user adds file name to path, this will create a subfolder with the same file name as logs
    QString save_path(path->text());
    if(!save_path.endsWith(QDir::separator()))save_path.append(QDir::separator());
    if(batch_running)
    {
        save_path.append(QString("Images_run_%1").arg(runs, 4, 10, QChar('0')));
        save_path.append(QDir::separator());
    }
    QDir save_dir(save_path);

    // 0 = Population count
    // 1 = Mean fitness
    // 2 = Coding genome as colour
    // 3 = NonCoding genome as colour
    // 4 = Gene Frequencies
    // 5 = Breed Attempts
    // 6 = Breed Fails
    // 7 = Settles
    // 8 = Settle Fails === Fails (R-Breed, G=Settle)
    // 9 = Breed Fails 2
    // 10 = Species

    //ARTS - Check to see what the mode is from the Population Window QComboBox; return the data as int.
    int currentSelectedMode = ui->populationWindowComboBox->itemData(ui->populationWindowComboBox->currentIndex()).toInt();

    // (0) Population Count
    //if (ui->actionPopulation_Count->isChecked()||save_population_count->isChecked())
    if (currentSelectedMode==0||save_population_count->isChecked())
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
        //if (ui->actionPopulation_Count->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image));
        if (currentSelectedMode==0)pop_item->setPixmap(QPixmap::fromImage(*pop_image));
        if (save_population_count->isChecked())
            if(save_dir.mkpath("population/"))
                pop_image_colour->save(QString(save_dir.path()+"/population/EvoSim_population_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    // (1) Fitness
    if (currentSelectedMode==1||save_mean_fitness->isChecked())
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
        if (currentSelectedMode==1)pop_item->setPixmap(QPixmap::fromImage(*pop_image));
        if (save_mean_fitness->isChecked())
            if(save_dir.mkpath("fitness/"))
                pop_image_colour->save(QString(save_dir.path()+"/fitness/EvoSim_mean_fitness_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    // (2) Genome as colour
    if (currentSelectedMode==2||save_coding_genome_as_colour->isChecked())
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

        if (currentSelectedMode==2) pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
        if (save_coding_genome_as_colour->isChecked())
            if(save_dir.mkpath("coding/"))
            pop_image_colour->save(QString(save_dir.path()+"/coding/EvoSim_coding_genome_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    // (3) Non-coding Genome
    if (currentSelectedMode==3||save_non_coding_genome_as_colour->isChecked())
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

        if(currentSelectedMode==3)pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
        if(save_non_coding_genome_as_colour->isChecked())
            if(save_dir.mkpath("non_coding/"))
                 pop_image_colour->save(QString(save_dir.path()+"/non_coding/EvoSim_non_coding_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    // (4) Gene Frequencies
    if (currentSelectedMode==4||save_gene_frequencies->isChecked())
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

        if (currentSelectedMode==4)pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
        if(save_gene_frequencies->isChecked())
            if(save_dir.mkpath("gene_freq/"))
                pop_image_colour->save(QString(save_dir.path()+"/gene_freq/EvoSim_gene_freq_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    // (5) Breed Attempts
    //RJG - No save option as no longer in the menu as an option.
    if (currentSelectedMode==5)
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

    // (6) Breed Fails
    //RJG - No save option as no longer in the menu as an option.
    if (currentSelectedMode==6)
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

    // (7) Settles
    if (currentSelectedMode==7||save_settles->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int value=(settles[n][m]*10)/RefreshRate;
            if (value>255) value=255;
            pop_image->setPixel(n,m,value);
        }

        if(currentSelectedMode==7)pop_item->setPixmap(QPixmap::fromImage(*pop_image));
        if(save_settles->isChecked())
            if(save_dir.mkpath("settles/"))
                pop_image_colour->save(QString(save_dir.path()+"/settles/EvoSim_settles_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    // (8) Settle Fails === Fails
    //RJG - this now combines breed fails (red) and settle fails (green)
    if (currentSelectedMode==8||save_fails_settles->isChecked())
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

        if(currentSelectedMode==8)pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
        if(save_fails_settles->isChecked())
            if(save_dir.mkpath("settles_fails/"))
                pop_image_colour->save(QString(save_dir.path()+"/settles_fails/EvoSim_settles_fails_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    // (9) Breed Fails 2
    if (currentSelectedMode==9)
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

    // (10) Species
    if (currentSelectedMode==10||save_species->isChecked()) //do visualisation if necessary
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

        if (currentSelectedMode==10)pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
        if (save_species->isChecked())
            if(save_dir.mkpath("species/"))
                pop_image_colour->save(QString(save_dir.path()+"/species/EvoSim_species_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    lastReport=generation;
}

//ARTS - environment window refresh function
void MainWindow::RefreshEnvironment()
{
    QString save_path(path->text());
    if(!save_path.endsWith(QDir::separator()))save_path.append(QDir::separator());
    if(batch_running)
        {
        save_path.append(QString("Images_run_%1").arg(runs, 4, 10, QChar('0')));
        save_path.append(QDir::separator());
        }
    QDir save_dir(save_path);

    for (int n=0; n<gridX; n++)
    for (int m=0; m<gridY; m++)
        env_image->setPixel(n,m,qRgb(environment[n][m][0], environment[n][m][1], environment[n][m][2]));

    env_item->setPixmap(QPixmap::fromImage(*env_image));
    if(save_environment->isChecked())
        if(save_dir.mkpath("environment/"))
            env_image->save(QString(save_dir.path()+"/environment/EvoSim_environment_it_%1.png").arg(generation, 7, 10, QChar('0')));

    //Draw on fossil records
    envscene->DrawLocations(FRW->FossilRecords,ui->actionShow_positions->isChecked());
}

//ARTS - resize windows on window size change event
void MainWindow::resizeEvent(QResizeEvent * /* unused */)
{
    Resize();
}

//ARTS - Force and window resize and rescale of the graphic view
void MainWindow::Resize()
{
    ui->GV_Population->fitInView(pop_item,Qt::KeepAspectRatio);
    ui->GV_Environment->fitInView(env_item,Qt::KeepAspectRatio);
}

void MainWindow::gui_checkbox_state_changed(bool dont_update)
{
    if(dont_update && QMessageBox::question(0, "Heads up", "If you don't update the GUI, images will also not be saved. OK?", QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)==QMessageBox::No) {gui_checkbox->setChecked(false);return;}

    gui=dont_update;
}

void MainWindow::save_all_checkbox_state_changed(bool all)
{
    save_population_count->setChecked(all);
    save_mean_fitness->setChecked(all);
    save_coding_genome_as_colour->setChecked(all);
    save_non_coding_genome_as_colour->setChecked(all);
    save_species->setChecked(all);
    save_gene_frequencies->setChecked(all);
    save_settles->setChecked(all);
    save_fails_settles->setChecked(all);
    save_environment->setChecked(all);
}

//RJG - At end of run in run for/batch mode, or on click when a run is going, this allows user to output the final log, along with the tree for the run
void MainWindow::dump_run_data()
{

    QString FinalLoggingFile(path->text());
    if(!FinalLoggingFile.endsWith(QDir::separator()))FinalLoggingFile.append(QDir::separator());
    FinalLoggingFile.append("REvoSim_end_run_log");
    if(batch_running)FinalLoggingFile.append(QString("_run_%1").arg(runs, 4, 10, QChar('0')));
    FinalLoggingFile.append(".txt");
    QFile outputfile(FinalLoggingFile);
    outputfile.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream out(&outputfile);

    out<<"End run log ";
    QDateTime t(QDateTime::currentDateTime());
    out<<t.toString(Qt::ISODate)<< "\n\n===================\n\n"<<print_settings()<<"\n\n===================\n";
    out<<"\nThis log features the tree from a finished run, in Newick format, and then data for all the species that have existed with more individuals than minimum species size. The exact data provided depends on the phylogeny tracking mode selected in the GUI.\n";
    out<<"\n\n===================\n\n";
    out<<"Tree:\n\n"<<HandleAnalysisTool(ANALYSIS_TOOL_CODE_MAKE_NEWICK);
    out<<"\n\nSpecies data:\n\n";
    out<<HandleAnalysisTool(ANALYSIS_TOOL_CODE_DUMP_DATA);
    outputfile.close();
}

void MainWindow::environment_mode_changed(int change_environment_mode, bool update_gui)
{
    int new_environment_mode=ENV_MODE_STATIC;
    if (change_environment_mode==ENV_MODE_ONCE) new_environment_mode=ENV_MODE_ONCE;
    if (change_environment_mode==ENV_MODE_LOOP) new_environment_mode=ENV_MODE_LOOP;
    if (change_environment_mode==ENV_MODE_BOUNCE) new_environment_mode=ENV_MODE_BOUNCE;

    if(update_gui)
        {
        if (change_environment_mode==ENV_MODE_STATIC)environmentModeStaticButton->setChecked(true);
        if (change_environment_mode==ENV_MODE_ONCE)environmentModeOnceButton->setChecked(true);
        if (change_environment_mode==ENV_MODE_LOOP)environmentModeLoopButton->setChecked(true);
        if (change_environment_mode==ENV_MODE_BOUNCE)environmentModeBounceButton->setChecked(true);

        }

    environment_mode=new_environment_mode;
}

void MainWindow::species_mode_changed(int change_species_mode)
{
    int new_species_mode=SPECIES_MODE_NONE;
    if (change_species_mode==SPECIES_MODE_PHYLOGENY) new_species_mode=SPECIES_MODE_PHYLOGENY;
    if (change_species_mode==SPECIES_MODE_PHYLOGENY_AND_METRICS) new_species_mode=SPECIES_MODE_PHYLOGENY_AND_METRICS;
    if (change_species_mode==SPECIES_MODE_BASIC) new_species_mode=SPECIES_MODE_BASIC;

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
                phylogeny_off_button->setChecked(true);
                return;
            }
        }

        if (species_mode==SPECIES_MODE_BASIC)
        {
            if (new_species_mode==SPECIES_MODE_PHYLOGENY || new_species_mode==SPECIES_MODE_PHYLOGENY_AND_METRICS)
            {
                QMessageBox::warning(this,"Error","Turning on phylogeny tracking is not allowed mid-simulation");
                basic_phylogeny_button->setChecked(true);
                return;
            }
        }
        //all other combinations are OK - hopefully
    }

    //uncheck species visualisation if needbe
    if (change_species_mode==SPECIES_MODE_NONE)
    {
        if (ui->actionSpecies->isChecked()) ui->actionGenome_as_colour->setChecked(true);
        ui->actionSpecies->setEnabled(false);
    }
    else ui->actionSpecies->setEnabled(true);

    species_mode=new_species_mode;
}

void MainWindow::report_mode_changed(QAction * /* unused */)
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

void MainWindow::redoImages(int oldrows, int oldcols)
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

void MainWindow::on_actionSettings_triggered()
{
    if(simulationSettingsDock->isVisible())
    {
        simulationSettingsDock->hide();
        organismSettingsDock->hide();
        outputSettingsDock->hide();
        settingsButton->setChecked(false);
    } else
    {
        simulationSettingsDock->show();
        organismSettingsDock->show();
        outputSettingsDock->show();
        settingsButton->setChecked(true);
    }
}


void MainWindow::on_actionMisc_triggered()
{
    TheSimManager->testcode();
}

void MainWindow::on_actionCount_Peaks_triggered()
{

    QString peaks(HandleAnalysisTool(ANALYSIS_TOOL_CODE_COUNT_PEAKS));
    //Count peaks returns empty string if error.
    if (peaks.length()<5)return;

    QString count_peaks_file(path->text());
    if(!count_peaks_file.endsWith(QDir::separator()))count_peaks_file.append(QDir::separator());
    count_peaks_file.append("REvoSim_count_peaks.txt");
    QFile outputfile(count_peaks_file);
    outputfile.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream out(&outputfile);

    out<<"REvoSim Peak Counting ";
    QDateTime t(QDateTime::currentDateTime());
    out<<t.toString(Qt::ISODate)<< "\n\n===================\n\n";
    out<<"\nBelow is a histogram showing the different fitnesses for all potential 32-bit organisms in REvoSim under the user-defined RGB levels.\n";
    out<<"\n\n===================\n\n";
    out<<peaks;

    outputfile.close();
}

bool MainWindow::on_actionEnvironment_Files_triggered()
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
    if(notsquare||different_size)QMessageBox::warning(this,"FYI","For speed REvoSim currently has static arrays for the environment, which limits out of the box functionality to 100 x 100 square environments. "
    "It looks like some of your Environment images don't meet this requirement. Anything smaller than 100 x 100 will be stretched (irrespective of aspect ratio) to 100x100. Anything bigger, and we'll use the top left corner. Should you wish to use a different size environment, please email RJG or MDS.");

    EnvFiles = files;
    CurrentEnvFile=0;
    TheSimManager->loadEnvironmentFromFile(environment_mode);
    RefreshEnvironment();

    //RJG - Reset for this new environment
    TheSimManager->SetupRun();

    //ARTS - Update the information bar
    QString environment_scene_value;
    environment_scene_value.sprintf("%d/%d",CurrentEnvFile+1,EnvFiles.count());
    ui->LabelEnvironment->setText(environment_scene_value);

    return true;
}

//ARTS - action to select the fossil log save directory
void MainWindow::on_actionChoose_Log_Directory_triggered()
{
    QString dirname = QFileDialog::getExistingDirectory(this,"Select directory to log fossil record to");

    if (dirname.length()==0) return;
    FRW->LogDirectory=dirname;
    FRW->LogDirectoryChosen=true;
    FRW->HideWarnLabel();
}

//RJG - Fitness logging to file not sorted on save as yet.
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
    out<<(int)FILEVERSION;


    //Ints
    out<<gridX;
    out<<gridY;
    out<<settleTolerance;
    out<<slotsPerSq;
    out<<startAge;
    out<<dispersal;
    out<<food;
    out<<breedCost;
    out<<mutate;
    out<<pathogen_mutate;
    out<<pathogen_frequency;
    out<<maxDiff;
    out<<breedThreshold;
    out<<target;
    out<<envchangerate;
    out<<environment_mode;
    out<<RefreshRate;
    out<<speciesSamples;
    out<<speciesSensitivity;
    out<<timeSliceConnect;
    out<<minspeciessize;

    //Bools
    out<<recalcFitness;
    out<<toroidal;
    out<<nonspatial;
    out<<breeddiff;
    out<<breedspecies;
    out<<path_on;
    out<<variableMutate;
    out<<allowexcludewithissue;
    out<<sexual;
    out<<asexual;
    out<<variableBreed;
    out<<logging;
    out<<gui;
    out<<enviroment_interpolate;
    out<<fitnessLoggingToFile;
    out<<autodump_checkbox->isChecked();
    out<<save_population_count->isChecked();
    out<<save_mean_fitness->isChecked();
    out<<save_coding_genome_as_colour->isChecked();
    out<<save_species->isChecked();
    out<<save_non_coding_genome_as_colour->isChecked();
    out<<save_gene_frequencies->isChecked();
    out<<save_settles->isChecked();
    out<<save_fails_settles->isChecked();
    out<<save_environment->isChecked();

    //Strings
    out<<path->text();

    //Environment mode
    out<<environment_mode;

    int vmode=0;
    vmode = ui->populationWindowComboBox->itemData(ui->populationWindowComboBox->currentIndex()).toInt();

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

    //ARTS - Genome Comparison Saving
    out<<genoneComparison->saveComparison();

    //and fossil record stuff
    FRW->WriteFiles(); //make sure all is flushed
    out<<FRW->SaveState();

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
    out<<enviroment_interpolate;
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

    out<<species_mode;

    outfile.close();
}

//RJG - Fitness logging to file not sorted on load as yet.
void MainWindow::on_actionLoad_triggered()
{

    QString filename = QFileDialog::getOpenFileName(
                            this,
                            "Save file",
                            "",
                            "EVOSIM files (*.evosim)");

    if (filename.length()==0) return;

    if (stopflag==false) stopflag=true;

    //Otherwise - serialise all my crap
    QFile infile(filename);
    infile.open(QIODevice::ReadOnly);

    QDataStream in(&infile);

    QString temp;
    in>>temp;
    if (temp!="EVOSIM file")
    {QMessageBox::warning(this,"","Not an REvoSim file"); return;}

    int version;
    in>>version;
    if (version>FILEVERSION)
    {QMessageBox::warning(this,"","Version too high - will try to read, but may go horribly wrong");}

    //Ints
    in>>gridX;
    in>>gridY;
    in>>settleTolerance;
    in>>slotsPerSq;
    in>>startAge;
    in>>dispersal;
    in>>food;
    in>>breedCost;
    in>>mutate;
    in>>pathogen_mutate;
    in>>pathogen_frequency;
    in>>maxDiff;
    in>>breedThreshold;
    in>>target;
    in>>envchangerate;
    in>>environment_mode;
    in>>RefreshRate;
    in>>speciesSamples;
    in>>speciesSensitivity;
    in>>timeSliceConnect;
    in>>minspeciessize;

    //Bools
    in>>recalcFitness;
    in>>toroidal;
    in>>nonspatial;
    in>>breeddiff;
    in>>breedspecies;
    in>>path_on;
    in>>variableMutate;
    in>>allowexcludewithissue;
    in>>sexual;
    in>>asexual;
    in>>variableBreed;
    in>>logging;
    in>>gui;
    in>>enviroment_interpolate;
    in>>fitnessLoggingToFile;
    bool in_bool;
    in>>in_bool;
    autodump_checkbox->setChecked(in_bool);
    in>>in_bool;
    save_population_count->setChecked(in_bool);
    in>>in_bool;
    save_mean_fitness->setChecked(in_bool);
    in>>in_bool;
    save_coding_genome_as_colour->setChecked(in_bool);
    in>>in_bool;
    save_species->setChecked(in_bool);
    in>>in_bool;
    save_non_coding_genome_as_colour->setChecked(in_bool);
    in>>in_bool;
    save_gene_frequencies->setChecked(in_bool);
    in>>in_bool;
    save_settles->setChecked(in_bool);
    in>>in_bool;
    save_fails_settles->setChecked(in_bool);
    in>>in_bool;
    save_environment->setChecked(in_bool);

    //Strings
    QString load_path;
    in>>load_path;
    path->setText(load_path);

    // 0 = Static
    // 1 = Once
    // 2 = Looped (default)
    // 3 = Bounce
    environment_mode=ENV_MODE_LOOP;
    in>>environment_mode;
    if (environment_mode==0) environmentModeStaticButton->setChecked(true);
    if (environment_mode==1) environmentModeOnceButton->setChecked(true);
    if (environment_mode==2) environmentModeLoopButton->setChecked(true);
    if (environment_mode==3) environmentModeBounceButton->setChecked(true);

    // 0 = Population count
    // 1 = Mean fitnessFails (R-Breed, G=Settle)
    // 2 = Coding genome as colour
    // 3 = NonCoding genome as colour
    // 4 = Gene Frequencies
    // 5 = Breed Attempts
    // 6 = Breed Fails
    // 7 = Settles
    // 8 = Settle Fails
    // 9 = Breed Fails 2
    // 10 = Species
    int vmode;
    in>>vmode;
    int index = ui->populationWindowComboBox->findData(vmode);
    if ( index != -1 ) { // -1 for not found
       ui->populationWindowComboBox->setCurrentIndex(index);
    }

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
    enviroment_interpolate = true;
    in>>enviroment_interpolate;

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

    update_gui_from_variables();
}

//ARTS - Genome Comparison UI
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

        //Makre sure this is updated
        lastSpeciesCalc=generation;

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
    //Need to sort out file name in batch mode, check breed list entries is actually working, etc. then deal with logging after run
    //RJG - write main ongoing log
    if(logging)
    {
        SpeciesLoggingFile=path->text();
        if(!SpeciesLoggingFile.endsWith(QDir::separator()))SpeciesLoggingFile.append(QDir::separator());
        SpeciesLoggingFile.append("REvoSim_log");
        if(batch_running)SpeciesLoggingFile.append(QString("_run_%1").arg(runs, 4, 10, QChar('0')));
        SpeciesLoggingFile.append(".txt");
        QFile outputfile(SpeciesLoggingFile);

       if (generation==0)
            {
                outputfile.open(QIODevice::WriteOnly|QIODevice::Text);
                QTextStream out(&outputfile);
                out<<"New run ";
                QDateTime t(QDateTime::currentDateTime());
                out<<t.toString(Qt::ISODate)<< "\n\n===================\n\n"<<print_settings()<<"\n\n===================\n";
                out<<"\nFor each iteration, this log features:\n";
                out<<"Iteration\nFor the grid - Number of living digital organisms\tMean fitness of living digital organisms\tNumber of entries on the breed list\tNumber of failed breed attempts\n";
                out<<"Species ID\tSpecies origin (iterations)\tSpecies parent\tSpecies current size (number of individuals)\tSpecies current genome (for speed this is the genome of a randomly sampled individual, not the modal organism)\n";
                out<<"Note that this excludes species with less individuals than Minimum species size, but is not able to exlude species without issue, which can only be achieved with the end-run log.\n\n";
                out<<"===================\n\n";
                outputfile.close();
            }

       outputfile.open(QIODevice::Append|QIODevice::Text);
       QTextStream out(&outputfile);

        out<<"Iteration\t"<<generation<<"\n";

        int gridNumberAlive=0, gridTotalFitness=0, gridBreedEntries=0, gridBreedFails=0;
        for (int i=0; i<gridX; i++)
            for (int j=0; j<gridY; j++)
                    {
                    gridTotalFitness+=totalfit[i][j];
                    //----RJG: Manually count breed stufffor grid
                    gridBreedEntries+=breedattempts[i][j];
                    gridBreedFails+=breedfails[i][j];
                    //----RJG: Manually count number alive thanks to maxused issue
                    for  (int k=0; k<slotsPerSq; k++)if(critters[i][j][k].fitness)gridNumberAlive++;
                    }
        double mean_fitness=(double)gridTotalFitness/(double)gridNumberAlive;

        out<<gridNumberAlive<<"\t"<<mean_fitness<<"\t"<<gridBreedEntries<<"\t"<<gridBreedFails<<"\n";

        //----RJG: And species details for each iteration
        for (int i=0; i<oldspecieslist.count(); i++)
           //----RJG: Unable to exclude species without issue, for obvious reasons.
           if(oldspecieslist[i].size>minspeciessize)
           {
            out<<(oldspecieslist[i].ID)<<"\t";
            out<<oldspecieslist[i].origintime<<"\t";
            out<<oldspecieslist[i].parent<<"\t";
            out<<oldspecieslist[i].size<<"\t";
            //---- RJG - output binary genome if needed
            out<<"\t";
            for (int j=0; j<63; j++)if (tweakers64[63-j] & oldspecieslist[i].type) out<<"1"; else out<<"0";
            if (tweakers64[0] & oldspecieslist[i].type) out<<"1"; else out<<"0";
            out<<"\n";
            }
        out<<"\n";
        outputfile.close();
      }


    // ----RJG recombination logging to separate file
    //Need to add this to GUI
    if (ui->actionRecombination_logging->isChecked())
    {
        QString rFile(path->text()+"REvoSim_recombination");
        if(batch_running)
            rFile.append(QString("_run_%1").arg(runs, 4, 10, QChar('0')));
        rFile.append(".txt");
        QFile routputfile(rFile);

        if (!(routputfile.exists()))
        {
            routputfile.open(QIODevice::WriteOnly|QIODevice::Text);
            QTextStream rout(&routputfile);

            // Info on simulation setup
            rout<<print_settings()<<"\n";
            rout<<"\nNote on log: this only calculates proportions when variable breeding is selected for speed, and also currently will only count total breed attempts when the fitness log is also running.";
            rout<<"For now, this is merely a list of:\nIteration\tAsexual breeds\tSexual breeds\tTotal breed attempts\tTotal breed fails\tTotal Alive\tPercent sexual.\n";

            routputfile.close();
        }

        routputfile.open(QIODevice::Append|QIODevice::Text);
        QTextStream rout(&routputfile);

        rout<<generation<<"\t";

        //RJG count breeding. There is probably a better way to do this, but keeping as is for now as not too slow
        int cntAsex=0, cntSex=0;
        int totalBreedAttempts=0, totalBreedFails=0;

        for (int i=0; i<gridX; i++)
                for (int j=0; j<gridY; j++)
                        {
                        for (int c=0; c<slotsPerSq; c++)
                            if (critters[i][j][c].fitness)
                                {
                                    if(critters[i][j][c].return_recomb()<0)cntAsex++;
                                    else if (critters[i][j][c].return_recomb()>0)cntSex++;
                                }
                        totalBreedAttempts+=breedattempts[i][j];
                        totalBreedFails+=breedfails[i][j];

                        }
        double percent=((float)cntSex/(float)(cntAsex+cntSex))*100.;
        rout<<cntAsex<<"\t"<<cntSex<<"\t"<<totalBreedAttempts<<"\t"<<totalBreedFails<<"\t"<<AliveCount<<"\t"<<percent<<"\n";

        routputfile.close();
    }

    //Add this to GUI too
    // ----RJG log fitness to separate file
    if (ui->actionFitness_logging_to_File->isChecked())
    {

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
            out<<print_settings()<<"\n";

            //Different versions of output, for reuse as needed
            //out<<"Each generation lists, for each pixel: mean fitness, entries on breed list";
            //out<<"Each line lists generation, then the grid's: total critter number, total fitness, total entries on breed list";
            //out<<"Each generation lists, for each pixel (top left to bottom right): total fitness, number of critters,entries on breed list\n\n";
            out<<"Each generation lists, for the grid: the number of living crittes, then the total fitness of all those critters, and then the number of entries on the breed list";
            out<<"\n";
           outputfile.close();
        }

        outputfile.open(QIODevice::Append|QIODevice::Text);
        QTextStream out(&outputfile);

        // ----RJG: Breedattempts was no longer in use - but seems accurate, so can be co-opted for this.
        //out<<"Iteration: "<<generation<<"\n";
        out<<generation<<"\t";
        int gridNumberAlive=0, gridTotalFitness=0, gridBreedEntries=0;

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
                    gridTotalFitness+=totalfit[i][j];

                    int critters_alive=0;

                     //----RJG: Manually count number alive thanks to maxused issue
                    for  (int k=0; k<slotsPerSq; k++)if(critters[i][j][k].fitness)
                                    {
                                    //numberalive++;
                                    gridNumberAlive++;
                                    critters_alive++;
                                    }

                    //total fitness, number of critters,entries on breed list";
                    //out<<totalfit[i][j]<<" "<<critters_alive<<" "<<breedattempts[i][j];

                    //----RJG: Manually count breed attempts for grid
                    gridBreedEntries+=breedattempts[i][j];

                    //out<<"\n";

                    }
            }

        //---- RJG: If outputting averages to log.
        //float avFit=(float)gridTotalFitness/(float)gridNumberAlive;
        //float avBreed=(float)gridBreedEntries/(float)gridNumberAlive;
        //out<<avFit<<","<<avBreed;

        //---- RJG: If outputting totals
        //critter - fitness - breeds

        double mean_fitness= (double)gridTotalFitness / (double)gridNumberAlive;

        out<<mean_fitness<<"\t"<<gridBreedEntries<<"\t"<<gridNumberAlive<<"\n";
        outputfile.close();
      }
}

// ARTS - general function to set the status bar text
void MainWindow::setStatusBarText(QString text)
{
    ui->statusBar->showMessage(text);
}

// ARTS - action to load custom random number files
void MainWindow::on_actionLoad_Random_Numbers_triggered()
{
    //RJG - have added randoms to resources and into constructor, load on launch to ensure true randoms are loaded by default.
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

//RJG - Handle analyses at end of run for data
QString MainWindow::HandleAnalysisTool(int code)
{
    //Tidied up a bit - MDS 14/9/2017
    //RJG - changed to return string 04/04/18 as analysis docker will be removed.

    //Is there a valid input file?
    AnalysisTools a;
    QString OutputString, FilenameString;

    if (a.doesthiscodeneedafile(code))
    {
        QFile f(ui->LogFile->text());
        if (!(f.exists()))
        {
            QMessageBox::warning(this,"Error","No valid input file set");
            return QString("Error, No valid input file set");
        }
    }

    switch (code)
    {
        case ANALYSIS_TOOL_CODE_GENERATE_TREE:
            OutputString = a.GenerateTree(ui->LogFile->text()); //deprecated - log file generator now removed
            break;

        case ANALYSIS_TOOL_CODE_RATES_OF_CHANGE://Needs to be recoded
            OutputString = a.SpeciesRatesOfChange(ui->LogFile->text());
            break;

        case ANALYSIS_TOOL_CODE_EXTINCT_ORIGIN://Needs to be recoded
            OutputString = a.ExtinctOrigin(ui->LogFile->text());
            break;

        case ANALYSIS_TOOL_CODE_STASIS:
            OutputString = a.Stasis(ui->LogFile->text(),ui->StasisBins->value() //Needs to be recoded
                                ,((float)ui->StasisPercentile->value())/100.0,ui->StasisQualify->value());
            break;

        case ANALYSIS_TOOL_CODE_COUNT_PEAKS:
            {
            bool ok;
            int red = QInputDialog::getInt(this, "Count peaks...","Red level?", 128, 0, 255, 1, &ok);
            if(!ok)return QString("");
            int green= QInputDialog::getInt(this, "Count peaks...","Green level?", 128, 0, 255, 1, &ok);
            if(!ok)return QString("");;
            int blue= QInputDialog::getInt(this, "Count peaks...","Green level?", 128, 0, 255, 1, &ok);
            if(!ok)return QString("");
            OutputString = a.CountPeaks(red,green,blue);
            break;
            }

        case ANALYSIS_TOOL_CODE_MAKE_NEWICK:
            if (phylogeny_button->isChecked()||phylogeny_and_metrics_button->isChecked())OutputString = a.MakeNewick(rootspecies, minspeciessize, allowexcludewithissue);
            else OutputString = "Species tracking is not enabled.";
            break;

        case ANALYSIS_TOOL_CODE_DUMP_DATA:
            if (phylogeny_and_metrics_button->isChecked())OutputString = a.DumpData(rootspecies, minspeciessize, allowexcludewithissue);
            else OutputString = "Species tracking is not enabled, or is set to phylogeny only.";
            break;

        default:
            QMessageBox::warning(this,"Error","No handler for analysis tool");
            return QString("Error, No handler for analysis tool");

    }

    //write result to screen
    //ui->plainTextEdit->clear();
    //ui->plainTextEdit->appendPlainText(OutputString);

    return OutputString;
 }


void MainWindow::on_actionGenerate_NWK_tree_file_triggered()
{
    dump_run_data();
}

void MainWindow::on_actionSpecies_sizes_triggered()
{
    dump_run_data();
}


QString MainWindow::print_settings()
{
    QString settings;
    QTextStream settings_out(&settings);

    settings_out<<"EvoSim settings. Integers - Grid X: "<<gridX;
    settings_out<<"; Grid Y: "<<gridY;
    settings_out<<"; Settle tolerance: "<<settleTolerance;
    settings_out<<"; Start age: "<<startAge;
    settings_out<<"; Disperal: "<<dispersal;
    settings_out<<"; Food: "<<food;
    settings_out<<"; Breed cost: "<<breedCost;
    settings_out<<"; Mutate: "<<mutate;
    settings_out<<"; Pathogen mutate: "<<pathogen_mutate;
    settings_out<<"; Pathogen frequency: "<<pathogen_frequency;
    settings_out<<"; Max diff to breed: "<<maxDiff;
    settings_out<<"; Breed threshold: "<<breedThreshold;
    settings_out<<"; Slots per square: "<<slotsPerSq;
    settings_out<<"; Fitness target: "<<target;
    settings_out<<"; Environmental change rate: "<<envchangerate;
    settings_out<<"; Minimum species size:"<<minspeciessize;
    settings_out<<"; Environment mode:"<<environment_mode;

    settings_out<<". Bools - recalculate fitness: "<<recalcFitness;
    settings_out<<"; Toroidal environment: "<<toroidal;
    settings_out<<"; Interpolate environment: "<<enviroment_interpolate;
    settings_out<<"; Nonspatial setling: "<<nonspatial;
    settings_out<<"; Enforce max diff to breed:"<<breeddiff;
    settings_out<<"; Only breed within species:"<<breedspecies;
    settings_out<<"; Pathogens enabled:"<<path_on;
    settings_out<<"; Variable mutate:"<<variableMutate;
    settings_out<<"; Exclude species without issue:"<<allowexcludewithissue;
    settings_out<<"; Breeding:";
    if(sexual)settings_out<<" sexual.";
    else if (asexual)settings_out<<" asexual.";
    else settings_out<<" variable.";

    return settings;
}

//RJG - Save and load settings (but not critter info, masks etc.)
void MainWindow::load_settings()
{
    QString settings_filename=QFileDialog::getOpenFileName(this, tr("Open File"),path->text(),"XML files (*.xml)");
    if (settings_filename.length()<3) return;
    QFile settings_file(settings_filename);
    if(!settings_file.open(QIODevice::ReadOnly))
            {
            setStatusBarText("Error opening file.");
            return;
            }

           QXmlStreamReader settings_file_in(&settings_file);

           while (!settings_file_in.atEnd()&& !settings_file_in.hasError())
               {

                /* Read next element.*/
                QXmlStreamReader::TokenType token = settings_file_in.readNext();
                /* If token is just StartDocument, we'll go to next.*/

                if(token == QXmlStreamReader::StartDocument)continue;
                if(token == QXmlStreamReader::StartElement)
                    {
                         //Ints
                         if(settings_file_in.name() == "revosim")continue;
                         if(settings_file_in.name() == "gridX")gridX=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "gridY")gridY=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "settleTolerance")settleTolerance=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "slotsPerSq")slotsPerSq=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "startAge")startAge=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "dispersal")dispersal=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "food")food=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "breedCost")breedCost=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "mutate")mutate=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "pathogen_mutate")pathogen_mutate=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "pathogen_frequency")pathogen_frequency=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "maxDiff")maxDiff=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "breedThreshold")breedThreshold=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "target")target=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "envchangerate")envchangerate=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "RefreshRate")RefreshRate=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "environment_mode")environment_mode_changed(settings_file_in.readElementText().toInt(),true);
                         //No Gui options for the remaining settings as yet.
                         if(settings_file_in.name() == "speciesSamples")speciesSamples=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "speciesSensitivity")speciesSensitivity=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "timeSliceConnect")timeSliceConnect=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "minspeciessize")minspeciessize=settings_file_in.readElementText().toInt();

                         //Bools
                         if(settings_file_in.name() == "recalcFitness")recalcFitness=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "toroidal")toroidal=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "nonspatial")nonspatial=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "breeddiff")breeddiff=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "breedspecies")breedspecies=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "path_on")path_on=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "variableMutate")variableMutate=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "allowexcludewithissue")allowexcludewithissue=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "sexual")sexual=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "asexual")asexual=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "variableBreed")variableBreed=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "logging")logging=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "gui")gui=settings_file_in.readElementText().toInt();
                         if(settings_file_in.name() == "enviroment_interpolate")enviroment_interpolate=settings_file_in.readElementText().toInt();
                         //No gui options for below
                         if(settings_file_in.name() == "fitnessLoggingToFile")fitnessLoggingToFile=settings_file_in.readElementText().toInt();
                         //Only GUI options
                         if(settings_file_in.name() == "autodump")autodump_checkbox->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_population_count")save_population_count->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_mean_fitness")save_mean_fitness->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_coding_genome_as_colour")save_coding_genome_as_colour->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_species")save_species->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_non_coding_genome_as_colour")save_non_coding_genome_as_colour->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_gene_frequencies")save_gene_frequencies->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_settles")save_settles->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_fails_settles")save_fails_settles->setChecked(settings_file_in.readElementText().toInt());
                         if(settings_file_in.name() == "save_environment")save_environment->setChecked(settings_file_in.readElementText().toInt());

                         //Strings
                         if(settings_file_in.name() == "path")path->setText(settings_file_in.readElementText());

                       }
               }
           // Error
           if(settings_file_in.hasError()) setStatusBarText("There seems to have been an error reading in the XML file. Not all settings will have been loaded.");
           else setStatusBarText("Loaded settings file");

           settings_file.close();

           update_gui_from_variables();
}

//RJG - Call this from either load settings, or plain load. Updates gui from those variables held as simulation globals
void MainWindow::update_gui_from_variables()
{
    //Ints
         gridX_spin->setValue(gridX);
         gridY_spin->setValue(gridY);
         settleTolerance_spin->setValue(settleTolerance);
         slots_spin->setValue(slotsPerSq);
         startAge_spin->setValue(startAge);
         dispersal_spin->setValue(dispersal);
         energy_spin->setValue(food);
         breedCost_spin->setValue(breedCost);
         mutate_spin->setValue(mutate);
         pathogen_mutate_spin->setValue(pathogen_mutate);
         pathogen_frequency_spin->setValue(pathogen_frequency);
         maxDiff_spin->setValue(maxDiff);
         breedThreshold_spin->setValue(breedThreshold);
         target_spin->setValue(target);
         environment_rate_spin->setValue(envchangerate);
         refreshRateSpin->setValue(RefreshRate);

    //Bools
         recalcFitness_checkbox->setChecked(recalcFitness);
         toroidal_checkbox->setChecked(toroidal);
         nonspatial_checkbox->setChecked(nonspatial);
         breeddiff_checkbox->setChecked(breeddiff);
         breedspecies_checkbox->setChecked(breedspecies);
         pathogens_checkbox->setChecked(path_on);
         variable_mutation_checkbox->setChecked(variableMutate);
         exclude_without_issue_checkbox->setChecked(allowexcludewithissue);
         sexual_radio->setChecked(sexual);
         asexual_radio->setChecked(asexual);
         variableBreed_radio->setChecked(variableBreed);
         logging_checkbox->setChecked(logging);
         gui_checkbox->setChecked(gui);
         interpolateCheckbox->setChecked(enviroment_interpolate);
}

void MainWindow::save_settings()
{
    QString settings_filename=QFileDialog::getSaveFileName(this, tr("Save file as..."),QString(path->text()+"REvoSim_settings.xml"));
    if(!settings_filename.endsWith(".xml"))settings_filename.append(".xml");
    QFile settings_file(settings_filename);
    if(!settings_file.open(QIODevice::WriteOnly|QIODevice::Text))
        {
            setStatusBarText("Error opening settings file to write to.");
            return;
        }

        QXmlStreamWriter settings_file_out(&settings_file);
        settings_file_out.setAutoFormatting(true);
        settings_file_out.setAutoFormattingIndent(-2);

        settings_file_out.writeStartDocument();

        settings_file_out.writeStartElement("revosim");

        //Ints
        settings_file_out.writeStartElement("gridX");
        settings_file_out.writeCharacters(QString("%1").arg(gridX));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("gridY");
        settings_file_out.writeCharacters(QString("%1").arg(gridY));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("settleTolerance");
        settings_file_out.writeCharacters(QString("%1").arg(settleTolerance));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("slotsPerSq");
        settings_file_out.writeCharacters(QString("%1").arg(slotsPerSq));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("startAge");
        settings_file_out.writeCharacters(QString("%1").arg(startAge));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("dispersal");
        settings_file_out.writeCharacters(QString("%1").arg(dispersal));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("food");
        settings_file_out.writeCharacters(QString("%1").arg(food));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("breedCost");
        settings_file_out.writeCharacters(QString("%1").arg(breedCost));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("mutate");
        settings_file_out.writeCharacters(QString("%1").arg(mutate));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("pathogen_mutate");
        settings_file_out.writeCharacters(QString("%1").arg(pathogen_mutate));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("pathogen_frequency");
        settings_file_out.writeCharacters(QString("%1").arg(pathogen_frequency));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("maxDiff");
        settings_file_out.writeCharacters(QString("%1").arg(maxDiff));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("breedThreshold");
        settings_file_out.writeCharacters(QString("%1").arg(breedThreshold));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("target");
        settings_file_out.writeCharacters(QString("%1").arg(target));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("envchangerate");
        settings_file_out.writeCharacters(QString("%1").arg(envchangerate));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("environment_mode");
        settings_file_out.writeCharacters(QString("%1").arg(environment_mode));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("RefreshRate");
        settings_file_out.writeCharacters(QString("%1").arg(RefreshRate));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("speciesSamples");
        settings_file_out.writeCharacters(QString("%1").arg(speciesSamples));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("speciesSensitivity");
        settings_file_out.writeCharacters(QString("%1").arg(speciesSensitivity));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("timeSliceConnect");
        settings_file_out.writeCharacters(QString("%1").arg(timeSliceConnect));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("minspeciessize");
        settings_file_out.writeCharacters(QString("%1").arg(minspeciessize));
        settings_file_out.writeEndElement();

        //Bools
        settings_file_out.writeStartElement("recalcFitness");
        settings_file_out.writeCharacters(QString("%1").arg(recalcFitness));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("toroidal");
        settings_file_out.writeCharacters(QString("%1").arg(toroidal));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("nonspatial");
        settings_file_out.writeCharacters(QString("%1").arg(nonspatial));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("breeddiff");
        settings_file_out.writeCharacters(QString("%1").arg(breeddiff));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("breedspecies");
        settings_file_out.writeCharacters(QString("%1").arg(breedspecies));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("path_on");
        settings_file_out.writeCharacters(QString("%1").arg(path_on));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("variableMutate");
        settings_file_out.writeCharacters(QString("%1").arg(variableMutate));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("allowexcludewithissue");
        settings_file_out.writeCharacters(QString("%1").arg(allowexcludewithissue));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("sexual");
        settings_file_out.writeCharacters(QString("%1").arg(sexual));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("asexual");
        settings_file_out.writeCharacters(QString("%1").arg(asexual));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("variableBreed");
        settings_file_out.writeCharacters(QString("%1").arg(variableBreed));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("logging");
        settings_file_out.writeCharacters(QString("%1").arg(logging));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("gui");
        settings_file_out.writeCharacters(QString("%1").arg(gui));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("enviroment_interpolate");
        settings_file_out.writeCharacters(QString("%1").arg(enviroment_interpolate));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("fitnessLoggingToFile");
        settings_file_out.writeCharacters(QString("%1").arg(fitnessLoggingToFile));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("autodump");
        settings_file_out.writeCharacters(QString("%1").arg(autodump_checkbox->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_population_count");
        settings_file_out.writeCharacters(QString("%1").arg(save_population_count->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_mean_fitness");
        settings_file_out.writeCharacters(QString("%1").arg(save_mean_fitness->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_coding_genome_as_colour");
        settings_file_out.writeCharacters(QString("%1").arg(save_coding_genome_as_colour->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_species");
        settings_file_out.writeCharacters(QString("%1").arg(save_species->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_non_coding_genome_as_colour");
        settings_file_out.writeCharacters(QString("%1").arg(save_non_coding_genome_as_colour->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_gene_frequencies");
        settings_file_out.writeCharacters(QString("%1").arg(save_gene_frequencies->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_settles");
        settings_file_out.writeCharacters(QString("%1").arg(save_settles->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_fails_settles");
        settings_file_out.writeCharacters(QString("%1").arg(save_fails_settles->isChecked()));
        settings_file_out.writeEndElement();

        settings_file_out.writeStartElement("save_environment");
        settings_file_out.writeCharacters(QString("%1").arg(save_environment->isChecked()));
        settings_file_out.writeEndElement();

        //Strings
        settings_file_out.writeStartElement("path");
        settings_file_out.writeCharacters(path->text());
        settings_file_out.writeEndElement();

        settings_file_out.writeEndElement();

        settings_file_out.writeEndDocument();

        settings_file.close();

       setStatusBarText("File saved");
}

//ARTS - Exit the application
void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

//ARTS - Trigger to open url to gihub repository
void MainWindow::on_actionCode_on_GitHub_triggered()
{
    QDesktopServices::openUrl(QUrl(QString(GITHUB_URL) + QString(GITREPOSITORY)));
}

//ARTS - Trigger to open url to online documentation
void MainWindow::on_actionOnline_User_Manual_triggered()
{
    QDesktopServices::openUrl(QUrl(QString(READTHEDOCS_URL)));
}
