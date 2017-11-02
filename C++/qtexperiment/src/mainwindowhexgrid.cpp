#include <stdio.h>
#include <math.h>

#include <QGraphicsView>
#include <QPen>

#include <Eigen/Dense>

#include <bwcnc.h>

#include "mainwindow.h"
#include "stdtforms.h"

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
    double tick_size;

    double scene_width;
    double scene_height;

} parms = {
  //90, 50, 1, .2,
    30, 20, 1, .1,
  //30, 20, 1, .2,
    1, 20, 0, 0,
  //1, 30, 0, 0,
    true,
    nullptr,     // don't show moveto lines
    "#009900",
    "#fe8736",
    1, 1, 0,
    .1,
    .4, .4
};


void mainwindow::refresh_hexgrid()
{
  //static const Eigen::Vector3d offset(5,5,0);
    BWCNC::PartContext k;
    BWCNC::PartContext k2;

    crosshatchwaves chw_tform;
    crosshatchwaves chw_tform2;

    parms.a_input = a_value;
    parms.b_input = b_value;
    parms.ticks   = ticks;

    chw_tform.ticks  = (ticks + 10) * parms.tick_size;
    chw_tform2.ticks =  ticks       * parms.tick_size;

    chw_tform2.shiftscale = chw_tform.shiftscale = (b_value - 499)/50.0;
#if 1
    chw_tform2.w          = chw_tform.w          =  (a_value - 499)/(M_PI * 100);
#else
    if( parms.a_input > 1 )
    {
        parms.cols = 30 + int(80*(parms.a_input/1000.0));
        parms.rows = 20 + int(30*(parms.a_input/1000.0));
        parms.sidelength = 1.0 + 4.0*(1.0 - parms.a_input/1000.0);
    }
    else
    {
        parms.cols = 30;
        parms.cols = 20;
        parms.sidelength = 5;
    }
#endif

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

    k.copy_into( k2 );

  //k.position_dependent_transform( skew_tform, shift_tform );
  //k.position_dependent_transform( skew_tform, nullptr );
  //k.position_dependent_transform( nullptr, shift_tform );
    k.position_dependent_transform( &chw_tform );
    k2.position_dependent_transform( &chw_tform2 );

  //k.remake_boundingbox();

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
    k2.scale( parms.scale );
#endif

  //shift2positive(k, &offset);
    //shift2positive(k);

    QImage img( w, h, QImage::Format_RGB32 );

    PixmapRenderer renderer( &img );

    hxgrd.set_renderer_colors( & renderer );
    renderer.render_all( k );

    renderer.set_backgd_color( nullptr );
    renderer.set_lineto_color( "#ff0000" );
    renderer.render_all( k2 );

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}

#if 0
        k.copy_into( kcopy );
        kcopy.remake_boundingbox();
        shift2center( kcopy );
        kcopy.position_dependent_transform( rotation_tform, nullptr );
        kcopy.remake_boundingbox();
        shift2positive( kcopy );
        kcopy.remake_boundingbox();

        //renderer.render_all( k );

        BWCNC::SVG renderer2;
        kcopy.remake_boundingbox();
        renderer2.render_all( kcopy );

#endif
