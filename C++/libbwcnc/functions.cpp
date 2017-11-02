#include <vector>
#include <string>
#include <sstream>

#include "functions.h"

namespace BWCNC {

std::vector<BWCNC::NumString> VectorToNumStringArray( const Eigen::Vector3d & v )
{
    std::vector<BWCNC::NumString> stringversion(3);

    for( int i = 0; i < 3; i++ )
        stringversion[i] =  v[i];

    return stringversion;
}

std::vector<BWCNC::NumString> VectorToNumStringArray( const Eigen::Vector2d v )
{
    Eigen::Vector3d u(v[0],v[1],0);
    return VectorToNumStringArray( u );
}

void pos_dep_tform( mvf_t mvf, vvf_t vvf, Eigen::Vector3d & v )
{
    Eigen::Vector3d v_new  = v;
    if( mvf != nullptr ) v_new = mvf( v ) * v;
    if( vvf != nullptr ) v_new += vvf( v );
    v = v_new;
}

// the above implements these three
void pos_dep_tform( pdt_t * t, Eigen::Vector3d & v )
{
    Eigen::Vector3d v_new  = v;
    if( t->has_mvf() ) v_new  = t->mvf( v ) * v;
    if( t->has_vvf() ) v_new += t->vvf( v );
    v = v_new;
}


void position_dependent_transform( mvf_t mvf, vvf_t vvf, Eigen::Vector3d & v ) { pos_dep_tform(    mvf,    vvf, v ); }
void position_dependent_transform( pdt_t * t, Eigen::Vector3d & v )            { pos_dep_tform( t, v ); }

};

