#!/usr/bin/perl -w

use IO::File;
use IO::Handle;

package Koike;

use strict;
use warnings;

BEGIN
{
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 0.01;
    @ISA         = qw(Exporter);
    @EXPORT      = qw();    # eg: qw(&func1 &func2 &func4);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();

}
our @EXPORT_OK;

sub new {
    my $class = shift;
    my $self = {};

    my %args = @_;

    $self->{eol} = "\r\n";

    $self->{DEBUG} = exists($args{debug}) ? $args{debug}    : 0;  # 0 means: no debug messages.
    $self->{p} = exists($args{protocol})  ? $args{protocol} : 'gcode'; # protocol: svg or gcode
    $self->{x} = undef;
    $self->{y} = undef;

    # bounding box.  XXX this doesn't correctly handle arcs,
    # whose extent will often pass beyond end points.
    $self->{xmin} = undef;
    $self->{xmax} = undef;
    $self->{ymin} = undef;
    $self->{ymax} = undef;
    $self->{height} = undef;
    $self->{width} = undef;

    # colors for svg mode
    my $dfltclr = 'rgb(255,0,0)';
    $self->{moveto_color} = exists($args{moveto_color}) ? $args{moveto_color}  : 'none';
    $self->{cut_color}    = exists($args{cut_color})    ? $args{cut_color}     : $dfltclr;
    $self->{line_color}   = exists($args{line_color})   ? $args{line_color}    : $dfltclr;
    $self->{curve_color}  = exists($args{curve_color})  ? $args{curve_color}   : $dfltclr;
    $self->{matrl_color}  = exists($args{material_color})  ? $args{material_color}   : 'rgb(200,255,255)';
    $self->{mark_start_color} = exists($args{mark_start_color})  ? $args{mark_start_color} : 'rgb(0,200,0)';
    $self->{mark_end_color}   = exists($args{mark_end_color})    ? $args{mark_end_color}   : 'rgb(230,0,0)';

    # adding a part fills this command list.  printing pulls from it.
    $self->{clist} = [];

    # offsets and scaling
    $self->{mult}         = exists($args{multsclr})     ? $args{multsclr}      : 1;
    $self->{xoffset}      = exists($args{xoffset})      ? $args{xoffset}       : 0;
    $self->{yoffset}      = exists($args{yoffset})      ? $args{yoffset}       : 0;

    # g-code options and state variables
    $self->{abscoords}    =  exists($args{abscoords})      ? $args{abscoords}     : 1;
    $self->{absIJKcoords} =  exists($args{absIJKcoords})   ? $args{absIJKcoords}  : 0;
    $self->{verbosegcode} =  exists($args{verbosegcode})   ? $args{verbosegcode}  : 0;
    $self->{addcutting}   =  exists($args{addcutting})     ? $args{addcutting}    : 0;
    $self->{cuttingon}    =  0; # is the cutting head turned on?  boolean.
    $self->{LASTGCMD}     = ''; # the present command (G00,G01,G02,...) is a machines state variable

    # svg options
    $self->{inc_html}     =  exists($args{includehtml})    ? $args{includehtml}     : 1;

    # create a handle to the output file (default: stdout) so that a replacement can be used transparently
    $self->{fh} = IO::Handle->new();
    $self->{fh}->fdopen(fileno(STDOUT),"w");

    bless($self,$class); # bless me! and all who are like me. bless us everyone.
    return $self;
}

sub p()
{
    my $self = shift;
    my $line = shift;
    local *FH = $self->{fh};

    if( defined( $line ) ){ print FH $line; } 
    if( ! defined($\) ){  print FH $self->{eol}; }
}

sub process_cmdlineargs()
{
    my $self = shift;
    my @argv = @_;

    # process command-line arguments
    if( grep(/--svg/, @argv) ) { $self->{p} = 'svg'; }
    if( (my @l = grep(/--mult=-?[0-9.]+/,  @argv)) ){ $l[0] =~ /--mult=(-?[0-9.]+)/;  $self->{mult}  = $1; }
    if( (my @l = grep(/--d(ebug)?=[0-9]+/, @argv)) ){ $l[0] =~ /--d(ebug)?=([0-9]+)/; $self->{DEBUG} = $2; }

    if( (my @l = grep(/--moveto-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i, @argv)) )
    {
        $l[0] =~ /--moveto-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i;
        my $clr = $1;
        $clr =~ tr/[A-Z]/[a-z]/; # switch to lower case
        if( $clr =~ /^#([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})/i )
        {
            my($r,$g,$b) = (hex($1),hex($2),hex($3));
            $clr = "rgb($r,$g,$g)";
        }
        $self->set_colors( moveto_color=>$clr );
    }
    if( (my @l = grep(/--cut-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i, @argv)) )
    {
        $l[0] =~ /--cut-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i;
        my $clr = $1;
        $clr =~ tr/[A-Z]/[a-z]/; # switch to lower case.  we don't want "NoNe"
        if( $clr =~ /^#([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})/i )
        {
            my($r,$g,$b) = (hex($1),hex($2),hex($3));
            $clr = "rgb($r,$g,$g)";
        }
        $self->set_colors( cut_color=>$clr );
    }
    if( (my @l = grep(/--material-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i, @argv)) )
    {
        $l[0] =~ /--material-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i;
        my $clr = $1;
        $clr =~ tr/[A-Z]/[a-z]/; # switch to lower case.  we don't want "NoNe"
        if( $clr =~ /^#([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})/i )
        {
            my($r,$g,$b) = (hex($1),hex($2),hex($3));
            $clr = "rgb($r,$g,$g)";
        }
        $self->set_colors( matrl_color=>$clr );
    }
    if( (my @l = grep(/--mark-start-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i, @argv)) )
    {
        $l[0] =~ /--mark-start-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i;
        my $clr = $1;
        $clr =~ tr/[A-Z]/[a-z]/; # switch to lower case.  we don't want "NoNe"
        if( $clr =~ /^#([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})/i )
        {
            my($r,$g,$b) = (hex($1),hex($2),hex($3));
            $clr = "rgb($r,$g,$g)";
        }
        $self->set_colors( mark_start_color=>$clr );
    }
    if( (my @l = grep(/--mark-end-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i, @argv)) )
    {
        $l[0] =~ /--mark-end-color="?(rgb\(\d+,\d+,\d+\)|#[0-9A-F]{6}|none)"?/i;
        my $clr = $1;
        $clr =~ tr/[A-Z]/[a-z]/; # switch to lower case.  we don't want "NoNe"
        if( $clr =~ /^#([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})/i )
        {
            my($r,$g,$b) = (hex($1),hex($2),hex($3));
            $clr = "rgb($r,$g,$g)";
        }
        $self->set_colors( mark_end_color=>$clr );
    }
}

sub set_colors()
{
    my $self = shift;
    my %args = @_;

    $self->{moveto_color} = exists($args{moveto_color}) ? $args{moveto_color}  : $self->{moveto_color};
    $self->{cut_color}    = exists($args{cut_color})    ? $args{cut_color}     : $self->{cut_color};
    $self->{line_color}   = exists($args{line_color})   ? $args{line_color}    : $self->{line_color};
    $self->{curve_color}  = exists($args{curve_color})  ? $args{curve_color}   : $self->{curve_color};
    $self->{matrl_color}  = exists($args{material_color})  ? $args{material_color}   : $self->{matrl_color};
    $self->{mark_start_color} = exists($args{mark_start_color})  ? $args{mark_start_color} : $self->{mark_start_color};
    $self->{mark_end_color}   = exists($args{mark_end_color})    ? $args{mark_end_color}   : $self->{mark_end_color};
}

sub get_colors()
{
    my $self = shift;

    return (
        moveto_color => $self->{moveto_color},
           cut_color => $self->{cut_color},
          line_color => $self->{line_color},
         curve_color => $self->{curve_color},
         matrl_color => $self->{matrl_color},
    mark_start_color => $self->{mark_start_color},
      mark_end_color => $self->{mark_end_color},

    );
}

sub get_protocol()
{
    my $self  = shift;
    return $self->{p};
}

sub get_color()
{
    my $self  = shift;
    my $clrnm = shift;
    die if $clrnm !~  /^(moveto|cut|line|curve|matrl|mark_start|mark_end)_color$/;
    return $self->{$clrnm};
}

sub update_bounds()
{
    my $self = shift;
    my $newx = shift;
    my $newy = shift;

    if( ! defined($self->{xmin}) ) 
    {
        $self->{xmin} = $self->{xmax} = $newx;
        $self->{ymin} = $self->{ymax} = $newy;
    }
    else
    {
        $self->{xmin} = ( $self->{xmin} < $newx ? $self->{xmin} : $newx );
        $self->{xmax} = ( $self->{xmax} > $newx ? $self->{xmax} : $newx );
        $self->{ymin} = ( $self->{ymin} < $newy ? $self->{ymin} : $newy );
        $self->{ymax} = ( $self->{ymax} > $newy ? $self->{ymax} : $newy );
    }
}

sub update_position()
{
    my $self = shift;
    my $newx = shift;
    my $newy = shift;

    if( defined($self->{x}) && $self->{x} == $newx && $self->{y} == $newy )
    {
        # did the position change?
        return 0;
    }

    $self->update_bounds( $newx, $newy );

    $self->{x} = $newx;
    $self->{y} = $newy;

    return 1;
}

sub get_x() { my $self = shift; return $self->{x}; }
sub get_y() { my $self = shift; return $self->{y}; }

sub get_current_position()
{
    my $self = shift;
    return ( $self->{x}, $self->{y} );
}

sub set_rectagular_material_bounds()
{
    my $self = shift;
    my $x1 = shift;
    my $y1 = shift;
    my $x2 = shift;
    my $y2 = shift;

    push( @{$self->{clist}},
        {
            cmd=>'rb',   clr=>$self->{matrl_color},
            sox=>$x1, soy=>$y1,
            tox=>$x2, toy=>$y2
        });
}

sub add_part()
{
    my $self = shift;
    my $p = shift;

    $p->rewind_command_list(); # just in case this was added before.
    my $c;
    while( ($c = $p->next_command()) )
    {
        $self->update_position( $c->{sox}, $c->{soy} );
        if( exists( $c->{tox} ) )
        {
            $self->update_position( $c->{tox}, $c->{toy} );
            if( $c->{cmd} eq 'a' )
            {
                # XXX a trade off.
                # this set of points will cover a whole circle and so is
                # guaranteed to give adequate space for the parts.  on the
                # other hand, this can very easily over extend the bounds.
                # a better solution takes into account the sweep, largearc,
                # and start and stop position parameters, but that is
                # complicated and difficult.
                $self->update_bounds( $c->{cx} + $c->{radius}, $c->{cy} );
                $self->update_bounds( $c->{cx} - $c->{radius}, $c->{cy} );
                $self->update_bounds( $c->{cx}, $c->{cy} + $c->{radius} );
                $self->update_bounds( $c->{cx}, $c->{cy} - $c->{radius} );
            }
        }

        push( @{$self->{clist}}, $c);
    }
}

sub svg_lineto()
{
    my $self = shift;
    my $h = shift;

    $self->p(
        sprintf( '<line x1="%.03f" y1="%.03f" x2="%.03f" y2="%.03f" style="stroke:%s;stroke-width:1" />',
            ($h->{sox} + $self->{xoffset}) * $self->{mult},
            ($h->{soy} + $self->{yoffset}) * $self->{mult}, 
            ($h->{tox} + $self->{xoffset}) * $self->{mult},
            ($h->{toy} + $self->{yoffset}) * $self->{mult},
            $h->{clr})
    );
}

sub svg_arcto()
{
    my $self = shift;
    my $h = shift;
    my $r = $h->{radius};
    my $l = $h->{largearc};
    my $s = $h->{sweep};
    my $c = $h->{clr};

    $self->p(

        # d="move to where I already am, then draw an arc using our parameters."
        sprintf( '<path d="M%.03f %.03f A%.03f,%.03f 0 %d,%d %.03f,%.03f" style="stroke:%s;stroke-width:1;fill:none" />',

            ($h->{sox} + $self->{xoffset}) * $self->{mult},
            ($h->{soy} + $self->{yoffset}) * $self->{mult}, 

            $h->{radius} * $self->{mult},
            $h->{radius} * $self->{mult}, 

            $h->{largearc}, $h->{sweep}, 

            ($h->{tox} + $self->{xoffset}) * $self->{mult},
            ($h->{toy} + $self->{yoffset}) * $self->{mult},

            $h->{clr} )
    );
}

sub svg_moveto()
{
    my $self = shift;
    my $h = shift;
    if( exists($h->{clr}) && defined($h->{clr}) && $h->{clr} ne 'none' )
    {
        $self->svg_lineto( $h );
    }
}

sub svg_endpoint()
{
    my $self = shift;
    my $h = shift;

    $self->p(
        sprintf( '<circle cx="%.03f", cy="%.03f", r="%.03f", fill="%s" stroke="none" />', 
            (($h->{sox} + $self->{xoffset}) * $self->{mult}),
            (($h->{soy} + $self->{yoffset}) * $self->{mult}),
            ( $h->{r}              * 5 * log( $self->{mult} )),  # scale the size of the dot at a non-linear rate. 5*log(r) chosen after experimentation
            $h->{clr} )
    );

}

sub svg_rectangular_background()
{
    my $self = shift;
    my $h = shift;

    my $wd = abs($h->{sox} - $h->{tox});
    my $ht = abs($h->{soy} - $h->{toy});

    $self->p(
        sprintf( '<rect x="%.03f", y="%.03f", width="%.03f", height="%.03f", fill="%s" stroke="none" />', 
            (($h->{sox} + $self->{xoffset}) * $self->{mult}),
            (($h->{soy} + $self->{yoffset}) * $self->{mult}),
            abs($wd * $self->{mult}),
            abs($ht * $self->{mult}),
            $h->{clr} )
    );
}

sub gcode_cutting_on
{
    my $self = shift;

    if( $self->{cuttingon} ) { return; }

    # else
    $self->{cuttingon} = 1;
    $self->p( 'M15' );
}

sub gcode_cutting_off
{
    my $self = shift;

    if( ! $self->{cuttingon} ) { return; }

    # else
    $self->{cuttingon} = 0;
    $self->p( 'M16' );
}

sub gcode_linear_motion()
{
    my $self = shift;
    my $h = shift;
    my $s = shift;

    if( $self->{addcutting} )
    {
        # if changing between linear modes, enable or disable the cutting head appropriately
        if   ( $s eq 'G00' ) { $self->gcode_cutting_off(); }
        elsif( $s eq 'G01' ) { $self->gcode_cutting_on(); }
    }

    if( length($s) > 0 )
    {
        # data transfer to the the Koike is very slow, so reduce
        # characters whenever it makes sense. (or when possible.)
        if( $self->{LASTGCMD} eq $s && ! $self->{verbosegcode} )
        {
            $s = '';
        }
        else
        {
            $self->{LASTGCMD} = $s;
            $s .= ' ';
        }
    }

    # remove unnecessary trailing zeros.  again, this is to reduce the data size.
    my $xstr = sprintf( '%.03f%s', ($h->{tox} + $self->{xoffset}) * $self->{mult}, ($self->{verbosegcode} ? ' ' : ''));
    my $ystr = sprintf( '%.03f%s', ($h->{toy} + $self->{yoffset}) * $self->{mult}, ($self->{verbosegcode} ? ' ' : '') );

    if( ! $self->{verbosegcode} )
    {
        if( $xstr =~ s/0+$// ){ $xstr =~ s/\.$//; }
        if( $ystr =~ s/0+$// ){ $ystr =~ s/\.$//; }
    }

    $self->p( sprintf( '%sX%sY%s', $s, $xstr, $ystr ) );
}

sub gcode_lineto() { my $self = shift; my $h = shift; $self->gcode_linear_motion( $h, 'G01' ); }
sub gcode_moveto() { my $self = shift; my $h = shift; $self->gcode_linear_motion( $h, 'G00' ); }

sub gcode_arcto()
{
    my $self = shift; my $h = shift; 

    # In SVG, 'sweep' is a positive angle, but rotates clockwise.
    # In Koike gcode we decide that sweep is still positive, but counter-clockwise.
    # This correctly implements the mirror image flip around the x-axis
    my $g = $h->{sweep} ? 'G03' : 'G02' ;

    # I assume G02/3 are rarely used, so I won't bother, for now, with the space-saving code.
    # The purpose of this is just so that ($self->{LASTGCMD} != 'G00/1')
    $self->{LASTGCMD} = $g;

    if( $self->{addcutting} ) { $self->gcode_cutting_on(); }

    $self->print_debug( 1, sprintf("sox:%.02f soy:%.02f tox:%.02f toy:%.02f cx:%.02f cy:%.02f mult:%.02f",
                                   $h->{sox}, $h->{soy}, $h->{tox}, $h->{toy}, $h->{cx}, $h->{cy}, $self->{mult} ) );


    $self->p( sprintf( '%s X%.03f Y%.03f I%.03f J%.03f', $g,

            ($h->{tox} + $self->{xoffset}) * $self->{mult}, 
            ($h->{toy} + $self->{yoffset}) * $self->{mult}, 

            ($h->{cx} - $h->{sox}) * $self->{mult}, 
            ($h->{cy} - $h->{soy}) * $self->{mult} )
    );
}

sub set_height_width()     { my $self = shift; $self->{height}  = shift; $self->{width}    = shift; }
sub set_offsets()          { my $self = shift; $self->{xoffset} = shift; $self->{yoffset}  = shift; }
sub set_scale_multiplier() { my $self = shift; $self->{mult}    = shift; }
sub get_scale_multiplier() { my $self = shift; return $self->{mult}; }


sub printall()
{
    my $self  = shift;
    my $fname = shift;

    if( defined($fname) )
    {
        $self->{fh} = IO::File->new();
        $self->{fh}->open( $fname, '>' ) || die "failed to open $fname for writing\n";
    }

    my %cmds = ();
    if( $self->{p} eq 'svg' )
    {
        %cmds = (l=>\&svg_lineto,   a=>\&svg_arcto,   m=>\&svg_moveto,   start=>\&print_svg_html_start,    end=>\&print_svg_html_end,
                 os=>\&svg_endpoint, oe=>\&svg_endpoint, rb=>\&svg_rectangular_background );
    }
    else
    {
        %cmds = (l=>\&gcode_lineto, a=>\&gcode_arcto, m=>\&gcode_moveto, start=>\&print_koike_gcode_start, end=>\&end_koike_gcode );
    }

    &{$cmds{start}}( $self );

    # my $lref = $self->{clist}
    foreach my $i ( @{$self->{clist}} )
    {
        if( exists($cmds{$i->{cmd}}) ) { &{ $cmds{$i->{cmd}} }( $self, $i ); }
    }

    &{$cmds{end}}( $self );
}

sub print_koike_gcode_start()
{
    my $self = shift;

    # the asterisk before M08 is part of the communications protocol, and the remaining
    # codes basically turn the cutting heads off and stuff lke that.
    $self->p("*");
    $self->p("M08");
    $self->p("M16");
    $self->p("G20");
    $self->p("G40");
    $self->p("G90");

    $self->{LASTGCMD} = 'G90';

    # if our actual starting point has shifted, then we need to begin our gcode
    # commands with a 'moveto' (G00) because the Koike machine always starts
    # its cuts at 0,0.
    my $h;
    my $i=0;
    # find the first command that has a useful sox,soy. That's the actual starting point.
    do {
        $h = ${$self->{clist}}[$i];
        $i++;
    } while ( $h->{cmd} =~ /^(rb|start|end)$/ );

    $self->gcode_moveto( { sox=>0,  soy=>0, tox=>$h->{sox}, toy=>$h->{soy} } );
}

sub end_koike_gcode()
{
    my $self = shift;

    # M30 says "end of program" signals koike to stop reading serial input.
    $self->p("M30");
}




sub print_svg_html_start()
{
    my $self = shift;

    if( ! defined($self->{height}) ){ $self->{height} = $self->{ymax}; }
    if( ! defined($self->{width})  ){ $self->{width}  = $self->{xmax}; }

    my $h = ($self->{height} + abs($self->{yoffset})) * $self->{mult} + 1;
    my $w = ($self->{width}  + abs($self->{xoffset})) * $self->{mult} + 1;

    # round these values up to the nearest integer
    if( $h != int($h) ) { $h = sprintf("%d", abs($h)+.5 ); }
    if( $w != int($w) ) { $w = sprintf("%d", abs($w)+.5 ); }

    if( $self->{inc_html} ) { $self->p( '<!DOCTYPE html><html><body>' ); }
    $self->p( '<svg height="'.abs($h).'" width="'.abs($w).'">' );
}
 
sub print_svg_html_end()
{
    my $self = shift;
    $self->p( "Sorry, your browser does not support inline SVG.</svg>" );
    if( $self->{inc_html} ) { $self->p( '</body></html>' ); }
}

sub print_debug()
{
    my $self = shift;
    my $ordinal_importance = shift;  # the smaller the number used, the more likely it is that print_debug will generate output
    my $msg  = join('', @_);
    if( $self->{DEBUG} >= $ordinal_importance  )
    {
        my $d=`date "+%F %T"`;
        chomp($d);
        print STDERR $d,' debugging -- ',$msg,"\n";
    }
}


# /* vim: set ai et tabstop=4  shiftwidth=4: */
1;
