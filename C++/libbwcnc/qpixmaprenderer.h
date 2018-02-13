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
    PixmapRenderer( QImage * img ) : im(img) {}
    virtual ~PixmapRenderer() {}

    void set_image( QImage * img ) { im = img; }
    int debug_countdown = 0;
    bool renderonly_positive_z = false;

protected:
    QImage * im = nullptr;
    QPainter p;
    QPen pen; // lineto_pen; moveto_pen;

protected:
    virtual void drawline( const BWCNC::Command * cmd, const BWCNC::Color & clr ) ;
    virtual void draw_dot( const BWCNC::Command * cmd, const BWCNC::Color & clr ) ;
  //virtual void drawarc(  const BWCNC::Command * cmd, const BWCNC::Color & clr ) ;

    virtual void print_start( const BWCNC::Boundingbox & );
    virtual void print_end() { p.end(); }
};

};

#endif /* ifdef PIXMAPRENDERER_H__ */
