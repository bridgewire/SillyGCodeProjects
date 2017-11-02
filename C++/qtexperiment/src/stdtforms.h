#ifndef STDFORMS_H__
#define STDFORMS_H__

#include <math.h>

class crosshatchwaves : public BWCNC::position_dependent_transform_t
{
public:
    double ticks;
    double w;
    double shiftscale;
public:
    crosshatchwaves()
        : position_dependent_transform_t(false, true),
          ticks(0), w(1/(3*M_PI)), shiftscale(7)
    {}
    virtual ~crosshatchwaves(){}

    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & v ) { return shiftscale * Eigen::Vector3d( ::sin(w*(v[1]+ticks)), ::sin(w*(v[0]+ticks)), 0 ); }
};

#if 0
const Eigen::Vector3d crosshatchwaves::vvf( const Eigen::Vector3d & v )
{
    const double x = shiftscal * sin( w * ( v[1] + ticks ) );
    const double y = shiftscal * sin( w * ( v[0] + ticks ) );
    return Eigen::Vector3d( x, y, 0 );
}
#endif

#endif /* ifndef STDFORMS_H__ */
