#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QActionGroup>
#include <QAction>


#include "simmanager.h"
#include "populationscene.h"
#include "environmentscene.h"
#include "genomecomparison.h"
#include "fossrecwidget.h"

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

    bool pauseflag;
    int NextRefresh;
    EnvironmentScene *envscene;
    PopulationScene *popscene;
    QActionGroup *viewgroup, *viewgroup2;
    QActionGroup *envgroup;

    //RJG - options for batching
    bool batch_running;
    int runs, batch_iterations, batch_target_runs;

    //Now some things settable by the ui
    int RefreshRate;
    QTime timer;
    QImage *pop_image, *env_image, *pop_image_colour;
    QGraphicsPixmapItem *pop_item, *env_item;

    void ResetSquare(int n, int m);
    void ResizeImageObjects();
    void WriteLog();
    void CalcSpecies();
    void HandleAnalysisTool(int code);
    Analyser *a;

    QAction *startButton, *pauseButton, *runForButton, *resetButton, *reseedButton, *runForBatchButton, *settingsButton;

private slots:
    void on_actionReset_triggered();
    void on_actionReseed_triggered();
    void on_actionStart_Sim_triggered();
    void on_actionRun_for_triggered();
    void on_actionBatch_triggered();
    void on_actionPause_Sim_triggered();
    void on_actionRefresh_Rate_triggered();
    void on_actionSettings_triggered();
    void on_actionCount_Peaks_triggered();
    void on_actionMisc_triggered();
    void view_mode_changed(QAction *);
    void report_mode_changed(QAction *);
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
    void on_actionTracking_triggered();
    void on_actionLogging_triggered();
    void on_actionFitness_logging_to_File_triggered();
    void on_actionSet_Logging_File_triggered();
    void on_actionGenerate_Tree_from_Log_File_triggered();
    void on_actionExtinction_and_Origination_Data_triggered();
    void on_actionRates_of_Change_triggered();
    void on_actionStasis_triggered();
    void on_actionLoad_Random_Numbers_triggered();
    void on_SelectLogFile_pressed();
    void on_SelectOutputFile_pressed();
};



#endif // MAINWINDOW_H
