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

    void create_hexgrid_xhatchwaves();
    void refresh_hexgrid_xhatchwaves();

    void create_hexgrid_cylinder();
    void refresh_hexgrid_cylinder();

    bool toggle_timer();

protected:
    int tmr_interval_msec = 10;
    int ticks = 5000;
    bool tmr_active = false;
    QTimer tmr;
    QGraphicsScene * scene = nullptr;
    QGraphicsPixmapItem * pixmap_item = nullptr;
    int a_value = 440; // 440; // 400; // 425;
    int b_value = 600; // 780; // 600; // 750;

    bool p_bool = true;
    bool n_bool = true;
    bool l_bool = true;
    bool r_bool = true;

    bool b_cmd = false;
    bool is_fullscreenmode = false;
    BWCNC::PixmapRenderer hexes;

    BWCNC::PartContext kontext;
    bool kontext_isready = false;

private:
    Ui::mainwindow * ui = nullptr;
};

#endif // MAINWINDOW_H
