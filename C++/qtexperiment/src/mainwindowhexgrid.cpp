#include <stdio.h>
#include <math.h>

#include <QGraphicsView>
#include <QPen>

#include <Eigen/Dense>


#include <libbwcnc/part.h>
#include <libbwcnc/mceschliz.h>
#include <libbwcnc/bwcnc.h>
#include <libbwcnc/stdtforms.h>

#include "mainwindow.h"

//using namespace BWCNC;

void mainwindow::refresh_hexgrid()
{
    BWCNC::PartContext k;
    BWCNC::LizardGrid lg;

    lg.fill_partctx_with_grid( k );

    shift2positive( k );
    //shift2center_z( k );

    QImage img( scene->sceneRect().width(), scene->sceneRect().height(), QImage::Format_RGB32 );
    BWCNC::PixmapRenderer renderer( &img );
    renderer.renderonly_positive_z = p_bool;

    renderer.set_moveto_color( nullptr );
    renderer.set_lineto_color( "#ff0000" );
    renderer.set_backgd_color( "#fe8736" );

#if 1
    BWCNC::SVG svgrenderer;
    svgrenderer.render_all( k );
#endif

    renderer.render_all( k );

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}

