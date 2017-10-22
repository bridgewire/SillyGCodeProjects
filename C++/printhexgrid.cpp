
#include <Eigen/Dense>

#include "boundingbox.h"
#include "color.h"
#include "command.h"
#include "functions.h"
#include "numstring.h"
#include "part.h"
#include "renderer.h"
#include "hexgrid.h"

using namespace BWCNC;


struct cmdline_params {
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
} parms = {
    //5, 5, 3, 2,
    40, 40, 1, .2,
    .1, 100, 0, 0,
    true,
    nullptr,     // "#0000FF",
    "#FF0000"
};

const Eigen::Vector3d shift_tform( const Eigen::Vector3d & v )
{
    //const double w = (2 * M_PI)/15.0;
    const double w = (2 * M_PI)/40.0;
    return Eigen::Vector3d( 1.8 * sin(w*(v[1] + parms.yshift)), 1.8 * sin(w*(v[0] + parms.xshift)), 0 );
}

const Eigen::Matrix3d skew_tform( const Eigen::Vector3d & v )
{
    //const double w = (2 * M_PI)/15.0;
    const double w = (2 * M_PI)/10.0;
    Eigen::Matrix3d mat;

    mat <<  1.8 * sin(w*(v[1] + parms.yshift)), 0, 0,
            0, 1.8 * sin(w*(v[0] + parms.xshift)), 0,
            0, 0, 0 ;
    return mat;
}


void handle_params( int argc, char ** argv )
{
    for( int i = 1; i < argc; i++ )
    {
        if( 0 == strcmp( "--suppress_grid", argv[i] ) )
            parms.suppress_grid = true;
    }
}

void shift2center( PartContext & k )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    Eigen::Vector3d max = bbox.max;
    k.translate( Eigen::Vector3d( -fabs(max[0] - min[0])/2.0, -fabs(max[1] - min[1])/2.0, 0 ) );
}

void shift2positive( PartContext & k )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    k.translate( Eigen::Vector3d(-min[0], -min[1], -min[2]) );
}

int main( int argc, char ** argv )
{
    PartContext k;
    BWCNC::Boundingbox bbox;
    Eigen::Vector3d min, max;

    handle_params( argc, argv );

    BWCNC::HexGrid hxgrd(
            parms.cols, parms.rows, parms.sidelength, parms.scale,
            parms.nested, parms.nested_spacing, ! parms.suppress_grid,
            Eigen::Vector3d( parms.xshift, parms.yshift, 0),
            parms.lineto_clr, parms.moveto_clr );

    hxgrd.fill_partctx_with_hexgrid( k );
    k.remake_boundingbox();

    shift2center(k);

    k.position_dependent_transform( skew_tform, shift_tform );
    k.remake_boundingbox();

    shift2positive(k);

    k.scale( parms.scale );

    SVG renderer;
    renderer.set_moveto_color( parms.moveto_clr );
    renderer.set_lineto_color( parms.lineto_clr );

    renderer.render_all( k );

    return 0;
}

