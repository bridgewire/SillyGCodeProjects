
#include <Eigen/Dense>

#include "boundingbox.h"
#include "color.h"
#include "command.h"
#include "functions.h"
#include "numstring.h"
#include "part.h"
#include "renderer.h"
#include "hexgrid.h"

//using namespace BWCNC;

void BWCNC::HexGrid::nested_params( double spacing_ratio, int nth_from_outer, const Eigen::Vector3d & start, Eigen::Vector3d & pos, double & nsidelen )
{
    double hypot = spacing_ratio * nth_from_outer * m_sidelen;
    pos = start + hypot * Eigen::Vector3d( sin( M_PI/3.0 ), cos( M_PI/3.0 ), 0 );
    nsidelen = m_sidelen - hypot;
}

BWCNC::Part * BWCNC::HexGrid::make_inner_hexagon(
    BWCNC::PartContext & k,
    double inner_fraction,
    int nth_inner,
    const Eigen::Vector3d & start )
{
  Eigen::Vector3d pos(0,0,0);
  double nested_sidelen = 0;
  nested_params( inner_fraction, nth_inner, start, pos, nested_sidelen );
  return hexagon( k, nested_sidelen, pos );
}

BWCNC::Part * BWCNC::HexGrid::hexagon( BWCNC::PartContext & k, const double sidelen, const Eigen::Vector3d & start, const uint8_t * sides2skip )
{
  // orientation: left-right symmetry around the vertical center line through two vertices
  const double triangle_longside  = sidelen * cos(M_PI_2/3);
  const double triangle_shortside = sidelen * sin(M_PI_2/3);

  std::vector<Eigen::Vector3d> path;
  {
    path.push_back( Eigen::Vector3d( triangle_longside, -triangle_shortside, 0 ) );
    path.push_back( Eigen::Vector3d( triangle_longside,  triangle_shortside, 0 ) );
    path.push_back( Eigen::Vector3d(                 0,             sidelen, 0 ) );
    path.push_back( Eigen::Vector3d(-triangle_longside,  triangle_shortside, 0 ) );
    path.push_back( Eigen::Vector3d(-triangle_longside, -triangle_shortside, 0 ) );
    path.push_back( Eigen::Vector3d(                 0,            -sidelen, 0 ) );
  }

  BWCNC::Part * part = k.get_new_part();
  part->update_starting_position( start );

  Eigen::Vector3d vec = start;
  for( int i = 0; i < 6; i++ )
  {
    vec = vec + path[i];
    if( sides2skip && sides2skip[i] ) part->moveto( vec );
    else                              part->lineto( vec );
  }

  return part;
}


void BWCNC::HexGrid::fill_partctx_with_hexgrid( BWCNC::PartContext & k )
{
  const uint8_t skip0[]    = {1,0,0,0,0,0};
//const uint8_t skip01[]   = {1,1,0,0,0,0};
  const uint8_t skip05[]   = {1,0,0,0,0,1};
  const uint8_t skip015[]  = {1,1,0,0,0,1};
  const uint8_t skip1[]    = {0,1,0,0,0,0};
//const uint8_t skip2[]    = {0,0,1,0,0,0};  // debugging only
  const uint8_t skip5[]    = {0,0,0,0,0,1};

  //###############################################################################################################
  //#### Row #1
  //###############################################################################################################
  double x_ref = 0, y_ref = m_sidelen/2.0;
  Eigen::Vector3d start( x_ref, y_ref, 0);

  for( int i = m_nested; i > 0; i-- )
    k.add_part( make_inner_hexagon( k, m_nested_spacing, i, start ) );

  if( m_includegrid )
    k.add_part( hexagon( k, m_sidelen, start ) );

  for( int j = 1; j < m_cols; j++ )
  {
    start = Eigen::Vector3d( j * sqrt(3) * m_sidelen, y_ref, 0);

    for( int i =  m_nested; i > 0; i-- )
      k.add_part( make_inner_hexagon( k, m_nested_spacing, i, start ) );

    if( m_includegrid )
      k.add_part( hexagon( k, m_sidelen, start, skip5 ) );
  }
  //###############################################################################################################
  //#### End of Row #1
  //###############################################################################################################

  //###############################################################################################################
  //#### remaining rows
  //###############################################################################################################
  double xoffset;
  for( int row = 2; row <= m_rows; row++ )
  {
    bool row_iseven  = (row % 2 == 0);
    x_ref       = row_iseven ? (sqrt(3) * m_sidelen / 2.0) : 0.0;
    xoffset     = row_iseven ? 0.5 : 0.0;
    y_ref      += 1.5 * m_sidelen;

    start = Eigen::Vector3d( x_ref, y_ref, 0);

    for( int i = m_nested; i > 0; i-- )
      k.add_part( make_inner_hexagon( k, m_nested_spacing, i, start ) );

    if( m_includegrid )
    {
      if( row_iseven ) k.add_part( hexagon( k, m_sidelen, start, skip0 ) );
      else             k.add_part( hexagon( k, m_sidelen, start, skip1 ) );
    }


    for( int j = 1; j < m_cols; j++ )
    {
      x_ref = (j + xoffset) * sqrt(3) * m_sidelen;

      start = Eigen::Vector3d( x_ref, y_ref, 0);

      for( int i = m_nested; i > 0; i-- )
        k.add_part( make_inner_hexagon( k, m_nested_spacing, i, start ) );

      if( m_includegrid )
      {
        Part * p = nullptr;
        if( row_iseven )
        {
          if( j < m_cols - 1 ) p = hexagon( k, m_sidelen, start, skip015 );
          else                 p = hexagon( k, m_sidelen, start,  skip05 );
        }
        else
        {
          if( j == 1 )         p = hexagon( k, m_sidelen, start,   skip1 );
          else                 p = hexagon( k, m_sidelen, start, skip015 );
        }

        if( p != nullptr )
        {
          //std::cerr << "context: " << k.get_bbox();
          //std::cerr << " -- adding new: " << p->get_bbox();
          k.add_part( p );
          //std::cerr << " -- results in context: " << k.get_bbox() << "\n";
        }
        //else std::cerr << "no hexagon part was made\n";
      }
    }
  }
  //###############################################################################################################
  //#### end of remaining rows
  //###############################################################################################################
}

#ifdef HEXGRID_UNITTEST

struct cmdline_params {
    int cols;
    int rows;
    int nested;
    double nested_spacing;

    double sidelen;
    double scale;
    double yshift;
    double xshift;

    bool suppress_grid;
    const char * moveto_clr;
    const char * lineto_clr;
} parms = {
    //5, 5, 3, 2,
    40, 40, 1, .2,
    //4, 4, 1, .2,
    .2, 60, 0, 0,
    true,
    nullptr,     // "#0000FF",
    "#FF0000"
};



//static const double w = (2 * M_PI)/6.0;
static const double w = (2 * M_PI)/30;

static const Eigen::Vector3d shift_tform( const Eigen::Vector3d & v )
{
    return 1.8 * Eigen::Vector3d( cos(w*(v[1] + parms.yshift)), sin(w*(v[0] + parms.xshift)), 0 );
}

static const Eigen::Matrix3d rotation_tform( const Eigen::Vector3d & )
{
    Eigen::Matrix3d mat;
    double t = M_PI/2;
    mat <<  cos(t), -sin(t), 0,
            sin(t),  cos(t), 0,
            0, 0, 1 ;
    return mat;
}


static const Eigen::Matrix3d skew_tform( const Eigen::Vector3d & v )
{
    Eigen::Matrix3d mat;
    mat <<  cos(w*(v[1] + parms.yshift)), 0, 0,
            0, sin(w*(v[0] + parms.xshift)), 0,
            0, 0, 0 ;
    return 1.8 * mat;
}


static bool handle_params( int argc, char ** argv )
{
    for( int i = 1; i < argc; i++ )
    {
        if( 0 == strcmp( "--suppress_grid", argv[i] ) )
            parms.suppress_grid = true;
        else
            return false;
    }
    return true;
}

static void shift2center( BWCNC::PartContext & k )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    Eigen::Vector3d max = bbox.max;
    k.translate( Eigen::Vector3d( -fabs(max[0] - min[0])/2.0, -fabs(max[1] - min[1])/2.0, 0 ) );
}

static void shift2positive( BWCNC::PartContext & k )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    k.translate( Eigen::Vector3d(-min[0], -min[1], -min[2]) );
}

int main( int argc, char ** argv )
{
    BWCNC::PartContext k;
    BWCNC::PartContext kcopy;
  //BWCNC::Boundingbox bbox;
  //Eigen::Vector3d min, max;

    if( handle_params( argc, argv ) )
    {
        BWCNC::HexGrid grid( parms.cols, parms.rows, parms.sidelen, parms.scale,
                             parms.nested, parms.nested_spacing, ! parms.suppress_grid );

        grid.fill_partctx_with_hexgrid( k );
        k.remake_boundingbox();

        shift2center( k );

        k.position_dependent_transform( skew_tform, shift_tform );
        //k.position_dependent_transform( skew_tform, nullptr );
        //k.position_dependent_transform( nullptr, shift_tform );
        k.remake_boundingbox();

        shift2positive( k );

        k.scale( parms.scale );

        BWCNC::SVG renderer;
        //renderer.set_moveto_color( parms.moveto_clr );
        //renderer.set_lineto_color( parms.lineto_clr );

        k.copy_into( kcopy );
        //shift2center( kcopy );
      //kcopy.position_dependent_transform( skew_tform, shift_tform );
//        kcopy.scale( .1 );
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
    }

    return 0;
}
#endif

#if 0
    HexGrid( int c = 10, int r = 10, double len = 1, double scale = 30,
             int nested_count = 1, double nested_pcnt = .5, bool includegrid = true,
             Eigen::Vector3d offset = Eigen::Vector3d(0,0,0),
             const char * clr_lineto = "#ff0000",
             const char * clr_moveto = nullptr,
             const char * clr_bckgrd = "#ffffff" )
#endif

