#include <stdio.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGraphicsView>
#include <QKeyEvent>
#include <QPen>

void mainwindow::a_slider_changed(int value) { a_value = value; refresh_hexgrid(); }
void mainwindow::b_slider_changed(int value) { b_value = value; refresh_hexgrid(); }


mainwindow::mainwindow(QWidget *parent)
  : QMainWindow(parent),
    tmr_interval_msec(100),
    ticks(0), tmr_active(false),
    scene(nullptr), pixmap_item(nullptr),
    a_value(0), b_value(500),
    ui(new Ui::mainwindow)
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

    refresh_hexgrid();
}

mainwindow::~mainwindow()
{
    delete ui;
    printf( "~mainwindow() deleted ui\n" );
}

#if 0
void mainwindow::refresh_img()
{
    QPen linto_pen(Qt::red);
    linto_pen.setWidth(1);

    // scene->clear();
    qreal w = scene->sceneRect().width(); // QRectF
    qreal h = scene->sceneRect().height(); // QRectF
    printf( "w:%f, h:%f\n", w, h );
    QImage img( w, h, QImage::Format_RGB32 );
    img.fill( Qt::white );

    p.begin( &img );
    p.drawLine( (a_value * w)/1000, 0, w, (b_value * h)/1000 );
    p.end();

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}
#endif

void mainwindow::timer_event()
{
    ticks++;
    refresh_hexgrid();
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
        break;
    }
}


