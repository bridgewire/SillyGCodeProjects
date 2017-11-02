#ifndef BWCNC_BOUNDINGBOX_H__
#define BWCNC_BOUNDINGBOX_H__

#include <iostream>
#include <Eigen/Dense>

#include "functions.h"

namespace BWCNC {

class Boundingbox
{
public:
    Boundingbox() : isnil(true), haspoint(false) {}
    Boundingbox( const Eigen::Vector3d & root ) : min(root), max(root), isnil(true), haspoint(true) {}
    Boundingbox( const Eigen::Vector3d & root, const Eigen::Vector3d & extent ) : min(root), max(extent), isnil( min == max ), haspoint(true) {}
    virtual ~Boundingbox(){}

    explicit operator bool()  const { return haspoint; }   // these bool overloads
    bool operator!() const { return ! haspoint; } // probably aren't used anywhere

    void scale( double scalar ) { min *= scalar; max *= scalar; }
    virtual void translate( const Eigen::Vector3d & offset ) { min += offset; max += offset; }
    virtual void reposition( const Eigen::Vector3d & newp ){ translate(newp - min); translate(newp - max); } 
    virtual void transform( const Eigen::Matrix3d & mat ) { min = mat * min; max = mat * max; }
    virtual void pos_dep_tform( mvf_t mvf, vvf_t vvf ) { BWCNC::pos_dep_tform( mvf, vvf, min ); BWCNC::pos_dep_tform( mvf, vvf, max ); }
    virtual void position_dependent_transform( mvf_t mvf, vvf_t vvf ) { pos_dep_tform( mvf, vvf ); }

    virtual void pos_dep_tform( pdt_t * tform ) { BWCNC::pos_dep_tform( tform, min ); BWCNC::pos_dep_tform( tform, max ); }
    virtual void position_dependent_transform( pdt_t * tform ) { pos_dep_tform( tform ); }

    void update_bounds( const Eigen::Vector3d & newv )
    {
        if( ! haspoint )
        {
            min = newv;
            max = newv;
            haspoint = true;
        }
        else
        {
            for( int i = 0; i < 3; i++ )
            {
                min[i] = std::min( min[i], newv[i] );
                max[i] = std::max( max[i], newv[i] );
            }
            isnil = false;
        }
    }

    void union_with( const BWCNC::Boundingbox & obbox )
    {
        update_bounds( obbox.min );
        update_bounds( obbox.max );
    }

    double width(){  return max[0] - min[0]; }
    double height(){ return max[1] - min[1]; }

    std::ostream & to_ostream( std::ostream & s ) const;

public:
    Eigen::Vector3d min;
    Eigen::Vector3d max;
    bool isnil;
    bool haspoint;
};

};

std::ostream & operator<<( std::ostream & s, const BWCNC::Boundingbox & bbox );

#endif
