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

std::vector<BWCNC::NumString> VectorToNumStringArray( const Eigen::Vector2d & v )
{
    Eigen::Vector3d u(v[0],v[1],0);
    return VectorToNumStringArray( u );
}

void pos_dep_tform( const mvf_t mvf, const vvf_t vvf, Eigen::Vector3d & v, Eigen::Vector3d * out_v )
{
    if( out_v )
    {
        if( mvf != nullptr ) *out_v  = mvf( v ) * v;
        if( vvf != nullptr ) *out_v += vvf( v );
    }
    else
    {
        Eigen::Vector3d v_new = v;
        pos_dep_tform( mvf, vvf, v, &v_new );
        v = v_new;
    }
}

// o := A(v)*v + f(v)
void pos_dep_tform( const pdt_t * t, Eigen::Vector3d & v, Eigen::Vector3d * out_v )
{
    if( out_v )
    {
        if( t->has_mvf() ) *out_v  = t->mvf( v ) * v;
        if( t->has_vvf() ) *out_v += t->vvf( v );
    }
    else
    {
        Eigen::Vector3d v_new = v;
        pos_dep_tform( t, v, &v_new );
        v = v_new;
    }
}

void position_dependent_transform( const mvf_t mvf, const vvf_t vvf, Eigen::Vector3d & v, Eigen::Vector3d * out_v ) { pos_dep_tform( mvf, vvf, v, out_v ); }
void position_dependent_transform( const pdt_t * t,                  Eigen::Vector3d & v, Eigen::Vector3d * out_v ) { pos_dep_tform( t, v, out_v ); }

};

