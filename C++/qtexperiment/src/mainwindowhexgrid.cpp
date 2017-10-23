#include <stdio.h>
#include <math.h>

#include <QGraphicsView>
#include <QPen>

#include <Eigen/Dense>

#include <bwcnc.h>

#include "mainwindow.h"

//using namespace BWCNC;


static struct cmdline_params {
    int cols;
    int rows;
    int nested;
    double nested_spacing;

    double sidelength;
    double scale;
    double yshift;
    double xshift;

    bool suppress_grid;
    const char * moveto_clr;
    const char * lineto_clr;
    const char * backgd_clr;

    int a_input;
    int b_input;
    int ticks;

    double scene_width;
    double scene_height;

} parms = {
  //90, 50, 1, .2,
  //40, 25, 1, .2,
    20, 10, 1, .2,
  //1, 10, 0, 0,
    5, 10, 0, 0,
    true,
    nullptr,     // don't show moveto lines
    "#ff0000",
    "#fe8736",
    1, 1, 0,
    .4, .4
};

#if 0
static const Eigen::Vector3d shift_tform( const Eigen::Vector3d & v )
{
    const double w  = parms.sidelength * M_PI;
    const double u  = parms.sidelength * (1 + parms.b_input) / 400.0;
    const double o =  (parms.sidelength * parms.a_input / 50.0)/2;

    const double x = u * cos(( v[0] + o ) / w);
    const double y = u * cos(( v[1] - o ) / w);

    return Eigen::Vector3d( x, y, 0 );
}
#else
static const Eigen::Vector3d shift_tform( const Eigen::Vector3d & v )
{
    //const double w  = 2.0 * M_PI / 40.0
    const double w  = 1 / (3 * M_PI);
    //const double u  = parms.sidelength * (1 + ) / 400.0;
    //const double o =  (parms.sidelength * parms.a_input / 50.0)/2;

    const double x = 7.0 * sin( w * ( v[1] + parms.b_input + parms.ticks ) );
    const double y = 7.0 * sin( w * ( v[0] + parms.b_input + parms.ticks ) );

    return Eigen::Vector3d( x, y, 0 );
}
#endif

#if 0
static const Eigen::Matrix3d skew_tform( const Eigen::Vector3d & v )
{
    //const double w = (2 * M_PI)/15.0;
    const double w = 5000*(2 * M_PI)/(parms.a_input + 1);

    Eigen::Matrix3d mat;

    mat <<  1.8 * sin(w*(v[1] + parms.yshift)), 0, 0,
            0, 1.8 * sin(w*(v[0] + parms.xshift)), 0,
            0, 0, 0 ;
    return mat;
}
#endif

static void shift2center(   BWCNC::PartContext & k, Eigen::Vector3d * offset = nullptr );
static void shift2positive( BWCNC::PartContext & k, const Eigen::Vector3d * offset = nullptr );


void mainwindow::refresh_hexgrid()
{
  //static const Eigen::Vector3d offset(5,5,0);
    BWCNC::PartContext k;

    parms.a_input = a_value;
    parms.b_input = b_value;
    parms.ticks   = ticks;

    printf( "refreshing with ticks: %d\n", ticks );

    if( parms.a_input > 1 )
    {
        parms.cols = 20 + int(80*(parms.a_input/1000.0));
        parms.rows = 10 + int(30*(parms.a_input/1000.0));
        parms.sidelength = 1.0 + 4.0*(1.0 - parms.a_input/1000.0);
    }
    else
    {
        parms.cols = 20;
        parms.cols = 10;
        parms.sidelength = 5;
    }


    BWCNC::HexGrid hxgrd(
            parms.cols, parms.rows, parms.sidelength, parms.scale,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
            Eigen::Vector3d( parms.xshift, parms.yshift, 0),
            parms.lineto_clr, parms.moveto_clr, parms.backgd_clr );

    hxgrd.fill_partctx_with_hexgrid( k );
    //k.remake_boundingbox();

    parms.scene_width  = scene->sceneRect().width();
    parms.scene_height = scene->sceneRect().height();

    //shift2center(k);

  //k.position_dependent_transform( skew_tform, shift_tform );
  //k.position_dependent_transform( skew_tform, nullptr );
    k.position_dependent_transform( nullptr, shift_tform );
    k.remake_boundingbox();

    qreal w = scene->sceneRect().width(); // QRectF
    qreal h = scene->sceneRect().height(); // QRectF
    //printf( "w:%f, h:%f\n", w, h );

#if 0
    BWCNC::Boundingbox bbox = k.get_bbox();
    double bbw = bbox.width();
    double bbh = bbox.height();

    double hrtio = h/bbh;
    double wrtio = w/bbw;

    // just scale to the smaller of two possibles
    k.scale( hrtio > wrtio ? wrtio : hrtio );
#else
    k.scale( parms.scale );
#endif

  //shift2positive(k, &offset);
    //shift2positive(k);

    QImage img( w, h, QImage::Format_RGB32 );

    PixmapRenderer renderer( &img );
    hxgrd.set_renderer_colors( & renderer );

    renderer.render_all( k );

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}

static void shift2center(  BWCNC::PartContext & k, Eigen::Vector3d * )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    Eigen::Vector3d max = bbox.max;
    k.translate( Eigen::Vector3d( -fabs(max[0] - min[0])/2.0, -fabs(max[1] - min[1])/2.0, 0 ) );
}

static void shift2positive( BWCNC::PartContext & k, const Eigen::Vector3d * offset )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d shift = -1 * bbox.min;

    if( offset ) shift -= *offset;

    k.translate( shift );
}

