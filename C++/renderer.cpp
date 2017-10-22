#include <iostream>
#include <math.h>

#include <Eigen/Dense>

#include "color.h"
#include "renderer.h"
#include "part.h"

void BWCNC::Renderer::render_all( const BWCNC::PartContext & k, const std::string * /* filename */ ) const
{
#if 0
    if( filename != nullptr )
    {
        set_output( filename );
        closefileonfinish = true;
    }
#endif

    print_start( k.bbox );

    for( auto part_ptr : k.partlist )  // c++11 forall construct. p is an element of partlist, a pointer
        if( part_ptr )
            part_ptr->render( this );

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


void BWCNC::SVG::lineto( const BWCNC::Command * cmd ) const
{
    if( ! bool(lineto_color) ) return;
    drawline( cmd, lineto_color );
}
void BWCNC::SVG::moveto( const BWCNC::Command * cmd ) const
{
    if( ! bool(moveto_color) ) return;
    drawline( cmd, moveto_color );
}

#if 0
void BWCNC::SVG::arcto( const BWCNC::Command * cmd )
{
    if( ! bool(lineto_color) ) return;
    drawarc( cmd, lineto_color );
}

void BWCNC::SVG::drawarc( const BWCNC::Command & cmd, const BWCNC::Color & clr )
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


void BWCNC::SVG::drawline( const BWCNC::Command * cmd, const BWCNC::Color & clr ) const
{
    std::vector<BWCNC::NumString> begp = VectorToNumStringArray( (cmd->begin + offset) * scalar );
    std::vector<BWCNC::NumString> endp = VectorToNumStringArray( (cmd->end   + offset) * scalar );

    printf( "<line x1=\"%s\" y1=\"%s\" x2=\"%s\" y2=\"%s\" style=\"stroke:#%06x;stroke-width:%d\" />\n",
            begp[0].str().c_str(), begp[1].str().c_str(),
            endp[0].str().c_str(), endp[1].str().c_str(),
            clr.to_rgb24(), stroke_width );
}

void BWCNC::SVG::print_end() const { printf( "</svg>\n" ); }

void BWCNC::SVG::print_start( const BWCNC::Boundingbox & bounds ) const
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

