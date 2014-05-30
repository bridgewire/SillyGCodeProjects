#!/usr/bin/perl

use Koike;
use Koike::Part;

$k = new Koike(protocol=>"svg", multsclr=>20, xoffset=>20, moveto_color=>'rgb(0,0,255)');
$k->update_position(0,0);

$p = new Koike::Part( koikeobj=>$k );
$p->moveto(1,5);
$p->lineto(-0.1,5);
# draw two have circles to make a whole. perhaps there is a way to do this in one command?
$p->arcto(newx=>-8.1, newy=>5, radius=>4, sweep=>1, largearc=>1); 
$p->arcto(newx=>-0.1, newy=>5, radius=>4, sweep=>1, largearc=>1); 
$p->translate(4.1, -5);   # rotate around the center of the circle
$p->rotate(-90, "degrees"); 
$p->translate(-4.1, 5); 
$k->add_part($p); 

# two parabolas.
sub f(){ my $x = shift; return ($x**2)/10 ; }
sub g(){ my $x = shift; return (($x+10)**2)/10 ; }

# make a whole parabola
$p = new Koike::Part( koikeobj=>$k );
$p->moveto(10,&f(10));
for( my $i = 10; $i>-10; $i-=.01 ) { $p->lineto( $i, &f($i) );  }
$k->add_part($p); 

# make part of another and rotate it
# XXX notice that the 'moveto' blue line is also rotated so the path is not continuous. hm.
$p = new Koike::Part( koikeobj=>$k );
$p->moveto(0,&g(0));
for( my $i = 0; $i>-5; $i-=.01  ) { $p->lineto( $i, &g($i) );  }
$p->rotate(-20, "degrees"); 
$k->add_part($p); 

# finish the second one without rotating.
$p = new Koike::Part( koikeobj=>$k );
$p->moveto(-5.01,&g(-5.01));
for( my $i = -5.01; $i>-20; $i-=.01  ) { $p->lineto( $i, &g($i) );  }
$k->add_part($p); 

$k->printall("/tmp/simplesvg.html");
