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
    double xshift;
    double yshift;
    bool   center;

    bool suppress_grid;
    const char * moveto_clr;
    const char * lineto_clr;
    const char * backgd_clr;

    double tick_size;

} parms = {
  //cols, rows, 1, .1,
  cols, rows, 1, .18,
  //cols, rows, 1, .25,
  //cols, rows, 1, .5,
  //cols, rows, 1, .6,
  //cols, rows, 1, .7,
  //cols, rows, 1, .8,
  //cols, rows, 1, .9,

  //20, 20, 1, .2,
  //1, 10, 0, 0,
    1,  8, 100, 50, true,
  //1, 20, 0, 0,
    true,
    nullptr,     // don't show moveto lines
    "#009900",
    "#000000", // "#fe8736",
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

static void paint_z_blue( BWCNC::PartContext & k );
//static void paint_z_red( BWCNC::PartContext & k );

void render_eye_perspective( BWCNC::PartContext & k, double scene_width, double scene_height, bool isleft )
{
    leftrighteye3D leftright_tform;

  //leftright_tform.eye[0] = 100; // with 10' wide screen and 2048 pixels. average interpupillary distance is 2.47"
    leftright_tform.eye[0] = 21.16; // with 10' wide screen and 2048 pixels. average interpupillary distance is 2.47"
    if( isleft )                    // with bridge of nose at y == 0, (2.47/2) ... 1.24 * 2048/(10*12) =~ 21.16 pixels
        leftright_tform.eye[0] *= -1;
    leftright_tform.eye[2] = scene_width;

    k.translate( Eigen::Vector3d( -scene_width/2, -scene_height/2, 0 ) );
    k.position_dependent_transform( &leftright_tform );
    k.translate( Eigen::Vector3d( scene_width/2, scene_height/2, 0 ) );
}

#if 1
void mainwindow::refresh_hexgrid_xhatchwaves()
{
    static bool kontext_is_ready = false;
    if( ! kontext_is_ready )
    {
        kontext_is_ready = true;
        create_hexgrid_xhatchwaves();
    }

    crosshatchwaves chw_tform;

    if( ticks % 1000 == 0 )
        printf( "ticks == %d\n", ticks );

    chw_tform.ticks       = ticks * parms.tick_size;
    chw_tform.shiftscale  = (b_value - 499)/50.0;
    chw_tform.w           = (a_value - 499)/(M_PI * 100);

    double scene_width  = scene->sceneRect().width();
    double scene_height = scene->sceneRect().height();

    BWCNC::PartContext k;
    kontext.copy_into( k );

    // this transform acts periodically over a plane, so no shifting is needed.
    BWCNC::Boundingbox bbox = k.get_bbox();

    k.translate( Eigen::Vector3d( -bbox.width()/2, -bbox.height()/2, 0 ) );
    k.position_dependent_transform( &chw_tform );
    k.translate( Eigen::Vector3d( bbox.width()/2,   bbox.height()/2, 0 ) );

#if 0
    BWCNC::Boundingbox bbox = k.get_bbox();
    parms.scale = 1.5 * scene_width / bbox.width();
#else
    k.scale( parms.scale );
#endif

    QImage img( scene_width, scene_height, QImage::Format_RGB32 );
    QPainter painter;
    painter.begin(&img);

#if 1
    BWCNC::PixmapRenderer renderer_l( &img, &painter );
    renderer_l.render_positive_z = p_bool;
    renderer_l.render_negative_z = n_bool;
    renderer_l.set_backgd_color( parms.backgd_clr );

    BWCNC::PixmapRenderer renderer_r( &img, &painter );
    renderer_r.render_positive_z = p_bool;
    renderer_r.render_negative_z = n_bool;
    renderer_r.set_backgd_color( l_bool ? nullptr : parms.backgd_clr );

    BWCNC::PartContext kl;
    BWCNC::PartContext kr;

    k.copy_into( kl );
    k.copy_into( kr );

    paint_z_blue( kl );
    paint_z_blue( kr );
  //paint_z_red( kr );

    bbox = k.get_bbox();

    render_eye_perspective( kl, bbox.width(), bbox.height(), true /* isleft */ );
    render_eye_perspective( kr, bbox.width(), bbox.height(), false );

    // shift the image/grid toward the center of the image
    //kl.translate( Eigen::Vector3d( (scene_width - bbox.width())/2, (scene_height - bbox.height())/2, 0 ) );
    //kr.translate( Eigen::Vector3d( (scene_width - bbox.width())/2, (scene_height - bbox.height())/2, 0 ) );
    kl.translate( Eigen::Vector3d( parms.xshift, parms.yshift, 0 ) );
    kr.translate( Eigen::Vector3d( parms.xshift, parms.yshift, 0 ) );

    if( l_bool ) renderer_l.render_all_z_order( kl );
    if( r_bool ) renderer_r.render_all_z_order( kr );

    painter.end();
#else
    BWCNC::PixmapRenderer renderer( &img );
    renderer.render_positive_z = p_bool;
    renderer.render_negative_z = n_bool;
    renderer.set_backgd_color( parms.backgd_clr );

    paint_z_blue( k );
  //bbox = k.get_bbox();
  //k.translate( Eigen::Vector3d( (scene_width - bbox.width())/2, (scene_height - bbox.height())/2, 0 ) );
    k.translate( Eigen::Vector3d( parms.xshift, parms.yshift, 0 ) );
    renderer.render_all( k );
#endif


    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}

#else

static int leftorright = 0;

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
    BWCNC::Boundingbox bbox = k.get_bbox();

    k.translate( Eigen::Vector3d( -bbox.width()/2, -bbox.height()/2, 0 ) );
    k.position_dependent_transform( &chw_tform );
    k.translate( Eigen::Vector3d( bbox.width()/2,   bbox.height()/2, 0 ) );

    k.scale( parms.scale );

    QImage img( scene_width, scene_height, QImage::Format_RGB32 );

    bool doleft = ( leftorright++ % 2 == 0 );

    BWCNC::PixmapRenderer renderer_l( &img );
    renderer_l.render_positive_z = p_bool;
    renderer_l.render_negative_z = n_bool;
    renderer_l.set_backgd_color( parms.backgd_clr );

    BWCNC::PartContext kl;
    paint_z_blue( k );

    bbox = k.get_bbox();
    render_eye_perspective( k, bbox.width(), bbox.height(), doleft );
  //k.translate( Eigen::Vector3d( (scene_width - bbox.width())/2, (scene_height - bbox.height())/2, 0 ) );
    k.translate( Eigen::Vector3d( parms.xshift, parms.yshift, 0 ) );

    if( l_bool &&   doleft ) renderer_l.render_all( k );
    if( r_bool && ! doleft ) renderer_l.render_all( k );

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}
#endif

static void paint_z_blue( BWCNC::PartContext & k )
{
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
                    double multiplier = (avg_v[2] - least)/range;
                    int R, G, B;
                    G = R =  16 + 32 * multiplier; // =  0;
                    B     =  255 * multiplier;
                    cmd->clr = BWCNC::Color( R, G, B );
                }
            }
        }
    }
}


#if 0
static void paint_z_red( BWCNC::PartContext & k )
{
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
                    double multiplier = (avg_v[2] - least)/range;
                    int R, G, B;
                    G = B =  32 + 96 * multiplier; // =  0;
                    R     =  255 * multiplier;
                    cmd->clr = BWCNC::Color( R, G, B );

                }
            }
        }
    }
}


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
