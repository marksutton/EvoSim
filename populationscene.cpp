
/*********************************************

Subclassed QGraphicsScene object -
basically just has mouse handlers for main window


**********************************************/

#include "populationscene.h"
#include "simmanager.h"
#include <QGraphicsView>
#include <QDebug>
#include <QPointF>
#include "mainwindow.h"


PopulationScene::PopulationScene() : QGraphicsScene()
{
    selectedx=0;
    selectedy=0;
}

void PopulationScene::DoMouse(int x, int y, int button)
{
    if (button==1 && x>=0 && x<gridX && y>=0 && y<gridY)
        selectedx=x;
        selectedy=y;
        mw->RefreshReport();

    //---- ARTS: Genome Comparison UI
    if (button==2 && x>=0 && x<gridX && y>=0 && y<gridY)
        selectedx=x;
        selectedy=y;
        mw->genomeComparisonAdd();
}

void PopulationScene::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
//this gets string of calls if mouse button is held
{
    QPointF position=event->scenePos();
    int x,y;

    x=(int)position.x();
    y=(int)position.y();
    int but=0;
    if (event->button()==Qt::LeftButton) but=1;
    if (event->button()==Qt::RightButton) but=2;
    if (but>0) DoMouse(x,y,but);
    return;
}


void PopulationScene::mousePressEvent(QGraphicsSceneMouseEvent * event )
{
    QPointF position=event->scenePos();
    int x,y;

    x=(int)position.x();
    y=(int)position.y();

    int but=0;
    if (event->button()==Qt::LeftButton) but=1;
    if (event->button()==Qt::RightButton) but=2;

    DoMouse(x,y,but);
    return;
}

void PopulationScene::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event )
{
    //don't do anything
    return;
}
