
#include <algorithm>
#include "functions.h"
#include "part.h"

void BWCNC::Part::remake_boundingbox()
{
    Boundingbox newbbox( start, curpos );
    for( auto cmd : cmds )
    {
        if( cmd )
        {
            newbbox.update_bounds( cmd->begin );
            newbbox.update_bounds( cmd->end );
        }
    }
    bbox = newbbox;
}

void BWCNC::Part::update_bounds( const Eigen::Vector3d & newpoint )
{
    bbox.update_bounds( newpoint );
}

// update curpos
void BWCNC::Part::update_position( const Eigen::Vector3d & pos )
{
    curpos = pos;
    if( isnil )
        start = pos;
    isnil = false;
    update_bounds( pos );
}

void BWCNC::Part::lineto( const Eigen::Vector3d & to )
{
    cmds.push_back( new Line( curpos, to ) );
    update_position( to );
}

void BWCNC::Part::moveto( const Eigen::Vector3d & to )
{
    cmds.push_back( new Move( curpos, to ) );
    update_position( to );
}

void BWCNC::Part::reposition( const Eigen::Vector3d & pos )
{
    bool doit = false;
    for( int i = 0; ! doit && i < 3; i++ )
        doit = ( fabs(start[i] - pos[i]) > 1e-9 );

    if( doit )
        this->translate( pos - start );
}

void BWCNC::Part::translate(  const Eigen::Vector3d & offst )
{
    bbox.translate(offst);

    start  += offst;
    curpos += offst;

    for( auto cmd : cmds )
        if( cmd )
            cmd->translate( offst );
}
 // linear transform
void BWCNC::Part::transform(  const Eigen::Matrix3d & mat   )
{
    bbox.transform(  mat);

    start  = mat * start;
    curpos = mat * curpos;

    for( auto cmd : cmds )
        if( cmd )
            cmd->transform(  mat );
}
void BWCNC::Part::scale( const double scalar )
{
    Eigen::Matrix3d mat = scalar * Eigen::Matrix3d::Identity();

    bbox.scale(scalar);

    start  = mat * start;
    curpos = mat * curpos;

    transform( mat );
}

    // short and long names for  position_dependent_transform
void BWCNC::Part::pos_dep_tform( mvf_t mvf, vvf_t vvf )
{
    bbox.pos_dep_tform( mvf, vvf );

    BWCNC::pos_dep_tform( mvf, vvf, start );
    BWCNC::pos_dep_tform( mvf, vvf, curpos );

    for( auto cmd : cmds )
        if( cmd )
            cmd->pos_dep_tform( mvf, vvf );
}



/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


void BWCNC::PartContext::reposition( const Eigen::Vector3d & pos )
{
    bool doit = false;
    for( int i = 0; ! doit && i < 3; i++ )
        doit = ( fabs(bbox.min[i] - pos[i]) > 1e-9 );

    if( doit )
    {
        Eigen::Vector3d offset = (pos - bbox.min);

        bbox.translate( offset );

        for( auto prt : partlist )
            if( prt )
                prt->translate( offset );
    }
}

void BWCNC::PartContext::remake_boundingbox()
{
    if( partscnt > 0 )
    {
        bbox = Boundingbox();
        for( auto prt : partlist )
        {
            if( prt ) prt->remake_boundingbox();
            bbox.union_with( prt->bbox );
        }
    }
}

void BWCNC::PartContext::add_part( BWCNC::Part * newpart )
{
    if( partscnt != 0 )
    {
        // see the ruby version
        // if there's a position gap between the last part added and the new part being added then, at least
        // for g-code, there must be a command that bridges the gap. assume 'moveto' is that implicit command
        // and make it explicit by adding an additional one-command part before we follow through with the
        // requested Part addition.  XXX  This perhaps should be optional, so maybe add an option.

        if( partlist[partscnt-1]->curpos != newpart->start )
        {
            BWCNC::Part * linkingpart = new BWCNC::Part( partlist[partscnt-1]->curpos );
            linkingpart->moveto( newpart->start );
            partlist.push_back( linkingpart );
            partscnt++;
        }
    }

    partlist.push_back( newpart );
    partscnt++;
    bbox.union_with( newpart->bbox );
}

