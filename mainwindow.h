/**
 * @file
 * Header: Main Window
 *
 * All REVOSIM code is released under the GNU General Public License.
 * See GNUv3License.txt files in the programme directory.
 *
 * All REVOSIM code is Copyright 2018 by Mark Sutton, Russell Garwood,
 * and Alan R.T. Spencer.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version. This program is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QActionGroup>
#include <QAction>
#include <QStandardPaths>
#include <QCheckBox>
#include <QSpinBox>
#include <QRadioButton>
#include <QShortcut>

#include "simmanager.h"
#include "populationscene.h"
#include "environmentscene.h"
#include "genomecomparison.h"
#include "fossrecwidget.h"
#include "about.h"

extern MainWindow *MainWin;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    FossRecWidget *FRW;

    //---- ARTS: Add Genome Comparison UI
    GenomeComparison *genoneComparison;
    bool genomeComparisonAdd();
    void RefreshReport();
    void Resize();
    void RefreshEnvironment();
    void setStatusBarText(QString text);
    Ui::MainWindow *ui;

    int RefreshRate;

protected:
    void changeEvent(QEvent *e);
    void resizeEvent(QResizeEvent *event);

private:
    int ScaleFails(int fails, float gens);
    void closeEvent(QCloseEvent *event);
    void Report();
    void RunSetUp();
    void FinishRun();
    void RefreshPopulations();
    void UpdateTitles();
    void resetInformationBar();

    //RJG - some imporant variables
    bool stopflag;

    bool pauseflag;
    int waitUntilPauseSignalIsEmitted();

    int NextRefresh;

    //RJG - GUI stuff
    EnvironmentScene *envscene;
    PopulationScene *popscene;
    QActionGroup *viewgroup, *viewgroup2, *speciesgroup;
    QActionGroup *envgroup;
    QDockWidget *settings_dock, *org_settings_dock, *output_settings_dock;

    //RJG - GUI buttons and settings docker options which need to be accessible via slots.
    QAction *startButton, *stopButton, *pauseButton, *runForButton, *resetButton, *reseedButton, *runForBatchButton, *settingsButton, *orgSettingsButton, *logSettingsButton, *aboutButton;
    QCheckBox *gui_checkbox, *save_population_count, *save_mean_fitness, *save_coding_genome_as_colour, *save_species, *save_non_coding_genome_as_colour, *save_gene_frequencies, *save_settles, *save_fails_settles, *save_environment;
    QRadioButton *phylogeny_off_button, *basic_phylogeny_button, *phylogeny_button, *phylogeny_and_metrics_button;
    QSpinBox *mutate_spin;
    QLineEdit *path;

    //RJG - options for batching
    bool batch_running;
    int runs, batch_iterations, batch_target_runs;

    //Now some things settable by the ui
    QTime timer;
    QImage *pop_image, *env_image, *pop_image_colour;
    QGraphicsPixmapItem *pop_item, *env_item;

    void ResetSquare(int n, int m);
    void ResizeImageObjects();
    void WriteLog();
    void CalcSpecies();
    void HandleAnalysisTool(int code);
    Analyser *a;
    QString print_settings();

private slots:
    void on_actionReset_triggered();
    void on_actionReseed_triggered();
    void on_actionStart_Sim_triggered();
    void on_actionRun_for_triggered();
    void on_actionBatch_triggered();
    void on_actionPause_Sim_triggered();
    void on_actionStop_Sim_triggered();
    void on_actionSettings_triggered();
    void orgSettings_triggered();
    void logSettings_triggered();
    void on_actionCount_Peaks_triggered();
    void on_actionMisc_triggered();
    void view_mode_changed(QAction *);
    void report_mode_changed(QAction *);
    void gui_checkbox_state_changed(bool);
    void save_all_checkbox_state_changed(bool);
    void redoImages(int oldrows, int oldcols);
    bool on_actionEnvironment_Files_triggered();
    void on_actionSave_triggered();
    void on_actionLoad_triggered();
    void on_actionChoose_Log_Directory_triggered();
    void on_actionAdd_Regular_triggered();
    void on_actionAdd_Random_triggered();
    void on_actionSet_Active_triggered();
    void on_actionSet_Inactive_triggered();
    void on_actionSet_Sparsity_triggered();
    void on_actionShow_positions_triggered();
    void on_actionFitness_logging_to_File_triggered();
    void on_actionGenerate_Tree_from_Log_File_triggered();
    void on_actionExtinction_and_Origination_Data_triggered();
    void on_actionRates_of_Change_triggered();
    void on_actionStasis_triggered();
    void on_actionLoad_Random_Numbers_triggered();
    void on_SelectLogFile_pressed();
    void species_mode_changed(int change_species_mode);
    void on_actionGenerate_NWK_tree_file_triggered();
    void on_actionSpecies_sizes_triggered();
    void changepath_triggered();
    void about_triggered();
};



#endif // MAINWINDOW_H
