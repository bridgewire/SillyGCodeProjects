#include <stdio.h>
#include <math.h>

#include <QGraphicsView>
#include <QPen>

#include <Eigen/Dense>

#include <libbwcnc/bwcnc.h>
#include <libbwcnc/stdtforms.h>

#include "mainwindow.h"

//using namespace BWCNC;

static int cols = 58;
static int rows = round(1.13 * cols * 1080 / 2048.0);


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

} parms = {
 //120, 75, 1, .2,
  //90, 50, 1, .2,
  //30, 20, 1, .1,
  //60, 32, 1, .18,

  //cols, rows, 1, .1,
  //cols, rows, 1, .18,
  //cols, rows, 1, .25,
  //cols, rows, 1, .5,
  //cols, rows, 1, .7,
    cols, rows, 1, .8,
  //cols, rows, 1, .9,

  //20, 20, 1, .2,
    1, 10, 0, 0,
  //1, 20, 0, 0,
    true,
    nullptr,     // don't show moveto lines
    "#009900",
    "#fe8736",
    .1
};

#define DO2PARTCONTEXTS 0


void mainwindow::create_hexgrid_xhatchwaves()
{
    BWCNC::HexGrid grid(
            parms.cols, parms.rows, parms.sidelength, parms.scale,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
            Eigen::Vector3d( parms.xshift, parms.yshift, 0),
            parms.lineto_clr, parms.moveto_clr, parms.backgd_clr );

    grid.fill_partctx_with_grid( kontext );
}

void mainwindow::refresh_hexgrid_xhatchwaves()
{
    static bool kontext_is_ready = false;
    if( ! kontext_is_ready )
    {
        kontext_is_ready = true;
        create_hexgrid_xhatchwaves();
    }

    crosshatchwaves chw_tform;

    chw_tform.ticks       = ticks * parms.tick_size;
    chw_tform.shiftscale  = (b_value - 499)/50.0;
    chw_tform.w           = (a_value - 499)/(M_PI * 100);

    double scene_width  = scene->sceneRect().width();
    double scene_height = scene->sceneRect().height();

    BWCNC::PartContext k;
    kontext.copy_into( k );

    // this transform acts periodically over a plane, so no shifting is needed.
    k.position_dependent_transform( &chw_tform );
#if 0
    BWCNC::Boundingbox bbox = k.get_bbox();
    parms.scale = 1.5 * scene_width / bbox.width();
#endif
    k.scale( parms.scale );


    BWCNC::Boundingbox bbox = k.get_bbox();
    double range =  bbox.depth();
    double least =  bbox.min[2];

    for( auto p : k.partlist )
    {
        if( p )
        {
            for( auto cmd : p->cmds )
            {
                if( cmd )
                {
                    Eigen::Vector3d avg_v = (cmd->begin + cmd->end) / 2;
                    int R, G, B;
                    double multiplier = (least + avg_v[2])/range;
                    G = R =  128 * multiplier;
                    B     =  255 * multiplier;
                    cmd->clr = BWCNC::Color( R, G, B );
                }
            }
        }
    }

    QImage img( scene_width, scene_height, QImage::Format_RGB32 );

    BWCNC::PixmapRenderer renderer( &img );
    renderer.renderonly_positive_z = p_bool;

    renderer.set_moveto_color( parms.moveto_clr );
    renderer.set_lineto_color( parms.lineto_clr );
    renderer.set_backgd_color( parms.backgd_clr );

    //grid.set_renderer_colors( & renderer );
    renderer.render_all( k );

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}


#if 0
void mainwindow::refresh_hexgrid_xhatchwaves()
{
    BWCNC::PartContext k;

    crosshatchwaves chw_tform;

    chw_tform.ticks       = ticks * parms.tick_size;
    chw_tform.shiftscale  = (b_value - 499)/50.0;
    chw_tform.w           = (a_value - 499)/(M_PI * 100);

#if 1
    BWCNC::HexGrid grid(
            parms.cols, parms.rows, parms.sidelength, parms.scale,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
            Eigen::Vector3d( parms.xshift, parms.yshift, 0),
            parms.lineto_clr, parms.moveto_clr, parms.backgd_clr );
#else
    BWCNC::LizardGrid grid( parms.cols, parms.rows, parms.sidelength, parms.scale );
#endif

    grid.fill_partctx_with_grid( k );

    parms.scene_width  = scene->sceneRect().width();
    parms.scene_height = scene->sceneRect().height();

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
#endif
