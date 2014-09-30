#!/usr/bin/perl

use warnings;
use strict;

use Koike;
use Koike::Part;
use Math::Trig;

# my $k = new Koike(protocol=>"svg", multsclr=>20, 
my $k = new Koike(protocol=>"gcode",
        moveto_color=>'rgb(0,0,255)', verbosegcode=>1, addcutting=>1 );

$k->process_cmdlineargs( @ARGV );

if( $k->get_protocol() eq 'svg' && $k->get_scale_multiplier() == 1 ) { $k->set_scale_multiplier( 20 ); }


my $sl = 2;  ## length of hexagon side in inches, so distance between parallel sides is: 2*cos(pi/6)*$sl
             ## and the distance between opposite points (the diameter) is ($sl + 2*$sl*sin(pi/6)) == $sl*(1 + 2*sin(pi/6))
             ## because of how they're arranged, only the first row has the its diameter added in whole to find the overall heigh
my $hexperrow = 10;
my $rowcnt = 2;
my $nested_count = 3;
my $nested_spacing = .2;  # this is the fraction of length between the outer hexagon wall and its center
                          # this means that this relationship must hold: 1/$nested_count >= $nested_spacing 

# process hexagon-grid specific command-line arguments
if( (my @l = grep(/--side-length=[0-9.]+/,  @ARGV)) ){ $l[0] =~ /--side-length=([0-9.]+)/; $sl = $1; }
if( (my @l = grep(/--cols=[0-9]+/,  @ARGV)) ){ $l[0] =~ /--cols=([0-9.]+)/; $hexperrow = $1; }
if( (my @l = grep(/--rows=[0-9]+/,  @ARGV)) ){ $l[0] =~ /--rows=([0-9.]+)/; $rowcnt = $1; }
if( (my @l = grep(/--nested=[0-9]+/,  @ARGV)) ){ $l[0] =~ /--nested=([0-9.]+)/; $nested_count = $1; }
if( (my @l = grep(/--nested-spacing=\.[0-9]+/,  @ARGV)) ){ $l[0] =~ /--nested-spacing=(\.[0-9]+)/; $nested_spacing = $1; }

die "nested-spacing must be less than or equal to 1/nested" if( (1.0/$nested_count) < $nested_spacing );

$k->update_position(0,0);

$k->set_rectagular_material_bounds(
        0,
        0,    # where does this one really come from?  this is an anomoly from 
        ($hexperrow + .5)*2*cos(Math::Trig::pi()/6)*$sl,
        ($sl + 2 * $sl*sin(Math::Trig::pi()/6)) + ($rowcnt-1)*($sl + $sl*sin(Math::Trig::pi()/6))
); 


my $h = &hexagon_grid_firstrow( $k, $hexperrow, 0, 0, (1 - $nested_spacing), $nested_count );
my $bigpart;

if( $rowcnt != 1 )
{
    my $evn_g = &hexagon_grid_secondrow( $k, $hexperrow, 0, 0, (1 - $nested_spacing), $nested_count, 0 );
    my $odd_g;
    if( $rowcnt > 2 )
    {
        $odd_g = &hexagon_grid_secondrow( $k, $hexperrow, 0, 0, (1 - $nested_spacing), $nested_count, 1 );
        $odd_g->translate( -($sl * sqrt(3))/2.0, ($sl * 1.5) );
    }
    
    my $g = $evn_g->copy();
    $bigpart = $h->merge( $g );
    
    my $m;
    for( my $i = 0; $i < $rowcnt - 2; $i++ )
    {
        if( $i % 2 == 0 )
        {
            # in one-indexed, this is odd... ;)
            if( $i ) { $odd_g->translate( 0, 3*$sl ); }
            $m = $odd_g->copy();
        }
        else
        {
            $evn_g->translate( 0, 3*$sl );
            $m = $evn_g->copy();
        }

        $bigpart = $bigpart->merge( $m );
    }
}
else
{
    $bigpart = $h;
}

$k->add_part($bigpart);

$k->printall();

sub make_inner_hexagon()
{
    my $k = shift;
    my $is_primary = shift;
    my $inner_fraction = shift;
    my $nth_inner = shift;

    my $fracDistFrom1 = 1 - $inner_fraction ;

    my $actual_innerfrac = 1 - ($nth_inner * $fracDistFrom1 );

    my ($trans_x, $trans_y) = ( $sl * sqrt(3) * (1 + (1 - $actual_innerfrac) / 2.0),
                                $sl           * (1 + (1 - $actual_innerfrac) / 2.0) );
    if( ! $is_primary ) { $trans_x = ( $sl * sqrt(3) * (1 - $actual_innerfrac) )/ 2.0; } 

    my $g = new Koike::Part( koikeobj=>$k );
    &hexagon( $g, $sl * $actual_innerfrac );
    $g->translate( $trans_x, $trans_y );

    return $g;
}


sub hexagon_grid_secondrow()
{
    my $k = shift;
    my $count = shift;
    my $strtx = shift;
    my $strty = shift;
    my $inner_fraction = shift;
    my $inner_count = shift;
    my $is_odd = shift;

    my ( $h, $h2, $g );

    $h = new Koike::Part( koikeobj=>$k );
    &hexagon( $h, $sl, [5] );
    #&hexagon( $h, $sl );
    $h->translate( ($sl * sqrt(3)), $sl );

    # inner hexagons
    for( my $i = 0; $i < $inner_count; $i++ )
    {
        $h = $h->merge( ($g = &make_inner_hexagon($k, 1, $inner_fraction, 1+$i)) );
    }

    for( my $j = 0; $j < $count-1; $j++ )
    {
        $k->update_position( $strtx, $strty );
        $h2 = new Koike::Part( koikeobj=>$k );

        if( $is_odd &&  $j == $count-2 ) { &hexagon( $h2, $sl, [3,4  ] ); }
        else                             { &hexagon( $h2, $sl, [3,4,5] ); }

        $h2->translate( 0, $sl );

        # inner hexagons
        for( my $i = 0; $i < $inner_count; $i++ )
        {
            $h2 = $h2->merge( ($g = &make_inner_hexagon($k, 0, $inner_fraction, $i+1)) );
        }

        $h = $h->merge( $h2 );

        $h->translate( ($sl * sqrt(3)), 0 );
    }


    $h->translate( -($sl * sqrt(3))/2, ($sl * 1) );

    $h->mark_end();

    return $h;
}

sub hexagon_grid_firstrow()
{
    my $k = shift;
    my $count = shift;
    my $strtx = shift;
    my $strty = shift;
    my $inner_fraction = shift;
    my $inner_count = shift;

    my ( $h, $h2, $g );

    $h = new Koike::Part( koikeobj=>$k );

    $h->mark_start();

    # first hexagon in the row is slightly different from the rest.
    &hexagon( $h, $sl );
    $h->translate( ($sl * sqrt(3)), $sl );

    # inner hexagons
    for( my $i = 0; $i < $inner_count; $i++ )
    {
        $h = $h->merge( ($g = &make_inner_hexagon($k, 1, $inner_fraction, $i+1)) );
    }

    # genrate remaining hexagons
    for( my $j = 0; $j < $count-1; $j++ )
    {
        $k->update_position( $strtx, $strty );
        $h2 = new Koike::Part( koikeobj=>$k );
        &hexagon( $h2, $sl, [3] );
        $h2->translate( 0, $sl );

        # inner hexagons
        for( my $i = 0; $i < $inner_count; $i++ )
        {
            $h2 = $h2->merge( ($g = &make_inner_hexagon($k, 0, $inner_fraction, $i+1)) );
        }

        $h = $h->merge( $h2 );

        $h->translate( ($sl * sqrt(3)), 0 );
    }

    $h->translate( -($sl * sqrt(3)), -$sl/2.0 );

    $h->mark_end();

    return $h;
}

sub hexagon()
{
    my $p = shift;
    my $sidelen = shift;
    my $side2skip = shift;

    $p->mark_start();

    for( my $i = 0; $i < 6; $i++ )
    {
        if( defined($side2skip) && grep( /^$i$/, @$side2skip ) )
        {
            $p->moveto(0, $sidelen);
        }
        else
        {
            $p->lineto(0, $sidelen);
        }

        $p->translate(0, -$sidelen);  # move the shape by y=-$sidelen.  MORE IMPORTANTLY
                                      # we move the drawing tip (the current loc) to starting place.
        $p->rotate(60, "degrees");    # rotate everything through half of one interior angle
    }

    #$p->mark_end();
}
