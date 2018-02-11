#include <stdio.h>
#include <math.h>

#include <QGraphicsView>
#include <QPen>

#include <Eigen/Dense>

#include <libbwcnc/bwcnc.h>
#include <libbwcnc/stdtforms.h>

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
    .1, .4, .4
};

#define DO2PARTCONTEXTS 0


void mainwindow::refresh_hexgrid_xhatchwaves()
{
    BWCNC::PartContext k;

    crosshatchwaves chw_tform;

    chw_tform.ticks       = ticks * parms.tick_size;
    chw_tform.shiftscale  = (b_value - 499)/50.0;
    chw_tform.w           = (a_value - 499)/(M_PI * 100);

#if 0
    BWCNC::HexGrid grid(
            parms.cols, parms.rows, parms.sidelength, parms.scale,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
            Eigen::Vector3d( parms.xshift, parms.yshift, 0),
            parms.lineto_clr, parms.moveto_clr, parms.backgd_clr );
#else
    BWCNC::LizardGrid grid( parms.cols, parms.rows, parms.sidelength, parms.scale );
#endif

    grid.fill_partctx_with_grid( k );
    //k.remake_boundingbox();

    parms.scene_width  = scene->sceneRect().width();
    parms.scene_height = scene->sceneRect().height();

    //shift2center(k);

#if DO2PARTCONTEXTS
    BWCNC::PartContext k2;
    k.copy_into( k2 );
    crosshatchwaves chw_tform2;
    chw_tform2.ticks = (ticks + 10) * parms.tick_size;
    chw_tform2.shiftscale = chw_tform.shiftscale;
    chw_tform2.w          = chw_tform.w;
    k2.position_dependent_transform( &chw_tform2 );
    k2.scale( parms.scale );
#endif

    k.position_dependent_transform( &chw_tform );
    k.scale( parms.scale );

    QImage img( scene->sceneRect().width(),
                scene->sceneRect().height(),
                QImage::Format_RGB32 );

    BWCNC::PixmapRenderer renderer( &img );

    grid.set_renderer_colors( & renderer );
    renderer.render_all( k );

#if DO2PARTCONTEXTS
    renderer.set_backgd_color( nullptr );
    renderer.set_lineto_color( "#ff0000" );
    renderer.render_all( k2 );
#endif

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}

