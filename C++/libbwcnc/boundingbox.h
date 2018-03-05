#ifndef BWCNC_BOUNDINGBOX_H__
#define BWCNC_BOUNDINGBOX_H__

#include <iostream>
#include <Eigen/Dense>

#include "functions.h"

namespace BWCNC {

class Boundingbox
{
public:
    Boundingbox() :
        min(Eigen::Vector3d(0,0,0)), max(Eigen::Vector3d(0,0,0)), pointsum(Eigen::Vector3d(0,0,0)), pointcnt(0), isnil(true), haspoint(false)
    {}
    Boundingbox( const Eigen::Vector3d & root ) : min(root), max(root), pointsum(root), pointcnt(1), isnil(true), haspoint(true) {}
    Boundingbox( const Eigen::Vector3d & root, const Eigen::Vector3d & extent ) : min(root), max(extent), pointsum(root + extent), pointcnt(2), isnil( min == max ), haspoint(true) {}
    virtual ~Boundingbox(){}

    explicit operator bool()  const { return haspoint; }   // these bool overloads
    bool operator!() const { return ! haspoint; } // probably aren't used anywhere

    void scale( double scalar ) { min *= scalar; max *= scalar; pointsum *= scalar; }
    virtual void translate( const Eigen::Vector3d & offset ) { min += offset; max += offset; pointsum += pointcnt * offset; }
    virtual void reposition( const Eigen::Vector3d & newp ){ translate(newp - min); translate(newp - max); /* translate(pointcnt * newp - pointsum); */ } 
    //virtual void transform( const Eigen::Matrix3d & mat ) { min = mat * min; max = mat * max; pointsum = mat * pointsum; }
    //virtual void pos_dep_tform( mvf_t mvf, vvf_t vvf ) { BWCNC::pos_dep_tform( mvf, vvf, min ); BWCNC::pos_dep_tform( mvf, vvf, max ); }
    //virtual void position_dependent_transform( mvf_t mvf, vvf_t vvf ) { pos_dep_tform( mvf, vvf ); }

    //virtual void pos_dep_tform( pdt_t * tform ) { BWCNC::pos_dep_tform( tform, min ); BWCNC::pos_dep_tform( tform, max ); }
    //virtual void position_dependent_transform( pdt_t * tform ) { pos_dep_tform( tform ); }

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
        pointsum += newv;
        pointcnt += 1;
    }

    void union_with( const BWCNC::Boundingbox & obbox )
    {
        update_bounds( obbox.min );
        update_bounds( obbox.max );

        pointsum += obbox.pointsum;
        pointcnt += obbox.pointcnt;
    }

    double width()  const { return max[0] - min[0]; }
    double height() const { return max[1] - min[1]; }
    double depth()  const { return max[2] - min[2]; }

    Eigen::Vector3d avg() const { return pointsum / pointcnt; }

    std::ostream & to_ostream( std::ostream & s ) const;

public:
    Eigen::Vector3d min;
    Eigen::Vector3d max;

    // these can give us a kind of average/center
    Eigen::Vector3d pointsum;
    int64_t         pointcnt;

    bool isnil;
    bool haspoint;

    BWCNC::Boundingbox subtract( const BWCNC::Boundingbox & other ) const
    {
        Boundingbox b;
        b.min = min - other.min;
        b.max = max - other.max;

        // subtracting this may not make sense
        // doing an average might be better
        b.pointsum = pointsum - other.pointsum;
        b.pointcnt = pointcnt - other.pointcnt;

        b.isnil = isnil && other.isnil;
        return b;
    }
};

inline BWCNC::Boundingbox operator-( const BWCNC::Boundingbox & lha, const BWCNC::Boundingbox & rha ) { return lha.subtract(rha); }

};

std::ostream & operator<<( std::ostream & s, const BWCNC::Boundingbox & bbox );

#endif
