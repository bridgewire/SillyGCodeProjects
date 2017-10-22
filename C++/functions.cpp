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


void position_dependent_transform( mvf_t mvf, vvf_t vvf, Eigen::Vector3d & v )
{
    Eigen::Vector3d v_new  = v;

    if( mvf != nullptr ) v_new = mvf( v ) * v;
    if( vvf != nullptr ) v_new += vvf( v );

    v = v_new;
}

// this is just a shorter name for position_dependent_transform
void pos_dep_tform( mvf_t mvf, vvf_t vvf, Eigen::Vector3d & v )
{
    position_dependent_transform( mvf, vvf, v );
}

};

