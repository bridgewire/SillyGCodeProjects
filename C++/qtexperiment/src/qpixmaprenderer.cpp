#include "qpixmaprenderer.h"

void PixmapRenderer::drawline( const BWCNC::Command * cmd, const BWCNC::Color & clr )
{
    if( ! clr ) return;
    pen.setColor( QColor(clr.to_rgb24()) );
    p.setPen( pen );
    p.drawLine( cmd->begin[0], cmd->begin[1], cmd->end[0], cmd->end[1] );
}

void PixmapRenderer::print_start( const BWCNC::Boundingbox & )
{
    im->fill( QColor(backgd_color.to_rgb24()) );
    p.begin(im);
    p.setRenderHint( QPainter::Antialiasing );
    pen.setWidth(1);

}
