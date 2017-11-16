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
    void refresh_selected_hexgrid();
    void refresh_hexgrid();

    void refresh_hexgrid_xhatchwaves();

    void create_hexgrid_cylinder();
    void refresh_hexgrid_cylinder();

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
    int b_value = 500;
    bool p_bool = false;
    bool b_cmd = false;
    PixmapRenderer hexes;

    BWCNC::PartContext kontext;
    bool kontext_isready = false;

private:
    Ui::mainwindow * ui = nullptr;
};

#endif // MAINWINDOW_H
