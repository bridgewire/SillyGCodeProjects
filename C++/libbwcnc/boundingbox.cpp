#include "boundingbox.h"

std::ostream & operator<<( std::ostream & s, const BWCNC::Boundingbox & bbox ) { return bbox.to_ostream(s); }
//std::ostream & operator<<( std::ostream & s, const BWCNC::Boundingbox * bbox ) { return bbox->to_ostream(s); }

std::ostream & BWCNC::Boundingbox::to_ostream( std::ostream & s ) const
{
    //Eigen::Vector3d avg = Eigen::Vector3d(0,0,0);
    //if( pointcnt != 0 ) avg = pointsum / pointcnt;

    s << std::dec
      << "bbox{ min:" << "(" << min.transpose() << " )"
          << ", max:" << "(" << max.transpose() << " )"
          << ", avg:"

          << "(" << pointsum.transpose() << " ) / "  << pointcnt << " == "
          << "(" << avg().transpose() << " )"
          << " }";

    return s;
}

