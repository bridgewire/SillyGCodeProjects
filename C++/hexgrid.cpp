
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
  part->update_position( start );

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
          std::cerr << "context: " << k.get_bbox();
          std::cerr << " -- adding new: " << p->get_bbox(); 
          k.add_part( p );
          std::cerr << " -- results in context: " << k.get_bbox() << "\n";
        }
        else
          std::cerr << "no hexagon part was made\n";
      }
    }
  }
  //###############################################################################################################
  //#### end of remaining rows
  //###############################################################################################################
}

#if 0

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
    .3, 100, 0, 0,
    true,
    nullptr,     // "#0000FF",
    "#FF0000"
};


//typedef const Eigen::Matrix3d & (*matrix_valued_function)( const Eigen::Vector3d & v );
//typedef const Eigen::Vector3d & (*vector_valued_function)( const Eigen::Vector3d & v );

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
#endif
