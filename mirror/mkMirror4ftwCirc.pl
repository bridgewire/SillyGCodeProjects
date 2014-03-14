#!/usr/bin/perl -w

use strict;
use warnings;

my $stpsz  = 0.25;
my $mult   = 1; # 100;
my $fcllen = 18;
my ($x1, $x2, $y1, $y2);

my $do_svg = grep(/--svg/, @ARGV);
if( (my @l = grep(/--mult=-?[0-9.]+/, @ARGV)) ){ $l[0] =~ /--mult=(-?[0-9.]+)/; $mult=$1; }


if( $do_svg ) { &start_svg_html( $mult ); }
else          { &start_koike_gcode( $mult, $fcllen ); }

for( my $i=0; $i<48; $i+=$stpsz )
{ 
    if( $do_svg )
    {
        $x1 = $i;
        $x2 = $x1 + $stpsz; # the next step

        $y1 = &p( $fcllen, $x1, -1, 8, 24 );
        $y2 = &p( $fcllen, $x2, -1, 8, 24 );

        printf('<line x1="%.03f" y1="%.03f" x2="%.03f" y2="%.03f" style="stroke:rgb(255,0,0);stroke-width:1" />%s',
            $x1*$mult, $y1*$mult, $x2*$mult, $y2*$mult, "\n");
    }
    else
    {
        $x1 = -$i;
        $x2 = $x1 - $stpsz; # the next step

        $y1 = &p( $fcllen, $x1, 1, -8, -24 );
        $y2 = &p( $fcllen, $x2, 1, -8, -24 );

        # koike g-code instead
        &gcode_step( $x1, $y1, $x2, $y2, $mult, ( $i+$stpsz >= 48 ) );
    }
}



if( $do_svg ) { &end_svg_html(); }
else          { &end_koike_gcode($mult, $fcllen, $x1, $x2, $y1, $y2); }


sub gcode_step($$$$$$)
{
    my $x1 = shift;
    my $y1 = shift;
    my $x2 = shift;
    my $y2 = shift;
    my $mult = shift;
    my $dox2 = shift;

    printf('X%.03f Y%.03f%s', $x1*$mult, $y1*$mult, "\n");
    if( $dox2 )
    {
        printf('X%.03f Y%.03f%s', $x2*$mult, $y2*$mult, "\n");
    }
}

# parabola
sub p($$$$$)
{ 
    my $f=shift;
    my $x=shift;
    my $neg=shift;
    my $yoffset=shift;
    my $xoffset=shift;

    return $neg*(($x-$xoffset)**2)/(4.0*$f) + $yoffset;
} 

sub start_svg_html($)
{
    my $mult = shift;

    my $h = 20 * $mult;
    my $w = 50 * $mult;

    my $aw = 48 * $mult; # actual width

    my $cr = 28 * $mult;
    my $cystart = 4 * $mult;

    print <<"EOF";

<!DOCTYPE html>
<html>
 <body>

<svg height="$h" width="$w">

<path d="M0 0 L0 $cystart A$cr,$cr 0 0,0  $aw,$cystart L$aw 0" style="stroke:rgb(255,0,0);stroke-width:1;fill:none" />

EOF

}

sub end_svg_html()
{
    print <<"EOF";

      Sorry, your browser does not support inline SVG.
 </svg>

 </body>
</html>

EOF

}


##########################################################################################
##########################################################################################
##########################################################################################
###### README
##########################################################################################
##########################################################################################
sub start_koike_gcode($$)
{
    my $mult = shift;
    my $fcllen = shift;

    # REQUIRED!!!!!
    # 1. when the program starts, Koike's coords are set to 0,0
    # 2. 0,0 are the true coordinates of the starting cut point
    # 3. The starting cut point next to the operator and will cut
    #    in the negative x-direction (away from the op.) and in
    #    the negative y-direction (toward the op.).
    # 4. The first cut is the parabola.
    # 5. start with cutter off, move AWAY from the cut start
    #    point, along the parabola, then switch to G01 and start
    #    the cut.  this move away is intended to give the op.
    #    a chance to properly set up the cutting wire.

    my $x1 = 2;
    my $x2 = 1.5;
    my $y1 = &p( $fcllen, $x1, 1, -8, -24 );
    my $y2 = &p( $fcllen, $x2, 1, -8, -24 );

    print <<"EOF";
*
M08
M16
G20
G40
G90
EOF

    print "G00\n";
    &gcode_step($x1, $y1, undef, undef, $mult, 0);

    # begin the slow walk
    print "G01\n";
    &gcode_step($x2, $y2, undef, undef, $mult, 0);
}

sub end_koike_gcode($$$$$$)
{
    my $mult = shift;
    my $fcllen = shift;
    my $x1 = shift;
    my $x2 = shift;
    my $y1 = shift;
    my $y2 = shift;

    $x1 = $x2 - 2;
    $y1 = &p( $fcllen, $x1, 1, -8, -24 );
    &gcode_step($x1, $y1, undef, undef, $mult, 0);

    # switch to fast motion
    print "G00\n";

    &gcode_step($x1, 5, undef, undef, $mult, 0);

    print <<"EOF";
G00 X2.000 Y5.000
G00 X2.000 Y0.3923
G01
G02 X-50 Y0.3923 I-52 J0
G00 X$x1 Y5.000
G00 X2.000 Y5.000
G00 X2.000 Y0.000
G01 X0.000 Y0.000
M30
EOF

}
