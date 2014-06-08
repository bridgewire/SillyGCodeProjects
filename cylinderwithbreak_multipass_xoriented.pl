#!/usr/bin/perl

use Koike;
use Koike::Part;

my $outfile = undef;
my $cntperrow = 0;
my $rowcnt = 1;
my $dosvg = grep(/^--svg$/, @ARGV);
if( (my @l = grep(/--out=.+?\.gcode$/, @ARGV)) ){ $l[0] =~ /--out=(.+?\.gcode)$/; $outfile = $1; }
if( (my @l = grep(/--perrow=[0-9]+$/, @ARGV)) )  { $l[0] =~ /--perrow=([0-9]+)$/; $cntperrow = $1; }
if( (my @l = grep(/--rowcount=[0-9]+$/, @ARGV)) )  { $l[0] =~ /--rowcount=([0-9]+)$/; $rowcnt = $1; }

die "argument --perrow=<int> required" if $cntperrow == 0 ;


# $k = new Koike(multsclr=>20, xoffset=>20, yoffset=>20, moveto_color=>'rgb(0,0,255)');
$k = ($dosvg ? new Koike( xoffset=>50, yoffset=>5 ) : new Koike());

$k->process_cmdlineargs( @ARGV );

# the Koike always starts out at coordinates 0,0 
# we want the cutting head to be near us initially and want it to move
# toward the work in the y direction, which is in the negative y direction,
# and away from the operator in the x direction, which is also negative.
$k->update_position(0,0);

my $safe_y = 1;

# for now we're cutting a specific cylinder.  later this will maybe be generalized
my $kerf = 0.07;        # the hot wire produces a kerf of between .1 and .2 inches
my $inner_radius = 1.64 / 2 - $kerf/2 ; # 1.64 inches.  Koike is setup for English Units.
my $outer_radius = 1 + $kerf/2;         # 2 inches
my $interior_offset = 0.16;
my $zerotoyoffset_x = $interior_offset + $outer_radius;


my $x = 0;
my $minx = 0;
my $miny = (-$interior_offset - 2*$outer_radius);  # the is the smallest y while in the loop

for( my $row = 0; $row < $rowcnt; $row++ )
{

for( my $i = 0; $i < $cntperrow; $i++ )
{
   # initial motions
   $p = new Koike::Part( koikeobj=>$k );
   $p->moveto($x, $safe_y);
   $x -= $zerotoyoffset_x;
   $p->moveto($x, $safe_y);
   $p->moveto($x, 0.5 * $safe_y);
   $p->lineto($x, -$interior_offset);   # the last motion is not cutting but we wan slow movement.
   $k->add_part($p); 

   # gap connecting inner and outer
   $p = new Koike::Part( koikeobj=>$k );
   $p->lineto($x, -$interior_offset - ($outer_radius - $inner_radius));
   $k->add_part($p); 

   # inner circle first
   $p = new Koike::Part( koikeobj=>$k );
   $p->arcto(newy=>(-$interior_offset - ($outer_radius + $inner_radius)), newx=>$x, radius=>$inner_radius, sweep=>1, largearc=>1); 
   $p->arcto(newy=>(-$interior_offset - ($outer_radius - $inner_radius)), newx=>$x, radius=>$inner_radius, sweep=>1, largearc=>1); 
   $k->add_part($p); 

   # retrace gap from inner back to outer circles
   $p = new Koike::Part( koikeobj=>$k );
   $p->moveto( $x, -$interior_offset );
   $k->add_part($p); 

   # now the outer circle
   $p = new Koike::Part( koikeobj=>$k );
   $p->arcto(newy=>(-$interior_offset - 2*$outer_radius), newx=>$x, radius=>$outer_radius, sweep=>1, largearc=>1); 
   $p->arcto(newy=> -$interior_offset                   , newx=>$x, radius=>$outer_radius, sweep=>1, largearc=>1); 
   $k->add_part($p); 


   # retrace gap from inner back to outer circles
   $p = new Koike::Part( koikeobj=>$k );
   $p->moveto( $x, $safe_y );
   $k->add_part($p); 

   # this is the new x zero-point for the next ring
   $x -= $zerotoyoffset_x;

   # for each row, the change in x is monotone, so min changes everytime x changes
   $minx = $x;
}

}

# move back to the original zero position
$p = new Koike::Part( koikeobj=>$k );
$p->moveto( 0, $safe_y );
$p->moveto( 0, 0 );
$k->add_part($p); 

# now cut off the remnant so that we might make another pass
$miny -= $interior_offset;
$p = new Koike::Part( koikeobj=>$k );
$p->moveto( $interior_offset, $safe_y,  );
$p->moveto( $interior_offset, .1*$safe_y );
$p->lineto( $interior_offset, $miny );
$p->lineto( $minx, $miny );
$p->lineto( $minx, .1*$safe_y );
$p->moveto( $minx, $safe_y );
$k->add_part($p); 

# if ! defined( $outfile ) then prints to stdout
$k->printall( $outfile );
