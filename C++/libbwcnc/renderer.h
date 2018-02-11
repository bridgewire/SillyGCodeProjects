#ifndef BWCNC_RENDERER_H__
#define BWCNC_RENDERER_H__

#include <stdio.h>
#include <list>
#include <iostream>
#include "boundingbox.h"
#include "color.h"

namespace BWCNC {

class PartContext;
class Command;

class Renderer
{
public:
    Renderer(){}
    virtual ~Renderer(){}

    virtual void render_all( const BWCNC::PartContext & k, const std::string * filename = nullptr ) ;

    virtual void lineto( const BWCNC::Command * cmd ) = 0;
    virtual void moveto( const BWCNC::Command * cmd ) = 0;
    //virtual void arcto ( const BWCNC::Command * cmd )=0;

    virtual void print_start( const BWCNC::Boundingbox & bounds ) = 0;
    virtual void print_end() = 0;

    virtual void setoff( const Eigen::Vector3d & vec ){ offset = vec; }
    virtual void set_line_end( const char * str ){ eol = str; }

    virtual void set_moveto_color( const char * ) {}
    virtual void set_lineto_color( const char * ) {}
    virtual void set_backgd_color( const char * ) {}

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

protected:
    virtual void lineto( const BWCNC::Command * cmd ) { drawline( cmd, lineto_color ); }
    virtual void moveto( const BWCNC::Command * cmd ) { drawline( cmd, moveto_color ); }
  //virtual void arcto ( const BWCNC::Command * cmd ) { drawarc(  cmd, moveto_color ); }

    virtual void drawline( const BWCNC::Command * cmd, const BWCNC::Color & clr ) = 0;
  //virtual void drawarc(  const BWCNC::Command * cmd, const BWCNC::Color & clr );

    BWCNC::Color moveto_color = "#0000ff";
    BWCNC::Color lineto_color = "#ff0000";
    BWCNC::Color backgd_color = "#ffffff";
    int stroke_width = 1;
};

class SVG : public GraphicalRenderer
{
public:
    SVG(){;}
    virtual ~SVG(){;}

protected:
    virtual void drawline( const BWCNC::Command * cmd, const BWCNC::Color & clr );
  //virtual void drawarc(  const BWCNC::Command * cmd, const BWCNC::Color & clr );

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
