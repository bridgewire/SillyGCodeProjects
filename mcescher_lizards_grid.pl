#!/usr/bin/perl  -I/home/anajilly/local/lib/perl/x86_64-linux-gnu-thread-multi/

# Author:  Christiana Evelyn Johnson
# Copyright (c) 2014 Reno Bridgewire
# license: The MIT License (MIT)

use warnings;
use strict;

use Koike;
use Koike::Part;
use Math::Trig ':pi';

# my $k = new Koike(protocol=>"svg", multsclr=>20, 

my $rows = 1;
my $cols = 1;

my $k = new Koike(protocol=>"gcode", moveto_color=>'rgb(0,0,255)', verbosegcode=>0, addcutting=>1, yoffset=>.52, xoffset=>-27 ); #xoffset=>200, yoffset=>200 );
$k->process_cmdlineargs( @ARGV );

# process lizard-grid specific command-line arguments
if( (my @l = grep(/--cols=[0-9]+/,  @ARGV)) ){ $l[0] =~ /--cols=([0-9.]+)/; $cols = $1; }
if( (my @l = grep(/--rows=[0-9]+/,  @ARGV)) ){ $l[0] =~ /--rows=([0-9.]+)/; $rows = $1; }

my( $p, $p2, $x, $y );
my( @c2cdown, @t2tdown, @k2kdown );


#############################   line left of column 1
$c2cdown[0] = &make_cheek2cheek_down_through_knees( $k );
if( $rows > 1 ) { $c2cdown[0] = &duplicate_and_string_end2end( $c2cdown[0], $rows-1 ); }

# YYY tack on one more 'cheeks' line to help complete one more lizard
$p = &cheeks( $k );
$p->rotate( -60, 'degrees' );
$p->reposition( $c2cdown[0]->end_coords() );
$c2cdown[0] = $c2cdown[0]->merge( $p );
# YYY

$c2cdown[0]->translate( 40, 0 );
for( my $i = 0; $i < $cols-1; $i++ )
{
    $c2cdown[$i+1] = $c2cdown[$i]->copy();
    $c2cdown[$i+1]->translate( 6*40*cos(pi/6), 0 );
}

#############################   line left of column 2
$t2tdown[0] = &make_toe2toe_down_through_cheeks( $k );
if( $rows > 1 ) { $t2tdown[0] = &duplicate_and_string_end2end( $t2tdown[0], $rows-1 ); }

# YYY tack on one more 'toes' line to help complete one more lizard
$p = &toes( $k );
$p->rotate( -60, 'degrees' );
$p->reposition( $t2tdown[0]->end_coords() );
$t2tdown[0] = $t2tdown[0]->merge( $p );
# YYY

$t2tdown[0]->translate( 40 + 2*40*cos(pi/6) , 0 );
for( my $i = 0; $i < $cols-1; $i++ )
{
    $t2tdown[$i+1] = $t2tdown[$i]->copy();
    $t2tdown[$i+1]->translate( 6*40*cos(pi/6), 0 );
}

#############################   line left of column 3
$k2kdown[0] = &make_knee2knee_down_through_toes( $k );
if( $rows > 1 ) { $k2kdown[0] = &duplicate_and_string_end2end( $k2kdown[0], $rows-1 ); }

# YYY tack on one more 'knees' line to help complete one more lizard
$p = &knees( $k );
$p->rotate( -60, 'degrees' );
$p->reposition( $k2kdown[0]->end_coords() );
$k2kdown[0] = $k2kdown[0]->merge( $p );
# YYY

$k2kdown[0]->translate( 40 + 2*2*40*cos(pi/6) , 0 );
for( my $i = 0; $i < $cols-1; $i++ )
{
    $k2kdown[$i+1] = $k2kdown[$i]->copy();
    $k2kdown[$i+1]->translate( 6*40*cos(pi/6), 0 );
}

for( my $i = 0; $i < $cols; $i++ )
{
    $k->add_part( $c2cdown[$i] );
    $p = new Koike::Part( koikeobj=>$k );
    $p->update_position( $c2cdown[$i]->end_coords() );
    $p->moveto( $t2tdown[$i]->start_coords() );
    $k->add_part( $p );

    $k->add_part( $t2tdown[$i] );
    $p = new Koike::Part( koikeobj=>$k );
    $p->update_position( $t2tdown[$i]->end_coords() );
    $p->moveto( $k2kdown[$i]->start_coords() );
    $k->add_part( $p );

    $k->add_part( $k2kdown[$i] );
    if( $cols - 1 > $i )
    {
        $p = new Koike::Part( koikeobj=>$k );
        $p->update_position( $k2kdown[$i]->end_coords() );
        $p->moveto( $c2cdown[$i+1]->start_coords() );
        $k->add_part( $p );
    }
}



#######################################################################################################
### 2 repeated lines bind cheek2cheek_through_knees to toe2tow_through_cheeks, in column 1
#######################################################################################################
for( my $i = 0; $i <= $#t2tdown; $i++ )
{
    $p = new Koike::Part( koikeobj=>$k );
    $p->moveto( $t2tdown[$i]->start_coords() );
    $k->add_part( $p );

    $p = &toes( $k );
    $p->rotate( 60, 'degrees' );
    $p->reposition( $t2tdown[$i]->start_coords() );

    ($x, $y) = $p->end_coords();

    $p2 = &knees( $k );
    $p2->rotate( -60, 'degrees' );

    $p2->reposition( $x, $y + 40 );
    #$p2->reposition( 40 + 40*cos(pi/6) + $i*6*40*cos(pi/6), 80 );
    ( $x, $y )  = $p2->end_coords();
    $p2->moveto( $x, $y+40 );

    $p = $p->merge( $p2 );

    if( $rows > 1 ) { $p = &duplicate_and_string_end2end( $p, $rows-1 ); }

    # YYY add one more to the end to complete one last lizard
    $p2 = &toes( $k );
    $p2->rotate( 60, 'degrees' );
    $p2->reposition( $p->end_coords() );
    $p = $p->merge( $p2 );
    # YYY


    $k->add_part( $p );
}
#######################################################################################################
### done with column 1
#######################################################################################################

#######################################################################################################
### 2 repeated lines bind toe2tow_through_cheeks to make_knee2knee_down_through_toes, in column 2
#######################################################################################################
for( my $i = 0; $i <= $#k2kdown; $i++ )
{
    $p = new Koike::Part( koikeobj=>$k );
    $p->moveto( $k2kdown[$i]->start_coords() );
    $k->add_part( $p );

    $p = &knees( $k );
    $p->rotate( 60, 'degrees' );
    $p->reposition( $k2kdown[$i]->start_coords() );

    ($x, $y) = $p->end_coords();

    $p2 = &cheeks( $k );
    $p2->rotate( -60, 'degrees' );
    $p2->reposition( $x, $y + 40 );
    #$p2->reposition( 40 + ($i+1)*3*40*cos(pi/6), 80 );
    ( $x, $y )  = $p2->end_coords();
    $p2->moveto( $x, $y+40 );

    $p = $p->merge( $p2 );

    if( $rows > 1 ) { $p = &duplicate_and_string_end2end( $p, $rows-1 ); }

    # YYY add one more to the end to complete one last lizard
    $p2 = &knees( $k );
    $p2->rotate( 60, 'degrees' );
    $p2->reposition( $p->end_coords() );
    $p = $p->merge( $p2 );
    # YYY

    $k->add_part( $p );
}
#######################################################################################################
### done with column 2
#######################################################################################################

#######################################################################################################
### 2 repeated lines ...
#######################################################################################################
for( my $i = 1; $i <= $#c2cdown; $i++ )
{
    $p = new Koike::Part( koikeobj=>$k );
    $p->moveto( $c2cdown[$i]->start_coords() );
    $k->add_part( $p );

    $p = &cheeks( $k );
    $p->rotate( 60, 'degrees' );
    $p->reposition( $c2cdown[$i]->start_coords() );

    ($x, $y) = $p->end_coords();

    $p2 = &toes( $k );
    $p2->rotate( -60, 'degrees' );
    $p2->reposition( $x, $y + 40 );
    ( $x, $y )  = $p2->end_coords();
    $p2->moveto( $x, $y+40 );

    $p = $p->merge( $p2 );

    if( $rows > 1 ) { $p = &duplicate_and_string_end2end( $p, $rows-1 ); }

    # YYY add one more to the end to complete one last lizard
    $p2 = &cheeks( $k );
    $p2->rotate( 60, 'degrees' );
    $p2->reposition( $p->end_coords() );
    $p = $p->merge( $p2 );
    # YYY

    $k->add_part( $p );
}
#######################################################################################################
### done with column 3
#######################################################################################################



#printf( STDERR 'bounds: xmin:%.03f, ymin:%.03f, xmax:%.03f, ymax:%.03f'."\n",  $k->get_shifted_bounds() );
$k->set_rectagular_material_bounds();

$k->printall();

# prototype for toes(), cheeks(), knees():  f( $k,  [ $start_x, $start_y, $end_x, $end_y ] )
sub toes()   { return &make_path( &basic_path_handle_args( &toes_atom(),   @_ ) ); }
sub cheeks() { return &make_path( &basic_path_handle_args( &cheeks_atom(), @_ ) ); }
sub knees()  { return &make_path( &basic_path_handle_args( &knees_atom(),  @_ ) ); }


sub make_cheek2cheek_down_through_knees()
{
    my $k = shift;
    my ($p, $p_simple);

    $p = &cheeks( $k );
    $p->rotate( -60, 'degrees' );
    $p->translate( 0, 20 );
    $p_simple = $p->copy();

    $p = &knees( $k );
    $p->rotate( 180, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p->translate( 0, 40 );
    $p_simple = $p_simple->merge( $p );

    $p = &knees( $k );
    $p->rotate( 60, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p_simple = $p_simple->merge( $p );

    $p = &cheeks( $k );
    $p->rotate( 180, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p->translate( 0, 40 );
    $p_simple = $p_simple->merge( $p );

    return $p_simple;
}


sub make_toe2toe_down_through_cheeks()
{
    my $k = shift;
    my ($p, $p_simple);

    $p = &toes( $k );
    $p->rotate( -60, 'degrees' );
    $p->translate( 0, 20 );
    $p_simple = $p->copy();

    $p = &cheeks( $k );
    $p->rotate( 180, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p->translate( 0, 40 );
    $p_simple = $p_simple->merge( $p );

    $p = &cheeks( $k );
    $p->rotate( 60, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p_simple = $p_simple->merge( $p );

    $p = &toes( $k );
    $p->rotate( 180, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p->translate( 0, 40 );
    $p_simple = $p_simple->merge( $p );

    return $p_simple;
}

sub make_knee2knee_down_through_toes()
{
    my $k = shift;
    my ($p, $p_simple);

    $p = &knees( $k );
    $p->rotate( -60, 'degrees' );
    $p->translate( 0, 20 );
    $p_simple = $p->copy();

    $p = &toes( $k );
    $p->rotate( 180, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p->translate( 0, 40 );
    $p_simple = $p_simple->merge( $p );

    $p = &toes( $k );
    $p->rotate( 60, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p_simple = $p_simple->merge( $p );

    $p = &knees( $k );
    $p->rotate( 180, 'degrees' );
    $p->reposition( $p_simple->end_coords() );
    $p->translate( 0, 40 );
    $p_simple = $p_simple->merge( $p );

    return $p_simple;
}



sub duplicate_and_string_end2end()
{
    my $p = shift;  # the part to duplicate
    my $n = shift;  # the number of copies
    my ($pc, $p_final);

    $p_final = $p->copy();
    for( my $i = 0; $i < $n; $i++ )
    {
        $pc = $p->copy();
        $pc->reposition( $p_final->end_coords() );
        $p_final = $p_final->merge( $pc );
    }
    return $p_final;
}

sub add_moveto()
{
    return;

    my $k = shift;
    my $p = new Koike::Part( koikeobj=>$k );
    $p->moveto( @_ );
    $k->add_part( $p );
}

sub cheeks_atom()
{
    return [
      # [71,80], [84,93], [84,98], [71,108], [69,108], [56,102], [54,103], [53,104], [53,110], [54,111], [55,117], [59,117], [62,113], [63,114], [63,113], [72,119]
      # [72, 79], [84,93], [84,98], [71,108], [69,108], [56,102], [54,103], [53,104], [53,110], [54,111], [55,117], [59,117], [62,113], [63,114], [63,113],
      # [72,119] # length 40

      # repositioned to zero, vertical down, length 40
        [ 0, 0 ], [ 12, 14 ], [ 12, 19 ], [ -1, 29 ], [ -3, 29 ], [ -16, 23 ], [ -18, 24 ], [ -19, 25 ], [ -19, 31 ], [ -18, 32 ], [ -17, 38 ], [ -13, 38 ], [ -10, 34 ],  [ -9, 34 ], [ 0, 40 ],
    ];
}

sub toes_atom()
{
    return [
      # [104,136], [106,136], [117,133], [117,141], [112,147], [114,150], [113,152], [110,159], [108,161], [104,161], [96,160], [91,158], [82,150], [84,156], [86,159], [88,166], [104,172]

      # [104,136], [106,136], [117,133], [117,141], [112,147], [114,150], [113,152], [110,159], [108,161], [104,161], [96,160], [91,158], [82,150], [84,156], [86,159], [88,166],
      # [104,172]  # length  36

      # [104,134], [106,136], [117,133], [117,141], [112,147], [114,150], [113,152], [110,159], [108,161], [104,161], [96,160], [91,158], [82,150], [84,156], [86,159], [88,166],
      # [104,174]  # length  40

      # repositioned to zero, vertical down, length 40
      #[ 0, 0 ], [ 2, 2 ], [ 13, -1 ], [ 13, 7 ], [ 8, 13 ], [ 10, 16 ], [ 9, 18 ], [ 6, 25 ], [ 4, 27 ], [ 0, 27 ], [ -8, 26 ], [ -13, 24 ], [ -22, 16 ], [ -20, 22 ], [ -18, 25 ], [ -16, 32 ], [ 0, 40 ], 

      # experiment with changing the shape
      [ 0, 0 ],            [ 15, -3 ], [ 13, 7 ], [ 8, 13 ], [ 10, 16 ], [ 9, 18 ], [ 6, 25 ], [ 4, 27 ], [ 0, 27 ], [ -8, 26 ], [ -13, 24 ], [ -22, 16 ], [ -20, 22 ], [ -18, 25 ], [ -16, 32 ], [ 0, 40 ], 
    ];
}

sub knees_atom()
{
    return [
      # [136,80], [139,81], [150,84], [160,84], [159,87], [159,90], [158,91], [157,94], [156,96], [138,92], [136,92], [134,103], [129,110], [133,119]

      # [134, 80], [139,81], [150,84], [160,84], [159,87], [159,90], [158,91], [157,94], [156,96], [138,92], [136,92], [134,103], [129,110], 
      # [134,120] # length 40

      # repositioned to zero, vertical down, length 40
      [ 0, 0 ], [ 5, 1 ], [ 16, 4 ], [ 26, 4 ], [ 25, 7 ], [ 25, 10 ], [ 24, 11 ], [ 23, 14 ], [ 22, 16 ], [ 4, 12 ], [ 2, 12 ], [ 0, 23 ], [ -5, 30 ], [ 0, 40 ],
    ];
}

sub basic_path_handle_args()
{
    my $path = shift;
    my $k = shift;
    my $start_x = shift || ${$path}[0][0];
    my $start_y = shift || ${$path}[0][1];
    my $end_x   = shift || ${$path}[ $#{$path} ][0];
    my $end_y   = shift || ${$path}[ $#{$path} ][1];

    return  ( $k, $path, $start_x, $start_y, $end_x, $end_y );
}

sub make_path()
{
    my $k = shift;
    my $points  = shift;
    my $pointcount = $#{$points} + 1;

    # place the first and last elements of the @points array into a local var.
    # $ntv{sx},$ntv{sy} is the starting pair and $ntv{ex},$ntv{ey} is the end.
    my %ntv = ( sx=>${$points}[0][0], sy=>${$points}[0][1], ex=>${$points}[ $pointcount-1 ][0], ey=>${$points}[ $pointcount-1 ][1] );

    # these points are the default start and stop points
    my $start_x = shift || $ntv{sx};
    my $start_y = shift || $ntv{sy};
    my $end_x   = shift || $ntv{ex};
    my $end_y   = shift || $ntv{ey};


    # create the default part. will transform later if necessary.
    my $p = new Koike::Part( koikeobj=>$k );
    my $i = 0;
    foreach my $z ( @{$points} )
    {
        if( $i == 0 ){ $p->update_position( ${$z}[0], ${$z}[1] ); $p->mark_start(); }
        else         { $p->lineto( ${$z}[0], ${$z}[1] );  }
        $i++;
    }

    # now, translate if necessary
    if( $start_x != $ntv{sx} || $start_y != $ntv{sy} || $end_x != $ntv{ex} || $end_y != $ntv{ey} )
    {
        my ( $ux, $uy ) = ( $ntv{ex} - $ntv{sx}, $ntv{ey} - $ntv{sy} );
        my $u_len   = sqrt( $ux**2 + $uy**2 );    # this is floating point
        my $u_angle = atan2( $uy, $ux );          # this is a positive angle

        my ( $vx, $vy ) = ($end_x - $start_x, $end_y - $start_y);
        my $v_len   = sqrt( $vx**2 + $vy**2 );
        my $v_angle = atan2( $vy, $vx );
        if( $v_angle < 0 ) { $v_angle += pi2; }  # make sure this is positive
        
        $p->translate( -$ntv{sx}, -$ntv{sy} );

        $p->rotate( $v_angle - $u_angle );
        $p->scale ( $v_len / $u_len );

        $p->translate( $start_x, $start_y );
    }

    #$p->mark_end();

    return $p;
}


