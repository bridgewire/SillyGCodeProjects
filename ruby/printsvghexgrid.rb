#!/usr/bin/env ruby
#
## Author:  Christiana Evelyn Johnson
## Copyright (c) 2015
## license: The MIT License (MIT)

require './BWCNC'


# command-line argument processing located at the top as a way of documenting this program
def process_commandline_args

  params = { :sidelength => 1, :mult => 10, :stroke_width => 1,
             :cols => 3, :rows => 3,
             :nested => 1, :nested_spacing => 0.2,
             :suppress_grid => false,
             :moveto_color => '#0000ff', :lineto_color => '#ff0000',
             :xshift => 0, :yshift => 0
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
    else abort "\nArborting!!! -- Error: unknown argument #{a}\n\n"
    end
  }

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

  k = BWCNC::PartContext.new

  # this makes the hex-grid, storing it in k.
  fill_partctx_with_hexgrid( k, parms )

  # align center of grid with coordinate 0,0 to make whatever transform we're
  # executing probably-symetric with respect to the center of the grid.
  min = k.boundingbox[:min]
  max = k.boundingbox[:max]
  k.translate( Vector[ -(max[0] - min[0])/2.0, -(max[1] - min[1])/2.0, 0 ] )

  k.position_dependent_transform( 
  
  ##   # spiral
  ##   nil,
  ## 
  ##   lambda { |v|
  ##   
  ##     w=2*Math::PI/10.0;
  ## 
  ##     d=Math.sqrt(v[0]**2 + v[1]**2);
  ## 
  ##     d = (d == 0 ? 0.0000000001 : d );
  ## 
  ##     #x = Math.sin(d*w)
  ## 
  ##     Vector[ Math.cos(d*w)/1.2, Math.sin(d*w)/1.2, 0 ]
  ## 
  ##   } )
  
  
    ##########################################################
    # cross-hatch waves
    ##########################################################
    nil, # no multiplication of coords. translations only.
  
    # translation function returns: Vector[ sin(w*y), sin(w*x), 0 ]
    # where 'w' is a fixed constant. the x coordinate of a point shifts
    # according to the original value of the y coordinate and vice versa.
    lambda { |v|
    
      # 2*Math::PI * something makes it easy to relate side-lengths
      # into the number of oscillations.  a side-length of one makes
      # make it simpler still
      #
#      w=2*Math::PI/10.0; # one tenth of an oscillation per unit. 
      w=2*Math::PI/15.0; # one tenth of an oscillation per unit. 
#      w=2*Math::PI/20.0; # one twentieth of an oscillation per unit. 
#      w=2*Math::PI/40.0; # one fortieth of an oscillation per unit.

      # what will this do?
#      Vector[ Math.sin(w*(v[1] + parms[:yshift])), Math.sin(w*(v[0] + parms[:xshift])), 0 ]
#      Vector[ 1.5*Math.sin(w*(v[1] + parms[:yshift])), 1.5*Math.sin(w*(v[0] + parms[:xshift])), 0 ]
      Vector[ 1.8*Math.sin(w*(v[1] + parms[:yshift])), 1.8*Math.sin(w*(v[0] + parms[:xshift])), 0 ]
#      Vector[ 2*Math.sin(w*(v[1] + parms[:yshift])), 2*Math.sin(w*(v[0] + parms[:xshift])), 0 ]
#      Vector[ 3*Math.sin(w*(v[1] + parms[:yshift])), 3*Math.sin(w*(v[0] + parms[:xshift])), 0 ]
    } )


  k.scale( parms[:mult] )
  k.remake_boundingbox      # XXX why does this seem necessary? shouldn't be.
                            # the bounding boxes probably aren't updated by
                            # transforms. Only the points are update. a bug

  # shift the image again to make 0,0 the upper-left corner, making visible the svg.
  min = k.boundingbox[:min]
  max = k.boundingbox[:max]
  k.translate( Vector[ -min[0], -min[1], 0 ] )

  renderer = BWCNC::SVG.new
  BWCNC::SVG::moveto_color = parms[:moveto_color]
  BWCNC::SVG::lineto_color = parms[:lineto_color]
  BWCNC::SVG::stroke_width = parms[:stroke_width]
  renderer.render_all( k )

end #main

main()


