#ifndef STDFORMS_H__
#define STDFORMS_H__

#include <math.h>

class crosshatchwaves : public BWCNC::position_dependent_transform_t
{
public:
    double ticks = 0;
    double w = 1/(3*M_PI);
    double shiftscale = 7;
public:
    crosshatchwaves() : position_dependent_transform_t(false, true) {}
    virtual ~crosshatchwaves(){}

    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & v )
    {
#if 0
        return shiftscale * Eigen::Vector3d(
                ::sin(w*(v[1]+ticks)),
                ::sin(w*(v[0]-ticks)),
                ::sin(w*(v[1]+ticks)) + ::sin(w*(v[0]-ticks)) // the z transform will not be apparent unless specifically used
            );
#endif
        double x, y, z;;
        x = ::sin( w*(  v[0] * ::sin(ticks/100.0)
                      + v[1] * ::cos(ticks/100.0)
                      + ticks
                     ));

        y = ::sin( w*(  v[1] * ::sin(-ticks/200.0)
                      + v[0] * ::cos(-ticks/200.0)
                      + ticks
                     ));

        z = 10 * (::cos(x*M_PI/2) + ::cos(y*M_PI/2));

        return shiftscale * Eigen::Vector3d( x, y, z );
    }
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
