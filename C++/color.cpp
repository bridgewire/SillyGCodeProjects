#include "color.h"

std::ostream & operator<<( std::ostream & s, const BWCNC::Color & clr ) { return clr.to_ostream(s); }

