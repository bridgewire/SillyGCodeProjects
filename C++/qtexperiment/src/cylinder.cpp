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
    50, 20, 1, .2,
  //50, 10, 1, .2,
  //20, 20, 1, .2,
  //30, 20, 1, .1,
  //30, 20, 1, .2,
    1, 10, 0, 0,
  //1, 30, 0, 0,
    true,
    nullptr,     // don't show moveto lines
    "#000000",
    "#fe8736",
    1, 1, 0,
    .1,
    .4, .4
};

class rotationZ : public BWCNC::position_dependent_transform_t
{
public:
    double t; // theta
public:
    rotationZ( double angle = .01 )
        : position_dependent_transform_t(false, true), t(angle)
    {}
    virtual ~rotationZ(){}

    const Eigen::Matrix3d mvf( const Eigen::Vector3d & ) { Eigen::Matrix3d mat; mat << ::cos(t), -::sin(t), 0,   ::sin(t), ::cos(t), 0,   0, 0, 1; return mat; }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & ) { return Eigen::Vector3d(0,0,0); }
};

class rotationY : public BWCNC::position_dependent_transform_t
{
public:
    double t; // theta
public:
    rotationY( double angle = .01 )
        : position_dependent_transform_t(false, true), t(angle)
    {}
    virtual ~rotationY(){}

    const Eigen::Matrix3d mvf( const Eigen::Vector3d & ) { Eigen::Matrix3d mat; mat << ::cos(t), 0, ::sin(t),   0, 1, 0,   -::sin(t), 0, ::cos(t); return mat; }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & ) { return Eigen::Vector3d(0,0,0); }
};

class skew_X : public BWCNC::position_dependent_transform_t
{
public:
    double shiftratio; // theta
public:
    skew_X( double ratio = .01 )
        : position_dependent_transform_t(false, true), shiftratio(ratio)
    {}
    virtual ~skew_X(){}

    const Eigen::Matrix3d mvf( const Eigen::Vector3d & ) { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & v ) { return Eigen::Vector3d(0,shiftratio*v[0],0); }
};



class mkcylinder : public BWCNC::position_dependent_transform_t
{
public:
    //double ticks;

    double x_max;
    double x_min;
    double y_max;
    double y_min;

    //double shiftscale;
public:
    mkcylinder()
        : position_dependent_transform_t(false, true), x_max(0), x_min(0), y_max(0), y_min(0)
          //ticks(0), w(1/(3*M_PI)), shiftscale(7)
    {}
    virtual ~mkcylinder(){}

    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & v );
};

//class mkcylinder : public BWCNC::position_dependent_transform_t

const Eigen::Vector3d mkcylinder::vvf( const Eigen::Vector3d & v )
{
    // bend the grid from the xy plane so that it forms a round cylinder,
    // with radius r, centered on a line parallel to the y-axis through (0,r).
    // A transform that works on the part of the grid in the x-axis will work
    // perfectly on the whole grid. It bends from the x-axis into a circle in
    // the xz-plane.
    //
    // the grid is rectangular, extending from (-x_max,x_max), and 2*x_max will
    // be the circumferance. note that c = 2*pi*r = 2*x_max so r = x_max/pi
    //
    // pi*x/x_max maps to [-pi,pi]  using this:
    // set f:[-x_max,x_max]->[-pi,pi]; f(x) = pi*x/x_max
    // set t:[-x_max,x_max]->[-pi/2,3*pi/2]; f(x) = f(x) + pi/2
    //
    // we want x < 0 to generate the part of the cylinder in the -x region, where x==-x_max makes the point (-0,y,2*r), for all y
    //
    // (-0,y,2*r) == (0,y,r) + r*( cos(f(x)+pi/2), 0, sin(f(x)+pi/2) )
    //
    // the function we want will map: { x : [-x_max,x_max] } to (0,y,r) + r*(cos(pi/2 + pi*x/x_max), 0, sin(pi/2 + pi*x/x_max))
    // the function we want will map: { x : [-x_max,x_max] } to (0,y,r) + r*(cos(pi/2 + f(x)),       0, sin(pi/2 + f(x)))
    //
    // that'll work
    // that is, that is the map we want. that ending vector is what we want. an additional problem is that the vector
    // this function returns will be added to v to produce the final used vector, so...  subtract v from our desired restult, and return it.


  //double pi = M_PI;
    double x = v[0];
    double r = x_max/M_PI;
    double f = x/r; // == M_PI*x/x_max;
    double t = f + M_PI_2;

    return  (Eigen::Vector3d(0,v[1],r) + r * Eigen::Vector3d( ::cos(t), 0, ::sin(t) )) - v ;
}

static void shift2center_z( BWCNC::PartContext & k )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    Eigen::Vector3d max = bbox.max;
    k.translate( Eigen::Vector3d( 0, 0, -fabs(max[2] - min[2])/2.0 ) );
}


static void shift2center( BWCNC::PartContext & k )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    Eigen::Vector3d max = bbox.max;
    k.translate( Eigen::Vector3d( -fabs(max[0] - min[0])/2.0, -fabs(max[1] - min[1])/2.0, -fabs(max[2] - min[2])/2.0 ) );
}

static void shift2positive( BWCNC::PartContext & k )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    k.translate( Eigen::Vector3d(-min[0], -min[1], -min[2]) );
  //k.translate( Eigen::Vector3d(-min[0], -min[1], 0) );
}

void mainwindow::create_hexgrid_cylinder()
{
  //BWCNC::PartContext k;

    BWCNC::HexGrid hxgrd(
            parms.cols, parms.rows, parms.sidelength, parms.scale,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
            Eigen::Vector3d( parms.xshift, parms.yshift, 0),
            parms.lineto_clr, parms.moveto_clr, parms.backgd_clr );

    hxgrd.fill_partctx_with_hexgrid( kontext );

    kontext.scale( parms.scale );
    kontext.remake_boundingbox();

    shift2center(kontext);

    skew_X skew(1/30.0);
    kontext.position_dependent_transform( &skew );
    kontext.remake_boundingbox();

    BWCNC::Boundingbox bbox = kontext.get_bbox();
    mkcylinder cyltform;
    cyltform.x_max = 0.995 * bbox.max[0];
    kontext.position_dependent_transform( &cyltform );
    kontext.remake_boundingbox();

  //hxgrd.set_renderer_colors( & renderer );

    kontext_isready = true;
}


void mainwindow::refresh_hexgrid_cylinder()
{
  //static const Eigen::Vector3d offset(5,5,0);
    BWCNC::PartContext k;

    if( ! kontext_isready )
        create_hexgrid_cylinder();

    rotationY rotY( M_PI * (a_value - 500)/50000.0 );

    //const Eigen::Matrix3d mvf( const Eigen::Vector3d & ) { Eigen::Matrix3d mat; mat << ::cos(t), 0, -::sin(t),   0, 1, 0,   ::sin(t), 0, ::cos(t); return mat; }
  //const Eigen::Matrix3d mat = rotY.mvf( Eigen::Vector3d(0,0,0) );
  //std::cerr << mat;
    kontext.transform( rotY.mvf( Eigen::Vector3d(0,0,0) ) );

    kontext.copy_into( k );
    k.remake_boundingbox();
    shift2positive( k );
    shift2center_z( k );

    QImage img( scene->sceneRect().width(), scene->sceneRect().height(), QImage::Format_RGB32 );
    PixmapRenderer renderer( &img );
    renderer.renderonly_positive_z = p_bool;

    renderer.set_moveto_color( parms.moveto_clr );
    renderer.set_lineto_color( parms.lineto_clr );
    renderer.set_backgd_color( parms.backgd_clr );

#if 0
    k.scale( parms.scale );

    BWCNC::SVG svgrenderer;
    svgrenderer.render_all( k );

    renderer.set_backgd_color( nullptr );
    renderer.set_lineto_color( "#ff0000" );
    renderer.render_all( k2 );
#endif

    renderer.render_all( k );
  //renderer.render_all( kontext );


    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);
}

