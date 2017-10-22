#ifndef BWCNC_COMMAND_H__
#define BWCNC_COMMAND_H__

#include <list>
#include <Eigen/Dense>
#include "color.h"
#include "renderer.h"
#include "functions.h"


namespace BWCNC {

#if 0
struct CommandArgs
{
public:
    CommandArgs( const Eigen::Vector3d & from, const Eigen::Vector3d & to, const BWCNC::Color clr, bool anglegrows, bool followlargarc, double radius)
        : point_from(from), point_to(to), color(clr), sweep(anglegrows), largarc(followlargarc), radius(radius)
    {}

    Eigen::Vector3d point_from;
    Eigen::Vector3d point_to;
    BWCNC::Color color;
    bool sweep;     // sweep == true : start2finish travels along positive angle
    bool largearc;  // largearc == true : given start,stop,center, the larger possible arc is chosen
    double radius;
};
#endif

class Command
{
public:
    Command(){ begindflt(); endflt(); clrdflt(); }

    Command( const Eigen::Vector3d & f, const Eigen::Vector3d & t ) : begin(f), end(t) { clrdflt(); }
    Command( const Eigen::Vector3d & f, const Eigen::Vector3d & t, const BWCNC::Color & c ) : begin(f), end(t), clr(c) {}

    Command( const Eigen::Vector2d & f, const Eigen::Vector2d & t );
    Command( const Eigen::Vector2d & f, const Eigen::Vector2d & t, const BWCNC::Color & c );

    virtual ~Command(){}

    // us the renderer to "render" this command, according to the Renderer's design
    virtual void render( const BWCNC::Renderer * r ) = 0;

    // move part in the direction of the Vector argument: offset
    virtual void translate( const Eigen::Vector3d & offset )
    {
        begin += offset;
        end   += offset;
    }

    // translate part so that @start becomes equal to new_position
    virtual void reposition( const Eigen::Vector3d & new_position ){ translate( new_position - begin ); } 

    // general linear transform
    // arg: mat  ...  type 3x3 numeric Matrix
    virtual void transform( const Eigen::Matrix3d & mat )
    {
      begin = mat * begin;
      end   = mat * end;
    }

    virtual void pos_dep_tform( mvf_t mvf, vvf_t vvf );
    virtual void position_dependent_transform( mvf_t mvf, vvf_t vvf ) { pos_dep_tform( mvf, vvf ); }

protected:
    virtual void begindflt() { begin = Eigen::Vector3d(0,0,0); }
    virtual void endflt()    { end = begin; }
    virtual void clrdflt()   { clr = BWCNC::Color( (uint32_t)0x000000 ); }

public:
    Eigen::Vector3d begin;
    Eigen::Vector3d end;
    BWCNC::Color clr;
};

class Line : public Command
{
public:
    Line(){}

    Line( const Eigen::Vector3d & f, const Eigen::Vector3d & t ) : Command( f, t ) {}
    Line( const Eigen::Vector3d & f, const Eigen::Vector3d & t, const BWCNC::Color & c ) : Command( f, t, c ) {}

    Line( const Eigen::Vector2d & f, const Eigen::Vector2d & t )                         : Command( f, t ) {}
    Line( const Eigen::Vector2d & f, const Eigen::Vector2d & t, const BWCNC::Color & c ) : Command( f, t, c ) {}

    virtual void render( const BWCNC::Renderer * r ){ r->lineto( (const BWCNC::Command *)this ); }
};

class Move : public Command
{
public:
    Move(){}

    Move( const Eigen::Vector3d & f, const Eigen::Vector3d & t ) : Command( f, t ) {}
    Move( const Eigen::Vector3d & f, const Eigen::Vector3d & t, const BWCNC::Color & c ) : Command( f, t, c ) {}

    Move( const Eigen::Vector2d & f, const Eigen::Vector2d & t )                         : Command( f, t ) {}
    Move( const Eigen::Vector2d & f, const Eigen::Vector2d & t, const BWCNC::Color & c ) : Command( f, t, c ) {}

    virtual void render( const BWCNC::Renderer * r ){ r->moveto( this ); }
};

};

#endif
