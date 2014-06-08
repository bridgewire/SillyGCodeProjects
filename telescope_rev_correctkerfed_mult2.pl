#!/usr/bin/perl

use warnings;
use strict;

use Math::Trig qw(:pi);

use Koike;
use Koike::Part;

my( $outfile, $cntperrow, $rowcnt);
my $dosvg = grep(/^--svg$/, @ARGV);
my $kerf = 0.175; # mm
if( (my @l = grep(/--out=.+?\.html$/, @ARGV)) ){ $l[0] =~ /--out=(.+?\.html)$/; $outfile = $1; }
if( (my @l = grep(/--perrow=[0-9]+$/, @ARGV)) )  { $l[0] =~ /--perrow=([0-9]+)$/; $cntperrow = $1; }
if( (my @l = grep(/--rowcount=[0-9]+$/, @ARGV)) )  { $l[0] =~ /--rowcount=([0-9]+)$/; $rowcnt = $1; }
if( (my @l = grep(/--kerf=[.0-9]+$/, @ARGV)) )  { $l[0] =~ /--kerf=([.0-9]+)$/; $kerf = $1; }

#print STDERR "args -- dosvg:'".($dosvg?"true":"false")."' outfile:'$outfile' cntperrow:'$cntperrow' rowcnt:'$rowcnt' kerf:'$kerf'\n";

# $k = new Koike(multsclr=>20, xoffset=>20, yoffset=>20, moveto_color=>'rgb(0,0,255)');
my $k = new Koike();
$k->process_cmdlineargs( @ARGV );



my $p = &make_scope_spar($k, 0, 0);
$k->add_part($p); 

my %bb = $p->get_bounding_box();
my $xsize = abs($bb{xmax} - $bb{xmin});
my $ysize = abs($bb{ymax} - $bb{ymin});

my $p1 = $p->copy();
$p1->rotate(180,'degrees');
$p1->translate( $xsize, 1.5 * $ysize );
$k->add_part($p1); 

my ( $outer_radius, $inner_radius, $center_x, $center_y, $notch_wid, $notch_dpt );


# end stop ring
( $outer_radius, $inner_radius ) = ( (1.5 * 25.4)/2, 25.4/2 );
( $center_x, $center_y ) = ( $outer_radius + 2, 1.5 * $ysize + $outer_radius + 2 );
( $notch_wid, $notch_dpt ) = ( 3.1, 0.75 );
$k->add_part( &notched4_ring( $k, $center_x, $center_y, $inner_radius, $outer_radius, $notch_wid, $notch_dpt, $kerf ) );


# interior stop ring

( $outer_radius, $inner_radius ) = ( (1.5 * 25.4)/2, 25.4/2 );
( $center_x, $center_y ) = ( 2*($outer_radius + 2), 1.5 * $ysize + $outer_radius + 2 );
( $notch_wid, $notch_dpt ) = ( 3.1, 0.75 );
$k->add_part( &notched4_ring( $k, $center_x, $center_y, $inner_radius, $outer_radius, $notch_wid, $notch_dpt, $kerf ) );



    $u{x} +=  3.1;


#my $p2 = $p->copy();
#$p2->translate( 0, 1.8 * $ysize );
#$k->add_part($p2); 
#
#my $p3 = $p1->copy();
#$p3->translate( 0 , 1.8 * $ysize );
#$k->add_part($p3); 


$k->printall( $outfile );


sub make_scope_spar()
{
    my $k = shift;
    my $startx = shift;
    my $starty = shift;

    $k->update_position( $startx, $starty );

    my $p = new Koike::Part( koikeobj=>$k );
    my( %u, %v );
    $u{x} = $v{x} = $startx;
    $u{y} = $v{y} = $starty;

    # notch for end stop
                                    $v{y} += 1.4 + $kerf;           $p->lineto( $v{x}, $v{y} );
                                    $u{y} += 1.4;
    $v{x} +=  3.1 - $kerf;                                          $p->lineto( $v{x}, $v{y} );
    $u{x} +=  3.1;
                                    $v{y} += 1.55;                  $p->lineto( $v{x}, $v{y} );
                                    $u{y} += 1.55;

    # travel to field lens location
    $v{x} +=  30.5 + 2*$kerf;                                       $p->lineto( $v{x}, $v{y} );


    # field lens
                                    $v{y} -=  0.75;                 $p->lineto( $v{x}, $v{y} );
                                    $u{y} -=  0.75;
    $v{x} +=  1.1 - 2*$kerf;                                        $p->lineto( $v{x}, $v{y} );
    $u{x} +=  1.1;
                                    $v{y} +=  0.75;                 $p->lineto( $v{x}, $v{y} );
                                    $u{y} +=  0.75;


    # slant from field lens to interior stop
    $v{x} += 24.15 + 2*$kerf;       $v{y} +=  2.341;                $p->lineto( $v{x}, $v{y} );
    $u{x} += 24.15;                 $u{y} +=  2.341;


    # interior stop
                                    $v{y} -=  2.64;                 $p->lineto( $v{x}, $v{y} );
                                    $u{y} -=  2.64;
    $v{x} +=  3.1 - 2*$kerf;                                        $p->lineto( $v{x}, $v{y} );
    $u{x} +=  3.1;
                                    $v{y} +=  2.64;                 $p->lineto( $v{x}, $v{y} );
                                    $u{y} +=  2.64;


    # slope from interior stop to eye piece level
    $v{x} += 33.15;                 $v{y} +=  5.384;                $p->lineto( $v{x}, $v{y} );
    $u{x} += 33.15;                 $u{y} +=  5.384;
    # flat space before lens
    $v{x} +=   1.5  + 2*$kerf;                                      $p->lineto( $v{x}, $v{y} );
    $u{x} +=   1.5;

    my $maxy = $v{y};

    # the eye piece
                                    $v{y} -=  0.75;                 $p->lineto( $v{x}, $v{y} );
                                    $u{y} -=  0.75;
    $v{x} +=   2.5;                                                 $p->lineto( $v{x}, $v{y} );
    $u{x} +=   2.5;
                                    $v{y} -= 4.9625;                $p->lineto( $v{x}, $v{y} );
                                    $u{y} -= 4.9625;
    $v{x} +=   3.1 - $kerf;                                         $p->lineto( $v{x}, $v{y} );
    $u{x} +=   3.1;
                                    $v{y} -= (4.9625 + $kerf);
                                    $u{y} -= 4.9625;
    die "y == ".$v{y}." when it should be zero"  if abs($v{y}) > 1e10;  # avoid: -0
    $v{y} = 0;
                                                                    $p->lineto( $v{x}, $v{y} );

    my $maxx = $v{x};

    # back home
    $v{x} -= 102.2 - $kerf;
    $u{x} -= 102.2;
    die "x == ".$v{x}." when it should be zero" if abs($v{x}) > 1e10;  # avoid: -0
    $v{x} = 0;
                                                                    $p->lineto( $v{x}, $v{y} );

    return $p;
}



sub notched4_ring()
{
    my $k = shift;   # Koike object

    my $xc = shift;  # coords of center
    my $yc = shift;

    my $ir = shift;  # inner radius
    my $or = shift;  # outer radius

    my $nw = shift;  # notch width
    my $nd = shift;  # notch depth
    my $kerf = shift;

    die "notch must be non-zero width in notched4_ring()"  if $nw == 0;

    my $theta = atan2( ($or + $kerf), (($nw - $kerf)/2.0) );

    # the symmetry of the object allows these number or their
    # negatives to be used for all notch coordinates
    my $notch_u = ($or + $kerf) * cos( $theta ) ;
    my $notch_v = ($or + $kerf) * sin( $theta );

    #print "theta:$theta or:$or kerf:$kerf notch_u:$notch_u notch_v:$notch_v\n";

    

    # a complication
    # our target object lives in a left-handed coordinate systems
    # whereas cos, sin, atan, etc live in a right handed system.
    #
    # It may be that an easy way to deal with this is to make a 
    # notch and an arc, then rotate the object pi/4 and repeat.
    # So don't set the center now.  Once the rotations are done
    # do a translation.


    my $p = new Koike::Part( koikeobj=>$k );

    for( my $i = 0; $i < 4; $i++ )
    {
        # initial position
        my( $x, $y );
        $x = -$notch_u; $y = $notch_v;                               if( $i == 0 ){ $p->update_position( $x, $y ); }

        # notch
                            $y -= ($nd + $kerf);                     $p->lineto( $x, $y );
        $x = $notch_u;                                               $p->lineto( $x, $y );
                            $y += ($nd + $kerf);                     $p->lineto( $x, $y );

        # arc
        ( $x, $y ) = ( $notch_v, $notch_u );                      

        $p->arcto( newx=>$x, newy=>$y, radius=>($or + $kerf), sweep=>0, largearc=>0 );

        $p->rotate( 90, 'degrees' );
    }

    # inner ring
    my $kerfed_ir = $ir - $kerf;
    $p->update_position( 0, $kerfed_ir );
    $p->arcto( newx=>0, newy=>-$kerfed_ir,  radius=>$kerfed_ir, sweep=>0, largearc=>1 );
    $p->arcto( newx=>0, newy=> $kerfed_ir,  radius=>$kerfed_ir, sweep=>0, largearc=>1 );

    if( $xc != 0 || $yc != 0 ) { $p->translate( $xc, $yc ); }

    return $p;
}
