
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

