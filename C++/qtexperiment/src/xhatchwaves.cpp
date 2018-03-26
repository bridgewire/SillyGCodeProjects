#include <stdio.h>
#include <math.h>

#include <QGraphicsView>
#include <QPen>

#include <Eigen/Dense>

#include <libbwcnc/bwcnc.h>
#include <libbwcnc/stdtforms.h>

#include "mainwindow.h"

//using namespace BWCNC;

static int cols = 40; //108; // 58;
static int rows = round(1.13 * cols * 1080 / 2048.0);
static double s = 8 * 58.0 / cols;

static struct cmdline_params {
    int cols;
    int rows;
    int nested;
    double nested_spacing;

    double sidelength;
    double scale;
    double xshift;
    double yshift;
    double zshift;
    bool   center;

    bool suppress_grid;
    const char * moveto_clr;
    const char * lineto_clr;
    const char * backgd_clr;

    double tick_size;
    int ticks_start;

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
    1,  s, s/7*100, s/7*50, 0, true,
  //1, 20, 0, 0,
    true,
    nullptr,     // don't show moveto lines
    "#009900",
    "#000000", // "#fe8736",
    .151515, -1600
};

#define DO2PARTCONTEXTS 0


void mainwindow::create_hexgrid_xhatchwaves()
{
    BWCNC::HexGrid grid(
            parms.cols, parms.rows, parms.sidelength,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
            parms.lineto_clr, parms.moveto_clr, parms.backgd_clr );

    grid.fill_partctx_with_grid( kontext );
    printf( "passing %d to workers.start_threads()\n", threadcnt );
    workers.start_threads( threadcnt );
    kontext.setup_shareable_workers( &workers );
}

static void paint_z_blue( const char * label, BWCNC::PartContext & ktx );
//static void paint_z_red( BWCNC::PartContext & k );

void render_eye_perspective( BWCNC::PartContext & k, double scene_width, double scene_height, bool isleft )
{
    //                                                      x                y                 z
  //leftrighteye3D leftright_tform( (isleft ? -1 : 1) * 21.16,  scene_height/4,  2 * scene_width);
    leftrighteye3D leftright_tform( (isleft ? -1 : 1) * 21.16,  scene_height/8,  2 * scene_width);
#if 0
    leftright_tform.eye[1] = 0;
    leftright_tform.eye[0] = 21.16; // with 10' wide screen and 2048 pixels. average interpupillary distance is 2.47"
    if( isleft )                    // with bridge of nose at y == 0, (2.47/2) ... 1.24 * 2048/(10*12) =~ 21.16 pixels
        leftright_tform.eye[0] *= -1;
    leftright_tform.eye[2] = 2 * scene_width;

    // with bridge of nose at y == 0, (2.47/2) ... 1.24 * 2048/(10*12) =~ 21.16 pixels
    if( isleft )
        leftright_tform.eye[0] *= -1;

    Eigen::Vector3d back2;
#endif

    //shift2( k, BWCNC::to_center, BWCNC::to_center, BWCNC::to_none, &back2 );
    k.translate( Eigen::Vector3d( -scene_width/2, -scene_height/2, 0 ) );
    k.position_dependent_transform( &leftright_tform );
    k.translate( Eigen::Vector3d( scene_width/2, scene_height/2, 0 ) );
    //undo_shift2( k, back2 );
}

#if 1
void mainwindow::refresh_hexgrid_xhatchwaves()
{
    if( ! l_bool && ! r_bool ) return;

    static bool kontext_is_ready = false;
    if( ! kontext_is_ready )
    {
        kontext_is_ready = true;
        create_hexgrid_xhatchwaves();
    }

    BWCNC::Boundingbox kontext_bbox = kontext.get_bbox();

    const double w = scene->sceneRect().width();
    const double h = scene->sceneRect().height();

    QImage img( w, h, QImage::Format_RGB32 );
    QPainter painter;

    BWCNC::PixmapRenderer renderer_l( &img, &painter );
    BWCNC::PixmapRenderer renderer_r( &img, &painter );

    renderer_l.render_positive_z = p_bool;
    renderer_l.render_negative_z = n_bool;
    renderer_l.set_backgd_color( parms.backgd_clr );

    renderer_r.render_positive_z = p_bool;
    renderer_r.render_negative_z = n_bool;
    renderer_r.set_backgd_color( l_bool ? nullptr : parms.backgd_clr );

    /////////////////////////////////////////////////////////////////////////////////////
    // changing the ticksize without any compensation will catapult the image forward or
    // backward through the sequence. what we want is for it to just speed up or slow
    // down, so when the tick-size is changed, compensate using a ticks-start offset.
    double new_tick_size = exp( log_tick_stepsize );
    parms.ticks_start += (1 - new_tick_size/parms.tick_size)*(ticks + parms.ticks_start);
    parms.tick_size = new_tick_size;
    /////////////////////////////////////////////////////////////////////////////////////

    BWCNC::PartContext kl;
    BWCNC::PartContext kr;

    crosshatchwaves chw_tform(
            (ticks + parms.ticks_start) * parms.tick_size, // ticks
            (a_value - 499)/(M_PI * 100),                  // omega
            (b_value - 499)/50.0 );                        // scaler

    if( (ticks + parms.ticks_start) % 1000 == 0 )
        printf( "ticks == %d\n", ticks + parms.ticks_start );

    BWCNC::PartContext k;
    kontext.copy_into( k );

    // this transform acts periodically over a plane, so no shifting is needed.
    BWCNC::Boundingbox bbox = k.get_bbox();

    k.translate( Eigen::Vector3d( -bbox.width()/2, -bbox.height()/2, 0 ) );
    k.position_dependent_transform( &chw_tform );
    k.translate( Eigen::Vector3d(  bbox.width()/2,   bbox.height()/2, 0 ) );

  //k.translate( Eigen::Vector3d( 0, 0, parms.zshift ) );

    double zoomscale =    (1 - ::exp( -(ticks * parms.tick_size / 10) ));
    parms.zshift = -w*(1 - zoomscale);
    parms.xshift =  w*(1 - zoomscale)/2 + 150*zoomscale;
    parms.yshift =  h*(1 - zoomscale)/2 + 100*zoomscale;


#if 0
    BWCNC::Boundingbox bbox = k.get_bbox();
    parms.scale = 1.5 * w / bbox.width();
#else
    k.scale( parms.scale );
#endif

    painter.begin(&img);

#if 1
    if( l_bool ) k.copy_into( kl );
    if( r_bool ) k.copy_into( kr );

    // shift the image/grid toward the center of the image
#if 1
    if( l_bool ) kl.translate( Eigen::Vector3d( 0, 0, parms.zshift ) );
    if( r_bool ) kr.translate( Eigen::Vector3d( 0, 0, parms.zshift ) );
#else
    bbox = kl.get_bbox();
    parms.xshift = ( w - bbox.width()  )/2;
    parms.yshift = ( h - bbox.height() )/2;
    parms.zshift = 0;
    kl.translate( Eigen::Vector3d( parms.xshift, parms.yshift, parms.zshift ) );
    kr.translate( Eigen::Vector3d( parms.xshift, parms.yshift, parms.zshift ) );
#endif


    if( ! f_bool )  // we desire flat presentation, without perspective shifts?
    {
        bbox = l_bool ? kl.get_bbox() : kr.get_bbox() ;
        if( l_bool ) render_eye_perspective( kl, bbox.width(), bbox.height(), true /* isleft */ );
        if( r_bool ) render_eye_perspective( kr, bbox.width(), bbox.height(), false );
    }

    if( l_bool ) kl.translate( Eigen::Vector3d( parms.xshift, parms.yshift, 0 ) );
    if( r_bool ) kr.translate( Eigen::Vector3d( parms.xshift, parms.yshift, 0 ) );

    if( l_bool )
        bbox = k.get_bbox();
    else if( r_bool )
        bbox = k.get_bbox();
    if( l_bool || r_bool )
        printf( "z:[%.3f %.3f]\n", bbox.min[2], bbox.max[2] );

    // make the hue variable
    if( l_bool ) paint_z_blue( "kl", kl );
    if( r_bool ) paint_z_blue( "kr", kr );

    // render the objects in z-ascending order
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
  //k.translate( Eigen::Vector3d( (w - bbox.width())/2, (h - bbox.height())/2, 0 ) );
    k.translate( Eigen::Vector3d( parms.xshift, parms.yshift, 0 ) );
    renderer.render_all( k );
#endif

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);

#if 0
    BWCNC::Boundingbox bbox_posttform = kr.get_bbox();
    BWCNC::Boundingbox bbox_diff = bbox_posttform - kontext_bbox;
    std::cout <<  "bbox diff: " << bbox_diff << "\n";
#endif
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

static void paint_z_blue( const char * /*label*/, BWCNC::PartContext & ktx )
{
    BWCNC::Boundingbox bbox = ktx.get_bbox();
    double range =  1000; // bbox.depth();
    double least =  -500; // bbox.min[2];

    //double global_zmin = 1000;
    //double global_zmax = -1000;

    double multiplier = 1;
    int R, G, B;

//  double stats[10] = {0};
//  double cmd_stats[10] = {0};

    for( auto p : ktx.partlist )
    {
        if( p )
        {
            if( p->is_closed() )
            {
                const BWCNC::Boundingbox bbox = p->get_bbox();
              //double z_avg = bbox.min[2] + bbox.depth()/2;
                double avg_z = bbox.avg()[2];

                double multiplier = (avg_z - least)/range;
                int R, G, B;
                G = R =   32 + 64 * multiplier; // =  0;
                B     =  255 * multiplier;
#if 0
                stats[0] += 1;
                stats[1] += multiplier;
                stats[3] += bbox.min[2];
                stats[4] += bbox.depth();
                stats[5] += z_avg;
                stats[6] += avg_z;
                stats[7] += (z_avg - avg_z);
#endif
                //global_zmin = global_zmin < bbox.min[2] ? global_zmin : bbox.min[2];
                //global_zmax = global_zmax > bbox.max[2] ? global_zmax : bbox.max[2];

//                if( ++i % 1000 == 0 )
//                printf( "least:%f range:%f min:%f depth:%f -> multiplier:%f [%d,%d,%d]\n",
//                        least, range, bbox.min[2], bbox.depth(), multiplier, R, G, B );

                for( auto cmd : p->cmds ) if( cmd ) cmd->clr = BWCNC::Color( R, G, B );
            }
            else
                for( auto cmd : p->cmds )
            {
                if( cmd )
                {
                    double z_avg = (cmd->begin[2] + cmd->end[2]) / 2;
                    //Eigen::Vector3d avg_v = (cmd->begin + cmd->end) / 2;
                    //multiplier = (avg_v[2] - least)/range;
                    multiplier = (z_avg - least)/range;
                    G = R =   32 + 64 * multiplier; // =  0;
                    B     =  255 * multiplier;
                    cmd->clr = BWCNC::Color( R, G, B );

#if 0
                    cmd_stats[0] += 1;
                    cmd_stats[1] += z_avg;
                    cmd_stats[2] += cmd->begin[2];
                    cmd_stats[3] += cmd->end[2];
                    cmd_stats[4] += multiplier;
#endif
                }
            }
        }
    }

#if 0
    if( cmd_stats[0] > 0 )
    {
        printf( "cmd_stats(%s): least:%f range:%f cnt:%.0f z_avg:%f begin:%f end:%f mult:%f\n",
                    label,
                    least,
                    range,
                    cmd_stats[0],
                    cmd_stats[1] / cmd_stats[0],
                    cmd_stats[2] / cmd_stats[0],
                    cmd_stats[3] / cmd_stats[0],
                    cmd_stats[4] / cmd_stats[0]
                );
#if 0
                    cmd_stats[0] += 1;
                    cmd_stats[1] += z_avg;
                    cmd_stats[2] += cmd->begin[2];
                    cmd_stats[3] += cmd->end[2];
                    cmd_stats[4] += multiplier;
#endif
    }

    if( stats[0] > 0 )
    {
        printf( "stats(%s): Glblzrange{%f,%f} least:%f range:%f cnt:%.0f mult:%f min:%f depth:%f z_avg:%f avg_z:%f (z_avg-avg_z):%f\n",
                label,
                global_zmin, global_zmax,

                least, range,
                stats[0],
                stats[1] / stats[0],
                stats[3] / stats[0],
                stats[4] / stats[0],
                stats[5] / stats[0],
                stats[6] / stats[0],
                stats[7] / stats[0] );
#if 0
                stats[0] += 1;
                stats[1] += multiplier;
                stats[3] += bbox.min[2];
                stats[4] += bbox.depth();
                stats[5] += z_avg;
                stats[6] += avg_z;
                stats[7] += (z_avg - avg_z);
#endif
    }
#endif
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
            parms.cols, parms.rows, parms.sidelength,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
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
