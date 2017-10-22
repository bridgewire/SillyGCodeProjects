#include "boundingbox.h"

std::ostream & operator<<( std::ostream & s, const BWCNC::Boundingbox & bbox ) { return bbox.to_ostream(s); }
//std::ostream & operator<<( std::ostream & s, const BWCNC::Boundingbox * bbox ) { return bbox->to_ostream(s); }

std::ostream & BWCNC::Boundingbox::to_ostream( std::ostream & s ) const
{
    s << std::dec
      << "bbox{ min:" << "(" << min.transpose() << " )" << ", max:" << "(" << max.transpose() << " )" << " }";

    return s;
}

