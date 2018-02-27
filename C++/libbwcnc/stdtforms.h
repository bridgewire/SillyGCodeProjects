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
        double x, y, z;
#if 0
        x =  ::sin(w*(v[1]+ticks));
        y =  ::sin(w*(v[0]-ticks));
        z =  ::sin(w*(v[1]+ticks)) + ::sin(w*(v[0]-ticks)); // the z transform will not be apparent unless specifically used
#elif 0
        x = ::sin(w*(v[1]+ticks)),
        y = ::sin(w*(v[0]-ticks)),
        z = 5 * ::sin(x + y);
#else
        x = ::sin( w*(  v[0] * ::sin(ticks/100.0)
                      + v[1] * ::cos(ticks/100.0)
                      + ticks
                     ));

        y = ::sin( w*(  v[1] * ::sin(-ticks/200.0)
                      + v[0] * ::cos(-ticks/200.0)
                      + ticks
                     ));

      //z = 10 * (::cos(x*M_PI/2) - ::cos(y*M_PI/2));
        z = 10 * ::sin(x + y);
#endif
        return shiftscale * Eigen::Vector3d( x, y, z );
    }
};


class leftrighteye3D : public BWCNC::position_dependent_transform_t
{
public:
    bool is_positive = true;
    Eigen::Vector3d eye;
public:
    leftrighteye3D() : position_dependent_transform_t(false, true) {}
    virtual ~leftrighteye3D(){}

    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & p )
    {
#if 1
        Eigen::Vector3d v = p - eye;
        Eigen::Vector3d u = v;
      //double s          =  v[2]/eye[2];
        double s          = -eye[2]/v[2];
        u[2]              = -eye[2];
        Eigen::Vector3d c = s*v - u;
        c[1] = 0;
        c[2] = 0;
        return c;
#else
        Eigen::Vector3d v = p - eye;
        return Eigen::Vector3d( (eye[2] * -v[0]/v[2]) + eye[0] - p[0],0,0);
#endif
    }
};


class z_pendulum : public BWCNC::position_dependent_transform_t
{
protected:
    double z_shift = 0;
    double ticks   = 0;
public:
    void set_ticks( double t ) { z_shift = ::sin( theta_max * ::sin( ::sqrt(9.86/(L_mm/1000)) * ticks ) ); ticks = t; }
    double theta_max = 10;
    double L_mm = 100;
public:
    const Eigen::Matrix3d mvf( const Eigen::Vector3d & ) { return (z_shift/2 + 1) * Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & )
    {
      //double t = theta_max * ::sin( ::sqrt(9.86/(L_mm/1000)) * ticks );
        return Eigen::Vector3d(0, 0, z_shift);
    }
};


#endif /* ifndef STDFORMS_H__ */
