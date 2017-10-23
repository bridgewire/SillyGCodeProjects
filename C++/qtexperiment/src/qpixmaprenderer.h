#ifndef PIXMAPRENDERER_H__
#define PIXMAPRENDERER_H__

#include <QImage>
#include <QPainter>

#include <bwcnc.h>

//using namespace BWCNC;

class PixmapRenderer : public BWCNC::GraphicalRenderer 
{
public:
    PixmapRenderer() : im(nullptr) {}
    PixmapRenderer( QImage * img ) : im(img) {}
    virtual ~PixmapRenderer() {}
    void set_image( QImage * img ) { im = img; }

protected:
    QImage * im;
    QPainter p;
    QPen pen; // lineto_pen; moveto_pen;

protected:
    virtual void drawline( const BWCNC::Command * cmd, const BWCNC::Color & clr ) ;
  //virtual void drawarc(  const BWCNC::Command * cmd, const BWCNC::Color & clr ) ;

    virtual void print_start( const BWCNC::Boundingbox & );
    virtual void print_end() { p.end(); }
};


#endif /* ifdef PIXMAPRENDERER_H__ */
