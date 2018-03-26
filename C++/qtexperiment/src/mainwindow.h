#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <libbwcnc/qpixmaprenderer.h>
#include <libbwcnc/part.h>
#include <libbwcnc/concurrent.h>

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
    void set_threadcnt( int cnt ){ threadcnt = cnt; }
    bool toggle_timer();

protected:
    void refresh_selected_hexgrid();
    void refresh_hexgrid();

    void create_hexgrid_xhatchwaves();
    void refresh_hexgrid_xhatchwaves();

    void create_hexgrid_cylinder();
    void refresh_hexgrid_cylinder();

protected:
    int threadcnt = -1; // default is: same as cpu count
    int tmr_interval_msec = 10;
    int ticks = 0;
    double log_tick_stepsize = -2; //
    bool tmr_active = false;
    QTimer tmr;
    QGraphicsScene * scene = nullptr;
    QGraphicsPixmapItem * pixmap_item = nullptr;
    int a_value = 400; // 440; // 340; // 440; // 400; // 425;
    int b_value = 650; // 700; // 600; // 780; // 600; // 750;

    bool p_bool = true;
    bool n_bool = true;
    bool l_bool = true;
    bool r_bool = true;
    bool f_bool = false; // flat

    bool b_cmd = false;
    bool is_fullscreenmode = false;
    BWCNC::PixmapRenderer hexes;

    BWCNC::PartContext kontext;
    BWCNC::ShareableWorkQueueProcessor workers;

    bool kontext_isready = false;

private:
    Ui::mainwindow * ui = nullptr;
};

#endif // MAINWINDOW_H
