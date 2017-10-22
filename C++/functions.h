#ifndef BWCNC_FUNCTIONS_H__
#define BWCNC_FUNCTIONS_H__

#include <Eigen/Dense>
#include "numstring.h"

namespace BWCNC {

std::vector<BWCNC::NumString> VectorToNumStringArray( const Eigen::Vector3d & v );

typedef const Eigen::Matrix3d (*matrix_valued_function)( const Eigen::Vector3d & v );
typedef const Eigen::Vector3d (*vector_valued_function)( const Eigen::Vector3d & v );

// give these shorter names
typedef matrix_valued_function mvf_t;
typedef vector_valued_function vvf_t;

void position_dependent_transform( mvf_t mvf, vvf_t vvf, Eigen::Vector3d & v );
// this is just a shorter name for position_dependent_transform
void pos_dep_tform( mvf_t mvf, vvf_t vvf, Eigen::Vector3d & v );

};

#endif
