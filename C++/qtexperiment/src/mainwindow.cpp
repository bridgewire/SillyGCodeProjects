#include <stdio.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGraphicsView>
#include <QWindow>
#include <QKeyEvent>
#include <QPen>


//void mainwindow::refresh_selected_hexgrid(){ refresh_hexgrid_cylinder(); }
//void mainwindow::refresh_selected_hexgrid(){ refresh_hexgrid(); }
void mainwindow::refresh_selected_hexgrid(){ refresh_hexgrid_xhatchwaves(); }


void mainwindow::a_slider_changed(int value) { a_value = value; refresh_selected_hexgrid(); }
void mainwindow::b_slider_changed(int value) { b_value = value; refresh_selected_hexgrid(); }


mainwindow::mainwindow( QWidget *parent)
  : QMainWindow(parent), ui(new Ui::mainwindow)
{
    ui->setupUi(this);

  //void QAbstractSlider::valueChanged(int value)

  //scene = new QGraphicsScene( 0, 0, 1560, 860, this );
  //scene = new QGraphicsScene( 0, 0, 2048/2, 1080/2, this );
    scene = new QGraphicsScene( 0, 0, mainwindow::frame_size_x(), mainwindow::frame_size_y(), this );
    ui->graphicsView->setScene( scene ); //QGraphicsView

    QPixmap pxmp;
    pixmap_item = scene->addPixmap( pxmp );

    // if the spinbox changes, change the spinbox
    QObject::connect( ui->a_hSlider, &QAbstractSlider::valueChanged, ui->a_spinBox, &QSpinBox::setValue );
    QObject::connect( ui->b_hSlider, &QAbstractSlider::valueChanged, ui->b_spinBox, &QSpinBox::setValue );

    // if the spinbox changes, change the slider
    // is qt smart enough to stop circular recursion?
    //QObject::connect( ui->a_spinBox, &QSpinBox::valueChanged, ui->a_hSlider, &QAbstractSlider::setValue );
    //QObject::connect( ui->b_spinBox, &QSpinBox::valueChanged, ui->b_hSlider, &QAbstractSlider::setValue );


    ui->a_hSlider->setValue( a_value );
    ui->b_hSlider->setValue( b_value );

    QObject::connect( ui->a_hSlider, &QAbstractSlider::valueChanged, this, &mainwindow::a_slider_changed );
    QObject::connect( ui->b_hSlider, &QAbstractSlider::valueChanged, this, &mainwindow::b_slider_changed );

    //refresh_selected_hexgrid();
}

mainwindow::~mainwindow()
{
    delete ui;
    printf( "~mainwindow() deleted ui\n" );
}

void mainwindow::timer_event()
{
    ticks++;
    refresh_selected_hexgrid();
}

bool mainwindow::toggle_timer()
{
    if( tmr_active )
        tmr.stop();
    else
    {
        QObject::connect( &tmr, &QTimer::timeout, this, &mainwindow::timer_event );
        //tmr.start( tmr_interval_msec );
        tmr.start( 1 );
    }
    tmr_active = !  tmr_active;
    return tmr_active;
}


void mainwindow::keyPressEvent( QKeyEvent * e )
{
    switch( e->key() )
    {
    case Qt::Key_Escape:
    case Qt::Key_Q:
        qApp->quit();
        break;
    case Qt::Key_T:
        toggle_timer();
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        log_tick_stepsize += .01;
#if 0
        tmr_interval_msec++;
        printf( "tmr_interval_msec: %d\n", tmr_interval_msec );
        tmr.setInterval( tmr_interval_msec );
#endif
        break;
    case Qt::Key_Minus:
        log_tick_stepsize -= .01;
#if 0
        if( tmr_interval_msec > 1 )
        {
            tmr_interval_msec--;
            printf( "tmr_interval_msec: %d\n", tmr_interval_msec );
            tmr.setInterval( tmr_interval_msec );
        }
#endif
        break;
    case Qt::Key_F: f_bool = ! f_bool; break;
    case Qt::Key_R: r_bool = ! r_bool; break;
    case Qt::Key_L: l_bool = ! l_bool; break;
    case Qt::Key_P: p_bool = ! p_bool; break;
    case Qt::Key_N: n_bool = ! n_bool; break;
    case Qt::Key_B:
        b_cmd = true;
        break;
    case Qt::Key_F11:
        printf( "F11 pressed\n" );
        setVisible(true);
        if( is_fullscreenmode )
        {
            showMaximized();
            //showFullScreen();
            ui->graphicsView->showFullScreen();
        }
        else
        {
            ui->graphicsView->showNormal();
            showNormal();
        }

        is_fullscreenmode = ! is_fullscreenmode;
        break;
    case Qt::Key_S:
        ticks += 100;
        break;
    case Qt::Key_V:
        z_param += .01 * ((e->modifiers() & Qt::ShiftModifier) ? 1 : -1);
        if(      z_param > 1 ) z_param = 1;
        else if( z_param < 0 ) z_param = 0;
        break;
    case Qt::Key_Z:
        z_shift += ((e->modifiers() & Qt::ShiftModifier) ? 10 : -10);
        break;
    case Qt::Key_H:
    case Qt::Key_Question:
        printf( "-----------------------\n" );
        printf( "Q       :: quit\n" );
        printf( "Esc     :: quit\n" );
        printf( "S       :: add 100 to ticks\n" );
        printf( "T       :: toggle timer\n" );
        printf( "+/=     :: increase exp(tick_stepsize)\n" );
        printf( "-       :: decrease exp(tick_stepsize)\n" );
        printf( "f       :: toggle flat perspectiv\n" );
        printf( "r       :: toggle right-eye frame visibility\n" );
        printf( "l       :: toggle left-eye frame visibility\n" );
        printf( "n       :: toggle negative z visibility\n" );
        printf( "p       :: toggle positive z visibility\n" );
        printf( "z       :: decrease z shift\n" );
        printf( "Z       :: increase z shift\n" );
//      printf( "b       :: set b_cmd == true\n" );
        printf( "[0-9]   :: ticks = [0-9]*1000\n" );
        printf( "F11     :: make a futile attempt at full-screen toggle\n" );
        printf( "-----------------------\n" );
        break;
    case Qt::Key_0: ticks = 0;  break;
    case Qt::Key_1: ticks = 1000;  break;
    case Qt::Key_2: ticks = 2000;  break;
    case Qt::Key_3: ticks = 3000;  break;
    case Qt::Key_4: ticks = 4000;  break;
    case Qt::Key_5: ticks = 5000;  break;
    case Qt::Key_6: ticks = 6000;  break;
    case Qt::Key_7: ticks = 7000;  break;
    case Qt::Key_8: ticks = 8000;  break;
    case Qt::Key_9: ticks = 9000;  break;
    }

    switch( e->key() )
    {
    case Qt::Key_T:
    case Qt::Key_S:
    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
        printf( "ticks: %d\n", ticks );
        break;
    }
}


