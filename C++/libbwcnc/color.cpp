#include "color.h"

std::ostream & operator<<( std::ostream & s, const BWCNC::Color & clr ) { return clr.to_ostream(s); }


#ifdef BWCNCCOLOR_UNITTEST
int main()
{
    BWCNC::Color red = "#ff0000";
    BWCNC::Color clr = red;
    clr = "#0000ff";

    std::cout << "red == " << red << "\n";
    std::cout << "clr == " << clr << "\n";

}
#endif /* ifdef BWCNCCOLOR_UNITTEST */
