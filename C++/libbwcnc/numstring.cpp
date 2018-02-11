#include <sstream>
#include <iomanip>
#include <iostream>

#include <strings.h>
#include "numstring.h"

//using namespace BWCNC;

bool    BWCNC::NumString::terse = false;
uint8_t BWCNC::NumString::precision = 3;

std::string BWCNC::NumString::str()
{
    std::stringstream ss;
    if( ! terse ) ss << std::fixed;
    ss << std::setprecision( precision );
    ss << value;
    return ss.str();
}

#if 0
int main( int argc, char ** argv )
{
    BWCNC::NumString nm = 0.56;

    if( argc > 1 )
        nm = std::string(argv[1]);

    if( argc > 2 )
        BWCNC::NumString::precision = atoi(argv[2]);

    if( argc > 3 )
        BWCNC::NumString::terse = (0 == strcasecmp(argv[3],"t")) || (isdigit(argv[3][0]) ? (argv[3][0] - '0') : false) ;

    std::cout << nm.str() << "\n";

    return 0;
}
#endif

