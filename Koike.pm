#!/usr/bin/perl -w

use IO::File;
use IO::Handle;

package Koike;

use strict;
use warnings;

BEGIN
{
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 0.01;
    @ISA         = qw(Exporter);
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

    $self->{DEBUG} = exists($args{debug}) ? $args{debug}    : 10;  # 10 means: no debug messages.
    $self->{p} = exists($args{protocol})  ? $args{protocol} : 'gcode'; # protocol: svg or gcode
    $self->{x} = undef;
    $self->{y} = undef;

    # bounding box.  XXX this doesn't correctly handle arcs,
    # whose extent will often pass beyond end points.
    $self->{xmin} = undef;
    $self->{xmax} = undef;
    $self->{ymin} = undef;
    $self->{ymax} = undef;
    $self->{height} = undef;
    $self->{width} = undef;

    my $dfltclr = 'rgb(255,0,0)';
    $self->{moveto_color} = exists($args{moveto_color}) ? $args{moveto_color}  : 'none';
    $self->{cut_color}    = exists($args{cut_color})    ? $args{cut_color}     : $dfltclr;
    $self->{line_color}   = exists($args{line_color})   ? $args{line_color}    : $dfltclr;
    $self->{curve_color}  = exists($args{curve_color})  ? $args{curve_color}   : $dfltclr;
    $self->{matrl_color}  = exists($args{material_color})  ? $args{material_color}   : 'rgb(200,255,255)';
    $self->{clist} = [];

    # offsets and scaling
    $self->{mult}         = exists($args{multsclr})     ? $args{multsclr}      : 1;
    $self->{xoffset}      = exists($args{xoffset})      ? $args{xoffset}       : 0;
    $self->{yoffset}      = exists($args{yoffset})      ? $args{yoffset}       : 0;

    # g-code options and state variables
    $self->{abscoords}    =  exists($args{abscoords})      ? $args{abscoords}     : 1;
    $self->{absIJKcoords} =  exists($args{absIJKcoords})   ? $args{absIJKcoords}  : 0;
    $self->{verbosegcode} =  exists($args{verbosegcode})   ? $args{verbosegcode}  : 0;
    $self->{LASTGCMD} = ''; # the present command (G00,G01,G02,...) is a machines state variable

    # create a handle to the output file (default: stdout) so that a replacement can be used transparently
    $self->{fh} = IO::Handle->new();
    $self->{fh}->fdopen(fileno(STDOUT),"w");

    bless($self,$class); # bless me! and all who are like me. bless us everyone.
    return $self;
}

sub set_colors()
{
    my $self = shift;
    my %args = @_;

    $self->{moveto_color} = exists($args{moveto_color}) ? $args{moveto_color}  : $self->{moveto_color};
    $self->{cut_color}    = exists($args{cut_color})    ? $args{cut_color}     : $self->{cut_color};
    $self->{line_color}   = exists($args{line_color})   ? $args{line_color}    : $self->{line_color};
    $self->{curve_color}  = exists($args{curve_color})  ? $args{curve_color}   : $self->{curve_color};
    $self->{matrl_color}  = exists($args{material_color})  ? $args{material_color}   : $self->{matrl_color};
}

sub get_colors()
{
    my $self = shift;

    return (
        moveto_color => $self->{moveto_color},
        cut_color    => $self->{cut_color},
        line_color   => $self->{line_color},
        curve_color  => $self->{curve_color},
        matrl_color  => $self->{matrl_color},
    );
}

sub get_color()
{
    my $self  = shift;
    my $clrnm = shift;
    die if $clrnm !~  /^(moveto|cut|line|curve|matrl)_color$/;
    return $self->{$clrnm};
}

sub update_bounds()
{
    my $self = shift;
    my $newx = shift;
    my $newy = shift;

    if( ! defined($self->{xmin}) ) 
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

sub set_rectagular_material_bounds()
{
    my $self = shift;
    my $x1 = shift;
    my $y1 = shift;
    my $x2 = shift;
    my $y2 = shift;

    push($self->{clist},
        {
            cmd=>'rb',   clr=>$self->{matrl_color},
            sox=>$x1, soy=>$y1,
            tox=>$x2, toy=>$y2
        });
}

sub add_part()
{
    my $self = shift;
    my $p = shift;

    $p->rewind_command_list(); # just in case this was added before.
    my $c;
    while( ($c = $p->next_command()) )
    {
        $self->update_position( $c->{sox}, $c->{soy} );
        if( exists( $c->{tox} ) )
        {
            $self->update_position( $c->{tox}, $c->{toy} );
            if( $c->{cmd} eq 'a' )
            {
                # XXX a trade off.
                # this set of points will cover a whole circle and so is
                # guaranteed to give adequate space for the parts.  on the
                # other hand, this can very easily over extend the bounds.
                # a better solution takes into account the sweep, largearc,
                # and start and stop position parameters, but that is
                # complicated and difficult.
                $self->update_bounds( $c->{cx} + $c->{radius}, $c->{cy} );
                $self->update_bounds( $c->{cx} - $c->{radius}, $c->{cy} );
                $self->update_bounds( $c->{cx}, $c->{cy} + $c->{radius} );
                $self->update_bounds( $c->{cx}, $c->{cy} - $c->{radius} );
            }
        }

        push($self->{clist}, $c);
    }
}

### sub moveto()
### {
###     my $self = shift;
###     my $newx = shift;
###     my $newy = shift;
### 
###     my $sox = $self->{x};
###     my $soy = $self->{y};
###     
###     if( $self->update_position( $newx, $newy ) )
###     {
###         push($self->{clist},
###             {
###                 cmd=>'m',   clr=>$self->{moveto_color}, 
###                 sox=>$sox,  soy=>$soy,
###                 tox=>$newx, toy=>$newy
###             });
###     }
### }
### 
### sub mark_start()
### {
###     my $self = shift;
###     my $clr  = shift || 'rgb(0,200,0)';
###     push($self->{clist}, { cmd=>'os',  clr=>$clr, sox => $self->{x}, soy => $self->{y} });
### }
### 
### sub mark_end()
### {
###     my $self = shift;
###     my $clr  = shift || 'rgb(230,0,0)';
###     push($self->{clist}, { cmd=>'oe',  clr=> $clr, sox => $self->{x}, soy => $self->{y} });
### }
### 
### 
### sub lineto()
### {
###     my $self = shift;
###     my $newx = shift;
###     my $newy = shift;
### 
###     my $sox = $self->{x};
###     my $soy = $self->{y};
###     
###     if( $self->update_position( $newx, $newy ) )
###     {
###         push($self->{clist},
###             {
###                 cmd=>'l',   clr=>$self->{line_color}, 
###                 sox=>$sox,  soy=>$soy,
###                 tox=>$newx, toy=>$newy
###             });
###     }
### }
### 
### # svg can do ellipses, but gcode cannot. for now, allow only circular arcs.
### sub arcto()
### {
###     my $self = shift;
###     my %args = @_;
### 
###     my $tox = $args{newx};
###     my $toy = $args{newy};
### 
###     my $rdus = undef;
###     if( exists($args{radius}) ) { $rdus = $args{radius}; }
### 
###     my $sweep = undef;
###     if( exists($args{sweep})     ) { $sweep = $args{sweep}; } # ifdef sweep, ignore clockwise
### 
###     my $largearc = undef;
###     if( exists($args{largearc}) ) { $largearc = $args{largearc}; }
### 
###     my $cx = undef;
###     my $cy = undef;
### 
###     my $radius_tolerance = .00000001;  # arbitrary tiny number. this maybe needs to come from the machine spec.
### 
###     my $sox = $self->{x};
###     my $soy = $self->{y};
### 
###     if( exists($args{cx}) && exists($args{cy}) && ( defined($largearc) || defined($sweep) ) )
###     {
###         # combined with the starting point, ending point, and sweep,
###         # we can calculate the radius and largearc-flag
### 
###         $cx = $args{cx};
###         $cy = $args{cy};
### 
###         # to find the largearc-flag, do a cross-product.
###         # here's the algo.:
###         # u = s - c    # u is the vector from the center to start (sox, soy, 0)
###         # v = t - c    # v is the vector from the center to end   (tox, toy, 0)
###         # let (u x v)k be the third (the kth) coodinate of (u x v)
###         #
###         # if( (u x v)k > 0 &&   $sweep ) largearc-flag = 0;
###         # if( (u x v)k > 0 && ! $sweep ) largearc-flag = 1;
###         # if( (u x v)k < 0 &&   $sweep ) largearc-flag = 1;
###         # if( (u x v)k < 0 && ! $sweep ) largearc-flag = 0;
### 
###         my $ux = $sox - $cx;
###         my $uy = $soy - $cy;
###         my $vx = $tox - $cx;
###         my $vy = $toy - $cy;
###         my $cross = $ux*$vy - $uy*$vx;
### 
###         if( defined($sweep) && ! defined($largearc) )
###         {
###             if(    $cross  < 0 ){ $largearc =   $sweep; }
###             elsif( $cross  > 0 ){ $largearc = ! $sweep; }
###             else                { $largearc = 1; }
###         }
###         elsif( ! defined($sweep) && defined($largearc) )
###         {
###             if(    $cross  < 0 ){ $sweep =   $largearc; }
###             elsif( $cross  > 0 ){ $sweep = ! $largearc; }
###             else                { die "sweep is ambiguous with 180-degree arc. sweep must be defined"; }
###         }
###         else
###         {
###             # verify that the everything is in agreement
###             if( $cross  < 0 && $sweep != $largearc ){ die "sweep disagrees with largearc given start,stop,center"; }
###             if( $cross  > 0 && $sweep == $largearc ){ die "sweep disagrees with largearc given start,stop,center"; }
###         }
### 
###         # assert( at this point, largearc and sweep are defined and agree )
### 
###         my $r1 = sqrt( $ux**2 + $uy**2 );
###         my $r2 = sqrt( $vx**2 + $vy**2 );
### 
###         $self->print_debug( 5, sprintf( 'sox,soy == %.02f,%.02f; tox,toy == %.02f,%.02f; cx,cy == %.02f,%.02f; '.
###                                         'ux,uy == %.02f,%.02f; vx,vy == %.02f,%.02f; cross = %.02f; r1,r1 == %.02f,%.02f; %s',
###                                          $sox, $soy, $tox, $toy, $cx, $cy, $ux, $uy, $vx, $vy, $cross, $r1, $r2, "\n" ) );
### 
###         if( abs($r1 - $r2) > $radius_tolerance )
###         {
###             die "the calculated radii for a circular arc must be reasonably equal. r1:$r1 != r2:$r2  diff: ".abs($r1 - $r2);
###         }
### 
###         if( defined($rdus) )
###         {
###             if( abs( $rdus - $r2 ) > $radius_tolerance )
###             {
###                 die "the specified radius differs from the calculated radii by too much. ".
###                     "given:$rdus != calc'd:$r2. diff: ".abs( $rdus - $r2 );
###             }
###             # rdus is within tolerance.  leave it alone.
###         }
###         else
###         {
###             $rdus = ($r1 == $r2 ? $r1 : .5*($r1 + $r2));
###         }
###     }
### 
###     # if the center point is not defined then we need a radius, sweep, *and* largearc flag to find the center.
###     elsif( defined($rdus) && defined($sweep) && defined($largearc) )
###     {
###         # this of (ux,uy) as a vector rooted at t with its head at s. t toward s.  or as rooted at s pointing directly away from t.
###         my $ux = $sox - $tox;
###         my $uy = $soy - $toy;
###         my $dist = sqrt( $ux**2 + $uy**2 ); # distance between t and s.
### 
###         if( $rdus < .5*$dist && abs($rdus - .5*$dist) > $radius_tolerance )
###         {
###             die "radius between specified points is too short";
###         }
###         elsif( abs($rdus - .5*$dist) > $radius_tolerance )
###         {
###             # the radius is long enough that the center of the circle is not on the line
###             # between the two points (we're rotating trough a non-180 degree angle.)
### 
###             my $half_dist = .5 * $dist;
### 
###             # this is the midpoint between the s and t. below there is a better explanation
###             my $mx = $sox - .5 * $ux;
###             my $my = $soy - .5 * $uy;
### 
###             # ($ux,$uy) is the vector from s to t, so (-$uy,$ux) is a vector perpendicular
###             # to ($ux,$uy) with the same length.  to re-locate that vector so that the head
###             # of it actually falls on the line between s and t, we add the midpoint.  The center
###             # of the circle lies along this line we just found.  It's at a distance from the 
###             # midpoint given by the all important a^2 + b^2 = c^2 relationship.  'c' is 'rdus'
###             # 'b' is 'half_dist' thus:  a**2 == rdus**2 - half_dist**2, and we want 'a'.  this
###             # is the offset of the center of the circle from the vector ($mx,$my) along the line
###             # we found earlier.
### 
###             # this is the absolute distance offset of (cx,cy) from (mx,my)
###             my $a = sqrt($rdus**2 - $half_dist**2);
### 
###             # make a vector perpendicular to (ux,uy), called (vx,vy), with length $a, oriented
###             # such that (ux,uy,0) x (vx,vy,0) == |%|*(0,0, ux**2 + uy**2). i.e.: rhr positive.
###             my $tmplen = sqrt($ux**2 + $uy**2);
###             my $vx = $uy * (-1/$tmplen) * $a;
###             my $vy = $ux * ( 1/$tmplen) * $a;
### 
###             # now the desired (cx,cy) is either at (mx,my) + (vx,vy), or at (mx,my) - (vx,vy)
###             # depending on sweep and largearc.  the large side of the circle (largearc) will
###             # go on the side of (ux,uy) that has our center.  so...
###             #  ...  it can be shown that:
###             #  (cx,cy)  ==  (mx,my)  +  (-1)*(sweep xor largearc)*(vx,vy)
###             #  
###             # that is:  if(sweep != largearc){ then negate (vx,vy) }
###             # it's not too difficult to prove on paper.
###             $cx = $mx + (-1)*($sweep ^ $largearc)*$vx;
###             $cy = $my + (-1)*($sweep ^ $largearc)*$vy;
###         }
###         else
###         {
###             # we're doing a half circle. center is the mid-point.
###             # rationalle of this code below:
###             #   if sox (start-x) is less than tox (to-x), then the center point we need is
###             #   greater than sox. Since it's true also, in this case, that ($ux < 0) we add
###             #   ux/2 to sox by subtracting it's negative value from it.  so cx = sox - .5*ux.
###             #   on the other hand, if sox > tox, then ux > 0, AND we need a cx that's less
###             #   than sox;  so we need cx = sox - .5*ux, the same thing. (this is like the
###             #   simple harmonic oscilator from physics class.)  Same goes for cy.
### 
###             $cx = $sox - .5 * $ux;
###             $cy = $soy - .5 * $uy;
###         }
###     }
###     else
###     {
###         die "the arc center or the largearg-flag must be defined\n";
###     }
### 
### 
###     # for complete circles the next position will equal the start, so add command
###     # even if the current position didn't change.
###     push($self->{clist},
###         {
###             cmd=>'a',        clr=>(exists($args{clr}) ? $args{clr} : $self->{line_color}), 
###             sox=>$sox,       soy=>$soy,
###             tox=>$tox,       toy=>$toy,
###             sweep=>$sweep,   largearc=>$largearc,
###             cx=>$cx,         cy=>$cy,
###             radius=>$rdus
###         });
### 
###     $self->print_debug( 5,
###         sprintf( 'arc debug -- sox=>%.03f, soy=>%.03f, tox=>%.03f, toy=>%.03f, sweep=>%d, largearc=>%d, cx=>%.03f, cy=>%.03f, radius=>%.03f',
###             ($sox + $self->{xoffset}) * $self->{mult}, ($soy + $self->{yoffset}) * $self->{mult},
###             ($tox + $self->{xoffset}) * $self->{mult}, ($toy + $self->{yoffset}) * $self->{mult},
###             $sweep, $largearc,
###             ($cx + $self->{xoffset}) * $self->{mult}, ($cy + $self->{yoffset}) * $self->{mult},
###             $rdus * $self->{mult} ));
### 
###     $self->update_position( $tox, $toy );
### }

sub svg_lineto()
{
    my $self = shift;
    my $h = shift;
    local *FH = $self->{fh};
    printf( FH '<line x1="%.03f" y1="%.03f" x2="%.03f" y2="%.03f" style="stroke:%s;stroke-width:1" />%s',
        ($h->{sox} + $self->{xoffset}) * $self->{mult},
        ($h->{soy} + $self->{yoffset}) * $self->{mult}, 
        ($h->{tox} + $self->{xoffset}) * $self->{mult},
        ($h->{toy} + $self->{yoffset}) * $self->{mult},
        $h->{clr},"\n");
}

sub svg_arcto()
{
    my $self = shift;
    my $h = shift;
    my $r = $h->{radius};
    my $l = $h->{largearc};
    my $s = $h->{sweep};
    my $c = $h->{clr};

    # d="move to where I already am, then draw an arc using our parameters."
    local *FH = $self->{fh};
    printf( FH '<path d="M%.03f %.03f A%.03f,%.03f 0 %d,%d %.03f,%.03f" style="stroke:%s;stroke-width:1;fill:none" />%s',

        ($h->{sox} + $self->{xoffset}) * $self->{mult},
        ($h->{soy} + $self->{yoffset}) * $self->{mult}, 

        $h->{radius} * $self->{mult},
        $h->{radius} * $self->{mult}, 

        $h->{largearc}, $h->{sweep}, 

        ($h->{tox} + $self->{xoffset}) * $self->{mult},
        ($h->{toy} + $self->{yoffset}) * $self->{mult},

        $h->{clr},"\n" );
}

sub svg_moveto()
{
    my $self = shift;
    my $h = shift;
    if( exists($h->{clr}) && defined($h->{clr}) && $h->{clr} ne 'none' )
    {
        $self->svg_lineto( $h );
    }
}

sub svg_endpoint()
{
    my $self = shift;
    my $h = shift;
    local *FH = $self->{fh};
    printf( FH '<circle cx="%.03f", cy="%.03f", r="3", fill="%s" stroke="none" />%s', 
        (($h->{sox} + $self->{xoffset}) * $self->{mult}),
        (($h->{soy} + $self->{yoffset}) * $self->{mult}),
        $h->{clr}, "\n" );
}

sub svg_rectangular_background()
{
    my $self = shift;
    my $h = shift;

    my $wd = abs($h->{sox} - $h->{tox});
    my $ht = abs($h->{soy} - $h->{toy});

    local *FH = $self->{fh};
    printf( FH '<rect x="%.03f", y="%.03f", width="%.03f", height="%.03f", fill="%s" stroke="none" />%s', 
        (($h->{sox} + $self->{xoffset}) * $self->{mult}),
        (($h->{soy} + $self->{yoffset}) * $self->{mult}),
        $wd * $self->{mult},
        $ht * $self->{mult},
        $h->{clr}, "\n" );
}

sub gcode_linear_motion()
{
    my $self = shift;
    my $h = shift;
    my $s = shift;
    if( length($s) > 0 )
    {
        # data transfer to the the Koike is very slow, so reduce
        # characters whenever it makes sense. (or when possible.)
        if( $self->{LASTGCMD} eq $s && ! $self->{verbosegcode} )
        {
            $s = '';
        }
        else
        {
            $self->{LASTGCMD} = $s;
            $s .= ' ';
        }
    }

    # remove unnecessary trailing zeros.  again, this is to reduce the data size.
    my $xstr = sprintf( '%.03f%s', ($h->{tox} + $self->{xoffset}) * $self->{mult}, ($self->{verbosegcode} ? ' ' : ''));
    my $ystr = sprintf( '%.03f%s', ($h->{toy} + $self->{yoffset}) * $self->{mult}, ($self->{verbosegcode} ? ' ' : '') );

    if( ! $self->{verbosegcode} )
    {
        if( $xstr =~ s/0+$// ){ $xstr =~ s/\.$//; }
        if( $ystr =~ s/0+$// ){ $ystr =~ s/\.$//; }
    }

    local *FH = $self->{fh};
    printf( FH '%sX%sY%s%s', $s, $xstr, $ystr, "\n" );
}

sub gcode_lineto() { my $self = shift; my $h = shift; $self->gcode_linear_motion( $h, 'G01' ); }
sub gcode_moveto() { my $self = shift; my $h = shift; $self->gcode_linear_motion( $h, 'G00' ); }

sub gcode_arcto()
{
    my $self = shift; my $h = shift; 

    my $g = $h->{sweep} ? 'G03' : 'G02' ;

    # I assume G02/3 are rarely used, so I won't bother, for now, with the space-saving code.
    # The purpose of this is just so that ($self->{LASTGCMD} != 'G00/1')
    $self->{LASTGCMD} = $g;

    local *FH = $self->{fh};
    printf( FH '%s X%.03f Y%.03f I%.03f J%.03f%s', $g, #  $h->{tox}, $h->{toy}, ($h->{tox} - $h->{sox}), ($h->{toy} - $h->{soy}), "\n" );

        ($h->{tox} + $self->{xoffset}) * $self->{mult}, 
        ($h->{toy} + $self->{yoffset}) * $self->{mult}, 

        ($h->{cx} - $h->{sox}) * $self->{mult}, 
        ($h->{cy} - $h->{soy}) * $self->{mult}, "\n" );
}

sub set_height_width()     { my $self = shift; $self->{height}  = shift; $self->{width}    = shift; }
sub set_offsets()          { my $self = shift; $self->{xoffset} = shift; $self->{yoffset}  = shift; }
sub set_scale_multiplier() { my $self = shift; $self->{mult}    = shift; }


sub printall()
{
    my $self  = shift;
    my $fname = shift;

    if( defined($fname) )
    {
        $self->{fh} = IO::File->new();
        $self->{fh}->open( $fname, '>' ) || die "failed to open $fname for writing\n";
    }

    my %cmds = ();
    if( $self->{p} eq 'svg' )
    {
        %cmds = (l=>\&svg_lineto,   a=>\&svg_arcto,   m=>\&svg_moveto,   start=>\&print_svg_html_start,    end=>\&print_svg_html_end,
                 os=>\&svg_endpoint, oe=>\&svg_endpoint, rb=>\&svg_rectangular_background );
    }
    else
    {
        %cmds = (l=>\&gcode_lineto, a=>\&gcode_arcto, m=>\&gcode_moveto, start=>\&print_koike_gcode_start, end=>\&end_koike_gcode );
    }

    &{$cmds{start}}( $self );

    # my $lref = $self->{clist}
    foreach my $i ( @{$self->{clist}} )
    {
        if( exists($cmds{$i->{cmd}}) ) { &{ $cmds{$i->{cmd}} }( $self, $i ); }
    }

    &{$cmds{end}}( $self );
}

sub print_koike_gcode_start()
{
    my $self = shift;
    local *FH = $self->{fh};

    # the asterisk before M08 is part of the communications protocol, and the remaining
    # codes basically turn the cutting heads off and stuff lke that.
    print FH <<"EOF";
*
M08
M16
G20
G40
G90
EOF

    $self->{LASTGCMD} = 'G90';
}

sub end_koike_gcode()
{
    my $self = shift;
    local *FH = $self->{fh};

    # M30 says "end of program" signals koike to stop reading serial input.
    print FH <<"EOF";
M30
EOF

}




sub print_svg_html_start()
{
    my $self = shift;

    if( ! defined($self->{height}) ){ $self->{height} = $self->{ymax}; }
    if( ! defined($self->{width})  ){ $self->{width}  = $self->{xmax}; }

    my $h = ($self->{height} + abs($self->{yoffset})) * $self->{mult};
    my $w = ($self->{width}  + abs($self->{xoffset})) * $self->{mult};

    # round these values up to the nearest integer
    if( $h != int($h) ) { $h = sprintf("%d", $h+.5 ); }
    if( $w != int($w) ) { $w = sprintf("%d", $w+.5 ); }

    local *FH = $self->{fh};
    print FH <<"EOF";

<!DOCTYPE html>
<html>
 <body>

<svg height="$h" width="$w">

EOF
}

sub print_svg_html_end()
{
    my $self = shift;
    local *FH = $self->{fh};
    print FH <<"EOF";

      Sorry, your browser does not support inline SVG.
 </svg>

 </body>
</html>

EOF
}

sub print_debug()
{
    my $self = shift;
    my $importance = shift;
    my $msg  = join('', @_);
    if( $importance > $self->{DEBUG} )
    {
        my $d=`date "+%F %T"`;
        chomp($d);
        print STDERR $d,' debugging -- ',$msg,"\n";
    }
}


# /* vim: set ai et tabstop=4  shiftwidth=4: */
1;
