#ifndef BWCNC_PART_H__
#define BWCNC_PART_H__

#include <list>
#include <map>
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
protected:
    void copy_into( Part & p );

public:
    BWCNC::Part * new_copy() { BWCNC::Part * p = new BWCNC::Part; this->copy_into(*p); return p; } // aka clone

public:
    Part() : isnil(true) {}
    Part( const Eigen::Vector3d & startpoint )
        : isnil( false ), isclosed( false ), moveto_cnt( 0 ), lineto_cnt( 0 ), dot_at_cnt( 0 ), start( startpoint ), curpos( startpoint )
    {
        update_bounds( start );
    }
    virtual ~Part(){ for( auto c : cmds ) if( c ) delete c; }

    virtual void pull_commands_from( BWCNC::Part & p );
    virtual void pull_commands_from( BWCNC::Part * p );

    virtual bool update_starting_position( const Eigen::Vector3d & pos );
  //virtual void render( BWCNC::Renderer * r ) { for( auto cmd : cmds ) if( cmd ) cmd->render( r ); }
    virtual void render( BWCNC::Renderer * r )
    {
        if( isclosed )
            r->render( this );
        else
            for( auto cmd : cmds ) if( cmd ) cmd->render( r );
    }

    virtual bool reposition( const Eigen::Vector3d & pos   );
    virtual void translate(  const Eigen::Vector3d & offst );
    virtual void transform(  const Eigen::Matrix3d & mat, bool remake_bbox = true );
    virtual void scale(      const double scalar );
    virtual void rotate( double angle, bool degrees = false, int rotationaxis = 3 );

    // short and long names for  position_dependent_transform
    virtual void pos_dep_tform( mvf_t mvf, vvf_t vvf );
    virtual void pos_dep_tform( pdt_t * tform );
    virtual void position_dependent_transform( mvf_t mvf, vvf_t vvf ) { pos_dep_tform( mvf, vvf ); }
    virtual void position_dependent_transform( pdt_t * tform ) { pos_dep_tform( tform ); }

    virtual void remake_boundingbox();

  //virtual void arcto(  const Eigen::Vector3d & to );
    virtual void lineto( const Eigen::Vector3d & to, bool vecfromcur = false );
    virtual void moveto( const Eigen::Vector3d & to, bool vecfromcur = false );
    virtual void dot_at( const Eigen::Vector3d & to, bool vecfromcur = false );

  //virtual void arcto_close( bool & isok );   // makes a 'closed' polygon
    virtual void lineto_close( bool & isok );  // makes a 'closed' polygon
    virtual void moveto_start() { moveto( start ); }

    virtual BWCNC::Boundingbox get_bbox(){ return bbox; }
    virtual const BWCNC::Boundingbox & get_bbox() const { return bbox; }
    virtual bool is_closed(){ return isclosed; }

protected:
    virtual void update_position( const Eigen::Vector3d & pos );
    virtual void update_bounds( const Eigen::Vector3d & newpoint );

protected:
    bool isnil;
    bool isclosed;
    int moveto_cnt; // how many moveto segments are there?
    int lineto_cnt; // how many lineto segments are there?
    int dot_at_cnt; // how many dots are there?
    BWCNC::Boundingbox bbox;
    Eigen::Vector3d start;
    Eigen::Vector3d curpos;

public:
    std::vector<BWCNC::Command *> cmds;
};

class PartContext
{
    friend class BWCNC::SVG;
    friend class BWCNC::Renderer;

public:
    void copy_into( PartContext & k );

public:
    PartContext() : partscnt(0), isnil(true) {}
    virtual ~PartContext(){ for( auto p : partlist ) if( p ) delete p; }

    PartContext( const Eigen::Vector3d & startpoint ) : partscnt(0), bbox(startpoint), firstpoint(startpoint), isnil(false) {}

    BWCNC::Part * get_new_part() { return new BWCNC::Part( this->last_coords() ); }
    void add_part( BWCNC::Part * newpart );

    // append_part is like add_part but newpart is repositioned to end of context
    void append_part( BWCNC::Part * newpart );
    void append_part_list( std::list<BWCNC::Part *> parts );
    void append_part_list( std::vector<BWCNC::Part *> parts );

    virtual void reposition( const Eigen::Vector3d & pos, Eigen::Vector3d * offset_sum = nullptr );
    virtual void translate(  const Eigen::Vector3d & pos );
    virtual void transform(  const Eigen::Matrix3d & mat, bool update_bbox = true );
    virtual void scale( const double scalar );
    virtual void rotate( double angle, bool degrees = false, int rotationaxis = 3 );

    // short and long names for  position_dependent_transform
    virtual void pos_dep_tform( mvf_t mvf, vvf_t vvf );
    virtual void pos_dep_tform( pdt_t * tform );

    virtual void position_dependent_transform( mvf_t mvf, vvf_t vvf ) { pos_dep_tform( mvf, vvf ); }
    virtual void position_dependent_transform( pdt_t * tform )        { pos_dep_tform( tform ); }

    virtual void remake_boundingbox();  // this forces all parts to also remake their bounding boxes
    virtual void reunion_boundingbox(); // this assumes that all part bboxs are correct, and remakes from union

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
    BWCNC::Boundingbox bbox;
    Eigen::Vector3d firstpoint;
    bool isnil;

public:
    std::vector<BWCNC::Part *> partlist;
    std::multimap<double, BWCNC::Part *> partlist_z_ascending;

    void refresh_z_ascending();
};

typedef enum {
    to_none,
    to_center,
    to_positive
} shift2_t ;

void shift2( BWCNC::PartContext & k, shift2_t x_st, shift2_t y_st, shift2_t z_st, Eigen::Vector3d * offset_sum = nullptr );
inline void shift2center(   BWCNC::PartContext & k, Eigen::Vector3d * offset_sum = nullptr ) { shift2( k, to_center,   to_center,   to_center,   offset_sum ); }
inline void shift2positive( BWCNC::PartContext & k, Eigen::Vector3d * offset_sum = nullptr ) { shift2( k, to_positive, to_positive, to_positive, offset_sum ); }
inline void undo_shift2( BWCNC::PartContext & k, const Eigen::Vector3d & offset_sum ) { k.reposition( -offset_sum ); }

};

#endif
