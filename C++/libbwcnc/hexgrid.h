#ifndef BWCNCHEXGRID__
#define BWCNCHEXGRID__

#include <Eigen/Dense>


namespace BWCNC
{

class HexGrid
{
public:
    HexGrid( int c = 10, int r = 10, double len = 1, double scale = 30,
             int nested_count = 1, double nested_pcnt = .5, bool includegrid = true,
             Eigen::Vector3d offset = Eigen::Vector3d(0,0,0),
             const char * clr_lineto = "#ff0000",
             const char * clr_moveto = nullptr,
             const char * clr_bckgrd = "#ffffff" )
    : m_cols(c),
      m_rows(r),
      m_nested(nested_count),
      m_nested_spacing(nested_pcnt),
      m_sidelen(len),
      m_scale(scale),
      m_shift(offset),
      m_includegrid(includegrid),
      m_lineto_clr(clr_lineto),
      m_moveto_clr(clr_moveto),
      m_bckgrd_clr(clr_bckgrd)
    {}

    void set_colors( const char * clr_lineto = "#ff0000", const char * clr_moveto = nullptr, const char * clr_bckgrd = "#ffffff" )
    {
      m_lineto_clr = clr_lineto;
      m_moveto_clr = clr_moveto;
      m_bckgrd_clr = clr_bckgrd;
    }
    void set_offset( const Eigen::Vector3d & offset ) { m_shift = offset; }
    void set_nested_parms( int nested_count = 1, double nested_pcnt = .5, bool includegrid = true )
    { 
      m_nested = nested_count;
      m_nested_spacing = nested_pcnt;
      m_includegrid = includegrid;
    }
    void set_primary_params( int c = 10, int r = 10, double len = 1, double scale = 30 )
    {
      m_cols    = c;
      m_rows    = r;
      m_sidelen = len;
      m_scale   = scale;
    }
    void set_renderer_colors( BWCNC::Renderer * r )
    {
        r->set_moveto_color( m_moveto_clr );
        r->set_lineto_color( m_lineto_clr );
        r->set_backgd_color( m_bckgrd_clr );
    }

    virtual ~HexGrid(){}

    void fill_partctx_with_hexgrid( BWCNC::PartContext & k );

protected:
    int m_cols;
    int m_rows;
    int m_nested;
    double m_nested_spacing;

    double m_sidelen = 0;
    double m_scale = 0;

    Eigen::Vector3d m_shift;

    bool m_includegrid = true;
    const char * m_lineto_clr = "#aa0000";
    const char * m_moveto_clr =  nullptr;
    const char * m_bckgrd_clr = "#ffffff";

  //void          nested_params( double outer_sidelen, double spacing_ratio, int nth_from_outer, const Eigen::Vector3d & start, double & x, double & y, double & outsidelen );
  //BWCNC::Part * make_inner_hexagon( BWCNC::PartContext & k, double main_sidelen, double inner_fraction, int nth_inner, double start_x, double start_y, bool start_mark = false );
  //BWCNC::Part * hexagon( BWCNC::PartContext & k, double sidelen, double x_start, double y_start, uint8_t * sides2skip = nullptr, bool start_mark = false );

    void          nested_params( double spacing_ratio, int nth_from_outer, const Eigen::Vector3d & start, Eigen::Vector3d & pos, double & nsidelen );
    BWCNC::Part * make_inner_hexagon( BWCNC::PartContext & k, double inner_fraction, int nth_inner, const Eigen::Vector3d & start );
    BWCNC::Part * hexagon( BWCNC::PartContext & k, const double sidelen, const Eigen::Vector3d & start, const uint8_t * sides2skip = nullptr );
};

};

#endif /* BWCNCHEXGRID__ */
