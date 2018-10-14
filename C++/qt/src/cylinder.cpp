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

    int a_input;
    int b_input;
    int ticks;
    double tick_size;

    double scene_width;
    double scene_height;

} parms = {
  //40, 30, 1, .2,  // latest, large
  //50, 40, 1, .2,
  //50, 10, 1, .2,
  //20, 20, 1, .2,
  //30, 20, 1, .1,
  //30, 20, 1, .2,
  //30, 3, 0, 0,
    3, 10, 0, 0,
    1, 1, 0, 0,
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

    const BWCNC::Color    cvf( const Eigen::Vector3d & ) const { return BWCNC::Color(); }
    const Eigen::Matrix3d mvf( const Eigen::Vector3d & ) const { Eigen::Matrix3d mat; mat << ::cos(t), -::sin(t), 0,   ::sin(t), ::cos(t), 0,   0, 0, 1; return mat; }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & ) const { return Eigen::Vector3d(0,0,0); }
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

    const BWCNC::Color    cvf( const Eigen::Vector3d & ) const { return BWCNC::Color(); }
    const Eigen::Matrix3d mvf( const Eigen::Vector3d & ) const { Eigen::Matrix3d mat; mat << ::cos(t), 0, ::sin(t),   0, 1, 0,   -::sin(t), 0, ::cos(t); return mat; }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & ) const { return Eigen::Vector3d(0,0,0); }
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

    const BWCNC::Color    cvf( const Eigen::Vector3d &   ) const { return BWCNC::Color(); }
    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) const { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & v ) const { return Eigen::Vector3d(0,shiftratio*v[0],0); }
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

    const BWCNC::Color    cvf( const Eigen::Vector3d &   ) const { return BWCNC::Color(); }
    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) const { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & v ) const ;
};

//class mkcylinder : public BWCNC::position_dependent_transform_t

const Eigen::Vector3d mkcylinder::vvf( const Eigen::Vector3d & v ) const 
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

class mkdisk : public BWCNC::position_dependent_transform_t
{
public:
    //double ticks;

    double x_max;
    double x_min;
    double y_max;
    double y_min;

    //double shiftscale;
public:
    mkdisk()
        : position_dependent_transform_t(false, true), x_max(0), x_min(0), y_max(0), y_min(0)
          //ticks(0), w(1/(3*M_PI)), shiftscale(7)
    {}
    virtual ~mkdisk(){}

    const BWCNC::Color    cvf( const Eigen::Vector3d &   ) const { return BWCNC::Color(); }
    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) const { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & v ) const ;
};



const Eigen::Vector3d mkdisk::vvf( const Eigen::Vector3d & v ) const 
{
    // mkdisk transform is almost the same thing as mkcylinder, the difference
    // being that the rotation doesn't have a fixed radius and doesn't move a
    // point out of the plane in which the point lies with respect to the axis
    // of rotation. let's see if we just just copy the mkcylinder stuff, un-fix
    // the radius, and change the rotation axis.  it will probably be good
    // enough.  in this case, we'll use the z-axis as the rotation axis, and
    // the radius of a given point will be its distance from the y-axis, hence
    // lines extending in the y-direction that are near the y-axis, will be
    // shortened, and similar lines far from the y-axis will be lengthened. a
    // line on the y-axis will be collapsed to a group of points all at the
    // z-axis.

    // the grid is rectangular, extending from (-y_max,y_max), and 2*pi*x will
    // be the circumferance of any collection of points at a given x. the y
    // coordinate all that is needed to produce an angle for sin() and cos().
    // That angle is given by: t = M_PI*y/y_max, so that y<0 is a negative
    // rotation.  that will work.

    double x = v[0];
    double y = v[1];
    double r = x/M_PI;
  //double f = x/r; // == M_PI*x/x_max;
    double t = M_PI*y/y_max;

    // we don't touch the z-dimension at all
    return  r * Eigen::Vector3d( ::cos(t), ::sin(t), 0 ) - Eigen::Vector3d( v[0], v[1], 0 );
}


void mainwindow::create_hexgrid_cylinder()
{
#if 0
    BWCNC::HexGrid grid(
            parms.cols, parms.rows, parms.sidelength, parms.scale,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
            Eigen::Vector3d( parms.xshift, parms.yshift, 0),
            parms.lineto_clr, parms.moveto_clr, parms.backgd_clr );
#else
    BWCNC::LizardGrid grid( parms.cols, parms.rows, parms.sidelength, parms.scale, true );
#endif

    grid.fill_partctx_with_grid( kontext );

    kontext.scale( parms.scale );

    shift2center(kontext);

#if 0
  //skew_X skew(1/30.0);
    skew_X skew(1/15.0);
    kontext.position_dependent_transform( &skew );
#endif

    BWCNC::Boundingbox bbox = kontext.get_bbox();
    mkcylinder cyltform;
  //cyltform.x_max = bbox.max[0];
    cyltform.x_max = bbox.max[0] * (parms.cols * (2/3000.0) + .978) ;  // imperically developed for Lizards on cylinder
  //cyltform.x_max = bbox.max[0] * 0.998;  // 0.98:3cols  ::  

    kontext.position_dependent_transform( &cyltform );

    shift2center( kontext );

    rotationY rotY(M_PI);  // 180 degrees
    kontext.transform( rotY.mvf( Eigen::Vector3d(0,0,0) ) );

    kontext_isready = true;
}


void mainwindow::refresh_hexgrid_cylinder()
{
    static Eigen::Vector3d dmy_v;                         // this vector isn't used. see rotationY::mvf
    BWCNC::PartContext k;

    if( ! kontext_isready )                               // when this function first runs, create a grid and make
        create_hexgrid_cylinder();                        // a cylinder out of it. reuse on following refreshes.

    rotationY rotY( M_PI * (a_value - 500)/50000.0 );     // rotate the cylinder around its cental axis every refresh
                                                          // this transform works on the perminantly stored kontext
    kontext.transform( rotY.mvf( dmy_v ) );               // rotY.mvf() returns a rotation matrix

    kontext.copy_into( k );                               // now, make a copy of kontext into k so further transforms
                                                          // can be thrown out between refreshes

#if 1
    shift2(k, BWCNC::to_positive,
              BWCNC::to_center,
              BWCNC::to_center);
    mkdisk disktform;                                     // now, take the cylinder and apply the disk transform
    k.translate( Eigen::Vector3d(50,0,0) );               // in order to create a torus. this means our function
    BWCNC::Boundingbox bbox = k.get_bbox();               // name is a lie. this is an experiment. making a disk,
    disktform.y_max = /* .65 */ .508 * bbox.max[1];       // a cylinder, and a torus will be standard transforms.
    k.position_dependent_transform( &disktform );
    k.scale( 10*(b_value/1000.0)  );
#endif
    shift2(k, BWCNC::to_positive, BWCNC::to_positive, BWCNC::to_center);

    if( b_cmd )
    {
        BWCNC::Boundingbox bbox = k.get_bbox();
        std::cerr << "bbox : " << bbox << "\n" ;
    }

    QImage img( scene->sceneRect().width(), scene->sceneRect().height(), QImage::Format_RGB32 );
    BWCNC::PixmapRenderer renderer( &img );
    renderer.render_positive_z = p_bool;
    renderer.render_negative_z = n_bool;

    renderer.set_moveto_color( parms.moveto_clr );
    renderer.set_lineto_color( parms.lineto_clr );
    renderer.set_backgd_color( parms.backgd_clr );

    renderer.render_all( k );

    QPixmap pxmp;
    if( pxmp.convertFromImage( img ) )
        pixmap_item->setPixmap(pxmp);

    if( b_cmd ) b_cmd = false;
}

