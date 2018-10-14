#include <iostream>
#include <math.h>

#include <Eigen/Dense>

#include "color.h"
#include "renderer.h"
#include "part.h"

void BWCNC::Renderer::render_all( const BWCNC::PartContext & k, const std::string * /* filename */ )
{
    print_start( k.bbox );

    // c++11 forall construct. p is an element of partlist, a pointer
    for( const auto & part_ptr : k.partlist )
        part_ptr->render( this );

    print_end();
}

void BWCNC::Renderer::render_all_z_order( BWCNC::PartContext & k )
{
    k.refresh_z_ascending();

    print_start( k.bbox );

    for( const auto & part_ptr : k.partlist_z_ascending )
        part_ptr.second->render( this );

    print_end();
}


#if 0
protected:

    std::ostream & strm;
    Eigen::Vector3d offset;
    BWCNC::Bouningbox bbox;
    const char * eol = "\r\n";
    double scalar = 1;
};
#endif


#if 0
void BWCNC::SVG::arcto( const BWCNC::Command * cmd )
{
    if( ! bool(lineto_color) ) return;
    drawarc( cmd, lineto_color );
}

void BWCNC::SVG::drawarc( const BWCNC::Command & cmd )
{
    td::vector<NumString> begp = VectorToNumStringArray( (cmd.begin + offset) * scalar );
    td::vector<NumString> endp = VectorToNumStringArray( (cmd.end   + offset) * scalar );
    double rdus = radiu * scalar;

    printf("<path d=\"M%s %s A%s,%s 0 %d,%d %s,%s\" style=\"stroke:#%06x;stroke-width:%d;fill:none\" />\n",
            begp[0].str().c_str(), begp[1].str().c_str(),
            rdus, rdus,
            (cmd.largearc ? 1 : 0), (cmd.sweep ? 1 : 0),
            endp[0].str().c_str(), endp[1].str().c_str(),
            clr.to_rgb24(), stroke_width );
}
#endif

void BWCNC::SVG::draw_dot( const BWCNC::Command * cmd )
{
    BWCNC::Color c = cmd->clr;
    if( ! bool(cmd->clr) )
    {
        if( ! bool(dot_color) )
            return;
        c = dot_color;
    }
    //if( ! bool(lineto_color) ) return;

    std::vector<BWCNC::NumString> begp = VectorToNumStringArray( Eigen::Vector3d((cmd->begin + offset) * scalar) );
    std::vector<BWCNC::NumString> endp = VectorToNumStringArray( Eigen::Vector3d((cmd->end   + offset) * scalar) );

    printf( "<circle cx=\"%s\" cy=\"%s\" r=\"1\" fill=\"#%06x\" />\n",
            begp[0].str().c_str(), begp[1].str().c_str(), c.to_rgb24() );
}

void BWCNC::SVG::drawpart( const BWCNC::Part    * /* prt */ )
{
}


void BWCNC::SVG::drawline( const BWCNC::Command * cmd )
{
    BWCNC::Color c = cmd->clr;
    if( ! bool(cmd->clr) )
    {
        if( ! bool(lineto_color) )
            return;
        c = lineto_color;
    }
    //if( ! bool(lineto_color) ) return;

    std::vector<BWCNC::NumString> begp = VectorToNumStringArray( Eigen::Vector3d((cmd->begin + offset) * scalar) );
    std::vector<BWCNC::NumString> endp = VectorToNumStringArray( Eigen::Vector3d((cmd->end   + offset) * scalar) );

    printf( "<line x1=\"%s\" y1=\"%s\" x2=\"%s\" y2=\"%s\" style=\"stroke:#%06x;stroke-width:%d\" />\n",
            begp[0].str().c_str(), begp[1].str().c_str(),
            endp[0].str().c_str(), endp[1].str().c_str(),
            c.to_rgb24(), stroke_width );
}

void BWCNC::SVG::print_end() { printf( "</svg>\n" ); }

void BWCNC::SVG::print_start( const BWCNC::Boundingbox & bounds )
{
    if( ! bounds ) return;

    // svg is 2 dimensional
    long w = ::ceill(bounds.max[0]) + 1;
    long h = ::ceill(bounds.max[1]) + 1;

    printf( "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n" );
    printf( "<svg xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns=\"http://www.w3.org/2000/svg\" height=\"%ld\" width=\"%ld\">\n", h, w );
    if( bool(backgd_color) )
        printf( "<rect height=\"100%%\" width=\"100%%\" style=\"fill:#%06x;\" />\n", backgd_color.to_rgb24() );
    else
        printf( "<rect height=\"100%%\" width=\"100%%\" />\n" );
}


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

