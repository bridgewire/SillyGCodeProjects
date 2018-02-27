#include "command.h"
#include "qpixmaprenderer.h"

void BWCNC::PixmapRenderer::drawline( const BWCNC::Command * cmd, const BWCNC::Color & clr )
{
    //if( ! clr ) return;
    if( ! cmd->clr ) return;
    if( ! render_positive_z && (cmd->begin[2] > 0 || cmd->end[2] > 0) ) return;
    if( ! render_negative_z && (cmd->begin[2] < 0 || cmd->end[2] < 0) ) return;

    pen.setColor( QColor(cmd->clr.to_rgb24()) );
    p->setPen( pen );
    p->drawLine( cmd->begin[0], cmd->begin[1], cmd->end[0], cmd->end[1] );

    if( debug_countdown > 0 )
    {
        printf( "%d:(%f,%f,%f) -> (%f,%f,%f)\n", debug_countdown, cmd->begin[0], cmd->begin[1], cmd->begin[2], cmd->end[0], cmd->end[1], cmd->end[2] );
        debug_countdown--;
        if( debug_countdown <= 0 )
            printf( "\n" );
    }
}

void BWCNC::PixmapRenderer::draw_dot( const BWCNC::Command * cmd, const BWCNC::Color & clr )
{
    if( ! clr ) return;
    if( ! render_positive_z && cmd->begin[2] > 0 ) return;
    if( ! render_negative_z && cmd->begin[2] < 0 ) return;

    pen.setColor( QColor(clr.to_rgb24()) );
    p->setPen( pen );
    p->drawEllipse( QPoint(cmd->begin[0], cmd->begin[1]), 1, 1 );

    if( debug_countdown > 0 )
    {
        printf( "%d:(%f,%f,%f)\n", debug_countdown, cmd->begin[0], cmd->begin[1], cmd->begin[2] );
        debug_countdown--;
        if( debug_countdown <= 0 )
            printf( "\n" );
    }
}



void BWCNC::PixmapRenderer::print_start( const BWCNC::Boundingbox & )
{
  //debug_countdown = 5;

    if( bool(backgd_color) )
        im->fill( QColor(backgd_color.to_rgb24()) );
    p->begin(im);
    p->setRenderHint( QPainter::Antialiasing );
    pen.setWidth(1);

}
