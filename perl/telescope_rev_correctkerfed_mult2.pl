#!/usr/bin/perl

use warnings;
use strict;

use Math::Trig qw(:pi);

use Koike;
use Koike::Part;

my( $outfile, $cntperrow, $rowcnt);
my $dosvg = grep(/^--svg$/, @ARGV);
#my $kerf = 0.175; # mm
my $kerf = 0.185; # mm
if( (my @l = grep(/--out=.+?\.html$/, @ARGV)) ){ $l[0] =~ /--out=(.+?\.html)$/; $outfile = $1; }
if( (my @l = grep(/--perrow=[0-9]+$/, @ARGV)) )  { $l[0] =~ /--perrow=([0-9]+)$/; $cntperrow = $1; }
if( (my @l = grep(/--rowcount=[0-9]+$/, @ARGV)) )  { $l[0] =~ /--rowcount=([0-9]+)$/; $rowcnt = $1; }
if( (my @l = grep(/--kerf=[.0-9]+$/, @ARGV)) )  { $l[0] =~ /--kerf=([.0-9]+)$/; $kerf = $1; }

#print STDERR "args -- dosvg:'".($dosvg?"true":"false")."' outfile:'$outfile' cntperrow:'$cntperrow' rowcnt:'$rowcnt' kerf:'$kerf'\n";

# $k = new Koike(multsclr=>20, xoffset=>20, yoffset=>20, moveto_color=>'rgb(0,0,255)');
my $k = new Koike();
$k->process_cmdlineargs( @ARGV );



################################################################################################################  spars
my $p = &make_scope_spar($k, 0, 0, $kerf);
$k->add_part($p); 

my %bb = $p->get_bounding_box();
my $xsize = abs($bb{xmax} - $bb{xmin});
my $ysize = abs($bb{ymax} - $bb{ymin});

my $p1 = $p->copy();
$p1->rotate(180,'degrees');
$p1->translate( $xsize, 1.5 * $ysize );
$k->add_part($p1); 

my $p2 = $p->copy();
$p2->translate( 0, 1.8 * $ysize );
$k->add_part($p2); 

my $p3 = $p1->copy();
$p3->translate( 0 , 1.8 * $ysize );
$k->add_part($p3); 
################################################################################################################  spars


################################################################################################################  rings
my ( $outer_radius, $inner_radius, $center_x, $center_y, $notch_wid, $notch_loc, $notch_dpt );

# end stop ring
( $outer_radius, $inner_radius ) = ( (1.5 * 25.4)/2, 25.4/2 );                       # 1.5inch outer and 1inch inner radii
( $center_x, $center_y ) = ( $outer_radius + 1, 3.5 * $ysize + $outer_radius + 2 );  # where on the material to from
( $notch_wid, $notch_dpt ) = ( 3.1, 2 );                                             # 3.1mm is material thickness;  2mm is counterpart
$k->add_part( &notched4_ring( $k, $center_x, $center_y, $inner_radius, $outer_radius, $notch_wid, $notch_dpt, $kerf ) );


# interior stop ring
( $outer_radius, $inner_radius ) = ( (1.5 * 25.4)/2, 25.4/2 );
( $center_x, $center_y ) = ( 3*($outer_radius + 1), 3.5 * $ysize + $outer_radius + 2 );
( $notch_wid, $notch_dpt ) = ( 3.1, 3.0 );  #  2.375 );
$k->add_part( &notched4_ring( $k, $center_x, $center_y, $inner_radius, $outer_radius, $notch_wid, $notch_dpt, $kerf ) );

# eye piece stop ring
( $outer_radius, $inner_radius ) = ( (1.5 * 25.4)/2 + 5.5, 7.9 );
( $center_x, $center_y ) = ( 4.3*$outer_radius + 1, 3.5 * $ysize + $outer_radius + 2 );
( $notch_wid, $notch_dpt, $notch_loc ) = ( 3.1, 11, $inner_radius + 0.6);
#$k->add_part( &notched4_ring( $k, $center_x, $center_y, $inner_radius, $outer_radius, $notch_wid, $notch_dpt, $kerf ) );
$k->add_part( &notch_holed_4_ring( $k, $center_x, $center_y, $inner_radius, $outer_radius, $notch_wid, $notch_dpt, $notch_loc, $kerf ) );



################################################################################################################  rings


$k->printall( $outfile );


sub make_scope_spar()
{
    my $k = shift;
    my $startx = shift;
    my $starty = shift;
    my $kerf   = shift;

    $k->update_position( $startx, $starty );

    my $p = new Koike::Part( koikeobj=>$k );
    my( %u, %v );
    $u{x} = $v{x} = $startx;
    $u{y} = $v{y} = $starty;


    # beginning nub before end stop
                                    $v{y} += 4 + $kerf;            $p->lineto( $v{x}, $v{y} );
                                    $u{y} += 4 ;
    $v{x} +=  3.1 + 2*$kerf;                                       $p->lineto( $v{x}, $v{y} );
    $u{x} +=  3.1;

    # notch for end stop
                                    $v{y} -= 2;                     $p->lineto( $v{x}, $v{y} );
                                    $u{y} -= 2;
    $v{x} +=  3.1 - 2*$kerf;                                        $p->lineto( $v{x}, $v{y} );
    $u{x} +=  3.1;
                                    $v{y} += 2;                     $p->lineto( $v{x}, $v{y} );
                                    $u{y} += 2;
    print STDERR "end stop position: ".$u{x}."\n";

    # travel to field lens location
    $v{x} +=  29.2 + 2*$kerf;                                       $p->lineto( $v{x}, $v{y} );

    # field lens
                                    $v{y} -=  1.5;                  $p->lineto( $v{x}, $v{y} );
                                    $u{y} -=  1.5;
    $v{x} +=  1.3 - 2*$kerf;                                        $p->lineto( $v{x}, $v{y} );
    $u{x} +=  1.3;
    printf STDERR "field lens position: (%.04f, %.04f)\n ", $u{x}, $u{y} ;
                                    $v{y} +=  1.5;                  $p->lineto( $v{x}, $v{y} );
                                    $u{y} +=  1.5;


    # slant from field lens to interior stop
    $v{x} += 24.50 + 2*$kerf;       $v{y} +=  2.291;                $p->lineto( $v{x}, $v{y} );
    $u{x} += 24.50;                 $u{y} +=  2.291;


    # interior stop
    printf STDERR "interior stop position: %.04f\n", $u{x};
                                    $v{y} -=  3.0;                 $p->lineto( $v{x}, $v{y} );
                                    $u{y} -=  3.0;
    $v{x} +=  3.1 - 2*$kerf;                                        $p->lineto( $v{x}, $v{y} );
    $u{x} +=  3.1;
                                    $v{y} +=  3.0;                 $p->lineto( $v{x}, $v{y} );
                                    $u{y} +=  3.0;

    # slope from interior stop to eye piece level
    $v{x} += 33.80;                 $v{y} +=  6.134;                $p->lineto( $v{x}, $v{y} );
    $u{x} += 33.80;                 $u{y} +=  6.134;
    # flat space before lens
    $v{x} +=   0.5  + 2*$kerf;                                      $p->lineto( $v{x}, $v{y} );
    $u{x} +=   0.5;

    my $maxy = $v{y};

    # the eye piece
                                    $v{y} -=  1.25;                 $p->lineto( $v{x}, $v{y} );
                                    $u{y} -=  1.25;

    $v{x} +=   2.5 - 2*$kerf;                                       $p->lineto( $v{x}, $v{y} );
    $u{x} +=   2.5;

    printf STDERR "position of the center of the eye piece: (%.04f, %.04f)\n", ($u{x} - 2.5/2), $u{y} ;

    #                               $v{y} += 0.2;                  $p->lineto( $v{x}, $v{y} );
    #                               $u{y} += 0.2;
    $v{x} +=   3.1 + $kerf;                                          $p->lineto( $v{x}, $v{y} );
    $u{x} +=   3.1;
    #                               $v{y} -= (10.5 + $kerf);
    #                               $u{y} -=  10.5 ;
                                    $v{y} -= (10.3 + $kerf);
                                    $u{y} -=  10.3 ;
    die "y == ".$v{y}." when it should be zero"  if abs($v{y}) > 1e10;  # avoid: -0
    $v{y} = 0;
                                                                    $p->lineto( $v{x}, $v{y} );

    my $maxx = $v{x};

    # back home
    $v{x} -= (105.3) - $kerf;
    $u{x} -= (105.3);
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

    # the symmetry of the object allows these numbers and their
    # negatives to be used for all notch coordinates
    my $notch_u = ($or - $kerf) * cos( $theta ) ;
    my $notch_v = ($or - $kerf) * sin( $theta );

    #print "theta:$theta or:$or kerf:$kerf notch_u:$notch_u notch_v:$notch_v\n";

    

    # a complication
    # our target object lives in a left-handed coordinate systems whereas cos,
    # sin, atan, etc live in a right handed system.
    #
    # The easy way to deal with this is probably to make a notch and an arc,
    # then rotate the object pi/4 and repeat.  So don't set the center now.
    # Once the rotations are done do a translation.

    my $p = new Koike::Part( koikeobj=>$k, cut_color=>'rgb(0,255,0)' );

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

#####################################   this... doesn't work. it's a place holder.  the code within is a copy of notched4_ring()
sub notch_holed_4_ring()
{
    my $k = shift;   # Koike object

    my $xc = shift;  # coords of center
    my $yc = shift;

    my $ir = shift;  # inner radius
    my $or = shift;  # outer radius

    my $nw = shift;  # notch width
    my $nd = shift;  # notch depth
    my $nr = shift;  # notch inner radius position

    my $kerf = shift;

    die "notch must be non-zero width in notched4_ring()"  if $nw == 0;


    my $p = new Koike::Part( koikeobj=>$k, cut_color=>'rgb(0,0,255)' );

    # make inner and outer circles first
    $p->update_position( 0, ($ir - $kerf) );
    $p->arcto( newx=>0, newy=>-($ir - $kerf), radius=>($ir - $kerf), sweep=>0, largearc=>0 );
    $p->arcto( newx=>0, newy=> ($ir - $kerf), radius=>($ir - $kerf), sweep=>0, largearc=>0 );

    $p->update_position( 0, ($or + $kerf) );
    $p->arcto( newx=>0, newy=>-($or + $kerf), radius=>($or + $kerf), sweep=>0, largearc=>0 );
    $p->arcto( newx=>0, newy=> ($or + $kerf), radius=>($or + $kerf), sweep=>0, largearc=>0 );

    my( $x, $y );
    for( my $i = 0; $i < 4; $i++ )
    {
        # initial position
        ( $x, $y ) = ( $nw/2.0 - $kerf, $nr + $kerf );          $p->update_position( $x, $y );
        $x -= ($nw - $kerf);                                    $p->lineto( $x, $y );
                            $y += ($nd - $kerf);                $p->lineto( $x, $y );
        $x += ($nw - $kerf);                                    $p->lineto( $x, $y );
                            $y -= ($nd - $kerf);                $p->lineto( $x, $y );

        $p->rotate( 90, 'degrees' );
    }

    if( $xc != 0 || $yc != 0 ) { $p->translate( $xc, $yc ); }

    return $p;
}
