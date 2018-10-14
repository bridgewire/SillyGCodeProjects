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
    virtual void render( BWCNC::Renderer * r ) = 0;

    // move part in the direction of the Vector argument: offset
    virtual void translate( const Eigen::Vector3d & offset, Command * into = nullptr );

    // translate part so that @start becomes equal to new_position
    virtual void reposition( const Eigen::Vector3d & new_position, Command * into = nullptr );

    // general linear transform
    // arg: mat  ...  type 3x3 numeric Matrix
    virtual void transform( const Eigen::Matrix3d & mat, Command * into = nullptr );

    virtual void pos_dep_tform( const mvf_t mvf, const vvf_t vvf );
    virtual void pos_dep_tform( const pdt_t * tform );
    virtual void position_dependent_transform( const mvf_t mvf, const vvf_t vvf ) { pos_dep_tform( mvf, vvf ); }
    virtual void position_dependent_transform( const pdt_t * tform )              { pos_dep_tform( tform ); }

    virtual Command * new_copy() const = 0;

protected:
    virtual void begindflt() { begin = Eigen::Vector3d(0,0,0); }
    virtual void endflt()    { end = begin; }
    virtual void clrdflt()   { clr = BWCNC::Color(); /* BWCNC::Color( 0x000000 ); */ }

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

    virtual void render( BWCNC::Renderer * r ){ r->lineto( (const BWCNC::Command *)this ); }

    virtual Line * new_copy() const { Line * nl = new Line; *nl = *this; return nl; }
};

class Move : public Command
{
public:
    Move(){}

    Move( const Eigen::Vector3d & f, const Eigen::Vector3d & t ) : Command( f, t ) {}
    Move( const Eigen::Vector3d & f, const Eigen::Vector3d & t, const BWCNC::Color & c ) : Command( f, t, c ) {}

    Move( const Eigen::Vector2d & f, const Eigen::Vector2d & t )                         : Command( f, t ) {}
    Move( const Eigen::Vector2d & f, const Eigen::Vector2d & t, const BWCNC::Color & c ) : Command( f, t, c ) {}

    virtual void render( BWCNC::Renderer * r ){ r->moveto( this ); }

    virtual Move * new_copy() const { Move * nm = new Move; *nm = *this; return nm; }
};

class Dot : public Command
{
public:
    Dot(){}

    Dot( const Eigen::Vector3d & a ) : Command( a, a ) {}
    Dot( const Eigen::Vector3d & a, const BWCNC::Color & c ) : Command( a, a, c ) {}

    Dot( const Eigen::Vector2d & a )                         : Command( a, a ) {}
    Dot( const Eigen::Vector2d & a, const BWCNC::Color & c ) : Command( a, a, c ) {}

    virtual void render( BWCNC::Renderer * r ){ r->dot_at( this ); }

    virtual Dot * new_copy() const { Dot * nd = new Dot; *nd = *this; return nd; }
};


};

#endif
