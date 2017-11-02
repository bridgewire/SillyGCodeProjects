#ifndef BWCNC_COLOR_H__
#define BWCNC_COLOR_H__

#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

namespace BWCNC {

class Color
{
public:
    Color()                                  : isnil(true),  rgb(std::vector<uint8_t>(3)) { init( (uint32_t)0 ); }
    Color( uint32_t rgb24 )                  : isnil(false), rgb(std::vector<uint8_t>(3)) { init( rgb24 );   }
    Color( uint8_t r, uint8_t g, uint8_t b ) : isnil(false), rgb(std::vector<uint8_t>(3)) { init( r, g, b ); }
    Color( uint8_t RGB[3]  )                 : isnil(false), rgb(std::vector<uint8_t>(3)) { init( RGB ); }

    Color( const char * clrstr )             : isnil(false), rgb(std::vector<uint8_t>(3)) { init( clrstr ); }
    Color( const std::string &  clrstr )     : isnil(false), rgb(std::vector<uint8_t>(3)) { init( clrstr ); }

    virtual ~Color(){}

    uint32_t to_rgb24() const { uint32_t o = 0; for( int i = 0; i < 3; i++ ) o |= ((uint32_t)rgb[i]) << (16 - (i * 8)); return o; }
    std::ostream & to_ostream( std::ostream & s ) const { char b[8]; snprintf(b,8,"%06x", to_rgb24()); s << '#' << b; return s; } ;

    bool operator!() const { return isnil; }
    explicit operator bool() const { return ! isnil; }

protected:
    bool isnil;
    std::vector<uint8_t> rgb;

    void init( uint8_t RGB[3] )                  { for(int i = 0; i < 3; i++ ) rgb[i]=RGB[i]; }
    void init( uint8_t r, uint8_t g, uint8_t b ) { rgb[0]=r; rgb[1]=g; rgb[2]=b; }
    void init( uint32_t rgb24 ) { init( 0xff & (rgb24 >> 16), 0xff & (rgb24 >> 8), 0xff & rgb24 ); }
    void init( const char * s )
    {
        if( ! s )
            isnil = true;
        else if( s[0] != '#' || strlen(s) != 7 )
            throw "invalid error specification";
        else
            init( (uint32_t)strtoul( s+1, 0, 16 ) );
    }
    void init( const std::string & s ) { init( s.c_str() ); }

};

};

std::ostream & operator<<( std::ostream & s, const BWCNC::Color & clr );

#endif
