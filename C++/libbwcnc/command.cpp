
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

// move part in the direction of the Vector argument: offset
void BWCNC::Command::translate( const Eigen::Vector3d & offset, Command * into )
{
    if( into )
    {
        into->begin = begin + offset;
        into->end   = end   + offset;
    }
    else
    {
        begin += offset;
        end   += offset;
    }
}

// translate part so that @start becomes equal to new_position
void BWCNC::Command::reposition( const Eigen::Vector3d & new_position, Command * into ){ translate( new_position - begin, into ); }

// general linear transform
// arg: mat  ...  type 3x3 numeric Matrix
void BWCNC::Command::transform( const Eigen::Matrix3d & mat, Command * into )
{
    if( into )
    {
        into->begin = mat * begin;
        into->end   = mat * end;
    }
    else
    {
        begin = mat * begin;
        end   = mat * end;
    }
}


//                   position_dependent_transform( mvf_t mvf, vvf_t vvf )
void BWCNC::Command::pos_dep_tform( const mvf_t mvf, const vvf_t vvf )
{
    BWCNC::pos_dep_tform( mvf, vvf, begin );
    BWCNC::pos_dep_tform( mvf, vvf, end );
}

void BWCNC::Command::pos_dep_tform( const pdt_t * tform )
{
    BWCNC::pos_dep_tform( tform, begin );
    BWCNC::pos_dep_tform( tform, end );
}

