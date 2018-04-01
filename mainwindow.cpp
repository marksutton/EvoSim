
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
#include "analysistools.h"
#include "version.h"
#include "math.h"

#ifndef M_SQRT1_2 //not defined in all versions
#define M_SQRT1_2 0.7071067811865475
#endif

SimManager *TheSimManager;
MainWindow *MainWin;

#include <QThread>

/* To do:

-- sort out logging - get rid of logging variables other than logging bool that I have created, and implement this in a sensible way
-- Programme logo
-- Save path needs to go in there as well (not toolbar) - and button on toolbar needs to show/hide the docker, not the old window.

1.  Tidy GUI  - RG
2. Remove bits that are not for release  - RG
3. Document GUI   - RG
4. Write introduction - MS
5. Tidy how it works section - MS
6. Write discussion and applications stuff

To do coding:
Option to load/save without critter data also needed
Load and Save don't include everything - they need to!
Check how species logging actually works with analysis dock / do logging
-->Logging should have a single option to turn logging on for whole grid stuff - species and other data too. Just log the lot. This can be in settings, but also can have a icon on toolbar.
Species tracking - need a menu item for Species/Phylogeny. Includes tracking options, also a 'generate tree now' and a 'generate tree after batch' option, which trigger the Generate NWK tree item.
Genome comparison - say which is noncoding half / document

Visualisation:
Settles - does it work at all?
Fails - green scaling

To remove:
Dual reseed
Species and logging - recombination logging remove, others document
Fossil record - what does it do, does it actually work, is logging compatible with newer systems - just remove from release?
We entirely lose the Analysis Tools menu. Phylogeny settings will become part of settings.

To do long term:
Add variable mutation rate depent on population density:
-- Count number of filled slots (do as percentage of filled slots)
-- Use percentage to dictate probability of mutation (between 0 and 1), following standard normal distribution
-- But do this both ways around so really full mutation rate can be either very high, or very low

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

    startButton->setEnabled(false);
    runForButton->setEnabled(false);
    pauseButton->setEnabled(false);
    reseedButton->setEnabled(false);
    runForBatchButton->setEnabled(false);

    //---- RJG docker toggles - Mar 17.
    settingsButton = new QAction(QIcon(QPixmap(":/toolbar/globesettingsButton-Enabled.png")), QString("Simulation"), this);
    settingsButton->setCheckable(true);
    orgSettingsButton = new QAction(QIcon(QPixmap(":/toolbar/settingsButton-Enabled.png")), QString("Organism"), this);
    orgSettingsButton->setCheckable(true);
    logSettingsButton = new QAction(QIcon(QPixmap(":/toolbar/logButton-Enabled.png")), QString("Output"), this);
    logSettingsButton->setCheckable(true);

    ui->toolBar->addAction(startButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(runForButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(runForBatchButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(pauseButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(resetButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(reseedButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(orgSettingsButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(settingsButton);ui->toolBar->addSeparator();
    ui->toolBar->addAction(logSettingsButton);

    //----RJG - Connect button signals to slot. Note for clarity: Reset = start again with random individual. Reseed = start again with user defined genome
    QObject::connect(startButton, SIGNAL(triggered()), this, SLOT(on_actionStart_Sim_triggered()));
    QObject::connect(runForButton, SIGNAL(triggered()), this, SLOT(on_actionRun_for_triggered()));
    QObject::connect(pauseButton, SIGNAL(triggered()), this, SLOT(on_actionPause_Sim_triggered()));
    QObject::connect(resetButton, SIGNAL(triggered()), this, SLOT(on_actionReset_triggered()));
    QObject::connect(reseedButton, SIGNAL(triggered()), this, SLOT(on_actionReseed_triggered()));
    QObject::connect(runForBatchButton, SIGNAL(triggered()), this, SLOT(on_actionBatch_triggered()));
    QObject::connect(settingsButton, SIGNAL(triggered()), this, SLOT(on_actionSettings_triggered()));
    QObject::connect(orgSettingsButton, SIGNAL(triggered()), this, SLOT(orgSettings_triggered()));
    QObject::connect(logSettingsButton, SIGNAL(triggered()), this, SLOT(logSettings_triggered()));

    //Spacer
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    empty->setMaximumWidth(10);
    empty->setMaximumHeight(5);
    ui->toolBar->addWidget(empty);

    ui->toolBar->addSeparator();
    aboutButton = new QAction(QIcon(QPixmap(":/toolbar/aboutButton-Enabled.png")), QString("About"), this);
    ui->toolBar->addAction(aboutButton);
    QObject::connect(aboutButton, SIGNAL (triggered()), this, SLOT (about_triggered()));

    //----RJG - set up settings docker.
    settings_dock = new QDockWidget("Simulation", this);
    settings_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    settings_dock->setFeatures(QDockWidget::DockWidgetMovable);
    settings_dock->setFeatures(QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, settings_dock);

    QGridLayout *settings_grid = new QGridLayout;
    settings_grid->setAlignment(Qt::AlignTop);

    QLabel *environment_label= new QLabel("Environmental Settings");
    environment_label->setStyleSheet("font-weight: bold");
    settings_grid->addWidget(environment_label,0,1,1,2);

    QLabel *environment_rate_label = new QLabel("Environment refresh rate:");
    QSpinBox *environment_rate_spin = new QSpinBox;
    environment_rate_spin->setMinimum(0);
    environment_rate_spin->setMaximum(100000);
    environment_rate_spin->setValue(envchangerate);
    settings_grid->addWidget(environment_rate_label,1,1);
    settings_grid->addWidget(environment_rate_spin,1,2);
    //----RJG - Note in order to use a lamda not only do you need to use C++11, but there are two valueCHanged signals for spinbox - and int and a string. Need to cast it to an int
    connect(environment_rate_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i ) { envchangerate=i; });

    QCheckBox *toroidal_checkbox = new QCheckBox("Toroidal");
    toroidal_checkbox->setChecked(toroidal);
    settings_grid->addWidget(toroidal_checkbox,2,1,1,2);
    connect(toroidal_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { toroidal=i; });

    QLabel *simulation_size_label= new QLabel("Simulation size");
    simulation_size_label->setStyleSheet("font-weight: bold");
    settings_grid->addWidget(simulation_size_label,3,1,1,2);

    QLabel *gridX_label = new QLabel("Grid X:");
    QSpinBox *gridX_spin = new QSpinBox;
    gridX_spin->setMinimum(1);
    gridX_spin->setMaximum(256);
    gridX_spin->setValue(gridX);
    settings_grid->addWidget(gridX_label,4,1);
    settings_grid->addWidget(gridX_spin,4,2);
    connect(gridX_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { int oldrows=gridX; gridX=i;redoImages(oldrows,gridY);});

    QLabel *gridY_label = new QLabel("Grid Y:");
    QSpinBox *gridY_spin = new QSpinBox;
    gridY_spin->setMinimum(1);
    gridY_spin->setMaximum(256);
    gridY_spin->setValue(gridY);
    settings_grid->addWidget(gridY_label,5,1);
    settings_grid->addWidget(gridY_spin,5,2);
    connect(gridY_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {int oldcols=gridY; gridY=i;redoImages(gridX,oldcols);});

    QLabel *slots_label = new QLabel("Slots:");
    QSpinBox *slots_spin = new QSpinBox;
    slots_spin->setMinimum(1);
    slots_spin->setMaximum(256);
    slots_spin->setValue(slotsPerSq);
    settings_grid->addWidget(slots_label,6,1);
    settings_grid->addWidget(slots_spin,6,2);
    connect(slots_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { slotsPerSq=i;redoImages(gridX,gridY); });

    QLabel *simulation_settings_label= new QLabel("Simulation settings");
    simulation_settings_label->setStyleSheet("font-weight: bold");
    settings_grid->addWidget(simulation_settings_label,7,1,1,2);

    QLabel *target_label = new QLabel("Fitness target:");
    QSpinBox *target_spin = new QSpinBox;
    target_spin->setMinimum(1);
    target_spin->setMaximum(96);
    target_spin->setValue(target);
    settings_grid->addWidget(target_label,8,1);
    settings_grid->addWidget(target_spin,8,2);
    connect(target_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { target=i; });

    QLabel *energy_label = new QLabel("Energy input:");
    QSpinBox *energy_spin = new QSpinBox;
    energy_spin->setMinimum(1);
    energy_spin->setMaximum(20000);
    energy_spin->setValue(food);
    settings_grid->addWidget(energy_label,9,1);
    settings_grid->addWidget(energy_spin,9,2);
    connect(energy_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { food=i; });

    QLabel *settleTolerance_label = new QLabel("Settle tolerance:");
    QSpinBox *settleTolerance_spin = new QSpinBox;
    settleTolerance_spin->setMinimum(1);
    settleTolerance_spin->setMaximum(30);
    settleTolerance_spin->setValue(settleTolerance);
    settings_grid->addWidget(settleTolerance_label,10,1);
    settings_grid->addWidget(settleTolerance_spin,10,2);
    connect(settleTolerance_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { settleTolerance=i; });

    QCheckBox *recalcFitness_checkbox = new QCheckBox("Recalculate fitness");
    recalcFitness_checkbox->setChecked(toroidal);
    settings_grid->addWidget(recalcFitness_checkbox,11,1,1,2);
    connect(recalcFitness_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { recalcFitness=i; });

    QLabel *phylogeny_settings_label= new QLabel("Phylogeny settings");
    phylogeny_settings_label->setStyleSheet("font-weight: bold");
    settings_grid->addWidget(phylogeny_settings_label,12,1,1,1);

    QGridLayout *phylogeny_grid = new QGridLayout;
    QRadioButton *phylogeny_off_button = new QRadioButton("Off");
    QRadioButton *basic_phylogeny_button = new QRadioButton("Basic");
    QRadioButton *phylogeny_button = new QRadioButton("Phylogeny");
    QRadioButton *phylogeny_and_metrics_button = new QRadioButton("Phylogeny and metrics");
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
    settings_grid->addLayout(phylogeny_grid,13,1,1,2);

    QWidget *settings_layout_widget = new QWidget;
    settings_layout_widget->setLayout(settings_grid);
    settings_layout_widget->setMinimumWidth(300);
    settings_dock->setWidget(settings_layout_widget);
    settings_dock->adjustSize();

    //----RJG - second settings docker.
    org_settings_dock = new QDockWidget("Organism", this);
    org_settings_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    org_settings_dock->setFeatures(QDockWidget::DockWidgetMovable);
    org_settings_dock->setFeatures(QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, org_settings_dock);

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

    QCheckBox *variable_mutation_checkbox = new QCheckBox("Variable mutation");
    org_settings_grid->addWidget(variable_mutation_checkbox,3,1,1,1);
    variable_mutation_checkbox->setChecked(variableMutate);
    connect(variable_mutation_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { variableMutate=i; mutate_spin->setEnabled(!i); });

    QLabel *startAge_label = new QLabel("Chance of mutation:");
    QSpinBox *startAge_spin = new QSpinBox;
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
    QSpinBox *breedThreshold_spin = new QSpinBox;
    breedThreshold_spin->setMinimum(1);
    breedThreshold_spin->setMaximum(5000);
    breedThreshold_spin->setValue(breedThreshold);
    org_settings_grid->addWidget(breedThreshold_label,6,1);
    org_settings_grid->addWidget(breedThreshold_spin,6,2);
    connect(breedThreshold_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {breedThreshold=i;});

    QLabel *breedCost_label = new QLabel("Breed cost:");
    QSpinBox *breedCost_spin = new QSpinBox;
    breedCost_spin->setMinimum(1);
    breedCost_spin->setMaximum(10000);
    breedCost_spin->setValue(breedCost);
    org_settings_grid->addWidget(breedCost_label,7,1);
    org_settings_grid->addWidget(breedCost_spin,7,2);
    connect(breedCost_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {breedCost=i;});

    QLabel *maxDiff_label = new QLabel("Max difference to breed:");
    QSpinBox *maxDiff_spin = new QSpinBox;
    maxDiff_spin->setMinimum(1);
    maxDiff_spin->setMaximum(31);
    maxDiff_spin->setValue(maxDiff);
    org_settings_grid->addWidget(maxDiff_label,8,1);
    org_settings_grid->addWidget(maxDiff_spin,8,2);
    connect(maxDiff_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {maxDiff=i;});

    QCheckBox *breeddiff_checkbox = new QCheckBox("Use max diff to breed");
    org_settings_grid->addWidget(breeddiff_checkbox,9,1,1,1);
    breeddiff_checkbox->setChecked(breeddiff);
    connect(breeddiff_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { breeddiff=i;});

    QCheckBox *breedspecies_checkbox = new QCheckBox("Breed only within species");
    org_settings_grid->addWidget(breedspecies_checkbox,10,1,1,1);
    breedspecies_checkbox->setChecked(breedspecies);
    connect(breedspecies_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { breedspecies=i;});

    QLabel *breed_mode_label= new QLabel("Breed mode:");
    org_settings_grid->addWidget(breed_mode_label,11,1,1,2);
    QRadioButton *sexual_radio = new QRadioButton("Sexual");
    QRadioButton *asexual_radio = new QRadioButton("Asexual");
    QRadioButton *variableBreed_radio = new QRadioButton("Variable");
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
    QSpinBox *dispersal_spin = new QSpinBox;
    dispersal_spin->setMinimum(1);
    dispersal_spin->setMaximum(200);
    dispersal_spin->setValue(dispersal);
    org_settings_grid->addWidget(dispersal_label,16,1);
    org_settings_grid->addWidget(dispersal_spin,16,2);
    connect(dispersal_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {dispersal=i;});

    QCheckBox *nonspatial_checkbox = new QCheckBox("Nonspatial settling");
    org_settings_grid->addWidget(nonspatial_checkbox,17,1,1,2);
    nonspatial_checkbox->setChecked(nonspatial);
    connect(nonspatial_checkbox,&QCheckBox::stateChanged,[=](const bool &i) {nonspatial=i;});

    QLabel *pathogen_settings_label= new QLabel("Pathogen settings");
    pathogen_settings_label->setStyleSheet("font-weight: bold");
    org_settings_grid->addWidget(pathogen_settings_label,18,1,1,2);

    QCheckBox *pathogens_checkbox = new QCheckBox("Pathogens layer");
    org_settings_grid->addWidget(pathogens_checkbox,19,1,1,2);
    pathogens_checkbox->setChecked(path_on);
    connect(pathogens_checkbox,&QCheckBox::stateChanged,[=](const bool &i) {path_on=i;});

    QLabel *pathogen_mutate_label = new QLabel("Pathogen mutation:");
    QSpinBox *pathogen_mutate_spin = new QSpinBox;
    pathogen_mutate_spin->setMinimum(1);
    pathogen_mutate_spin->setMaximum(255);
    pathogen_mutate_spin->setValue(pathogen_mutate);
    org_settings_grid->addWidget(pathogen_mutate_label,20,1);
    org_settings_grid->addWidget(pathogen_mutate_spin,20,2);
    connect(pathogen_mutate_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {pathogen_mutate=i;});

    QLabel *pathogen_frequency_label = new QLabel("Pathogen frequency:");
    QSpinBox *pathogen_frequency_spin = new QSpinBox;
    pathogen_frequency_spin->setMinimum(1);
    pathogen_frequency_spin->setMaximum(1000);
    pathogen_frequency_spin->setValue(pathogen_frequency);
    org_settings_grid->addWidget(pathogen_frequency_label,21,1);
    org_settings_grid->addWidget(pathogen_frequency_spin,21,2);
    connect(pathogen_frequency_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) {pathogen_frequency=i;});

    QWidget *org_settings_layout_widget = new QWidget;
    org_settings_layout_widget->setLayout(org_settings_grid);
    org_settings_dock->setWidget(org_settings_layout_widget);

    //---- RJG Third settings docker
    output_settings_dock = new QDockWidget("Output", this);
    output_settings_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    output_settings_dock->setFeatures(QDockWidget::DockWidgetMovable);
    output_settings_dock->setFeatures(QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, output_settings_dock);

    QGridLayout *output_settings_grid = new QGridLayout;
    output_settings_grid->setAlignment(Qt::AlignTop);

    QGridLayout *images_grid = new QGridLayout;

    QLabel *images_label= new QLabel("Save Images");
    images_label->setStyleSheet("font-weight: bold");
    images_grid->addWidget(images_label,1,1,1,1);

    save_population_count = new QCheckBox("Population count");
    images_grid->addWidget(save_population_count,2,1,1,1);
    save_mean_fitness = new QCheckBox("Mean fitness");
    images_grid->addWidget(save_mean_fitness,2,2,1,1);
    save_coding_genome_as_colour = new QCheckBox("Coding genome");
    images_grid->addWidget(save_coding_genome_as_colour,3,1,1,1);
    save_non_coding_genome_as_colour = new QCheckBox("Noncoding genome");
    images_grid->addWidget(save_non_coding_genome_as_colour,3,2,1,1);
    save_species = new QCheckBox("Species");
    images_grid->addWidget(save_species,4,1,1,1);
    save_gene_frequencies = new QCheckBox("Gene frequencies");
    images_grid->addWidget(save_gene_frequencies,4,2,1,1);
    save_settles = new QCheckBox("Settles");
    images_grid->addWidget(save_settles,5,1,1,1);
    save_fails_settles = new QCheckBox("Fails + settles");
    images_grid->addWidget(save_fails_settles,5,2,1,1);
    save_environment = new QCheckBox("Environment");
    images_grid->addWidget(save_environment,6,1,1,1);

    QCheckBox *save_all_images_checkbox = new QCheckBox("All");
    save_all_images_checkbox->setStyleSheet("font-style: italic");
    images_grid->addWidget(save_all_images_checkbox,6,2,1,1);
    QObject::connect(save_all_images_checkbox, SIGNAL (toggled(bool)), this, SLOT(save_all_checkbox_state_changed(bool)));

    QLabel *output_settings_label= new QLabel("Output/GUI");
    output_settings_label->setStyleSheet("font-weight: bold");
    images_grid->addWidget(output_settings_label,7,1,1,2);

    QCheckBox *logging_checkbox = new QCheckBox("Logging");
    logging_checkbox->setChecked(logging);
    images_grid->addWidget(logging_checkbox,8,1,1,1);
    connect(logging_checkbox,&QCheckBox::stateChanged,[=](const bool &i) { logging=i; });

    gui_checkbox = new QCheckBox("Don't update GUI");
    gui_checkbox->setChecked(gui);
    images_grid->addWidget(gui_checkbox,8,2,1,1);
    QObject::connect(gui_checkbox ,SIGNAL (toggled(bool)), this, SLOT(gui_checkbox_state_changed(bool)));

    output_settings_grid->addLayout(images_grid,1,1,1,2);

    RefreshRate=50;
    QLabel *RefreshRate_label = new QLabel("Refresh/polling rate:");
    QSpinBox *RefreshRate_spin = new QSpinBox;
    RefreshRate_spin->setMinimum(1);
    RefreshRate_spin->setMaximum(10000);
    RefreshRate_spin->setValue(RefreshRate);
    output_settings_grid->addWidget(RefreshRate_label,2,1);
    output_settings_grid->addWidget(RefreshRate_spin,2,2);
    connect(RefreshRate_spin,(void(QSpinBox::*)(int))&QSpinBox::valueChanged,[=](const int &i) { RefreshRate=i; });

    //---- RJG - savepath for all functions.
    QLabel *save_path_label = new QLabel("Save path");
    save_path_label->setStyleSheet("font-weight: bold");
    output_settings_grid->addWidget(save_path_label,3,1,1,2);
    QString program_path(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    program_path.append("/");
    path = new QLineEdit(program_path);
    output_settings_grid->addWidget(path,4,1,2,2);
    QPushButton *change_path = new QPushButton("&Change");
    output_settings_grid->addWidget(change_path,6,1,1,2);
    connect(change_path, SIGNAL (clicked()), this, SLOT(changepath_triggered()));

    QWidget *output_settings_layout_widget = new QWidget;
    output_settings_layout_widget->setLayout(output_settings_grid);
    output_settings_dock->setWidget(output_settings_layout_widget);

    //RJG - Make docks tabbed
    tabifyDockWidget(org_settings_dock,settings_dock);
    tabifyDockWidget(settings_dock,output_settings_dock);
    org_settings_dock->hide();
    settings_dock->hide();
    output_settings_dock->hide();

    //RJG - Set up counts shortcut
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P), this, SLOT(on_actionCount_Peaks_triggered()));
    QObject::connect(ui->actionCount_peaks, SIGNAL(triggered()), this, SLOT(on_actionCount_Peaks_triggered()));

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
    this->setWindowTitle("REVOSIM v"+vstring+" - compiled - "+__DATE__);

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
    if ((ui->actionRecombination_logging->isChecked()||ui->actionSpecies_logging->isChecked()|| ui->actionFitness_logging_to_File->isChecked())&&!batch_running)
                                            QMessageBox::warning(this,"Logging","This will append logs from the new run onto your last one, unless you change directories or move the old log file. It will also overwrite any images within that folder.");

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
    ui->LabelBatch->setText(tr("1/1"));

    while (pauseflag==false)
    {
        Report();
        qApp->processEvents();
        if (ui->actionGo_Slow->isChecked()) Sleeper::msleep(30);
        int environment_mode=0;
        if (ui->actionOnce->isChecked()) environment_mode=1;
        if (ui->actionBounce->isChecked()) environment_mode=3;
        if (ui->actionLoop->isChecked()) environment_mode=2;
        //ARTS - set pause flag to returns true if reached end
        if (TheSimManager->iterate(environment_mode,ui->actionInterpolate->isChecked())) pauseflag=true;
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

    bool ok = false;
    int i;
    if(batch_running)
    {
        i=batch_iterations;
        if(i>=2)ok=true; //ARTS needs >=2 else you can't run an iteration value of 2
    } else {
        i= QInputDialog::getInt(this, "",tr("Iterations: "), 1000, 1, 10000000, 1, &ok);
        ui->LabelBatch->setText(tr("1/1"));
    }
    if (!ok) return;

    //ARTS - issue with pausing a batched run...
    RunSetUp();

    while (pauseflag==false && i>0)
    {
        Report();
        qApp->processEvents();
        int environment_mode=0;
        if (ui->actionOnce->isChecked()) environment_mode=1;
        if (ui->actionBounce->isChecked()) environment_mode=3;
        if (ui->actionLoop->isChecked()) environment_mode=2;
        if (TheSimManager->iterate(environment_mode,ui->actionInterpolate->isChecked())) pauseflag=true;
        FRW->MakeRecords();
        i--;
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
    form.addRow(new QLabel("You may: 1) set the number of runs you require; 2) set the number of iterations per run; and 3) chose to repeat or not to repeat the environment each run."));

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

        ui->LabelBatch->setText(tr("%1/%2").arg(1).arg(batch_target_runs));

        //qDebug() << batch_iterations;
        //qDebug() << batch_target_runs;
        //qDebug() << repeat_environment;

    } else {
        //qDebug() << "Batch Setup Canceled";
        return;
    }

    //ARTS - run the batch
    do {
        QString new_path(save_path);
        new_path.append(QString("mutate_%1_run_%2/").arg(mutate).arg(runs, 4, 10, QChar('0')));
        path->setText(new_path);

        QDir save_dir(path->text());

        save_dir.mkpath("Fitness/");

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
        ui->LabelBatch->setText(tr("%1/%2").arg((runs+1)).arg(batch_target_runs));

        on_actionRun_for_triggered();

        if(ui->actionSpecies_logging->isChecked())HandleAnalysisTool(ANALYSIS_TOOL_CODE_MAKE_NEWICK);
        if(ui->actionWrite_phylogeny->isChecked())HandleAnalysisTool(ANALYSIS_TOOL_CODE_DUMP_DATA);

        on_actionReset_triggered();

        runs++;
    } while(runs<batch_target_runs);

    path->setText(save_path);
    runs=0;
    batch_running=false;

    QMessageBox::information(0,tr("Batch Finished"),tr("The batch of %1 runs with %2 iterations has finished.").arg(batch_target_runs).arg(batch_iterations));
    ui->LabelBatch->setText(tr("1/1"));
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

    //----RJG disabled this to stop getting automatic logging at end of run, thus removing variability making analysis harder.
    //NextRefresh=0;
    //Report();
}

void MainWindow::closeEvent(QCloseEvent * /* unused */)
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

    //RJG - make path if required - this way as if user adds file name to path, this will create a subfolder with the same file name as logs
    QString save_path(path->text());
    if(!save_path.endsWith(QDir::separator()))save_path.append(QDir::separator());
    QDir save_dir(save_path);

    //check to see what the mode is
    if (ui->actionPopulation_Count->isChecked()||save_population_count->isChecked())
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
        if (save_population_count->isChecked())
                 if(save_dir.mkpath("population/"))
                             pop_image_colour->save(QString(save_dir.path()+"/population/EvoSim_population_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    if (ui->actionMean_fitness->isChecked()||save_mean_fitness->isChecked())
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
        if (save_mean_fitness->isChecked())
                 if(save_dir.mkpath("fitness/"))
                             pop_image_colour->save(QString(save_dir.path()+"/fitness/EvoSim_mean_fitness_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    if (ui->actionGenome_as_colour->isChecked()||save_coding_genome_as_colour->isChecked())
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
       if (save_coding_genome_as_colour->isChecked())
                if(save_dir.mkpath("coding/"))
                            pop_image_colour->save(QString(save_dir.path()+"/coding/EvoSim_coding_genome_it_%1.png").arg(generation, 7, 10, QChar('0')));
    }

    if (ui->actionSpecies->isChecked()||save_species->isChecked()) //do visualisation if necessary
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
         if (save_species->isChecked())
                  if(save_dir.mkpath("species/"))
                              pop_image_colour->save(QString(save_dir.path()+"/species/EvoSim_species_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    if (ui->actionNonCoding_genome_as_colour->isChecked()||save_non_coding_genome_as_colour->isChecked())
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
        if(save_non_coding_genome_as_colour->isChecked())
                 if(save_dir.mkpath("non_coding/"))
                             pop_image_colour->save(QString(save_dir.path()+"/non_coding/EvoSim_non_coding_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    if (ui->actionGene_Frequencies_012->isChecked()||save_gene_frequencies->isChecked())
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
          if(save_gene_frequencies->isChecked())
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

    if (ui->actionSettles->isChecked()||save_settles->isChecked())
    {
        //Popcount
        for (int n=0; n<gridX; n++)
        for (int m=0; m<gridY; m++)
        {
            int value=(settles[n][m]*10)/RefreshRate;
            if (value>255) value=255;
            pop_image->setPixel(n,m,value);
        }

       if(ui->actionSettles->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image));
       if(save_settles->isChecked())
               if(save_dir.mkpath("settles/"))
                           pop_image_colour->save(QString(save_dir.path()+"/settles/EvoSim_settles_it_%1.png").arg(generation, 7, 10, QChar('0')));

    }

    if (ui->actionSettle_Fails->isChecked()||save_fails_settles->isChecked())
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
        if(ui->actionSettle_Fails->isChecked())pop_item->setPixmap(QPixmap::fromImage(*pop_image_colour));
        if(save_fails_settles->isChecked())
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
    if(save_environment->isChecked())
        if(save_dir.mkpath("environment/"))
                    env_image->save(QString(save_dir.path()+"/environment/EvoSim_environment_it_%1.png").arg(generation, 7, 10, QChar('0')));
    //Draw on fossil records
    envscene->DrawLocations(FRW->FossilRecords,ui->actionShow_positions->isChecked());
}

void MainWindow::resizeEvent(QResizeEvent * /* unused */)
{
    //force a rescale of the graphic view
    Resize();
}

void MainWindow::Resize()
{
    ui->GV_Population->fitInView(pop_item,Qt::KeepAspectRatio);
    ui->GV_Environment->fitInView(env_item,Qt::KeepAspectRatio);
}

void MainWindow::view_mode_changed(QAction * /* unused */)
{
    UpdateTitles();
    RefreshPopulations();
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
    if(settings_dock->isVisible())
        {
        settings_dock->hide();
        settingsButton->setChecked(false);
        }
    else
        {
        settings_dock->show();
        settingsButton->setChecked(true);
        }
}


void MainWindow::orgSettings_triggered()
{
    if(org_settings_dock->isVisible())
        {
        org_settings_dock->hide();
        orgSettingsButton->setChecked(false);
        }
    else
        {
        org_settings_dock->show();
        orgSettingsButton->setChecked(true);
        }
}

void MainWindow::logSettings_triggered()
{
    if(output_settings_dock->isVisible())
        {
        output_settings_dock->hide();
        logSettingsButton->setChecked(false);
        }
    else
        {
        output_settings_dock->show();
        logSettingsButton->setChecked(true);
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
    if (!ui->actionRecombination_logging->isChecked() && !ui->actionFitness_logging_to_File->isChecked()) return;


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


    // ----RJG recombination logging to separate file
    if (ui->actionRecombination_logging->isChecked())
    {
        QString rFile(path->text()+"EvoSim_recombination");
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

        //RJG count breeding. Bit of a bodge, probably a better way
        int cntAsex=0, cntSex=0;
        int totalBreedAttempts=0, totalBreedFails=0;

        for (int i=0; i<gridX; i++)
                for (int j=0; j<gridY; j++)
                        {
                        for (int c=0; c<100; c++)
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
            {
            bool ok;
            int red = QInputDialog::getInt(this, "Count peaks...","Red level?", 128, 0, 255, 1, &ok);
            if(!ok)return;
            int green= QInputDialog::getInt(this, "Count peaks...","Green level?", 128, 0, 255, 1, &ok);
            if(!ok)return;
            int blue= QInputDialog::getInt(this, "Count peaks...","Green level?", 128, 0, 255, 1, &ok);
            if(!ok)return;
            OutputString = a.CountPeaks(red,green,blue);
            break;
            }

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
    if (FilenameString.length()>1 && ui->actionSpecies_logging->isChecked()) //i.e. if not blank and loging is on
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


QString MainWindow::print_settings()
{
    QString settings;
    QTextStream settings_out(&settings);

    settings_out<<"EvoSim settings - integers - Grid X: "<<gridX;
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
    settings_out<<"; Environmental change rate: "<<EnvChangeCounter;
    settings_out<<"; Years per iteration: "<<yearsPerIteration;
    settings_out<<"; EvoSim settings - bools - recalculate fitness: "<<recalcFitness;
    settings_out<<"; Toroidal environment: "<<toroidal;
    settings_out<<"; Nonspatial setling: "<<nonspatial;
    settings_out<<"; Enforce max diff to breed:"<<breeddiff;
    settings_out<<"; Only breed within species:"<<breedspecies;
    settings_out<<"; Pathogens enabled:"<<path_on;
    settings_out<<"; Variable mutate:"<<variableMutate;
    settings_out<<"; Breeding:";
    if(sexual)settings_out<<" sexual.";
    else if (asexual)settings_out<<" asexual.";
    else settings_out<<" variable.";

    return settings;
}
