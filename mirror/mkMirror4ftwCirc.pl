#!/usr/bin/perl -w -I..

use strict;
use warnings;

use Koike;
use Koike::Part;

# usage: mkMirror4ftwCirc.pl [--svg] [--mult=<mult>]
#        where <prot> := "gcode" | "svg"
#        and   <mult> := float;                 (scales drawing.)


# the parabola is generated so that its vertex is at the origin (0,0) and
# afterward offsets and the scaling multiplier are applied.  I think this is
# the most natural coordinate frame.  The circle, then, in the same coordinate
# frame would have its center as (x,y)==(0,18) because its center is at the
# focal point of the parabola.  So, we start with the parabola and circle
# situated properly with respect to each other, but distant from their needed
# positions and sizes in the svg or koike frames. We give the offsets, and
# scaling factor (mult) to the KoikeCustom module and it uses them to alter all
# points, of the circle and parabola.
our %pconst = ( negative=>1, focallength=>18, stepsize=>0.25, negative=> 1, xoffset=>-24, yoffset=>-8 );
our %cconst = ( negative=>1, cx=>0, cy=>18, radius=>28, sweep => 0, largearc => 0 );
our %argv   = ( do_svg=>0, mult=>1 );


# process command-line arguments
$argv{'do_svg'} = grep(/--svg/, @ARGV);
if( (my @l = grep(/--mult=-?[0-9.]+/, @ARGV)) ){ $l[0] =~ /--mult=(-?[0-9.]+)/; $argv{mult} = $1; }

# the svg and koike coordinate frames are different. alter the contants to move between coord. frames.
if( $argv{'do_svg'} )
{
    # for clarity in svg, nudge the offsets away from the edge of the image.
    $pconst{xoffset} -= 5;  $pconst{yoffset} -= 5;

    $pconst{negative} *= -1;  $pconst{xoffset} *= -1;  $pconst{yoffset} *= -1;
    $cconst{negative} *= -1;                           $cconst{cy} *= -1;
    $cconst{sweep} = 1;  # "positive" angle is clockwise.  woops!!

    # XXX sweep == 1 means move in the CLOCKWISE direction.  the docs say
    # clockwise is "positive" ... grr.  oh!  the convention is that the x-axis
    # is the position of the zero angle and that an angle growing from there
    # toward the positive y-axis is positive.  The y-axis grows from top to
    # bottom in the svg system (as is usual in image libraries) so the sign of
    # an angle is expressed as increasing in the clockwise direction.  weird.
    #
    # XXX In the gcode piece I use sweep == 1, positive angle, to mean counter-
    # clockwise, which if seen from above the koike positive angle is indeed
    # counterclockwise.  This might be confusing.  What to do about this?
}


&main();

sub main()
{
    our %pconst;
    our %argv;

    my $k = new Koike(
        protocol      => ( $argv{'do_svg'} ? 'svg' : 'gcode'),
        moveto_color  => 'rgb(0,0,200)',

        multsclr      => $argv{mult},

        xoffset       => $pconst{xoffset},
        yoffset       => $pconst{yoffset},
        #debug         => 1,
    );

    # svg only
    $k->set_height_width(100,100);

    my $x_zero = 24;
    my $y_zero =  8;

    $k->set_rectagular_material_bounds( -$x_zero, -$y_zero, $x_zero, 4*$x_zero -$y_zero );

    $k->update_position( $x_zero, &p($x_zero) ); # initialize position. this becomes 0,0 in gcode


    my $p = new Koike::Part( koikeobj=>$k );
    $p->moveto( $x_zero + 2, &p($x_zero + 2) );  # fast motion to point outside cut area, on parabola

    for( my $i = $x_zero + 1; $i >= -1 - $x_zero; $i -= $pconst{stepsize} )
    {
        my $x = $i;

        if( $x ==  $x_zero - $pconst{stepsize} ) { $p->mark_start(); }
        if( $x == -$x_zero - $pconst{stepsize} ) { $p->mark_end(); }

        $p->lineto( $x, &p($x) );
    }

    my $outside_x = 26;
     
    $p->moveto(-$outside_x, &p(-$outside_x) );   # move to a point where free x-motion is possible
    $p->moveto( $outside_x, &p( $outside_x) );   # we will cut the circle, also, along a path away
    $p->moveto( $x_zero + 1, &c( $x_zero + 1) ); # from the op. So go home.

    $p->arcto( newx=>$x_zero, newy=>&c($x_zero), 
        sweep    => $cconst{sweep},
        largearc => $cconst{largearc},
        radius   => $cconst{radius},
        cx => $cconst{cx}, 
        cy => $cconst{cy}
    );


    $p->mark_start();

    # cut the circle
    $p->arcto( newx=>$x_zero-10, newy=>&c($x_zero-10), 
        sweep    => $cconst{sweep},
        largearc => $cconst{largearc},
        radius   => $cconst{radius},
        cx => $cconst{cx}, 
        cy => $cconst{cy} );

    $p->mark_end();

    # find the position of the bottom of the platform.
    # modifying return value from c() requires use of cconst
    my $platformy = &c(0) - $cconst{negative} * 4;
    my $platformy_plus1 = &c(0) - $cconst{negative} * 3;

    $p->mark_start('rgb(180,0,180)');
    $p->lineto( $x_zero, $platformy_plus1 );
    $p->mark_end();
    $p->lineto( $x_zero +    1, $platformy_plus1 );
    $p->moveto( $x_zero +    1, &c($x_zero-10) );
    $p->moveto( $x_zero - 9.75, &c($x_zero-10) );
    $p->lineto( $x_zero -   10, &c($x_zero-10) );

    $p->arcto( newx=>-$x_zero, newy=>&c(-$x_zero), 
        sweep    => $cconst{sweep},
        largearc => $cconst{largearc},
        radius   => $cconst{radius},
        cx => $cconst{cx}, 
        cy => $cconst{cy} );

    $p->mark_end();

    $p->arcto( newx=>-$outside_x, newy=>&c(-$outside_x), 
        sweep    => $cconst{sweep},
        largearc => $cconst{largearc},
        radius   => $cconst{radius},
        cx => $cconst{cx}, 
        cy => $cconst{cy} );

    $p->moveto( -$outside_x,  $platformy_plus1 );
    $p->moveto( -1 - $x_zero, $platformy_plus1 );
    $p->lineto(     -$x_zero, $platformy_plus1 );
    $p->mark_start();
    $p->lineto( -22, &c(-22) );
    $p->mark_end();
    $p->lineto( -23, &c(-22) );

    $outside_x += .5;

    $p->moveto( -$outside_x, &c(-22) );

    $p->moveto( -$outside_x, &c(-$outside_x) ); # away from material, along the path
    $p->moveto( -$outside_x, &p(-$outside_x) ); # move to a point where free x-motion is possible
    $p->moveto(  $outside_x, &p( $outside_x) ); # move back toward the operator.

    # these moves use "outside_x" but there's lineto() in here that cuts the material.
    # it moves from an outside point to another outside point cutting all along the way.
    $p->moveto( $outside_x, $platformy );
    $p->moveto( $x_zero + 1,$platformy );
    $p->lineto( $x_zero,    $platformy );
    $p->mark_start();
    $p->lineto(-$x_zero,    $platformy );
    $p->mark_end();
    $p->lineto(-$x_zero - 1,$platformy );
    $p->moveto(-$outside_x, $platformy );

    # the koike may be set to automatically return to 0,0 when the commands are completed,
    # and will do it without regard to what it was cutting.  we need to do this safely.
    $outside_x += .5;   # it's nice to distinguish return-to-zero paths in the drawing.
    $p->moveto(-$outside_x, $platformy );
    $p->moveto(-$outside_x, &p(-$outside_x) );
    $p->moveto( $outside_x, &p( $outside_x) );
    # now we can move to 0,0 without hitting material still on the table

    $k->add_part( $p );
    $k->printall();
}

# parabola
sub p($)
{ 
    my $x=shift;
    our %pconst;

    #return $pconst{negative} * ( ( $x - $pconst{xoffset} )**2) / (4.0 * $pconst{focallength}) + $pconst{yoffset} ;
    return  $pconst{negative} * ($x**2) / (4.0 * $pconst{focallength});
} 

# circle
sub c($)
{ 
    my $x=shift;
    our %cconst;

    # (x - cx)**2 + (y - cy)**2 = r**2
    # (y - cy)**2 = r**2 - (x - cx)**2
    # y - cy = (+/-)sqrt(r**2 - (x - cx)**2)
    # y = (+/-)sqrt(r**2 - (x - cx)**2) + cy
    # circle top-half:  y = sqrt(r**2 - (x - cx)**2) + cy
    # circle bottom-half:  y = cy - sqrt(r**2 - (x - cx)**2) 
    # we want the bottom half so multiply the sqrt() by -1

    return -1 * $cconst{negative} * sqrt( $cconst{radius}**2 - ($x - $cconst{cx})**2) + $cconst{cy} ;
} 

# /* vim: set ai et tabstop=4  shiftwidth=4: */
