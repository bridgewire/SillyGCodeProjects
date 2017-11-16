
#include <Eigen/Dense>

#include "boundingbox.h"
#include "color.h"
#include "command.h"
#include "functions.h"
#include "numstring.h"
#include "part.h"
#include "renderer.h"
#include "mceschliz.h"

//using namespace BWCNC;


struct quickpoint {
    double x;
    double y;
};

// cheeks_atom
#define CHEEKSATOM_PNTCNT 15
static const quickpoint cheek[CHEEKSATOM_PNTCNT] = {
    { 0, 0 }, { 12, 14 }, { 12, 19 }, { -1, 29 }, { -3, 29 }, { -16, 23 }, { -18, 24 }, { -19, 25 }, { -19, 31 }, { -18, 32 }, { -17, 38 }, { -13, 38 }, { -10, 34 },  { -9, 34 }, { 0, 40 }
};

// toes_atom
#define TOESATOM_PNTCNT 16
static const quickpoint toes[TOESATOM_PNTCNT] = {
    { 0, 0 }, { 15, -3 }, { 13, 7 }, { 8, 13 }, { 10, 16 }, { 9, 18 }, { 6, 25 }, { 4, 27 }, { 0, 27 }, { -8, 26 }, { -13, 24 }, { -22, 16 }, { -20, 22 }, { -18, 25 }, { -16, 32 }, { 0, 40 }
};

// knees_atom
#define KNEESATOM_PNTCNT 14
static const quickpoint knees[KNEESATOM_PNTCNT] = {
    { 0, 0 }, { 5, 1 }, { 16, 4 }, { 26, 4 }, { 25, 7 }, { 25, 10 }, { 24, 11 }, { 23, 14 }, { 22, 16 }, { 4, 12 }, { 2, 12 }, { 0, 23 }, { -5, 30 }, { 0, 40 }
};




static BWCNC::Part * quickpoints2part( const quickpoint * points, const int listlen, bool reverse = false )
{
	BWCNC::Part * part = nullptr;
    const int firstpnt = reverse ? listlen - 1 : 0;
    const int lastpnt  = reverse ? -1 : listlen;
    const int stepsize = reverse ? -1 : 1;
    for( int i = firstpnt; i != lastpnt; i += stepsize )
    {
        const quickpoint * q = points + i;
        if( i == firstpnt )
	        part = new BWCNC::Part( Eigen::Vector3d( q->x, q->y, 0 ) );
        else
            part->lineto( Eigen::Vector3d( q->x, q->y, 0 ) );
    }
    return part;
}

BWCNC::Part * BWCNC::LizardGrid::make_cheek_atom( bool reverse ) { return quickpoints2part( cheek, CHEEKSATOM_PNTCNT, reverse ); }
BWCNC::Part * BWCNC::LizardGrid::make_toes_atom(  bool reverse ) { return quickpoints2part( toes,  TOESATOM_PNTCNT, reverse ); }
BWCNC::Part * BWCNC::LizardGrid::make_knees_atom( bool reverse ) { return quickpoints2part( knees, KNEESATOM_PNTCNT, reverse ); } 


void BWCNC::LizardGrid::make_toe2toe_down_through_cheeks( BWCNC::PartContext & k, bool finish )
{
    BWCNC::Part * part = nullptr;

    part = make_toes_atom();
    part->rotate( -60, true );
    k.append_part( part );

    part = make_cheek_atom( true );
    part->rotate( 180, true );
    k.append_part( part );

    part = make_cheek_atom();
    part->rotate( 60, true );
    k.append_part( part );

    part = make_toes_atom( true );
    part->rotate( 180, true );
    k.append_part( part );

    if( finish )
    {
        part = make_toes_atom();
        part->rotate( -60, true );
        k.append_part( part );
    }
}


void BWCNC::LizardGrid::make_cheek2cheek_down_through_knees( BWCNC::PartContext & k, bool finish )
{
    BWCNC::Part * part = nullptr;

    part = make_cheek_atom();
    part->rotate( -60, true );
    k.append_part( part );

    part = make_knees_atom( true );
    part->rotate( 180, true );
    k.append_part( part );

    part = make_knees_atom();
    part->rotate( 60, true );
    k.append_part( part );

    part = make_cheek_atom( true );
    part->rotate( 180, true );
    k.append_part( part );

    if( finish )
    {
        part = make_cheek_atom();
        part->rotate( -60, true );
        k.append_part( part );
    }
}

void BWCNC::LizardGrid::make_knee2knee_down_through_toes( BWCNC::PartContext & k, bool finish )
{
    BWCNC::Part * part = nullptr;

    part = make_knees_atom();
    part->rotate( -60, true );
    k.append_part( part );

    part = make_toes_atom( true );
    part->rotate( 180, true );
    k.append_part( part );

    part = make_toes_atom();
    part->rotate( 60, true );
    k.append_part( part );

    part = make_knees_atom( true );
    part->rotate( 180, true );
    k.append_part( part );

    if( finish )
    {
        part = make_knees_atom();
        part->rotate( -60, true );
        k.append_part( part );
    }
}


static void addcolspacing( BWCNC::PartContext & k, Eigen::Vector3d & colstart )
{
    static double colspacing = 2 * 40 * ::cos(M_PI/6);
  //static double colspacing =     40 * ::cos(M_PI/6);

    colstart[0] += colspacing;
    BWCNC::Part * p = k.get_new_part();
    p->moveto( colstart );
    k.add_part( p );
}

static void addrowspacing( BWCNC::PartContext & k )
{
    Eigen::Vector3d pos(0,40,0);
    BWCNC::Part * p = k.get_new_part();
    p->moveto( pos, true );
    k.add_part( p );
}



void BWCNC::LizardGrid::fill_partctx_with_grid( BWCNC::PartContext & k )
{
    BWCNC::Part * p = nullptr;
    Eigen::Vector3d colstart(0,0,0);

    // create the long vertical lines
    for( int c = 0; c < m_cols; c++ )
    {
        for( int r = 0; r < m_rows; r++ ) { make_cheek2cheek_down_through_knees( k, (r + 1 == m_rows) ); }
        addcolspacing( k, colstart );

        for( int r = 0; r < m_rows; r++ ) { make_toe2toe_down_through_cheeks( k, (r + 1 == m_rows) ); }
        addcolspacing( k, colstart );

        for( int r = 0; r < m_rows; r++ ) { make_knee2knee_down_through_toes( k, (r + 1 == m_rows) ); }
        if( c < m_cols - 1 )
            addcolspacing( k, colstart );
    }

    colstart = Eigen::Vector3d(0,0,0);
    for( int c = 0; c < m_cols; c++ )
    {
        // moveto, back to top of second vertical
        // this batch connects the first two verticals (and it's cyclic)
        addcolspacing( k, colstart );
        for( int r = 0; r < m_rows; r++ )
        {
            p = make_toes_atom();
            p->rotate( 60, true );
            k.append_part( p );
            addrowspacing( k );

            p = make_knees_atom();
            p->rotate( -60, true );
            k.append_part( p );
            addrowspacing( k );
        }
        p = make_toes_atom(); // toes begin *and* end it
        p->rotate( 60, true );
        k.append_part( p );
        addrowspacing( k );


        // moveto, back to top of next vertical
        // this batch connects the second two verticals (and it's cyclic)
        addcolspacing( k, colstart );
        for( int r = 0; r < m_rows; r++ )
        {
            p = make_knees_atom();
            p->rotate( 60, true );
            k.append_part( p );
            addrowspacing( k );

            p = make_cheek_atom();
            p->rotate( -60, true );
            k.append_part( p );
            addrowspacing( k );
        }
        p = make_knees_atom();  // knees begin *and* end it
        p->rotate( 60, true );
        k.append_part( p );
        addrowspacing( k );


        if( c < m_cols - 1 || m_allow_dangling_crossings )
        {
            // moveto, back to top of next vertical
            // this batch connects the third two verticals (and it's cyclic)
            addcolspacing( k, colstart );
            for( int r = 0; r < m_rows; r++ )
            {
                p = make_cheek_atom();
                p->rotate( 60, true );
                k.append_part( p );
                addrowspacing( k );

                p = make_toes_atom();
                p->rotate( -60, true );
                k.append_part( p );
                addrowspacing( k );
            }
            p = make_cheek_atom();  // cheeks begin *and* end it
            p->rotate( 60, true );
            k.append_part( p );
            addrowspacing( k );
        }
    }
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
    5, 5, 3, 2,
    //40, 40, 1, .2,
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
        BWCNC::LizardGrid grid( parms.cols, parms.rows, parms.sidelen, parms.scale );

        grid.fill_partctx_with_grid( k );
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
