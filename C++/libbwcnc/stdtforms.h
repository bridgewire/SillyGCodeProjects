#ifndef STDFORMS_H__
#define STDFORMS_H__

#include <math.h>

class crosshatchwaves : public BWCNC::position_dependent_transform_t
{
public:
    const double ticks = 0;
    const double w = 1/(3*M_PI);
    const double shiftscale = 7;
    const double param = 1;
public:
    crosshatchwaves( double T = 0, double W = 1/(3*M_PI), double S = 7, double P = 1 )
        : position_dependent_transform_t(false, true), ticks(T), w(W), shiftscale(S), param(P)
    {}
    virtual ~crosshatchwaves(){}

    const BWCNC::Color    cvf( const Eigen::Vector3d &   ) const { return BWCNC::Color(); }
    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) const { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & v ) const
    {
        double x, y, z=0;
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

#if 1
#if 0
      //z =  10 * (::cos(x*M_PI/2) - ::cos(y*M_PI/2));
        z = -10 * (::sin(x - y) + ::cos(y - x));
      //z =  -1 * (::sin(x + y) + ::cos(x + y));
#else
        double shiftdist = ::sqrt(x*x + y*y);
        if( shiftdist < 1e-8 ) shiftdist = 1e-8;

      //z = -10 * (x+y > 0 ? 1 : -1) * :: pow( 1/shiftdist, 1.0/3 );
      //z = (x+y > 0 ? 1 : -1) * ::pow( 1/shiftdist, 1.0/3 );
        z =  10 * (x+y > 0 ? 1 : -1) * ::pow( 1/shiftdist, 1.0/3 );
      //z =  ::pow( 1/shiftdist, 1.0/3 );
      //printf( "%f\n", 20 * (x+y > 0 ? 1 : -1) * ::pow( 1/shiftdist, 1.0/3 )  );

        // limit the z result
        //double b = 20;
        //z = z >  b ?  b : z;
        //z = z < -b ? -b : z;

        z = (1 - param)*z + param * (-10 * (::sin(x - y) + ::cos(y - x)));
#endif
#endif
#endif
        return shiftscale * Eigen::Vector3d( x, y, z );
    }
};


// this is called leftrighteye3D but it's more like a general purpose z-zoom
// that takes binocular vision into account. experimental! expect changes.
class leftrighteye3D : public BWCNC::position_dependent_transform_t
{
public:
    const Eigen::Vector3d eye;
public:
    leftrighteye3D() : leftrighteye3D(Eigen::Vector3d(0,0,0)) {}
    leftrighteye3D(double eye_x, double eye_y, double eye_z) : leftrighteye3D(Eigen::Vector3d(eye_x,eye_y,eye_z)) {}
    leftrighteye3D(Eigen::Vector3d eye_pixel_position) : position_dependent_transform_t(false, true), eye(eye_pixel_position) {}

    virtual ~leftrighteye3D(){}

    const BWCNC::Color    cvf( const Eigen::Vector3d &   ) const { return BWCNC::Color(); }
    const Eigen::Matrix3d mvf( const Eigen::Vector3d &   ) const { return Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & p ) const
    {
#if 1
        Eigen::Vector3d v = p - eye;
      //Eigen::Vector3d v = eye - p;
        Eigen::Vector3d u = v;
        double s          = -eye[2]/v[2];
        u[2]              = -eye[2];
      //Eigen::Vector3d c = s*v - u;
      //return c;
        return              s*v - u;
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
    const BWCNC::Color    cvf( const Eigen::Vector3d & ) const { return BWCNC::Color(); }
    const Eigen::Matrix3d mvf( const Eigen::Vector3d & ) const { return (z_shift/2 + 1) * Eigen::Matrix3d::Identity(); }
    const Eigen::Vector3d vvf( const Eigen::Vector3d & ) const
    {
      //double t = theta_max * ::sin( ::sqrt(9.86/(L_mm/1000)) * ticks );
        return Eigen::Vector3d(0, 0, z_shift);
    }
};


#endif /* ifndef STDFORMS_H__ */
