#ifndef ENVIRONMENTSCENE_H
#define ENVIRONMENTSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QString>
#include <QList>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include "fossilrecord.h"

class MainWindow;

class EnvironmentScene : public QGraphicsScene
{
public:
        EnvironmentScene();
        MainWindow *mw;
        void DrawLocations(QList <FossilRecord *> frlist, bool show);
        int button;
        int grabbed;

protected:
     void mousePressEvent(QGraphicsSceneMouseEvent *event);
     void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
     void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
     QList<QGraphicsLineItem *> HorizontalLineList;
     QList<QGraphicsLineItem *> VerticalLineList;
     QList<QGraphicsSimpleTextItem *> LabelsList;
     void DoMouse(int x, int y);
 private slots:
     void ScreenUpdate();
};

#endif // ENVIRONMENTSCENE_H
