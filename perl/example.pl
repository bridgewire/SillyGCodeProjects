#!/usr/bin/perl

use Koike;
use Koike::Part;

$k = new Koike(protocol=>"svg", multsclr=>20, xoffset=>1, yoffset=>1, moveto_color=>'rgb(0,0,240)');
$k->process_cmdlineargs( @ARGV );

$k->update_position(0,0);

# two parabolas.
sub f(){ my $x = shift; return (($x**2)/-4.0) + 25; }
sub g(){ my $x = shift; return (($x-10)**2)/4.0; }

# isolate movement from later translations and rotations
# for this file, we want 'moveto' commands to maintains their coordinates.
#$p = new Koike::Part( koikeobj=>$k );
#$k->add_part($p); 

# make a symmetric parabola
$k->update_position(-10,&f(-10));
$p = new Koike::Part( koikeobj=>$k );
$p->mark_start();
for( my $x = -10; $x<=10; $x+=.01 ) { $p->lineto( $x, &f($x) ); }
$p->mark_end();
$p->translate(10, 0);  # visible SVG has positive coordinates only, we we have to move this part out of the negative x realm
$k->add_part($p); 

# create this 'moveto' object as a seperate part so that it's not rotated along with the next bit.
$p = new Koike::Part( koikeobj=>$k );
$p->moveto(5, &g(5));
$k->add_part($p); 

# make a piece parabola and rotate it
$p = new Koike::Part( koikeobj=>$k );
$p->mark_start();
for( my $x = 5; $x<=15; $x+=.01  ) { $p->lineto( $x, &g($x) );  }
$p->mark_end();

# rotate around our starting point.
# rotation is always around the 0,0 point so we need to translate, rotate, then translate back again.
$p->translate(-5, -&g(5));
$p->rotate(90, "degrees"); 
$p->translate(5, &g(5));
$k->add_part($p); 


# make another, sideways. (i.e. we reverse the meaning of x and y coordinates)
$p = new Koike::Part( koikeobj=>$k );
$p->moveto(&g(8)*5, 8);                 # multiply our function output by 5. g*5 == 1.25*(($x-10)**2)
$p->mark_start();
for( my $x = 8; $x <= 15; $x+=.01  ) { $p->lineto( &g($x)*5, $x );  }
$p->mark_end();
$k->add_part($p); 


$k->printall("/tmp/example.html");
