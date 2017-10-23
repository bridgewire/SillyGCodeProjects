#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include "qpixmaprenderer.h"

namespace Ui {
class mainwindow;
}

class mainwindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit mainwindow(QWidget *parent = 0);
    ~mainwindow();

public Q_SLOTS:
    void a_slider_changed(int value);
    void b_slider_changed(int value);

    void keyPressEvent(QKeyEvent*);
    void timer_event();

protected:
    //void refresh_img();
    void refresh_hexgrid();
    bool toggle_timer();

protected:
    int tmr_interval_msec = 100;
    int ticks = 0;
    bool tmr_active = false;
    QTimer tmr;
    QPainter p;
    QGraphicsScene * scene = nullptr;
    QGraphicsPixmapItem * pixmap_item = nullptr;
    int a_value = 0;
    int b_value = 0;
    PixmapRenderer hexes;

private:
    Ui::mainwindow *ui = nullptr;
};

#endif // MAINWINDOW_H
