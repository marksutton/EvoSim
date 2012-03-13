#ifndef RESIZECATCHER_H
#define RESIZECATCHER_H


#include <QObject>
#include <QEvent>

class ResizeCatcher : public QObject
{
    Q_OBJECT
public:
    explicit ResizeCatcher(QObject *parent = 0);

signals:

public slots:

protected:
    bool eventFilter(QObject *obj, QEvent *event);

};

#endif // RESIZECATCHER_H
