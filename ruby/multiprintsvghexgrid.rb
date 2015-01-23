#!/usr/bin/env ruby
#
## Author:  Christiana Evelyn Johnson
## Copyright (c) 2015
## license: The MIT License (MIT)

require './BWCNC'


# command-line argument processing located at the top as a way of documenting this program
def process_commandline_args

  params = { :sidelength => 1, :mult => 10, :stroke_width => 0.7,
             :cols => 10, :rows => 10,
             :nested => 1, :nested_spacing => 0.2,
             :suppress_grid => true,
             :moveto_color => 'none', :lineto_color => '#ff0000', :background_color => '#fe8736',
             :xshift => 0, :yshift => 0,
             :shiftstep => nil, :shiftstepx => 0, :shiftstepy => 0, :shiftsteps => 15,
             :outputstepfile => 'output/img_%.04d.svg'
  }

  ARGV.each { |a|
    if    v = a.match(/^--side-length=([0-9.]+)$/)       then params[:sidelength]     = v[1].to_f
    elsif v = a.match(/^--cols=([0-9]+)$/)               then params[:cols]           = v[1].to_i
    elsif v = a.match(/^--rows=([0-9]+)$/)               then params[:rows]           = v[1].to_i
    elsif v = a.match(/^--nested=([0-9]+)$/)             then params[:nested]         = v[1].to_i
    elsif v = a.match(/^--nested-spacing=(0?\.[0-9]+)$/) then params[:nested_spacing] = v[1].to_f
    elsif v = a.match(/^--suppress-grid(=([01]))?$/)     then params[:suppress_grid]  = (v[1].nil? || v[2] == "1")
    elsif v = a.match(/^--mult=([.0-9e]+)$/)             then params[:mult]           = v[1].to_f
    elsif v = a.match(/^--stroke-width=([.0-9]+)$/)      then params[:stroke_width]   = v[1].to_f
    elsif v = a.match(/^--moveto-color=(none|#(\h{3}|\h{6}))$/)
                                                         then params[:moveto_color]   = v[1]
    elsif v = a.match(/^--lineto-color=(none|#(\h{3}|\h{6}))$/)
                                                         then params[:lineto_color]   = v[1]
    elsif v = a.match(/^--xshift=([-.0-9]+)$/)           then params[:xshift]         = v[1].to_f
    elsif v = a.match(/^--yshift=([-.0-9]+)$/)           then params[:yshift]         = v[1].to_f

    elsif v = a.match(/^--shiftstep=([-.0-9]+)$/)        then params[:shiftstep]      = v[1].to_f
    elsif v = a.match(/^--shiftstepx=([-.0-9]+)$/)       then params[:shiftstepx]     = v[1].to_f
    elsif v = a.match(/^--shiftstepy=([-.0-9]+)$/)       then params[:shiftstepy]     = v[1].to_f
    elsif v = a.match(/^--shiftsteps=([0-9]+)$/)         then params[:shiftsteps]     = v[1].to_i
    elsif v = a.match(/^--outputstepfile=['"]*(.+\.svg)['"]*$/)
      then
      if v[1].match(/%[.0-9]*d/)
        params[:outputstepfile] = v[1]
        STDERR.puts "got outputstepfile == #{params[:outputstepfile]}"
      end
    else abort "\nArborting!!! -- Error: unknown argument #{a}\n\n"
    end
  }

  unless params[:shiftstep].nil? 
    params[:shiftstepx] = params[:shiftstep]
    params[:shiftstepy] = params[:shiftstep]
  end

  params
end


# hexagon orientation: symmetric around vertical axis through two vertecees
#
# (x_start, y_start): are coordinates of upper-lefthand vertex
# outer_sidelen:      standard hex-grid side length (length of grid-hexagon side, not nested side)
# spacing_ratio:      distance from vertex to center is 100% == 1. spacing_ratio
#                     gives the part (as ratio) of that that distance to the first
#                     nested hex
# nth_from_outer:     which nexted hex are we finding the coordinates for?
#
# returns: coords of upper-left vertex of target nested hex, and sidelength of same
#
def nested_params( outer_sidelen, spacing_ratio, nth_from_outer, x_start, y_start )
    hypot = spacing_ratio * nth_from_outer * outer_sidelen
    x = x_start + hypot * Math.sin( Math::PI/3.0 )
    y = y_start + hypot * Math.cos( Math::PI/3.0 )
    [ x, y, outer_sidelen - hypot ]
end


def make_inner_hexagon( k, main_sidelen, inner_fraction, nth_inner, x_reference=nil, y_reference=nil, start_mark=false )

  if x_reference.nil? then x_reference, y_reference = k.get_current_position end

  x, y, sidelen = nested_params( main_sidelen, inner_fraction, nth_inner, x_reference, y_reference )
  hexagon( k, sidelen, x, y, nil, start_mark )
end

def hexagon( k, sidelen, x_start, y_start, sides2skip=nil, start_mark=false )

  part = k.get_new_part.update_position( Vector[x_start, y_start, 0] )

  # orientation: left-right symmetry around the vertical center line through two vertices
  triangle_longside  = sidelen * Math.cos(Math::PI/6)
  triangle_shortside = sidelen * Math.sin(Math::PI/6)

  path = [  Vector[ triangle_longside, -triangle_shortside, 0],
            Vector[ triangle_longside,  triangle_shortside, 0],
            Vector[                 0,             sidelen, 0],
            Vector[-triangle_longside,  triangle_shortside, 0],
            Vector[-triangle_longside, -triangle_shortside, 0],
            Vector[                 0,            -sidelen, 0] ]

  vec = Vector[ x_start, y_start, 0 ]

  (0...6).each { |i|
    vec = vec + path[i]
    if ! sides2skip.nil? && sides2skip.include?( i )
      part.moveto( BWCNC::CommandArgs.new( vec ) )
    else
      v = BWCNC::CommandArgs.new( vec )
      part.lineto( v )
    end
  }

  part

end

def fill_partctx_with_hexgrid( k, parms )

  ###############################################################################################################
  #### Row #1
  ###############################################################################################################
  x_ref, y_ref = [ 0, parms[:sidelength]/2.0 ]

  (0..parms[:nested]-1).each { |i| i = parms[:nested] - i # counting down ...  k, k-1, k-2, ..., 1
    k.add_part( make_inner_hexagon( k, parms[:sidelength], parms[:nested_spacing], i, x_ref, y_ref ) )
  }

  if ! parms[:suppress_grid] then k.add_part( hexagon( k, parms[:sidelength], x_ref, y_ref ) ) end

  (1...parms[:cols]).each { |j|

    x_ref = j * Math.sqrt(3) * parms[:sidelength]

    (0..parms[:nested]-1).each { |i| i = parms[:nested] - i
      k.add_part( make_inner_hexagon( k, parms[:sidelength], parms[:nested_spacing], i, x_ref, y_ref ) )
    }

    if ! parms[:suppress_grid] then k.add_part( hexagon( k, parms[:sidelength], x_ref, y_ref, [5] ) ) end
  }
  ###############################################################################################################
  #### End of Row #1
  ###############################################################################################################

  ###############################################################################################################
  #### remaining rows
  ###############################################################################################################
  (2..parms[:rows]).each { |row|
    row_iseven  = (row % 2 == 0)
    x_ref       = row_iseven ? Math.sqrt(3) * parms[:sidelength] / 2.0 : 0
    xoffset     = row_iseven ? 0.5 : 0
    y_ref      += 1.5 * parms[:sidelength]

    (0..parms[:nested]-1).each { |i| i = parms[:nested] - i
      k.add_part(  make_inner_hexagon( k, parms[:sidelength], parms[:nested_spacing], i, x_ref, y_ref ) )
    }

    if ! parms[:suppress_grid]
      if row_iseven then k.add_part( hexagon( k, parms[:sidelength], x_ref, y_ref, [0,1] ) )
      else               k.add_part( hexagon( k, parms[:sidelength], x_ref, y_ref,   [1] ) )
      end
    end


    (1...parms[:cols]).each { |j|

      x_ref = (j + xoffset) * Math.sqrt(3) * parms[:sidelength]

      (0..parms[:nested]-1).each { |i| i = parms[:nested] - i
        k.add_part( make_inner_hexagon( k, parms[:sidelength], parms[:nested_spacing], i, x_ref, y_ref ) )
      }

      if ! parms[:suppress_grid]
        if row_iseven
          if j < parms[:cols] - 1 then k.add_part( hexagon( k, parms[:sidelength], x_ref, y_ref, [0,1,5] ) )
          else                         k.add_part( hexagon( k, parms[:sidelength], x_ref, y_ref, [0,  5] ) )
          end
        else
          if j == 0                then k.add_part( hexagon( k, parms[:sidelength], x_ref, y_ref, [  1  ] ) )
          else                          k.add_part( hexagon( k, parms[:sidelength], x_ref, y_ref, [0,1,5] ) )
          end
        end
      end
    }
  }
  ###############################################################################################################
  #### end of remaining rows
  ###############################################################################################################

  k

end


def main

  parms = process_commandline_args()

  renderer = BWCNC::SVG.new
  BWCNC::SVG::moveto_color = parms[:moveto_color]
  BWCNC::SVG::lineto_color = parms[:lineto_color]
  BWCNC::SVG::background_color = parms[:background_color]
  BWCNC::SVG::stroke_width = parms[:stroke_width]

  k = BWCNC::PartContext.new

  # this makes the hex-grid, storing it in k.
  fill_partctx_with_hexgrid( k, parms )

  # align center of grid with coordinate 0,0 to make whatever transform we're
  # executing probably-symetric with respect to the center of the grid.
  min = k.boundingbox[:min]
  max = k.boundingbox[:max]

  # make sure the minimum point in both directions is zero
  unless min[0] == 0 && min[1] == 0
    k.translate( Vector[ -min[0], -min[1], 0 ] )
  end

  min = k.boundingbox[:min]
  max = k.boundingbox[:max]

  shiftedmax = max * parms[:mult]

  k.translate( max * -0.5 )

  parms[:shiftsteps].times { |i|

    l = k.clone # makes a deep-clone
    l.position_dependent_transform( nil, 
      lambda { |v|
#        w=2*Math::PI/15.0; # one fifteenth of an oscillation per unit. 
        w=2*Math::PI/40.0; # one fifteenth of an oscillation per unit. 
        Vector[ 7*Math.sin(w*(v[1] + parms[:yshift] + i * parms[:shiftstepx])),
                7*Math.sin(w*(v[0] + parms[:xshift] + i * parms[:shiftstepy])), 0 ]
      } )

    l.translate( max * 0.5 )
    l.scale( parms[:mult] )
    l.remake_boundingbox      # XXX why does this seem necessary? shouldn't be.

    STDERR.puts "rendering into file: #{parms[:outputstepfile] % i}"
    renderer.render_all( l, parms[:outputstepfile] % i )
  }

end #main

main()


