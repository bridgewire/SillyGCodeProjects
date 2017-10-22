#include <iostream>

#include "functions.h"
#include "numstring.h"
#include "color.h"
#include "boundingbox.h"
#include "command.h"
#include "part.h"
#include "renderer.h"

using namespace BWCNC;

bool test_VectorToNumStringArray()
{
    std::vector<NumString> result = VectorToNumStringArray( Eigen::Vector3d( 3.004005, 5.6820, 7.802930 ) );
    for( std::vector<NumString>::iterator i = result.begin(); i != result.end(); i++ )
        std::cout << i->str() << " ";
    std::cout << "\n";
    return true;
}


int main( int argc, char ** argv )
{
    if( argc > 2 ) BWCNC::NumString::precision = atoi(argv[2]);
    if( argc > 3 ) BWCNC::NumString::terse = (0 == strcasecmp(argv[3],"t")) || (isdigit(argv[3][0]) ? (argv[3][0] - '0') : false) ;

    test_VectorToNumStringArray();

    BWCNC::Color c = 0x445566;
    std::cout << "testing <<(osream, BWCNC::Color &) expect:#445566 :: " << c;
    c = BWCNC::Color( 255, 0, 0x55 );
    std::cout << " ... expect:#ff0055 :: " << c
              << " ... expect:#55ffaa :: " << (BWCNC::Color( 0x55, 0xff, 0xaa ))  << "\n";

    return 0;
}

