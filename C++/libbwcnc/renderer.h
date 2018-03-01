#ifndef BWCNC_RENDERER_H__
#define BWCNC_RENDERER_H__

#include <stdio.h>
#include <list>
#include <iostream>
#include "color.h"
#include "boundingbox.h"

namespace BWCNC {

class PartContext;
class Part;
class Command;

class Renderer
{
public:
    Renderer(){}
    virtual ~Renderer(){}

    virtual void render_all( const BWCNC::PartContext & k, const std::string * filename = nullptr ) ;
    virtual void render_all_z_order( BWCNC::PartContext & k );

    virtual void render( const BWCNC::Part    * prt ) = 0;
    virtual void lineto( const BWCNC::Command * cmd ) = 0;
    virtual void moveto( const BWCNC::Command * cmd ) = 0;
    virtual void dot_at( const BWCNC::Command * cmd ) = 0;
    //virtual void arcto ( const BWCNC::Command * cmd )=0;

    virtual void print_start( const BWCNC::Boundingbox & bounds ) = 0;
    virtual void print_end() = 0;

    virtual void setoff( const Eigen::Vector3d & vec ){ offset = vec; }
    virtual void set_line_end( const char * str ){ eol = str; }

    virtual void set_moveto_color( const char * ) {}
    virtual void set_lineto_color( const char * ) {}
    virtual void set_backgd_color( const char * ) {}
    virtual void set_dot_color( const char * ) {}

protected:
    Eigen::Vector3d offset;
    BWCNC::Boundingbox bbox;
    const char * eol = "\r\n";
    double scalar = 1;
};

class GraphicalRenderer : public Renderer
{
public:
    GraphicalRenderer(){;}
    virtual ~GraphicalRenderer(){;}

    virtual void set_moveto_color( const char * spec ) { moveto_color = BWCNC::Color(spec); }
    virtual void set_lineto_color( const char * spec ) { lineto_color = BWCNC::Color(spec); }
    virtual void set_backgd_color( const char * spec ) { backgd_color = BWCNC::Color(spec); }
    virtual void set_dot_color( const char * spec ) { dot_color = BWCNC::Color(spec); }

protected:
    virtual void render( const BWCNC::Part    * prt ) { drawpart( prt ); }
    virtual void lineto( const BWCNC::Command * cmd ) { drawline( cmd ); }
    virtual void moveto( const BWCNC::Command * cmd ) { drawline( cmd ); }
    virtual void dot_at( const BWCNC::Command * cmd ) { draw_dot( cmd ); }
  //virtual void arcto ( const BWCNC::Command * cmd ) { drawarc(  cmd ); }

    virtual void drawpart( const BWCNC::Part    * prt ) = 0;
    virtual void drawline( const BWCNC::Command * cmd ) = 0;
    virtual void draw_dot( const BWCNC::Command * cmd ) = 0;
  //virtual void drawarc(  const BWCNC::Command * cmd ) = 0;

    BWCNC::Color moveto_color = "#0000ff";
    BWCNC::Color lineto_color = "#ff0000";
    BWCNC::Color backgd_color = "#ffffff";
    BWCNC::Color dot_color;
    int stroke_width = 1;
};

class SVG : public GraphicalRenderer
{
public:
    SVG(){;}
    virtual ~SVG(){;}
    bool render_positive_z = true;
    bool render_negative_z = true;

protected:
    virtual void drawpart( const BWCNC::Part    * prt );
    virtual void drawline( const BWCNC::Command * cmd );
    virtual void draw_dot( const BWCNC::Command * cmd );
  //virtual void drawarc(  const BWCNC::Command * cmd );

    virtual void print_start( const BWCNC::Boundingbox & bounds );
    virtual void print_end();
};

#if 0
class GCode : public Renderer
{
public:
    GCode(){}
    virtual ~GCode(){}

    void lineto();
    void moveto();

protected:
    void linear_motion();
    void print_start();
    void print_end();
    void print_cmd();
    void cutting_off();
};
#endif

};

#endif
