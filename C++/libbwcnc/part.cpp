
#include <algorithm>
#include "functions.h"
#include "part.h"

void BWCNC::Part::remake_boundingbox()
{
//  Boundingbox newbbox( start, curpos );
    Boundingbox newbbox;
    for( auto cmd : cmds )
    {
        if( cmd )
        {
            newbbox.update_bounds( cmd->begin );
            newbbox.update_bounds( cmd->end );
        }
    }
    bbox = newbbox;

#ifdef DEBUG
    std::cerr << "updated bounding box: " << bbox << "\n";
    std::cerr << "start:\n" << start << "\n";
    std::cerr << "curpos:\n" << curpos << "\n\n";
#endif
}

void BWCNC::Part::copy_into( Part & p )
{
    p.isnil = isnil;
    p.isclosed = isclosed;
    p.moveto_cnt = moveto_cnt;
    p.lineto_cnt = lineto_cnt;
    p.bbox       = bbox;
    p.start      = start;
    p.curpos     = curpos;

    for( auto c : cmds )
    {
        if( c )
        {
            BWCNC::Command * newc = c->new_copy();
            p.cmds.push_back( newc );
            //printf( "just now pushed a command back into a new part\n" );
        }
        else
            p.cmds.push_back( nullptr );
    }
}


void BWCNC::Part::update_bounds( const Eigen::Vector3d & newpoint )
{
    bbox.update_bounds( newpoint );
}

bool BWCNC::Part::update_starting_position( const Eigen::Vector3d & pos )
{
    if( lineto_cnt == 0 && moveto_cnt == 0 )
    {
        update_position( pos );
        return true;
    }

    return reposition( pos );
}

// update curpos
void BWCNC::Part::update_position( const Eigen::Vector3d & pos )
{
    curpos = pos;
    if( isnil )
    {
        start = pos;
        bbox = Boundingbox( start, pos );
        isnil = false;
    }
    update_bounds( pos );
}

void BWCNC::Part::lineto( const Eigen::Vector3d & to )
{
    if( curpos == to ) return;  // don't add null segments

    lineto_cnt++;
    isclosed = false;  // any added segment breaks 'closed' condition
                       // but see lineto_close() for clarification
    cmds.push_back( new Line( curpos, to ) );
    update_position( to );
}

void BWCNC::Part::lineto_close( bool & isok )
{
    isok = true;
    if( isclosed )        {               return; }  // already done
    if( curpos != start ) { isok = false; return; }
    if( moveto_cnt != 0 ) { isok = false; return; }
    if( lineto_cnt  < 2 ) { isok = false; return; }  // must be a triangle at minimum

    // otherwise, just to keep things simple, assume the user doesn't need protection from stupidity
    lineto( start );
    isclosed = true;
}


void BWCNC::Part::moveto( const Eigen::Vector3d & to )
{
    if( curpos == to ) return;  // don't add null segments

    moveto_cnt++;
    isclosed = false;  // any added movement breaks 'closed' condition
                       // see lineto_close() for clarification

    cmds.push_back( new Move( curpos, to ) );
    update_position( to );
}



bool BWCNC::Part::reposition( const Eigen::Vector3d & pos )
{
    bool doit = false;
    for( int i = 0; ! doit && i < 3; i++ )
        if( fabs(start[i] - pos[i]) > 1e-9 )
        {
            doit = true;
            break;
        }

    if( doit )
        this->translate( pos - start );

    return doit;
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

    // short and long names for  position_dependent_transform
void BWCNC::Part::pos_dep_tform( pdt_t * tform )
{
    bbox.pos_dep_tform( tform );

    BWCNC::pos_dep_tform( tform, start );
    BWCNC::pos_dep_tform( tform, curpos );

    for( auto cmd : cmds )
        if( cmd )
            cmd->pos_dep_tform( tform );
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

