#include <stdio.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGraphicsView>
#include <QKeyEvent>
#include <QPen>


void mainwindow::refresh_selected_hexgrid(){ refresh_hexgrid_cylinder(); }

void mainwindow::a_slider_changed(int value) { a_value = value; refresh_selected_hexgrid(); }
void mainwindow::b_slider_changed(int value) { b_value = value; refresh_selected_hexgrid(); }


mainwindow::mainwindow(QWidget *parent)
  : QMainWindow(parent), ui(new Ui::mainwindow)
{
    ui->setupUi(this);

    //void QAbstractSlider::valueChanged(int value)

    scene = new QGraphicsScene( 0, 0, 1560, 860, this );
    ui->graphicsView->setScene( scene ); //QGraphicsView

    QPixmap pxmp;
    pixmap_item = scene->addPixmap( pxmp );

    // if the spinbox changes, change the spinbox
    QObject::connect( ui->a_hSlider, &QAbstractSlider::valueChanged, ui->a_spinBox, &QSpinBox::setValue );
    QObject::connect( ui->b_hSlider, &QAbstractSlider::valueChanged, ui->b_spinBox, &QSpinBox::setValue );

    // if the spinbox changes, change the slider
    //QObject::connect( ui->a_spinBox, &QSpinBox::valueChanged, ui->a_hSlider, &QAbstractSlider::setValue );
    //QObject::connect( ui->b_spinBox, &QSpinBox::valueChanged, ui->b_hSlider, &QAbstractSlider::setValue );

    // is qt smart enough to stop circular recursion?

    QObject::connect( ui->a_hSlider, &QAbstractSlider::valueChanged, this, &mainwindow::a_slider_changed );
    QObject::connect( ui->b_hSlider, &QAbstractSlider::valueChanged, this, &mainwindow::b_slider_changed );


    ui->a_hSlider->setValue( a_value );
    ui->b_hSlider->setValue( b_value );

    refresh_selected_hexgrid();
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
        tmr.start( tmr_interval_msec );
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
        printf( "ticks: %d\n", ticks );
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        tmr_interval_msec++;
        printf( "tmr_interval_msec: %d\n", tmr_interval_msec );
        tmr.setInterval( tmr_interval_msec );
        break;
    case Qt::Key_Minus:
        if( tmr_interval_msec > 1 )
        {
            tmr_interval_msec--;
            printf( "tmr_interval_msec: %d\n", tmr_interval_msec );
            tmr.setInterval( tmr_interval_msec );
        }
    case Qt::Key_P:
        p_bool = ! p_bool;
        break;
    }
}


