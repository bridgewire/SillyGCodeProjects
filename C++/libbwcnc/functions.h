#ifndef BWCNC_FUNCTIONS_H__
#define BWCNC_FUNCTIONS_H__

#include <Eigen/Dense>
#include "numstring.h"
#include "color.h"

namespace BWCNC {

class position_dependent_transform_t;
typedef position_dependent_transform_t pdt_t;

std::vector<BWCNC::NumString> VectorToNumStringArray( const Eigen::Vector3d & v );

typedef const Eigen::Matrix3d (*matrix_valued_function)( const Eigen::Vector3d & v );
typedef const Eigen::Vector3d (*vector_valued_function)( const Eigen::Vector3d & v );
typedef const BWCNC::Color    (*color_valued_function)(  const Eigen::Vector3d & v );

// give these shorter names
typedef matrix_valued_function mvf_t;
typedef vector_valued_function vvf_t;
typedef color_valued_function  cvf_t; // this isn't used, yet.

void position_dependent_transform( mvf_t mvf, vvf_t vvf, Eigen::Vector3d & v );
void position_dependent_transform( pdt_t * t, Eigen::Vector3d & v );
// these are just a shorter names for position_dependent_transform above
void pos_dep_tform( mvf_t mvf, vvf_t vvf, Eigen::Vector3d & v );
void pos_dep_tform( pdt_t * t, Eigen::Vector3d & v );


class position_dependent_transform_t
{
protected:
    bool m_has_mvf;
    bool m_has_vvf;
    bool m_has_cvf;

public:
    position_dependent_transform_t( bool hsmvf = false, bool hsvvf = false, bool hscvf = false ) : m_has_mvf(hsmvf), m_has_vvf(hsvvf), m_has_cvf(hscvf) {}
    virtual ~position_dependent_transform_t(){}

    virtual bool has_mvf(){ return m_has_mvf; }
    virtual bool has_vvf(){ return m_has_vvf; }
    virtual bool has_cvf(){ return m_has_cvf; }

    virtual const Eigen::Matrix3d mvf( const Eigen::Vector3d & v ) = 0; // note: these have has the same basic
    virtual const Eigen::Vector3d vvf( const Eigen::Vector3d & v ) = 0; // signatures as mvf_t and vvf_t
    virtual const BWCNC::Color    cvf( const Eigen::Vector3d & v ) = 0;
};

};

#endif
