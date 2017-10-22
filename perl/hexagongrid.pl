#!/usr/bin/perl

use warnings;
use strict;

use Koike;
use Koike::Part;
use Math::Trig;



# my $k = new Koike(protocol=>"svg", multsclr=>20, 
my $k = new Koike(protocol=>"gcode", # verbosegcode=>1, 
        moveto_color=>'rgb(0,0,255)', addcutting=>1, includehtml=>0 );


$k->process_cmdlineargs( @ARGV );


my $sl = 2;  ## length of hexagon side in inches, so distance between parallel sides is: 2*cos(pi/6)*$sl
             ## and the distance between opposite points (the diameter) is ($sl + 2*$sl*sin(pi/6)) == $sl*(1 + 2*sin(pi/6))
             ## because of how they're arranged, only the first row has the its diameter added in whole to find the overall heigh
my $colcnt = 10;
my $rowcnt = 2;
my $nested_count = 3;
my $nested_spacing = .2;  # this is the fraction of length between the outer hexagon wall and its center
                          # this means that this relationship must hold: 1/$nested_count >= $nested_spacing 
my $supress_grid = 0;     # supressing the grid means we only cut inner hexagons.

# process hexagon-grid specific command-line arguments
if( (my @l = grep(/--side-length=[0-9.]+/,  @ARGV)) ){ $l[0] =~ /--side-length=([0-9.]+)/; $sl = $1; }
if( (my @l = grep(/--cols=[0-9]+/,  @ARGV)) ){ $l[0] =~ /--cols=([0-9.]+)/; $colcnt = $1; }
if( (my @l = grep(/--rows=[0-9]+/,  @ARGV)) ){ $l[0] =~ /--rows=([0-9.]+)/; $rowcnt = $1; }
if( (my @l = grep(/--nested=[0-9]+/,  @ARGV)) ){ $l[0] =~ /--nested=([0-9.]+)/; $nested_count = $1; }
if( (my @l = grep(/--nested-spacing=0?\.[0-9]+/,  @ARGV)) ){ $l[0] =~ /--nested-spacing=(0?\.[0-9]+)/; $nested_spacing = $1; }
if( (my @l = grep(/--supress-grid(=[01])?/,  @ARGV)) ){ if($l[0] =~ /--supress-grid=([01])/){ $supress_grid = $1; }else{ $supress_grid = 1; }  }

if( $nested_count != 0 )
{
    die "nested-spacing must be less than or equal to 1/nested" if( (1.0/$nested_count) < $nested_spacing );
}

$k->update_position(0,0);

my @parts = ();

my $m = $k->new_part();
$m->mark_start();
$m->dump_part("m");
push( @parts, $m );

############################################################################################################### Row #1
my ( $x_ref, $y_ref ) = ( 0, $sl/2.0 );
for( my $i = $nested_count; $i > 0; $i-- ) { push( @parts, &make_inner_hexagon( $k, $sl, $nested_spacing, $i, $x_ref, $y_ref ) ); }
if( ! $supress_grid ){ push( @parts, &hexagon( $k, undef, $sl, $x_ref, $y_ref ) ); }

for( my $j = 1; $j < $colcnt; $j++ )
{
    $x_ref = $j*sqrt(3)*$sl;
    for( my $i = $nested_count; $i > 0; $i-- ) { push( @parts, &make_inner_hexagon( $k, $sl, $nested_spacing, $i, $x_ref, $y_ref ) ); }
    if( ! $supress_grid ){ push( @parts, &hexagon( $k, undef, $sl, $x_ref, $y_ref, [5] ) ); }
}
############################################################################################################### End of Row #1


############################################################################################################### remaining rows
for( my $row = 2; $row <= $rowcnt; $row++ )
{
    my $row_iseven  = ! ($row % 2);
    $x_ref          = ( $row_iseven ? sqrt(3)*$sl/2.0 : 0);
    my $xoffset     = ( $row_iseven ? 0.5 : 0);
    $y_ref         += 1.5*$sl;
    for( my $i = $nested_count; $i > 0; $i-- ) { push( @parts, &make_inner_hexagon( $k, $sl, $nested_spacing, $i, $x_ref, $y_ref ) ); }

    if( ! $supress_grid )
    { 
        if( $row_iseven ) { push( @parts, &hexagon( $k, undef, $sl, $x_ref, $y_ref, [0,1] ) ); }
        else              { push( @parts, &hexagon( $k, undef, $sl, $x_ref, $y_ref,   [1] ) );  }
    }
    

    for( my $j = 1; $j < $colcnt; $j++ )
    {
        $x_ref = ($j + $xoffset)*sqrt(3)*$sl;
        for( my $i = $nested_count; $i > 0; $i-- ) { push( @parts, &make_inner_hexagon( $k, $sl, $nested_spacing, $i, $x_ref, $y_ref ) ); }
        if( ! $supress_grid )
        { 
            if( $row_iseven )
            {
                if( $j < $colcnt-1 ) { push( @parts, &hexagon( $k, undef, $sl, $x_ref, $y_ref, [0,1,5] ) ); }
                else                 { push( @parts, &hexagon( $k, undef, $sl, $x_ref, $y_ref, [0,  5] ) ); }
            }
            else
            {
                if( $j == 0 )        { push( @parts, &hexagon( $k, undef, $sl, $x_ref, $y_ref, [  1  ] ) ); }
                else                 { push( @parts, &hexagon( $k, undef, $sl, $x_ref, $y_ref, [0,1,5] ) ); }
            }
        }
    }
}
############################################################################################################### End of rows generation

# nz == "not zero", useful for avoiding division by zero
sub nz() { my $u = shift; return( $u == 0 ? .00000001 : $u ); }

my $bigpart = undef;
foreach my $p (@parts) { $bigpart = ( ! defined($bigpart) ? $p : $bigpart->merge($p) ); }

my $nonlinmat = 
    [[ sub{ my $x = shift; my $y = shift; return cos(3*(sqrt($x**2 + $y**2))); }, sub {0;} ],
     [ sub {0;}, sub{ my $x = shift; my $y = shift; return cos(3*(sqrt($x**2 + $y**2))); } ]];

#my $nonlinoffset = [
#   sub{ my $x = shift; my $y = shift; return cos(sqrt($x**2 + $y**2)); },
#   sub{ my $x = shift; my $y = shift; return cos(sqrt($x**2 + $y**2)); }
#];


# zero out left and top locations
my %bb = $bigpart->get_bounding_box();
if( $bb{xmin} != 0 || $bb{ymin} != 0 ) { $bigpart->translate( -1*$bb{xmin}, -1*$bb{ymin} ); }

# now shift part so that its center is located at 0,0. this will make tranform symetric around center.
%bb = $bigpart->get_bounding_box();
$bigpart->translate( (-1/2.0)*($bb{xmax} - $bb{xmin}), (-1/2.0)*($bb{ymax} - $bb{ymin}) );


$bigpart->nonlinear_transform( $nonlinmat, undef );
#$bigpart->nonlinear_transform( undef, $nonlinoffset  );

%bb = $bigpart->get_bounding_box();
# zero out left and top locations again.
if( $bb{xmin} != 0 || $bb{ymin} != 0 ) { $bigpart->translate( -1*$bb{xmin}, -1*$bb{ymin} ); }

$k->add_part($bigpart);

$k->set_rectagular_material_bounds();

$k->printall();


sub conspacecoords()
{
    my $k = shift;
    my $outer_sidelen  = shift;
    my $spacing_ratio  = shift;
    my $nth_from_outer = shift;
    my $x_start = shift;
    my $y_start = shift;

    # starting from the standard upper left-hand corner of vertically oriented hexagon...
    # we find coordinates on the line between that point and the center, a piont that is
    # a distance of ($spacing_ratio * $nth_from_outer * $outer_sidelen) along that line
    # toward the center. this line is one side of two equilateral triangles where the
    # easily visible sides of the other two triangles are the two edges of the hexagon
    # attached to the line.

    my $hypot = ($spacing_ratio * $nth_from_outer * $outer_sidelen);
    my $x = $x_start + $hypot * sin( pi/3 );
    my $y = $y_start + $hypot * cos( pi/3 );

    return ( $x, $y, $outer_sidelen - $hypot );
}




sub make_inner_hexagon()
{
    my $k = shift;
    my $main_sidelen = shift;
    my $inner_fraction = shift;
    my $nth_inner = shift;
    my $x_reference = shift;
    my $y_reference = shift;
    my $start_mark  = shift;

    if( ! defined( $x_reference ) ) { ($x_reference, $y_reference ) = $k->get_current_position(); }
    if( ! defined( $start_mark)   ) { $start_mark = 0; }

    my ( $x, $y, $sidelen ) = &conspacecoords( $k, $main_sidelen, $inner_fraction, $nth_inner, $x_reference, $y_reference );
    return &hexagon( $k, undef, $sidelen, $x, $y, undef, $start_mark );
}

sub hexagon()
{
    my $k = shift;
    my $p = shift || $k->new_part();
    my $sidelen = shift;  $sidelen = abs($sidelen);
    my $x_start = shift;
    my $y_start = shift;
    my $sides2skip = shift;
    my $start_mark = shift;

    $p->update_position( $x_start, $y_start );
    if( defined($start_mark) && $start_mark ) { $p->mark_start(); }

    # orientation: left-right symmetry around the vertical center line through two vertices
    my $triangle_longside  = $sidelen * cos(pi/6);
    my $triangle_shortside = $sidelen * sin(pi/6);
    my @path = (
            [    $triangle_longside,    -$triangle_shortside],
            [    $triangle_longside,     $triangle_shortside],
            [                     0,     $sidelen],
            [   -$triangle_longside,     $triangle_shortside ],
            [   -$triangle_longside,    -$triangle_shortside ],
            [                     0,    -$sidelen] );

    my ($x, $y) = ( $x_start, $y_start );
        
    for( my $i = 0; $i < 6; $i++ )
    {
        my ( $dx, $dy ) = ( ${$path[$i]}[0], ${$path[$i]}[1] );

        $x += $dx;
        $y += $dy;

        if( defined($sides2skip) && grep( /^$i$/, @$sides2skip ) )
        {
            $p->moveto($x, $y);
        }
        else
        {
            $p->lineto($x, $y);
        }

        $k->print_debug( 2, sprintf('hexagon()        : %.03f,%.03f via: %.03f,%.03f', $x, $y, $dx, $dy) );
    }

    return $p;
}


