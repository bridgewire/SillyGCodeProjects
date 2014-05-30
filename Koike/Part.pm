#!/usr/bin/env perl

use v5.10.0;
package Koike::Part;

use warnings;
use strict;

#use Koike;
use Math::Trig;

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

    $self->{startx} = exists($args{startx}) ? $args{startx} : $kx;  # our starting point.
    $self->{starty} = exists($args{starty}) ? $args{starty} : $ky;
    $self->{x} = $self->{startx};                                  # is also current position
    $self->{y} = $self->{starty};

#    $self->{p} = exists($args{protocol}) ? $args{protocol} : 'gcode'; # protocol: svg or gcode
#    $self->{x} = undef;
#    $self->{y} = undef;


    # bounding box.  XXX this doesn't correctly handle arcs,
    # whose extent will often pass beyond end points.
    $self->{xmin} = $self->{x};
    $self->{xmax} = $self->{x};
    $self->{ymin} = $self->{y};
    $self->{ymax} = $self->{y};

    # the part may be composed of cuts and non-cuts
    $self->{moveto_color} = exists($args{moveto_color}) ? $args{moveto_color} : $self->{k}->get_color('moveto_color') ;
    $self->{cut_color}    = exists($args{cut_color})    ? $args{cut_color}    : $self->{k}->get_color('cut_color') ;


    # offsets and scaling
    $self->{mult}    = exists($args{multsclr})     ? $args{multsclr}      : 1;

    # 
    $self->{mark_start_color} = 'rgb(0,200,0)';  # green for go
    $self->{mark_end_color}   = 'rgb(230,0,0)';  # red for stop
    $self->{mark_dot_radius}  = 3;               # how big around?


    bless($self,$class); # bless me! and all who are like me. bless us everyone.
    return $self;
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

sub translate()
{
    my $self = shift;
    my $xshift = shift;
    my $yshift = shift;

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
    return ( $$mat[0][0] * $$vec[0] + $$mat[0][1] * $$vec[1], $$mat[1][0] * $$vec[0] + $$mat[1][1] * $$vec[1] );
}

sub linear_transform()
{
    my $self  = shift;
    my $mat   = shift;

    my ($x,$y);
    for( my $i = 0; $i <=  $#{$self->{clist}}; $i++ )
    {
        ( ${$self->{clist}}[$i]->{sox}, ${$self->{clist}}[$i]->{soy} ) = 
            $self->matrix_mult( $mat, [${$self->{clist}}[$i]->{sox}, ${$self->{clist}}[$i]->{soy}] );

        if( exists( ${$self->{clist}}[$i]->{toy} ) )
        {
            ( ${$self->{clist}}[$i]->{tox}, ${$self->{clist}}[$i]->{toy} ) = 
                $self->matrix_mult( $mat, [${$self->{clist}}[$i]->{tox}, ${$self->{clist}}[$i]->{toy}] );
        }

        if( ${$self->{clist}}[$i]->{cmd} eq 'a' )
        {
            (${$self->{clist}}[$i]->{cx}, ${$self->{clist}}[$i]->{cy} ) =
                $self->matrix_mult( $mat, [ ${$self->{clist}}[$i]->{cx}, ${$self->{clist}}[$i]->{cy}] );
        }
    }

    # apply transform also to the start and current positions
    ($self->{startx}, $self->{starty}) = $self->matrix_mult( $mat, [$self->{startx}, $self->{starty}] );
    ($self->{x},      $self->{y})      = $self->matrix_mult( $mat, [$self->{x},      $self->{y}] );

    $self->auto_remake_bounding_box();
}


sub rotate()
{
    my $self  = shift;
    my $angle = shift;
    my $units = shift || 'radians';  # what's the angle unit?  default is radians.

    if( $units =~ /degree/ ) { $angle = deg2rad($angle); }

    $self->linear_transform( [[cos($angle),-sin($angle)],[sin($angle),cos($angle)]] );
}


sub set_colors()
{
    my $self = shift;
    my %args = @_;
    $self->{moveto_color} = exists($args{move}) ? $args{move} : $self->{moveto_color};
    $self->{cut_color}  = exists($args{cut})  ? $args{cut}   : $self->{cut_color};

    if( exists($args{mark_start}) ){ $self->set_marker_parameters( mark_start_color => $args{mark_start} ); }
    if( exists($args{mark_end})   ){ $self->set_marker_parameters( mark_end_color   => $args{mark_end} ); }
}

sub set_marker_parameters()
{
    my $self = shift;
    my %args = @_;
    $self->{mark_start_color} = exists($args{mark_start_color}) ?  $args{mark_start_color} : $self->{mark_start_color};
    $self->{mark_end_color}   = exists($args{mark_end_color})   ?  $args{mark_end_color}   : $self->{mark_end_color};
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

    if( defined($self->{x}) && $self->{x} == $newx && $self->{y} == $newy )
    {
        # did the position change?
        return 0;
    }

    $self->update_bounds( $newx, $newy );

    $self->{x} = $newx;
    $self->{y} = $newy;

    return 1;
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
    
    if( $self->update_position( $newx, $newy ) )
    {
        push($self->{clist},
            {
                cmd=>'m',   clr=>$self->{k}->get_color('moveto_color'),
                sox=>$sox,  soy=>$soy,
                tox=>$newx, toy=>$newy
            });
    }
}


sub do_mark()
{
    my $self = shift;
    my $cmd  = shift;
    my $clr  = shift;
    push($self->{clist}, { cmd=>$cmd,  clr=>$clr, sox => $self->{x}, soy => $self->{y}, r => $self->{mark_dot_radius} });
}

sub mark_start() { my $self = shift; my $clr  = shift || $self->{mark_start_color}; $self->do_mark('os',$clr); }
sub mark_end()   { my $self = shift; my $clr  = shift || $self->{mark_end_color};   $self->do_mark('oe',$clr); }


sub lineto()
{
    my $self = shift;
    my $newx = shift;
    my $newy = shift;

    my $sox = $self->{x};
    my $soy = $self->{y};
    
    if( $self->update_position( $newx, $newy ) )
    {
        push($self->{clist},
            {
                cmd=>'l',   clr=>$self->{cut_color}, 
                sox=>$sox,  soy=>$soy,
                tox=>$newx, toy=>$newy
            });
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
                                            'ux,uy == %.02f,%.02f; vx,vy == %.02f,%.02f; cross = %.02f; r1,r1 == %.02f,%.02f; %s',
                                            $sox, $soy, $tox, $toy, $cx, $cy, $ux, $uy, $vx, $vy, $cross, $r1, $r2, "\n" ) );

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
    elsif( defined($rdus) && defined($sweep) && defined($largearc) )
    {
        # this of (ux,uy) as a vector rooted at t with its head at s. t toward s.  or as rooted at s pointing directly away from t.
        my $ux = $sox - $tox;
        my $uy = $soy - $toy;
        my $dist = sqrt( $ux**2 + $uy**2 ); # distance between t and s.

        if( $rdus < .5*$dist && abs($rdus - .5*$dist) > $radius_tolerance )
        {
            die "radius ($rdus) between specified points (s: $sox,$soy) -> (t: $tox,$toy) is too short (dist:$dist)";
        }
        elsif( abs($rdus - .5*$dist) > $radius_tolerance )
        {
            # the radius is long enough that the center of the circle is not on the line
            # between the two points (we're rotating trough a non-180 degree angle.)

            my $half_dist = .5 * $dist;

            # this is the midpoint between the s and t. below there is a better explanation
            my $mx = $sox - .5 * $ux;
            my $my = $soy - .5 * $uy;

            # ($ux,$uy) is the vector from s to t, so (-$uy,$ux) is a vector perpendicular
            # to ($ux,$uy) with the same length.  to re-locate that vector so that the head
            # of it actually falls on the line between s and t, we add the midpoint.  The center
            # of the circle lies along this line we just found.  It's at a distance from the 
            # midpoint given by the all important a^2 + b^2 = c^2 relationship.  'c' is 'rdus'
            # 'b' is 'half_dist' thus:  a**2 == rdus**2 - half_dist**2, and we want 'a'.  this
            # is the offset of the center of the circle from the vector ($mx,$my) along the line
            # we found earlier.

            # this is the absolute distance offset of (cx,cy) from (mx,my)
            my $a = sqrt($rdus**2 - $half_dist**2);

            # make a vector perpendicular to (ux,uy), called (vx,vy), with length $a, oriented
            # such that (ux,uy,0) x (vx,vy,0) == |%|*(0,0, ux**2 + uy**2). i.e.: rhr positive.
            my $tmplen = sqrt($ux**2 + $uy**2);
            my $vx = $uy * (-1/$tmplen) * $a;
            my $vy = $ux * ( 1/$tmplen) * $a;

            # now the desired (cx,cy) is either at (mx,my) + (vx,vy), or at (mx,my) - (vx,vy)
            # depending on sweep and largearc.  the large side of the circle (largearc) will
            # go on the side of (ux,uy) that has our center.  so...
            #  ...  it can be shown that:
            #  (cx,cy)  ==  (mx,my)  +  (-1)*(sweep xor largearc)*(vx,vy)
            #  
            # that is:  if(sweep != largearc){ then negate (vx,vy) }
            # it's not too difficult to prove on paper.
            $cx = $mx + (-1)*($sweep ^ $largearc)*$vx;
            $cy = $my + (-1)*($sweep ^ $largearc)*$vy;
        }
        else
        {
            # we're doing a half circle. center is the mid-point.
            # rationalle of this code below:
            #   if sox (start-x) is less than tox (to-x), then the center point we need is
            #   greater than sox. Since it's true also, in this case, that ($ux < 0) we add
            #   ux/2 to sox by subtracting it's negative value from it.  so cx = sox - .5*ux.
            #   on the other hand, if sox > tox, then ux > 0, AND we need a cx that's less
            #   than sox;  so we need cx = sox - .5*ux, the same thing. (this is like the
            #   simple harmonic oscilator from physics class.)  Same goes for cy.

            $cx = $sox - .5 * $ux;
            $cy = $soy - .5 * $uy;
        }
    }
    else
    {
        die "(the arc center and (largearc or sweep) must be defined) or\n(the radius sweep and largearc must all be defined)\n";
    }


    # for complete circles the next position will equal the start, so add command
    # even if the current position didn't change.
    push($self->{clist},
        {
            cmd=>'a',        clr=>(exists($args{clr}) ? $args{clr} : $self->{cut_color}), 
            sox=>$sox,       soy=>$soy,
            tox=>$tox,       toy=>$toy,
            sweep=>$sweep,   largearc=>$largearc,
            cx=>$cx,         cy=>$cy,
            radius=>$rdus
        });

    $self->{k}->print_debug( 5,
        sprintf( 'arc debug -- sox=>%.03f, soy=>%.03f, tox=>%.03f, toy=>%.03f, sweep=>%d, largearc=>%d, cx=>%.03f, cy=>%.03f, radius=>%.03f',

            $sox,       $soy,
            $tox,       $toy,
            $sweep,     $largearc,
            $cx,        $cy,
            $rdus * $self->{mult} ));

    $self->update_position( $tox, $toy );
}


# /* vim: set ai et tabstop=4  shiftwidth=4: */
1;
