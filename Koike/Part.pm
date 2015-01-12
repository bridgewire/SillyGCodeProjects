#!/usr/bin/env perl

# Author:  Christiana Evelyn Johnson
# Copyright (c) 2014 Reno Bridgewire
# license: The MIT License (MIT)

use v5.10.0;
package Koike::Part;

use warnings;
use strict;

use Math::Trig;
use Clone 'clone';

BEGIN
{
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 0.01;
    @ISA         = qw(Koike);
    @EXPORT      = qw();    # eg: qw(&func1 &func2 &func4);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();

}
our @EXPORT_OK;

sub new {
    my $class = shift;
    my $self = {};

    my %args = @_;

    # this is the command list, which is the heart of a Part.
    # think of this most as a list of coordinates, but also "how to get there."
    $self->{clist} = [];
    $self->{clist_position} = -1; # for use in "get_next_command()"

    die "a Koike::Part needs an instances of a Koike object in koikeobj" if ! exists($args{koikeobj});
    $self->{k} = $args{koikeobj};
    my( $kx, $ky ) = $self->{k}->get_current_position();

#    $self->{startx} = exists($args{startx}) ? $args{startx} : $kx;  # our starting point.
#    $self->{starty} = exists($args{starty}) ? $args{starty} : $ky;
    if( exists($args{startx}) )
    {
        $self->{startx} = $args{startx};
        $self->{starty} = $args{starty};
        $self->{k}->print_debug( 2, sprintf('Koike::Part::new() got startx,starty from args -- %.03f,%.03f', $self->{startx}, $self->{starty} )  );
    }
    else
    {
        $self->{startx} = $kx;  # our starting point.
        $self->{starty} = $ky;
        $self->{k}->print_debug( 2, sprintf('Koike::Part::new() got startx,starty from k -- %.03f,%.03f', $self->{startx}, $self->{starty} )  );
    }

    $self->{x} = $self->{startx};                                   # is also current position
    $self->{y} = $self->{starty};
    $self->{k}->update_position( $self->{x}, $self->{y} );

    # bounding box.  XXX this doesn't correctly handle arcs,
    # whose extent will often pass beyond end points.
    $self->{xmin} = $self->{x};
    $self->{xmax} = $self->{x};
    $self->{ymin} = $self->{y};
    $self->{ymax} = $self->{y};

    # offsets and scaling
    $self->{mult}    = exists($args{multsclr})     ? $args{multsclr}      : 1;

    $self->{mark_dot_radius}  = .2; # how big around should the mark be?


    bless($self,$class); # bless me! and all who are like me. bless us everyone.
    return $self;
}

sub copy()
{
    my $self = shift;

    my $c = new Koike::Part( koikeobj => $self->{k} );

    $c->set_otherargs( 
        startx          => $self->{startx},
        starty          => $self->{starty},

        clist           => $self->{clist},

        clist_position  => $self->{clist_position},
        x => $self->{x},
        y => $self->{y},

        xmin => $self->{xmin},
        xmax => $self->{xmax},
        ymin => $self->{ymin},
        ymax => $self->{ymax},

        mult => $self->{mult},

        mark_dot_radius   => $self->{mark_dot_radius}
    );

    return $c;
}

# go through the whole path of $self and reverse the direction of travel.
# the last command becomes the first, the last set of coordinates becomes
# startx,starty, and the old startx,starty becomes the current position: x,y.
sub reverse_path()
{
    my $self = shift;

    $self->{k}->print_debug( 2, 'reverse_path() running' );


    my ( $new_startx, $new_starty ) = $self->end_coords();
    my ( $new_curx  , $new_cury   ) = $self->start_coords();
    my $new_clist = [];

    my ($x,$y);

    my $h;
    for( my $i = $#{$self->{clist}}; $i >= 0; $i-- )
    {
        $h = ${$self->{clist}}[$i];

        if( exists( $h->{toy} ) )
        {
            my ($oldtox, $oldtoy) = ( $h->{tox}, $h->{toy} );

            $h->{tox} = $h->{sox};
            $h->{toy} = $h->{soy};
            $h->{sox} = $oldtox;
            $h->{soy} = $oldtoy;
        }
        elsif( $h->{cmd} eq 'os' || $h->{cmd} eq 'oe' )
        {
            # start and end marks may need to look ahead (or 'back'...
            # decrementing $i) in the list to see where they should actually be

            if( $i <  $#{$self->{clist}} && $i > 0 )
            {
                # NOTE:  $prev_h is already altered but $next_h is not
                my $prev_h = ${$self->{clist}}[$i+1];
                my $next_h = ${$self->{clist}}[$i-1];

                if( $h->{cmd} eq 'os' && exists($prev_h->{toy}) )
                {
                    # os (martk-start) got it's coordinates from $i-1:tox,toy and gave them to $i+1:$sox,soy
                    # but should take them now from $i+1

                    if( $h->{sox} == $prev_h->{tox} && $h->{soy} == $prev_h->{toy} )
                    {
                        # if verified, execute the switch
                        $h->{sox} = $prev_h->{sox};
                        $h->{soy} = $prev_h->{soy};
                    }
                }
                elsif( $h->{cmd} eq 'oe' && exists($next_h->{toy}) )
                {
                    # start got it's coordinates from $i+1
                    # but should take them now from $i-1

                    # if verified, execute the switch
                    if( $h->{sox} == $prev_h->{sox} && $h->{soy} == $prev_h->{soy} )
                    {
                        $h->{sox} = $next_h->{tox};
                        $h->{soy} = $next_h->{toy};
                    }
                }
            }
            elsif ( $i == 0 )
            {
                my $prev_h = ${$self->{clist}}[1];
                if( $h->{sox} == $prev_h->{sox} && $h->{soy} == $prev_h->{soy} )
                {
                    $h->{sox} = $prev_h->{tox};
                    $h->{soy} = $prev_h->{toy};
                }
            }
            elsif ( $i == $#{$self->{clist}} )
            {
                # this is a mark that was at the end and should now be at the start
                my $next_h = ${$self->{clist}}[$i-1];
                if( $h->{sox} == $next_h->{tox} && $h->{soy} == $next_h->{toy} )
                {
                    $h->{sox} = $next_h->{sox};
                    $h->{soy} = $next_h->{soy};
                }
            }
        }

        # reversing the direction of travel means reversing direction of the angle too.
        # The center and long vs. short arc length don't change, so this completes the arc changes.
        if( exists( $h->{sweep} ) )
        {
            $h->{sweep} = ! $h->{sweep};
        }

        # hopefully it is helpful to reverse start and end marks
        if( $h->{cmd} eq 'os' || $h->{cmd} eq 'oe' )
        {
            $h->{cmd} = ( $h->{cmd} eq 'oe' ? 'os' : 'oe' );
            if   ( $h->{cmd} eq 'os' ) { $h->{clr} = $self->{k}->get_color('mark_start_color'); }
            elsif( $h->{cmd} eq 'oe' ) { $h->{clr} = $self->{k}->get_color('mark_end_color'); }
        }

        push( @{$new_clist}, $h );
    }

    ( $self->{startx}, $self->{starty} ) = ( $new_startx, $new_starty );
    $self->update_position( $new_curx, $new_cury, 0 );

    $self->{clist} = $new_clist;
}



# the intended syntax of merge:  '$part->merge( $otherpart )' suggests to me
# that if one or the other of these parts is 'dominant' then it would be $part
# since that's the one whose member function is being called. The mechanics of
# how this is actually done, however, due to ease of access to the clist, the
# actual implemenation likely appends $part to $otherpart, which has the 'feel'
# of $otherpart being dominant. so we export 'merge' as the implementor of this
# functionality, but in order to make merge operate more intuitively, we add
# 'reverse_merge' which is the actual implementation, and merge() simply calls
# reverse_merge after, itself, reversing the sense of the call.
sub reverse_merge()
{
    # begin with cloning otherpart
    #  'this' part cyles through its own command-list (the clist) and uses the otherpart's push_cmd
    #  method to append our commands to it's commands

    my $self = shift;
    my $root = shift;  # 'root' is the part that self gets merged into
    my $n;             # 'n' will be the new part, a combination of the two merging

    my ($r_xs, $r_ys, $r_xe, $r_ye) = ( $root->start_coords(), $root->end_coords() );
    my ($s_xs, $s_ys, $s_xe, $s_ye) = ( $self->start_coords(), $self->end_coords() );


    $self->{k}->print_debug( 2, sprintf('reverse_merge got: root(xs:%f, ys:%f, xe:%f, ye:%f) -- self(xs:%f, ys:%f, xe:%f, ye:%f) -- r_xe is%s equal to s_xe',

    $r_xs, $r_ys, $r_xe, $r_ye,
    $s_xs, $s_ys, $s_xe, $s_ye, 

    (($r_xe == $s_xe) ? "" : " not") )
   
    );


    # simplest case: if self starts where root ends, then append self to root
    if( abs($s_xs - $r_xe) < .00001 && abs($s_ys - $r_ye) < .00001 )
    {
        $self->{k}->print_debug( 2, 'reverse_merge handling simples case' );

        $n = $root->copy();
        for( my $i = 0; $i <= $#{$self->{clist}}; $i++ )
        {
            $n->push_cmd( ${$self->{clist} }[$i] );
        }
        $n->update_position( $self->{x}, $self->{y}, 0 );
        $n->auto_remake_bounding_box();
    }
    # next simplest.
    # else: if other start where self ends, then use recursion to reverse the roles
    #  XXX or...  if we want to preserve the roles, do we reverse both paths?
    #elsif( $r_xs == $s_xe && $r_ys == $s_ye )
    elsif( abs($r_xs - $s_xe) < .00001 && abs($r_ys - $s_ye) < .00001 )
    {
        $self->{k}->print_debug( 2, 'reverse_merge handling next simples case' );
        $n = $root->reverse_merge( $self );
    }
    # more complex
    # else:
    # we have already established that the start of neither is the end of the other
    # so if the two parts have any endpoints in common, either the starts are the
    # same or the ends are the same.  first test for common starts.
    #elsif( $r_xs == $s_xs && $r_ys == $s_ys )
    elsif( abs($r_xs - $s_xs) < .00001 && abs($r_ys - $s_ys) < .00001 )
    {
        # how to deal with this? in the paradigm used by this library (a part
        # is a single path with a clear start and clear end, with possible
        # self-intersection), we cannot start in the middle of a path.  We have
        # to start at one end or the other, thus one of the two parts must be
        # reversed. Since we want 'root' to provide the startx,starty to the
        # merged part, we reverse root so that its last coordinate becomes its
        # first, reversing commands too.  this is most trick with respect to 
        # arcs. we'll have to reverse the sweep. do this in a seperate function,
        # called 'reverse_path'

        $self->{k}->print_debug( 2, 'reverse_merge doing root->reverse_path' );
        $root->reverse_path();
        $n = $root->merge( $self );
    }
    # else:
    # the two parts meet at their respective ends.
    # reverse the path of 'self' (as opposed to reversing root)
    #elsif( $r_xe == $s_xe && $r_ye == $s_ye )
    elsif( abs($r_xe - $s_xe) < .00001 && abs($r_ye - $s_ye) < .00001 )
    {
        $self->{k}->print_debug( 2, 'reverse_merge doing self->reverse_path' );
        $self->reverse_path();
        # now we have ( $r_xe = $s_xs && $r_ye = $s_ys )
        ($s_xs, $s_ys, $s_xe, $s_ye) = ( $self->start_coords(), $self->end_coords() );
        #die "assertion failed" if !  ( $s_xs == $r_xe && $s_ys == $r_ye );
        die "assertion failed" if !  (abs($s_xs - $r_xe) < .00001 && abs($s_ys - $r_ye) < .00001);
        
        $n = $root->merge( $self );
    }
    # if none of the endpoints match up then we just do a 'moveto' from the
    # enpoint
    else
    {
        $self->{k}->print_debug( 2, 'reverse_merge doing the original algo.' );
        $root->dump_part('root');  # dump produces output at debug level 7.
        $self->dump_part('self');
        $self->{k}->print_debug( 2, sprintf('adding a moveto from: root(xe:%f, ye:%f) -- self(xs:%f, ys:%f)', $r_xe, $r_ye, $s_xs, $s_ys ) );

        $n = $root->copy();
        $n->moveto( $self->start_coords() );

        for( my $i = 0; $i <= $#{$self->{clist}}; $i++ )
        {
            $n->push_cmd( ${$self->{clist} }[$i] );
        }
        $n->update_position( $self->{x}, $self->{y}, 0 );
        $n->auto_remake_bounding_box();
    }

    return $n;
}

sub merge()
{
    my $self = shift;
    my $o = shift;  # 'o' is the 'otherpart'

    # to make the 'merge' syntax more intuitive (to me) internally we reverse the order...
    return $o->reverse_merge( $self );
}

sub set_otherargs()
{
    my $self = shift;

    my %args = @_;

    $self->{clist}          = clone( $args{clist} );
    $self->{clist_position} = $args{clist_position};

    $self->{startx} = $args{startx};
    $self->{starty} = $args{starty};

    $self->{x} = $args{x};
    $self->{y} = $args{y};

    $self->{xmin} = $args{xmin};
    $self->{xmax} = $args{xmax};
    $self->{ymin} = $args{ymin};
    $self->{ymax} = $args{ymax};

    $self->{mult}         = $args{mult};

    $self->{mark_dot_radius}  = $args{mark_dot_radius};
}

sub rewind_command_list()
{
    my $self = shift;
    $self->{clist_position} = -1;
}

sub next_command()
{
    my $self = shift;

    $self->{clist_position} += 1;
    if( exists( ${$self->{clist}}[ $self->{clist_position} ]) )
    {
        return ${$self->{clist}}[ $self->{clist_position} ];
    }
    $self->{clist_position} -= 1;
    return undef;
}

sub start_coords()
{
    my $self = shift;
    my ( $x, $y ) = ( $self->{startx}, $self->{starty} );

    my $h = $self->first_command();
    if( defined($h) && exists( $h->{sox} ) && ($h->{sox} != $x || $h->{soy} != $y) )
    {
        $self->{k}->print_debug( 2, sprintf('start_coords fouind that startx,starty:(%.03f,%.03f) != first_command(%s):sox,soy:(%.03f,%.03f). returning sox,soy;',
                    $x, $y, $h->{cmd}, $h->{sox}, $h->{soy}) );
        ( $x, $y ) = ($h->{sox}, $h->{soy});
    }

    return ( $x, $y );
}

sub end_coords()
{
    my $self = shift;
    my ( $x, $y ) = ( $self->{startx}, $self->{starty} );

    my $h = $self->last_command();
    if( defined($h) )
    {
        my $debug_coords_source = 'sox,soy';
        if( exists( $h->{toy} ) )
        {
            ( $x, $y ) = ($h->{tox}, $h->{toy});
            $debug_coords_source = 'tox,toy';
        }
        else
        {
            ( $x, $y ) = ($h->{sox}, $h->{soy});
        }

        $self->{k}->print_debug( 2, sprintf('end_coords pulled %s: %.02f,%.02f via cmd:%s', $debug_coords_source, $x, $y, $h->{cmd} ) );
    }

    return ( $x, $y );
}


sub first_command()
{
    my $self = shift;
    if( $#{$self->{clist}} > -1 ) { return ${$self->{clist}}[0]; }
    return undef;
}

sub last_command()
{
    my $self = shift;
    if( $#{$self->{clist}} > -1 ) { return ${$self->{clist}}[$#{$self->{clist}}]; }
    return undef;
}

# this is like translate() but it does an absolute shift instead of a relative shift
# $shiftfrom_x and $shiftfrom_y allow a different point in $self to represent the "handle" of $self
sub reposition()
{
    my $self = shift;
    my $shifto_x = shift;
    my $shifto_y = shift;
    my $shiftfrom_x = shift || $self->{startx};
    my $shiftfrom_y = shift || $self->{starty};

    #$self->{k}->print_debug( 2, sprintf('resposition doing translate() and translate()', -$shiftfrom_x, -$shiftfrom_y,  $shifto_x, $shifto_y ) );

    $self->translate( -$shiftfrom_x, -$shiftfrom_y );
    $self->translate( $shifto_x, $shifto_y );
}

sub translate()
{
    my $self = shift;
    my $xshift = shift;
    my $yshift = shift;

    my $id = 1000000*rand();

    if( ${$self->{clist}}[0]->{sox} != $self->{startx} ||  ${$self->{clist}}[0]->{soy} != $self->{starty} )
    {
        $self->{k}->print_debug( 1, sprintf('%d: start-point mismatch at start of translate: sox,soy:(%.03f,%.03f) != startx,starty:(%.03f,%.03f)', $id, 
                                            ${$self->{clist}}[0]->{sox}, ${$self->{clist}}[0]->{soy}, $self->{startx}, $self->{starty} ) );
    }

    # shift every coordinate
    for( my $i = 0; $i <= $#{$self->{clist}}; $i++ )
    {
        ${$self->{clist}}[$i]->{sox} += $xshift;
        ${$self->{clist}}[$i]->{soy} += $yshift;

        if( exists( ${$self->{clist}}[$i]->{toy} ) )
        {
            ${$self->{clist}}[$i]->{tox} += $xshift;
            ${$self->{clist}}[$i]->{toy} += $yshift;
        }

        if( ${$self->{clist}}[$i]->{cmd} eq 'a' )
        {
            ${$self->{clist}}[$i]->{cx} += $xshift;
            ${$self->{clist}}[$i]->{cy} += $yshift;
        }
    }

    # all meta-coordinates are aspects of this part too...
    $self->{x} += $xshift;
    $self->{y} += $yshift;

    $self->{k}->update_position( $self->{x}, $self->{y} );

    $self->{startx} += $xshift;
    $self->{starty} += $yshift;

    $self->auto_remake_bounding_box();

    if( ${$self->{clist}}[0]->{sox} != $self->{startx} ||  ${$self->{clist}}[0]->{soy} != $self->{starty} )
    {
        $self->{k}->print_debug( 1, sprintf('%d: start-point mismatch at end of translate: sox,soy:(%.03f,%.03f) != startx,starty:(%.03f,%.03f)', $id,
                                            ${$self->{clist}}[0]->{sox}, ${$self->{clist}}[0]->{soy}, $self->{startx}, $self->{starty} ) );
    }
}

sub scale()
{
    my $self = shift;
    my $scaler = shift;

    $self->linear_transform( [[$scaler,0],[0,$scaler]] );
}

sub matrix_mult()
{
    my $self  = shift;
    my $mat   = shift;
    my $vec = shift;
    my $dbg = shift || "";


    $self->{k}->print_debug( 10, 
          $dbg.
          '$$mat[0][0] == '.(exists($$mat[0][0]) && defined($$mat[0][0])?"exists:".$$mat[0][0]:"nonesuch").
          '$$vec[0] == '.(exists($$vec[0]) && defined($$vec[0])?"exists:".$$vec[0]:"nonesuch").
          '$$mat[0][1] == '.(exists($$mat[0][1]) && defined($$mat[0][1])?"exists:".$$mat[0][1]:"nonesuch").
          '$$vec[1] == '.(exists($$vec[1]) && defined($$vec[1])?"exists:".$$vec[1]:"nonesuch").
          '$$mat[1][0] == '.(exists($$mat[1][0]) && defined($$mat[1][0])?"exists:".$$mat[1][0]:"nonesuch").
          '$$vec[0] == '.(exists($$vec[0]) && defined($$vec[0])?"exists:".$$vec[0]:"nonesuch").
          '$$mat[1][1] == '.(exists($$mat[1][1]) && defined($$mat[1][1])?"exists:".$$mat[1][1]:"nonesuch").
          '$$vec[1] == '.(exists($$vec[1]) && defined($$vec[1])?"exists:".$$vec[1]:"nonesuch")
  );

    return ( 
            $$mat[0][0] * $$vec[0] + 
            $$mat[0][1] * $$vec[1], 

            $$mat[1][0] * $$vec[0] + 
            $$mat[1][1] * $$vec[1] );
}

sub linear_transform()
{
    my $self  = shift;
    my $mat   = shift;

    my ($x,$y);
    for( my $i = 0; $i <=  $#{$self->{clist}}; $i++ )
    {
        ( ${$self->{clist}}[$i]->{sox}, ${$self->{clist}}[$i]->{soy} ) = 
            $self->matrix_mult( $mat, [${$self->{clist}}[$i]->{sox}, ${$self->{clist}}[$i]->{soy}], "a - " );

        if( exists( ${$self->{clist}}[$i]->{toy} ) )
        {
            ( ${$self->{clist}}[$i]->{tox}, ${$self->{clist}}[$i]->{toy} ) = 
                $self->matrix_mult( $mat, [${$self->{clist}}[$i]->{tox}, ${$self->{clist}}[$i]->{toy}], "b - " );
        }

        if( ${$self->{clist}}[$i]->{cmd} eq 'a' )
        {
            (${$self->{clist}}[$i]->{cx}, ${$self->{clist}}[$i]->{cy} ) =
                $self->matrix_mult( $mat, [ ${$self->{clist}}[$i]->{cx}, ${$self->{clist}}[$i]->{cy}], "c - "  );
        }
    }

    # apply transform also to the start and current positions
    ($self->{startx}, $self->{starty}) = $self->matrix_mult( $mat, [$self->{startx}, $self->{starty}], "d - " );
    $self->update_position( $self->matrix_mult( $mat, [$self->{x},      $self->{y}], "f - " ), 0 ); # ($self->{x},      $self->{y})      = 

    $self->auto_remake_bounding_box();
}

sub nonlinear_transform_helper()
{
    my $self = shift;
    my $mat  = shift;
    my $vec  = shift;
    my $x    = shift;
    my $y    = shift;
    my $dbgl = shift;

    my $xx = $$mat[0][0];
    my $xy = $$mat[0][1];
    my $yx = $$mat[1][0];
    my $yy = $$mat[1][1];
    my $vx = $$vec[0];
    my $vy = $$vec[1];

    my $m = [ [&$xx( $x, $y ), &$xy( $x, $y )], [&$yx( $x, $y ), &$yy( $x, $y )]];
    my ($newx, $newy) = $self->matrix_mult( $m, [ $x, $y ], $dbgl );
    my ($xoffset, $yoffset) = ( &$vx( $x, $y ), &$vy( $x, $y ) );
    return ($newx + $xoffset, $newy + $yoffset);
}

sub make_numeric_coderef($)
{
    my $ref = shift;

    if( ref $ref ne "CODE" )
    {
        if( ref $ref eq "" )
        {
            if( $ref  =~ /^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/ )
            {
                return sub { return $ref; };
            }
            else
            {
                return sub { return 0; };
            }
        }
        elsif( ref $ref eq "SCALAR" )
        {
            my $v = $$ref;
            if( $v =~ /^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/ )
            {
                return sub { return $v; };
            }
            else
            {
                return sub { return 0; };
            }
        }
        else
        {
            return sub { return 0; };
        }
    }

    return $ref;
}



sub nonlinear_transform()
{
    my $self  = shift;
    my $mat   = shift;
    my $vec   = shift;

    if( ref $mat ne 'ARRAY' ) { $mat = [[sub{1;},sub{0;}],[sub{0;},sub{1;}]]; } # default transform matrix is identity
    $$mat[0][0] = &make_numeric_coderef( $$mat[0][0] );
    $$mat[1][0] = &make_numeric_coderef( $$mat[1][0] );
    $$mat[0][1] = &make_numeric_coderef( $$mat[0][1] );
    $$mat[1][1] = &make_numeric_coderef( $$mat[1][1] );

    if( ref $vec ne 'ARRAY' ) { $vec = [sub{0;}, sub{0;}]; }                    # default offset vector is zero
    $$vec[0] = &make_numeric_coderef( $$vec[0] );
    $$vec[1] = &make_numeric_coderef( $$vec[1] );

    my ($x,$y,$m);
    for( my $i = 0; $i <=  $#{$self->{clist}}; $i++ )
    {
        ( ${$self->{clist}}[$i]->{sox}, ${$self->{clist}}[$i]->{soy} ) =
            $self->nonlinear_transform_helper( $mat, $vec, ${$self->{clist}}[$i]->{sox}, ${$self->{clist}}[$i]->{soy}, "a - " );

        if( exists( ${$self->{clist}}[$i]->{toy} ) )
        {
            ( ${$self->{clist}}[$i]->{tox}, ${$self->{clist}}[$i]->{toy} ) =
                $self->nonlinear_transform_helper( $mat, $vec, ${$self->{clist}}[$i]->{tox}, ${$self->{clist}}[$i]->{toy}, "b - " );
        }

        #  XXX this is inadequate.
        if( ${$self->{clist}}[$i]->{cmd} eq 'a' )
        {
            ( ${$self->{clist}}[$i]->{cx}, ${$self->{clist}}[$i]->{cy} ) =
                $self->nonlinear_transform_helper( $mat, $vec, ${$self->{clist}}[$i]->{cx}, ${$self->{clist}}[$i]->{cy}, "c - " );
        }
    }

    # apply transform also to the start and current positions
    ($self->{startx}, $self->{starty}) = $self->nonlinear_transform_helper( $mat, $vec, $self->{startx}, $self->{starty}, "d - " );
    $self->update_position( $self->nonlinear_transform_helper( $mat, $vec, $self->{x}, $self->{y}, "f - " ), 0 );

    $self->auto_remake_bounding_box();
}



sub rotate()
{
    my $self  = shift;
    my $angle = shift;
    my $units = shift || 'radians';  # what's the angle unit?  default is radians.

    $self->{k}->print_debug( 9, "rotate() received -- angle:$angle , units:$units" );

    if( $units =~ /degree/ ) { $angle = deg2rad($angle); }

    $self->linear_transform( [[cos($angle),-sin($angle)],[sin($angle),cos($angle)]] );
}


sub set_colors()
{
    my $self = shift;
    $self->{k}->set_colors( @_ );
}

sub set_marker_parameters()
{
    my $self = shift;
    my %args = @_;
    if( exists($args{mark_start_color}) ) { $self->{k}->set_colors(mark_start_color=>$args{mark_start_color}); }
    if( exists($args{mark_end_color}) )   { $self->{k}->set_colors(  mark_end_color=>$args{mark_end_color}); }
    $self->{mark_dot_radius}  = exists($args{mark_dot_radius})  ?  $args{mark_dot_radius}  : $self->{mark_dot_radius};
}


sub update_bounds()
{
    my $self = shift;
    my $newx = shift;
    my $newy = shift;

    if( ! exists($self->{xmin}) || ! defined($self->{xmin}) ) 
    {
        $self->{xmin} = $self->{xmax} = $newx;
        $self->{ymin} = $self->{ymax} = $newy;
    }
    else
    {
        $self->{xmin} = ( $self->{xmin} < $newx ? $self->{xmin} : $newx );
        $self->{xmax} = ( $self->{xmax} > $newx ? $self->{xmax} : $newx );
        $self->{ymin} = ( $self->{ymin} < $newy ? $self->{ymin} : $newy );
        $self->{ymax} = ( $self->{ymax} > $newy ? $self->{ymax} : $newy );
    }
}

sub get_bounding_box()
{
    my $self = shift;
    return ( 'xmin'=>$self->{xmin}, 'ymin'=>$self->{ymin}, 'xmax'=>$self->{xmax}, 'ymax'=>$self->{ymax} );
}
sub set_bounding_box()
{
    my $self = shift;
    my %args = @_;
    $self->{xmin} = exists($args{xmin}) ? $args{xmin} : $self->{xmin};
    $self->{xmax} = exists($args{xmax}) ? $args{xmax} : $self->{xmax};
    $self->{ymin} = exists($args{ymin}) ? $args{ymin} : $self->{ymin};
    $self->{ymax} = exists($args{ymax}) ? $args{ymax} : $self->{ymax};
}
sub auto_remake_bounding_box()
{
    my $self = shift;

    $self->{xmin} = $self->{xmax} = $self->{startx};
    $self->{ymin} = $self->{ymax} = $self->{starty};

    my ($x,$y);
    for( my $i = 0; $i <=  $#{$self->{clist}}; $i++ )
    {
        $self->update_bounds( ${$self->{clist}}[$i]->{sox}, ${$self->{clist}}[$i]->{soy} );
        if( exists( ${$self->{clist}}[$i]->{toy} ) )
        {
            $self->update_bounds( ${$self->{clist}}[$i]->{tox}, ${$self->{clist}}[$i]->{toy} );
        }
    }
}

sub update_position()
{
    my $self = shift;
    my $newx = shift;
    my $newy = shift;
    my $update_startxy = shift;
    $update_startxy = 1 if ( ! defined( $update_startxy ) );

    if( defined($self->{x}) && $self->{x} == $newx && $self->{y} == $newy )
    {
        # did the position change?
        return 0; # no it did not. return false.
    }

    $self->update_bounds( $newx, $newy );

    # if we haven't started yet, then update_position() changes the start position
    if( $update_startxy && $#{$self->{clist}} < 0 )
    {
        $self->{startx} = $newx;
        $self->{starty} = $newy;
    }

    $self->{x} = $newx;
    $self->{y} = $newy;

    $self->{k}->update_position( $self->{x}, $self->{y} );

    return 1; # return true
}

sub get_current_position()
{
    my $self = shift;
    return ( $self->{x}, $self->{y} );
}


sub moveto()
{
    my $self = shift;
    my $newx = shift;
    my $newy = shift;

    my $sox = $self->{x};
    my $soy = $self->{y};
    
    if( $self->update_position( $newx, $newy, 0 ) )
    {
        $self->push_cmd( {
                cmd=>'m',   clr=>$self->{k}->get_color('moveto_color'),
                sox=>$sox,  soy=>$soy,
                tox=>$newx, toy=>$newy } );
    }
}


sub do_mark()
{
    my $self = shift;
    my $cmd  = shift;
    my $clr  = shift;

    $self->push_cmd( { cmd=>$cmd,  clr=>$clr, sox => $self->{x}, soy => $self->{y}, r => $self->{mark_dot_radius} } );
}

sub mark_start() { my $self = shift; my $clr  = shift || $self->{k}->get_color('mark_start_color'); $self->do_mark('os',$clr); }
sub mark_end()   { my $self = shift; my $clr  = shift || $self->{k}->get_color('mark_end_color');   $self->do_mark('oe',$clr); }


sub lineto()
{
    my $self = shift;
    my $newx = shift;
    my $newy = shift;

    my $sox = $self->{x};
    my $soy = $self->{y};

    if( $self->update_position( $newx, $newy, 0 ) )
    {
        $self->push_cmd( {
                cmd=>'l',   clr=>$self->{k}->get_color('cut_color'), 
                sox=>$sox,  soy=>$soy,
                tox=>$newx, toy=>$newy } );
    }
}

# svg can do ellipses, but gcode cannot. for now, allow only circular arcs.
sub arcto()
{
    my $self = shift;
    my %args = @_;

    my $tox = $args{newx};
    my $toy = $args{newy};

    my $rdus = undef;
    if( exists($args{radius}) ) { $rdus = $args{radius}; }

    my $sweep = undef;
    if( exists($args{sweep})     ) { $sweep = $args{sweep}; } # ifdef sweep, ignore clockwise

    my $largearc = undef;
    if( exists($args{largearc}) ) { $largearc = $args{largearc}; }

    my $cx = undef;
    my $cy = undef;

    my $radius_tolerance = .00000001;  # arbitrary tiny number. this maybe needs to come from the machine spec.

    my $sox = $self->{x};
    my $soy = $self->{y};

    if( exists($args{cx}) && exists($args{cy}) && ( defined($largearc) || defined($sweep) ) )
    {
        # combined with the starting point, ending point, and sweep,
        # we can calculate the radius and largearc-flag

        $cx = $args{cx};
        $cy = $args{cy};

        # to find the largearc-flag, do a cross-product.
        # here's the algo.:
        # u = s - c    # u is the vector from the center to start (sox, soy, 0)
        # v = t - c    # v is the vector from the center to end   (tox, toy, 0)
        # let (u x v)k be the third (the kth) coodinate of (u x v)
        #
        # if( (u x v)k > 0 &&   $sweep ) largearc-flag = 0;
        # if( (u x v)k > 0 && ! $sweep ) largearc-flag = 1;
        # if( (u x v)k < 0 &&   $sweep ) largearc-flag = 1;
        # if( (u x v)k < 0 && ! $sweep ) largearc-flag = 0;

        my $ux = $sox - $cx;
        my $uy = $soy - $cy;
        my $vx = $tox - $cx;
        my $vy = $toy - $cy;
        my $cross = $ux*$vy - $uy*$vx;

        if( defined($sweep) && ! defined($largearc) )
        {
            if(    $cross  < 0 ){ $largearc =   $sweep; }
            elsif( $cross  > 0 ){ $largearc = ! $sweep; }
            else                { $largearc = 1; }
        }
        elsif( ! defined($sweep) && defined($largearc) )
        {
            if(    $cross  < 0 ){ $sweep =   $largearc; }
            elsif( $cross  > 0 ){ $sweep = ! $largearc; }
            else                { die "sweep is ambiguous with 180-degree arc. sweep must be defined"; }
        }
        else
        {
            # verify that the everything is in agreement
            if( $cross  < 0 && $sweep != $largearc ){ die "sweep disagrees with largearc given start,stop,center"; }
            if( $cross  > 0 && $sweep == $largearc ){ die "sweep disagrees with largearc given start,stop,center"; }
        }

        # assert( at this point, largearc and sweep are defined and agree )

        my $r1 = sqrt( $ux**2 + $uy**2 );
        my $r2 = sqrt( $vx**2 + $vy**2 );

        $self->{k}->print_debug( 5, sprintf('sox,soy == %.02f,%.02f; tox,toy == %.02f,%.02f; cx,cy == %.02f,%.02f; '.
                                            'ux,uy == %.02f,%.02f; vx,vy == %.02f,%.02f; cross = %.02f; r1,r1 == %.02f,%.02f;',
                                            $sox, $soy, $tox, $toy, $cx, $cy, $ux, $uy, $vx, $vy, $cross, $r1, $r2 ) );

        if( abs($r1 - $r2) > $radius_tolerance )
        {
            die "the calculated radii for a circular arc must be reasonably equal. r1:$r1 != r2:$r2  diff: ".abs($r1 - $r2);
        }

        if( defined($rdus) )
        {
            if( abs( $rdus - $r2 ) > $radius_tolerance )
            {
                die "the specified radius differs from the calculated radii by too much. ".
                    "given:$rdus != calc'd:$r2. diff: ".abs( $rdus - $r2 );
            }
            # rdus is within tolerance.  leave it alone.
        }
        else
        {
            $rdus = ($r1 == $r2 ? $r1 : .5*($r1 + $r2));
        }
    }

    # if the center point is not defined then we need a radius, sweep, *and* largearc flag to find the center.
    # However, g-code interpreters seem to allow for a radius and sweep defined without also specifying which
    # arc to use. I will allow this, and assume, as interpreters seem to as a convention, to use largearc=0
    # when it's not specified.
#    elsif( defined($rdus) && defined($sweep) && defined($largearc) )
    elsif( defined($rdus) && defined($sweep) )
    {
        if( ! defined($largearc) ) { $largearc = 0; }

        # make a vector named U
        my $ux = $tox - $sox;
        my $uy = $toy - $soy;
        my $u_norm = sqrt( $ux**2 + $uy**2 );   # this is: ||U|| in description below

        if( $rdus < .5*$u_norm && abs($rdus - .5*$u_norm) > $radius_tolerance )
        {
            die "radius ($rdus) between specified points (s: $sox,$soy) -> (t: $tox,$toy) is too short (||t-s||:$u_norm)";
        }
        elsif( abs($rdus - .5*$u_norm) > $radius_tolerance )
        {
            # the radius is long enough that the center of the circle is not on the line
            # between the two points (we're rotating trough a non-180 degree angle.)

            ##################################  SVG
            # First, we're dealing with a left-hand coord. system.  This means the
            # cross product, at least for vectors in the x,y plane, has this
            # definition: U x V == i.0 + j.0 + k.(v1*u2 - v2*u1).  Thus if
            # U=(u1,u2) then defining V=(u2,-u1) and taking the above cross product
            # produces a vector in the (left-handed) positive z direction.
            # 
            # so, let...
            # T=(tox,toy)
            # S=(sox,soy)
            # U=T-S = (u1,u2)
            #  * note: U is a cord between S and T on the circle we're trying to define with center C and radius rdus
            #  * let V be a vector between the midpoint of U and the center of the circle C. notice that U and V are
            #  * at right angles from each other and that:  rdus**2 = (1/2 * ||U||)^2 + ||V||^2
            #  * therefore: ||V|| == sqrt( rdus**2 - (.5 * ||U||)^2 ).  This allows us to define V in terms of U and rdus
            # V=  ((u2,-u1)/||U||) * sqrt( rdus^2 - (1/4)*||U||^2)
            # remember, by this definition of V, that since this is a left-hand system that UxV is positive.
            # It follows that the center, C, of the circle is located either at:
            # S + (1/2)*U + V   ... or:
            # S + (1/2)*U - V
            # 
            # The sign given to V in the above sum is given by $sweep and $largearc
            # and can be denoted thus:
            # C == S + (1/2)*U + ( $sweep == $largearc ? -1 : 1 )*V

            ##################################  GCODE
            # For the Koike, we have an ordinary right handed coordinate
            # system.  the math is almost entirely the same but, for one, the
            # cross product and V must be defined differently.  The cross
            # product is defined differently (normally) because "counter
            # clockwise" is the usual positive angle direction, so 'sweep'...
            # may have a different meaning.

            # make U; made above
            #my $ux = $tox - $sox;
            #my $uy = $toy - $soy;

            # make V
            # my $u_norm = sqrt( $ux**2 + $uy**2 );   # this is: ||U||
            # my $vscaler = sqrt( ($rdus**2) - .25 *($u_norm**2) ) / $u_norm;
            my $vscaler = sqrt( ($rdus**2)/($ux**2 + $uy**2) - .25 );          # algebraicly equivalent
            my $vx =  $uy * $vscaler ;
            my $vy = -$ux * $vscaler ;

            # if gcode is in use we're using the ordinary right-handed coordinate system. adjust.
            if( $self->{k}->get_protocol() eq 'gcode' )
            {
                $vx = -$uy * $vscaler ;
                $vy =  $ux * $vscaler ;
            }

            # make C (the center vector)
            ($cx, $cy) = ( $sox + $ux/2.0, $soy + $uy/2.0 ) ;
            if( $sweep == $largearc )
            {
                $cx -= $vx;
                $cy -= $vy;
            }
            else
            {
                $cx += $vx;
                $cy += $vy;
            }
        }
        else
        {
            # This yields the point half way between T and S. see explanation above.
            ($cx, $cy) = ( $sox + $ux/2.0, $soy + $uy/2.0 ) ;
        }
    }
    else
    {
        die "(the arc center and (largearc or sweep) must be defined) or\n(the radius sweep and largearc must all be defined)\n";
    }


    # for complete circles the next position will equal the start, so add command
    # even if the current position didn't change.
    $self->push_cmd( {
            cmd=>'a',        clr=>(exists($args{clr}) ? $args{clr} : $self->{k}->get_color('cut_color') ), 
            sox=>$sox,       soy=>$soy,
            tox=>$tox,       toy=>$toy,
            sweep=>$sweep,   largearc=>$largearc,
            cx=>$cx,         cy=>$cy,
            radius=>$rdus } );

    $self->{k}->print_debug( 5,
        sprintf( 'arc debug -- sox=>%.03f, soy=>%.03f, tox=>%.03f, toy=>%.03f, sweep=>%d, largearc=>%d, cx=>%.03f, cy=>%.03f, radius=>%.03f',

            $sox,       $soy,
            $tox,       $toy,
            $sweep,     $largearc,
            $cx,        $cy,
            $rdus * $self->{mult} ));

    $self->update_position( $tox, $toy, 0 );
}

sub push_cmd
{
    my $self = shift;
    my $args = shift;

    my $href = {};

    if( exists($args->{sox})      ){ $href->{sox}      = $args->{sox}; }
    if( exists($args->{soy})      ){ $href->{soy}      = $args->{soy}; }
    if( exists($args->{tox})      ){ $href->{tox}      = $args->{tox}; }
    if( exists($args->{toy})      ){ $href->{toy}      = $args->{toy}; }
    if( exists($args->{r})        ){ $href->{r}        = $args->{r}; }
    if( exists($args->{cmd})      ){ $href->{cmd}      = $args->{cmd}; }
    if( exists($args->{clr})      ){ $href->{clr}      = $args->{clr}; }
    if( exists($args->{sweep})    ){ $href->{sweep}    = $args->{sweep}; }
    if( exists($args->{largearc}) ){ $href->{largearc} = $args->{largearc}; }
    if( exists($args->{cx})       ){ $href->{cx}       = $args->{cx}; }
    if( exists($args->{cy})       ){ $href->{cy}       = $args->{cy}; }
    if( exists($args->{radius})   ){ $href->{radius}   = $args->{radius}; }

    push(@{$self->{clist}}, $href) if(  keys %{$href} >= 1 ) ;
}

sub dump_part()
{
    my $self = shift;
    my $lbl = shift;

    if( $self->{k}->debug_level_sufficient( 7 ) )
    {
        for( my $i = 0; $i <= $#{$self->{clist}}; $i++ )
        {
            my $dbgs = '';
            my $cmd = ${$self->{clist}}[$i];
            foreach my $c ( 'cmd', 'sox', 'soy', 'tox', 'toy', 'r', 'sweep', 'largearc', 'cx', 'cy', 'radius', 'clr')
            {
                if( exists( ${$cmd}{$c} ) )
                {
                    $dbgs .= "$c: ".$self->mnum(${$cmd}{$c}, 3)."; ";
                }
            }
            $self->{k}->print_debug( 7, "$lbl: $dbgs" );
        }
    }
}

# massage_numeric
sub mnum()
{
    my $self = shift;
    my $arg  = shift;
    my $precision = shift;
    if( $arg =~ /^[-+.0-9e]+$/ )
    {
        if( abs($arg) != int(abs($arg)) )              # integers are clean as they are
        {
            my $fmt = '%f';
            if( defined($precision) )
            {
                $fmt = '%.0'.$precision.'f';
            }
            $arg = sprintf( $fmt, $arg );
        }
        $arg =~ s/^-([0.]+)$/$1/;                      # we don't like negative zero
    }
    return $arg;
}



# /* vim: set ai et tabstop=4  shiftwidth=4: */
1;
