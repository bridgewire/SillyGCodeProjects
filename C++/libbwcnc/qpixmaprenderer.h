#ifndef PIXMAPRENDERER_H__
#define PIXMAPRENDERER_H__

#include <QImage>
#include <QPainter>

#include "renderer.h"

namespace BWCNC
{

class PixmapRenderer : public BWCNC::GraphicalRenderer
{
public:
    PixmapRenderer() {}
    PixmapRenderer( QImage * img ) : im(img) { p = new QPainter; painter_is_owned = true; }
    PixmapRenderer( QImage * img, QPainter * pntr ) : im(img), p(pntr) {}
    virtual ~PixmapRenderer() { if(painter_is_owned) delete p; }

    void set_image( QImage * img ) { im = img; }
    int debug_countdown = 0;
    bool render_positive_z = true;
    bool render_negative_z = true;

protected:
    QImage * im = nullptr;
    QPainter * p = nullptr;
    QPen pen; // lineto_pen; moveto_pen;

private:
    bool painter_is_owned = false;

protected:
    virtual void drawline( const BWCNC::Command * cmd, const BWCNC::Color & clr ) ;
    virtual void draw_dot( const BWCNC::Command * cmd, const BWCNC::Color & clr ) ;
  //virtual void drawarc(  const BWCNC::Command * cmd, const BWCNC::Color & clr ) ;

    virtual void print_start( const BWCNC::Boundingbox & );
    virtual void print_end() { p->end(); }
};

};

#endif /* ifdef PIXMAPRENDERER_H__ */
