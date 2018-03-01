
#include <algorithm>
#include "functions.h"
#include "part.h"

void BWCNC::Part::remake_boundingbox()
{
//  Boundingbox newbbox( start, curpos );
//  Boundingbox newbbox( start );
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
    p.dot_at_cnt = dot_at_cnt;
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


void BWCNC::Part::pull_commands_from( BWCNC::Part & p )
{
    pull_commands_from( &p );
}
void BWCNC::Part::pull_commands_from( BWCNC::Part * p )
{
    p->reposition( curpos );  //
    for( auto cmd : p->cmds )
    {
        cmds.push_back( cmd );
        update_position( cmd->end );
    }

    moveto_cnt += p->moveto_cnt;
    lineto_cnt += p->lineto_cnt;
    dot_at_cnt += p->dot_at_cnt;
}

void BWCNC::Part::update_bounds( const Eigen::Vector3d & newpoint )
{
    bbox.update_bounds( newpoint );
}

bool BWCNC::Part::update_starting_position( const Eigen::Vector3d & pos )
{
    if( lineto_cnt == 0 && moveto_cnt == 0 && dot_at_cnt == 0 )
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

void BWCNC::Part::lineto( const Eigen::Vector3d & to, bool vecfromcur /* = false */ ) // default accepts 'to' as relative to origin
{                                                                                     // vecfromcur adds 'to' to curpos
    Eigen::Vector3d vec = to;
    if( vecfromcur )
        vec += curpos;

    if( curpos == vec ) return;  // don't add null segments

    lineto_cnt++;
    isclosed = false;  // any added segment breaks 'closed' condition
                       // but see lineto_close() for clarification
    cmds.push_back( new Line( curpos, vec ) );
    update_position( vec );
}

void BWCNC::Part::dot_at( const Eigen::Vector3d & to, bool vecfromcur /* = false */ ) // default accepts 'to' as relative to origin
{                                                                                     // vecfromcur adds 'to' to curpos
    Eigen::Vector3d vec = to;
    if( vecfromcur )
        vec += curpos;

    dot_at_cnt++;
    cmds.push_back( new Dot( vec ) );
    update_position( vec );
}


void BWCNC::Part::lineto_close( bool & isok )
{
    isok = true;
    if( isclosed )        {               return; }  // already done
    if( curpos == start ) { isok = false; return; }
    if( moveto_cnt != 0 ) { isok = false; return; }
    if( lineto_cnt  < 2 ) { isok = false; return; }  // must be a triangle at minimum

    // otherwise, just to keep things simple, assume the user doesn't need protection from stupidity
    lineto( start );
    isclosed = true;
}


void BWCNC::Part::moveto( const Eigen::Vector3d & to, bool vecfromcur /* = false */ ) // default accepts 'to' as relative to origin
{                                                                                     // vecfromcur adds 'to' to curpos
    Eigen::Vector3d vec = to;
    if( vecfromcur )
        vec += curpos;

    if( curpos == vec ) return;  // don't add null segments

    moveto_cnt++;
    isclosed = false;  // any added movement breaks 'closed' condition
                       // see lineto_close() for clarification

    cmds.push_back( new Move( curpos, vec ) );
    update_position( vec );
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
void BWCNC::Part::transform( const Eigen::Matrix3d & mat, bool remake_bbox )
{
    Boundingbox newbbox;

    start  = mat * start;
    curpos = mat * curpos;

    for( auto cmd : cmds )
    {
        if( cmd )
        {
            cmd->transform(  mat );
            if( remake_bbox )
            {
                newbbox.update_bounds( cmd->begin );
                newbbox.update_bounds( cmd->end );
            }
        }
    }

    if( remake_bbox )
        bbox = newbbox;
    else
        bbox.transform( mat );
}
void BWCNC::Part::scale( const double scalar )
{
    Eigen::Matrix3d mat = scalar * Eigen::Matrix3d::Identity();

    bbox.scale(scalar);

    start  = mat * start;
    curpos = mat * curpos;

    transform( mat, false );
}

void BWCNC::Part::rotate( double angle, bool degrees /* = false */, int rotationaxis /* = 3 */ )
{
    Eigen::Matrix3d mat;
    double radians = angle;
    if( degrees )
        radians = angle * M_PI / 180.0;
    switch( rotationaxis )
    {
    case 1: mat << 1,0,0,  0,::cos(radians),-::sin(radians),  0,::sin(radians),::cos(radians);  break;
    case 2: mat << ::cos(radians),0,::sin(radians),  0,1,0,  -::sin(radians),::cos(radians),0;  break;
    case 3: mat << ::cos(radians),-::sin(radians),0,  ::sin(radians),::cos(radians),0,  0,0,1;  break;
    default: throw "invalid axis specified";   break;
    }

    transform( mat );
}

    // short and long names for  position_dependent_transform
void BWCNC::Part::pos_dep_tform( mvf_t mvf, vvf_t vvf )
{
    //bbox.pos_dep_tform( mvf, vvf );

    BWCNC::pos_dep_tform( mvf, vvf, start );
    BWCNC::pos_dep_tform( mvf, vvf, curpos );

    for( auto cmd : cmds )
        if( cmd )
            cmd->pos_dep_tform( mvf, vvf );

    // position-dependent transforms aren't linear, which is to say that they
    // can strech and deform the part so strangely that points which were
    // internal become external and thus become part of the new edge. because
    // of this, the bounding box must be remade entirely after every pdt
    // use linear transforms whenever possible. they're much faster.
    remake_boundingbox();
}

    // short and long names for  position_dependent_transform
void BWCNC::Part::pos_dep_tform( pdt_t * tform )
{
    //bbox.pos_dep_tform( tform );

    BWCNC::pos_dep_tform( tform, start );
    BWCNC::pos_dep_tform( tform, curpos );

    for( auto cmd : cmds )
        if( cmd )
            cmd->pos_dep_tform( tform );

    // position-dependent transforms aren't linear, which is to say that they
    // can strech and deform the part so strangely that points which were
    // internal become external and thus become part of the new edge. because
    // of this, the bounding box must be remade entirely after every pdt.
    // use linear transforms whenever possible. they're much faster.
    remake_boundingbox();
}



/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


void BWCNC::PartContext::copy_into( PartContext & k )
{
    k.partscnt   = partscnt;
    k.firstpoint = firstpoint;
    k.bbox       = bbox;
    k.isnil      = isnil;

    for( auto p : partlist )
    {
        if( p )
        {
            BWCNC::Part * newp = p->new_copy();
            k.partlist.push_back( newp );
            //printf( "just now pushed a part back into a new part-context\n" );
        }
    }
}

void BWCNC::PartContext::translate(  const Eigen::Vector3d & offset )
{
    firstpoint += offset;
    bbox.translate( offset );

    for( auto prt : partlist )
        if( prt )
            prt->translate( offset );
}

void BWCNC::PartContext::transform( const Eigen::Matrix3d & mat, bool update_bbox )
{
    firstpoint = mat * firstpoint;

    for( auto prt : partlist )
        if( prt )
            prt->transform( mat, update_bbox );

    if( update_bbox )
        reunion_boundingbox();
    else
        bbox.transform( mat );
}

void BWCNC::PartContext::scale( const double scalar )
{
    firstpoint = scalar * firstpoint;
    bbox.scale( scalar );

    for( auto prt : partlist )
        if( prt )
            prt->scale( scalar );
}


void BWCNC::PartContext::rotate( double angle, bool degrees /* = false */, int rotationaxis /* = 3 */ )
{
    Eigen::Matrix3d mat;
    double radians = angle;
    if( degrees )
        radians = angle * M_PI / 180.0;
    switch( rotationaxis )
    {
    case 1: mat << 1,0,0,  0,::cos(radians),-::sin(radians),  0,::sin(radians),::cos(radians);  break;
    case 2: mat << ::cos(radians),0,::sin(radians),  0,1,0,  -::sin(radians),::cos(radians),0;  break;
    case 3: mat << ::cos(radians),-::sin(radians),0,  ::sin(radians),::cos(radians),0,  0,0,1;  break;
    default: throw "invalid axis specified";  break;
    }

    transform( mat );
}

void BWCNC::PartContext::pos_dep_tform( mvf_t mvf, vvf_t vvf )
{
    BWCNC::pos_dep_tform( mvf, vvf, firstpoint );
    for( auto prt : partlist )
        if( prt )
            prt->pos_dep_tform( mvf, vvf );
    reunion_boundingbox();
}


void BWCNC::PartContext::pos_dep_tform( pdt_t * tform )
{
    BWCNC::pos_dep_tform( tform, firstpoint );
    for( auto prt : partlist )
        if( prt )
            prt->pos_dep_tform( tform );
    reunion_boundingbox();
}


void BWCNC::PartContext::reposition( const Eigen::Vector3d & pos, Eigen::Vector3d * offset_sum )
{
    bool doit = false;
    for( int i = 0; ! doit && i < 3; i++ )
        doit = ( fabs(bbox.min[i] - pos[i]) > 1e-9 );

    if( doit )
    {
        Eigen::Vector3d offset = (pos - bbox.min);
        if( offset_sum ) *offset_sum += offset;
        translate( offset );
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

// this is a little quicker than remake_boundingbox()
void BWCNC::PartContext::reunion_boundingbox()
{
    if( partscnt > 0 )
    {
        bbox = Boundingbox();
        for( auto prt : partlist )
            bbox.union_with( prt->bbox );
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


void BWCNC::PartContext::append_part( BWCNC::Part * newpart )
{
    newpart->reposition( last_coords() );
    add_part( newpart );
}


void BWCNC::PartContext::append_part_list( std::list<BWCNC::Part *> parts )
{
    for( auto p : parts ) append_part( p );
}

void BWCNC::PartContext::append_part_list( std::vector<BWCNC::Part *> parts )
{
    for( auto p : parts ) append_part( p );
}

void BWCNC::PartContext::refresh_z_ascending()
{
    // clear the list without destroying the elements
    partlist_z_ascending.erase( partlist_z_ascending.begin(), partlist_z_ascending.end() );
    for( auto p : partlist )
    {
        double avg_z = (p->get_bbox().max[2] + p->get_bbox().min[2])/2;
        partlist_z_ascending.insert( std::pair<double,BWCNC::Part *>(avg_z, p) );
    }
}

void BWCNC::shift2( BWCNC::PartContext & k, shift2_t x_st, shift2_t y_st, shift2_t z_st, Eigen::Vector3d * offset_sum )
{
    BWCNC::Boundingbox bbox = k.get_bbox();
    Eigen::Vector3d min = bbox.min;
    Eigen::Vector3d max = bbox.max;
    Eigen::Vector3d shiftv;

    shift2_t dirs[3] = {x_st, y_st, z_st};

    for( int i = 0; i < 3; i++ )
    {
        switch(dirs[i])
        {
        case to_center:   shiftv[i] = -fabs(max[0] - min[0])/2.0; break;
        case to_positive: shiftv[i] =  0;                         break;
        default: break;
        }
    }

    k.reposition( shiftv, offset_sum );
}

