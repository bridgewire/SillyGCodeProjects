#ifndef BWCNC_PART_H__
#define BWCNC_PART_H__

#include <list>
#include <Eigen/Dense>
#include "boundingbox.h"
#include "command.h"
#include "functions.h"
#include "renderer.h"


namespace BWCNC {

class PartContext;

class Part
{
    friend class BWCNC::PartContext;
public:
    Part() : isnil(true) {}
    Part( const Eigen::Vector3d & startpoint ) : isnil(false) {}
    virtual ~Part(){ for( auto c : cmds ) if( c ) delete c; }

    virtual void update_position( const Eigen::Vector3d & pos );
    virtual void render( const BWCNC::Renderer * r ) { for( auto cmd : cmds ) if( cmd ) cmd->render( r ); }

    virtual void reposition( const Eigen::Vector3d & pos   );
    virtual void translate(  const Eigen::Vector3d & offst );
    virtual void transform(  const Eigen::Matrix3d & mat   );
    virtual void scale(      const double scalar );

    // short and long names for  position_dependent_transform
    virtual void pos_dep_tform( mvf_t mvf, vvf_t vvf );
    virtual void position_dependent_transform( mvf_t mvf, vvf_t vvf ) { pos_dep_tform( mvf, vvf ); }

    virtual void remake_boundingbox();

  //virtual void arcto(  const Eigen::Vector3d & to );
    virtual void lineto( const Eigen::Vector3d & to );
    virtual void moveto( const Eigen::Vector3d & to );

    virtual BWCNC::Boundingbox get_bbox(){ return bbox; }

protected:
    virtual void update_bounds( const Eigen::Vector3d & newpoint );

protected:
    bool isnil;
    std::vector<BWCNC::Command *> cmds;
    BWCNC::Boundingbox bbox;
    Eigen::Vector3d start;
    Eigen::Vector3d curpos;
};

class PartContext
{
    friend class BWCNC::SVG;
    friend class BWCNC::Renderer;
public:
    PartContext() : partscnt(0), isnil(true) {}
    virtual ~PartContext(){ for( auto p : partlist ) if( p ) delete p; }

    PartContext( const Eigen::Vector3d & startpoint ) : partscnt(0), bbox(startpoint), firstpoint(startpoint), isnil(false) {}
    void add_part( BWCNC::Part * newpart );
    BWCNC::Part * get_new_part() { return new BWCNC::Part( this->last_coords() ); }

    virtual void reposition( const Eigen::Vector3d & pos );
    virtual void translate(  const Eigen::Vector3d & pos ) { bbox.translate( pos); for( auto prt : partlist ) if( prt ) prt->translate( pos ); }
    virtual void transform(  const Eigen::Matrix3d & mat ) { bbox.transform( mat); for( auto prt : partlist ) if( prt ) prt->transform( mat ); }
    virtual void scale( const double scalar ) { bbox.scale(scalar); for( auto prt : partlist ) if( prt ) prt->scale( scalar ); }

    // short and long names for  position_dependent_transform
    virtual void pos_dep_tform( mvf_t mvf, vvf_t vvf ) { bbox.pos_dep_tform( mvf, vvf ); for( auto prt : partlist ) if( prt ) prt->pos_dep_tform( mvf, vvf ); }
    virtual void position_dependent_transform( mvf_t mvf, vvf_t vvf ) { pos_dep_tform( mvf, vvf ); }

    virtual void remake_boundingbox();

    virtual void get_current_position( Eigen::Vector3d & v ) { v = last_coords(); }
    virtual void get_current_position( double & x, double & y, double & z )
    { 
        if( partscnt < 1 )
        {
            x = firstpoint[0];
            y = firstpoint[1];
            z = firstpoint[2];
        }
        else
        {
            Eigen::Vector3d & v = partlist[partscnt-1]->curpos;
            x = v[0];
            y = v[1];
            z = v[2];
        }
    }

    virtual BWCNC::Boundingbox get_bbox(){ return bbox; }

protected:
    void update_bbox( const BWCNC::Part * newpart );
    Eigen::Vector3d last_coords(){ if( partscnt < 1 ){ return firstpoint; } return partlist[partscnt-1]->curpos; }

protected:
    int partscnt;
    std::vector<BWCNC::Part *> partlist;
    BWCNC::Boundingbox bbox;
    Eigen::Vector3d firstpoint;
    bool isnil;
};

};

#endif
