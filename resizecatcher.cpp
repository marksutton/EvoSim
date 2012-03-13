#include "resizecatcher.h"
#include "mainwindow.h"
//Exists to act as an event filter and catch resize of mainwidget of main window - avoid needs to subclass the widget
//while still catching dock resizes

ResizeCatcher::ResizeCatcher(QObject *parent) :
    QObject(parent)
{
    return;
}

bool ResizeCatcher::eventFilter(QObject *obj, QEvent *event)
{
    //if it's a resize event - do resize via Main Window
    if (event->type() == QEvent::Resize)
        MainWin->Resize();

    return QObject::eventFilter(obj, event);
}

