#ifndef BWCNC_NUMSTRING_H__
#define BWCNC_NUMSTRING_H__

#include <string>

namespace BWCNC {

class NumString
{
public:
    NumString() : value(0) {}
    NumString( double val ) : value(val) {}
    NumString( int    val ) : value(val) {}

    NumString & operator=( int       val ){ value = val; return *this; }
    NumString & operator=( double    val ){ value = val; return *this; }
    NumString & operator=( std::string s ){ value = std::stod(s); return *this; }

    virtual ~NumString(){}

    std::string str();

    static bool terse;
    static uint8_t precision;

protected:
    double value;

};

};

#endif
