#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QActionGroup>
#include <QAction>


#include "simmanager.h"
#include "populationscene.h"
#include "environmentscene.h"
#include "genomecomparison.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void RefreshReport();

    //---- ARTS: Add Genome Comparison UI
    GenomeComparison *genoneComparison;
    bool genomeComparisonAdd();

protected:
    void changeEvent(QEvent *e);
    void resizeEvent(QResizeEvent *event);

private:
    void closeEvent(QCloseEvent *event);
    void Report();
    void RunSetUp();
    void FinishRun();
    void RefreshPopulations();
    void RefreshEnvironment();
    void UpdateTitles();

    bool pauseflag;
    int NextRefresh;
    EnvironmentScene *envscene;
    PopulationScene *popscene;
    QActionGroup *viewgroup, *viewgroup2;
    QActionGroup *envgroup;
    //Now some things settable by the ui
    int RefreshRate;
    QTime timer;
    QImage *pop_image, *env_image, *pop_image_colour;
    QGraphicsPixmapItem *pop_item, *env_item;
    Ui::MainWindow *ui;
    void ResetSquare(int n, int m);

    QAction *startButton, *pauseButton, *runForButton, *resetButton;

private slots:
    void on_actionReseed_triggered();
    void on_actionStart_Sim_triggered();
    void on_actionRun_for_triggered();
    void on_actionPause_Sim_triggered();
    void on_actionRefresh_Rate_triggered();
    void on_actionSettings_triggered();
    void on_actionCount_Peaks_triggered();
    void on_actionMisc_triggered();
    void view_mode_changed(QAction *);
    void report_mode_changed(QAction *);
    void on_actionEnvironment_Files_triggered();
    void on_actionSave_triggered();
    void on_actionLoad_triggered();

};

#endif // MAINWINDOW_H
