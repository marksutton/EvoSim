#ifndef POPULATIONSCENE_H
#define POPULATIONSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QString>

class MainWindow;

class PopulationScene : public QGraphicsScene
{
public:
        PopulationScene();
        int selectedx;
        int selectedy;
        MainWindow *mw;

protected:
     void mousePressEvent(QGraphicsSceneMouseEvent *event);
     void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
     void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
     void DoMouse(int x, int y, int button);
 private slots:
     void ScreenUpdate();
};


#endif // POPULATIONSCENE_H
