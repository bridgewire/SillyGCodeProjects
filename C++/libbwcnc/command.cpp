
#include <list>

#include "command.h"
#include "functions.h"

BWCNC::Command::Command( const Eigen::Vector2d & f, const Eigen::Vector2d & t )
{
    clrdflt();
    begindflt(); for( int i = 0; i < 2; i++ ) begin[i] = f[i];
    endflt();    for( int i = 0; i < 2; i++ ) end[i]   = t[i];
}

BWCNC::Command::Command( const Eigen::Vector2d & f, const Eigen::Vector2d & t, const BWCNC::Color & c )
    : clr(c)
{
    begindflt(); for( int i = 0; i < 2; i++ ) begin[i] = f[i];
    endflt();    for( int i = 0; i < 2; i++ ) end[i]   = t[i];
}

//                   position_dependent_transform( mvf_t mvf, vvf_t vvf )
void BWCNC::Command::pos_dep_tform( mvf_t mvf, vvf_t vvf )
{
    BWCNC::pos_dep_tform( mvf, vvf, begin );
    BWCNC::pos_dep_tform( mvf, vvf, end );
}

void BWCNC::Command::pos_dep_tform( pdt_t * tform )
{
    BWCNC::pos_dep_tform( tform, begin );
    BWCNC::pos_dep_tform( tform, end );
}

