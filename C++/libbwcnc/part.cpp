
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



bool BWCNC::Part::reposition( const Eigen::Vector3d & pos, BWCNC::Part * newpart )
{
    bool doit = false;
    for( int i = 0; ! doit && i < 3; i++ )
        if( fabs(start[i] - pos[i]) > 1e-9 )
        {
            doit = true;
            break;
        }

    if( doit )
        this->translate( pos - start, newpart );
    else if( newpart )
        copy_into( *newpart );

    return doit;
}

void BWCNC::Part::translate( const Eigen::Vector3d & offst, BWCNC::Part * newpart )
{
    if( newpart )
    {
        copy_into( *newpart );
        newpart->translate( offst );
    }
    else
    {
        bbox.translate(offst);

        start  += offst;
        curpos += offst;

        for( auto cmd : cmds )
            if( cmd )
                cmd->translate( offst );
    }
}
 // linear transform

void BWCNC::Part::transform( const Eigen::Matrix3d & mat, BWCNC::Part * newpart, bool remake_bbox )
{
    if( newpart )
    {
        copy_into( *newpart );
        newpart->transform( mat, nullptr, remake_bbox );
    }
    else
    {
        Boundingbox newbbox;

        start  = mat * start;
        curpos = mat * curpos;

        for( auto cmd : cmds )
        {
            if( cmd )
            {
                cmd->transform( mat );
                if( remake_bbox )
                {
                    newbbox.update_bounds( cmd->begin );
                    newbbox.update_bounds( cmd->end );
                }
            }
        }

        if( remake_bbox )
            bbox = newbbox;
    }
}
void BWCNC::Part::scale( const double scalar, BWCNC::Part * newpart )
{
    Eigen::Matrix3d mat = scalar * Eigen::Matrix3d::Identity();

    bbox.scale(scalar);

    start  = mat * start;
    curpos = mat * curpos;

    transform( mat, newpart, false );
}


void BWCNC::Part::rotate_deg( double angle, BWCNC::Part * newpart /*= nullptr*/, int rotationaxis /*= 3*/ )
{
    rotate( angle * M_PI / 180.0, newpart, rotationaxis );
}
void BWCNC::Part::rotate( double radians, BWCNC::Part * newpart /*= nullptr*/, int rotationaxis /*= 3*/ )
{
    Eigen::Matrix3d mat;
    switch( rotationaxis )
    {
    case 1: mat << 1,0,0,  0,::cos(radians),-::sin(radians),  0,::sin(radians),::cos(radians);  break;
    case 2: mat << ::cos(radians),0,::sin(radians),  0,1,0,  -::sin(radians),::cos(radians),0;  break;
    case 3: mat << ::cos(radians),-::sin(radians),0,  ::sin(radians),::cos(radians),0,  0,0,1;  break;
    default: throw "invalid axis specified";   break;
    }

    transform( mat, newpart );
}

#if 0 // short and long names for  position_dependent_transform
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
#endif
    // short and long names for  position_dependent_transform

void BWCNC::Part::pos_dep_tform( pdt_t * tform, BWCNC::Part * newpart )
{
    if( newpart )
    {
        copy_into( *newpart );
        newpart->pos_dep_tform( tform, nullptr );
    }
    else
    {
        BWCNC::pos_dep_tform( tform, start );
        BWCNC::pos_dep_tform( tform, curpos );

        for( auto cmd : cmds )
            if( cmd )
                cmd->pos_dep_tform( tform );
#if 0
        bool is_ok = false;
        double a = area( is_ok );
        if( is_ok )
        {
            int z_cnt = 0;
            double avg_z = 0;
            for( auto cmd : cmds )
            {
                if( cmd )
                {
                    avg_z += cmd->begin[2];
                    z_cnt++;
                }
            }

            if( z_cnt )
                avg_z /= z_cnt;

            for( auto cmd : cmds )
                if( cmd )
                    cmd->end[2] = cmd->begin[2] = (a+100)/10;
        }
#endif
        // position-dependent transforms aren't linear, which is to say that they
        // can strech and deform the part so strangely that points which were
        // internal become external and thus become part of the new edge. because
        // of this, the bounding box must be remade entirely after every pdt.
        // use linear transforms whenever possible. they're much faster.
        remake_boundingbox();
    }
}

// see http://mathworld.wolfram.com/PolygonArea.html for info about this algorithm
double BWCNC::Part::area( bool & is_ok )
{
    if( ! isclosed )
    {
        is_ok = false;
        return 0;
    }

    is_ok = true;

    double det_sum = 0;
    for( auto cmd : cmds )
    {
        Eigen::Matrix2d A;
        A << cmd->begin[0], cmd->end[0],
             cmd->begin[1], cmd->end[1];

        det_sum += A.determinant();
    }

    return fabs(det_sum/2);
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

    if( workers && ! k.workers )
        k.workers = workers;

    for( auto p : partlist )
        if( p )
            k.partlist.push_back( p->new_copy() );
}

void BWCNC::PartContext::translate(  const Eigen::Vector3d & offset )
{
    firstpoint += offset;
    bbox.translate( offset );

    if( workers && workers->threads_running() )
    {
        ShareableWorkQueue sw_queue( partlist );
        ShareableWorkProcessor_translate sw_p( offset );
        workers->run_shareable_job( &sw_queue, &sw_p );
    }
    else
    {
        for( auto prt : partlist )
            if( prt )
                prt->translate( offset );
    }
}

void BWCNC::PartContext::transform( const Eigen::Matrix3d & mat, bool update_bbox )
{
    firstpoint = mat * firstpoint;

    if( workers && workers->threads_running() )
    {
        ShareableWorkQueue sw_queue( partlist );
        ShareableWorkProcessor_transform sw_p( mat );
        workers->run_shareable_job( &sw_queue, &sw_p );
    }
    else
    {
        for( auto prt : partlist )
            if( prt )
                prt->transform( mat, nullptr, update_bbox );
    }

    if( update_bbox )
        reunion_boundingbox();
}

#if 0
void BWCNC::PartContext::scale( const double scalar )
{
    firstpoint = scalar * firstpoint;
    bbox.scale( scalar );

    for( auto prt : partlist )
        if( prt )
            prt->scale( scalar );
}
#else
void BWCNC::PartContext::scale( const double scalar )
{
    bbox.scale( scalar );
    firstpoint = scalar * firstpoint;
    transform( scalar * Eigen::Matrix3d::Identity(), false );
}
#endif

void BWCNC::PartContext::rotate_deg( double angle, int rotationaxis /*= 3*/ )
{
    rotate( angle * M_PI / 180.0, rotationaxis );
}
void BWCNC::PartContext::rotate( double radians, int rotationaxis /*= 3*/ )
{
    Eigen::Matrix3d mat;
    switch( rotationaxis )
    {
    case 1: mat << 1,0,0,  0,::cos(radians),-::sin(radians),  0,::sin(radians),::cos(radians);  break;
    case 2: mat << ::cos(radians),0,::sin(radians),  0,1,0,  -::sin(radians),::cos(radians),0;  break;
    case 3: mat << ::cos(radians),-::sin(radians),0,  ::sin(radians),::cos(radians),0,  0,0,1;  break;
    default: throw "invalid axis specified";  break;
    }

    transform( mat );
}

#if 0
void BWCNC::PartContext::pos_dep_tform( mvf_t mvf, vvf_t vvf )
{
    BWCNC::pos_dep_tform( mvf, vvf, firstpoint );
    for( auto prt : partlist )
        if( prt )
            prt->pos_dep_tform( mvf, vvf );
    reunion_boundingbox();
}
#endif

void BWCNC::PartContext::pos_dep_tform( pdt_t * tform )
{
    BWCNC::pos_dep_tform( tform, firstpoint );

    if( workers && workers->threads_running() )
    {
        ShareableWorkQueue sw_queue( partlist );
        ShareableWorkProcessor_pdt_processor sw_p( tform );
        workers->run_shareable_job( &sw_queue, &sw_p );
    }
    else
    {
        for( auto prt : partlist )
            if( prt )
                prt->pos_dep_tform( tform );
    }

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

