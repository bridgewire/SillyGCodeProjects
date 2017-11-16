#ifndef BWCNC_MCESCHLIZ_H__
#define BWCNC_MCESCHLIZ_H__

#include <Eigen/Dense>


namespace BWCNC
{

class LizardGrid
{
public:
    LizardGrid( int c = 3, int r = 3, double len = 1, double scale = 20,
                bool make_whole_tile = false,
             //int nested_count = 1, double nested_pcnt = .5, bool includegrid = true,
             Eigen::Vector3d offset = Eigen::Vector3d(0,0,0),
             const char * clr_lineto = "#ff0000",
             const char * clr_moveto = "#00ff00",
             const char * clr_bckgrd = "#ffffff" )
    : m_cols(c),
      m_rows(r),
      //m_nested(nested_count),
      //m_nested_spacing(nested_pcnt),
      m_allow_dangling_crossings(make_whole_tile),
      m_sidelen(len),
      m_scale(scale),
      m_shift(offset),
      //m_includegrid(includegrid),
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
#if 0
    void set_nested_parms( int nested_count = 1, double nested_pcnt = .5, bool includegrid = true )
    { 
      m_nested = nested_count;
      m_nested_spacing = nested_pcnt;
      m_includegrid = includegrid;
    }
#endif
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

    virtual ~LizardGrid(){}

    void fill_partctx_with_grid( BWCNC::PartContext & k );

protected:
    BWCNC::Part * make_cheek_atom( bool reverse = false );
    BWCNC::Part * make_toes_atom(  bool reverse = false );
    BWCNC::Part * make_knees_atom( bool reverse = false );
    void make_cheek2cheek_down_through_knees( BWCNC::PartContext & k, bool finish = false );
    void make_toe2toe_down_through_cheeks( BWCNC::PartContext & k, bool finish = false );
    void make_knee2knee_down_through_toes( BWCNC::PartContext & k, bool finish = false );

protected:
    int m_cols;
    int m_rows;
    //int m_nested;
    //double m_nested_spacing;
    bool m_allow_dangling_crossings = false;

    double m_sidelen = 0;
    double m_scale = 0;

    Eigen::Vector3d m_shift;

    //bool m_includegrid = true;
    const char * m_lineto_clr = "#aa0000";
    const char * m_moveto_clr =  nullptr;
    const char * m_bckgrd_clr = "#ffffff";

  //void          nested_params( double outer_sidelen, double spacing_ratio, int nth_from_outer, const Eigen::Vector3d & start, double & x, double & y, double & outsidelen );
  //BWCNC::Part * make_inner_hexagon( BWCNC::PartContext & k, double main_sidelen, double inner_fraction, int nth_inner, double start_x, double start_y, bool start_mark = false );
  //BWCNC::Part * hexagon( BWCNC::PartContext & k, double sidelen, double x_start, double y_start, uint8_t * sides2skip = nullptr, bool start_mark = false );
#if 0
    void          nested_params( double spacing_ratio, int nth_from_outer, const Eigen::Vector3d & start, Eigen::Vector3d & pos, double & nsidelen );
    BWCNC::Part * make_inner_hexagon( BWCNC::PartContext & k, double inner_fraction, int nth_inner, const Eigen::Vector3d & start );
    BWCNC::Part * hexagon( BWCNC::PartContext & k, const double sidelen, const Eigen::Vector3d & start, const uint8_t * sides2skip = nullptr );
#endif
};

};

#endif /* BWCNC_MCESCHLIZ_H__ */
