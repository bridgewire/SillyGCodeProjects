#!/usr/bin/env ruby
#
## Author:  Christiana Evelyn Johnson
## Copyright (c) 2015
## license: The MIT License (MIT)

# Notice that all instances of the Vector class in this file have 3 dimensions even though
# this code, in general, only uses the first two dimensions.  This is done so in order to
# make "cross_product," a somewhat important addition, more general and properly defined.
# It's also done because this code may someday be used to make g-code for machines that
# have a 3rd axis. In fact, I will probably add an (optional) functional 3rd axis quite soon.

require 'matrix'

module BWCNC

  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class NumString
  #  NumString class usage:
  #   NumString.terse = false
  #   NumString.precision = 5
  #   "G01 X%s Y%s" % [ NumString.new( 34.298398 ).to_s, NumString.new( 34.00500 ).to_s ]   =>  "G01 X34.29840 Y34.00500"
  #
  #   NumString.terse = true
  #   "G01 X%s Y%s" % [ NumString.new( 34.298398 ).to_s, NumString.new( 34.00500 ).to_s ]   =>  "G01 X34.2984 Y34.005"
  #
  #   NumString.precision = 2
  #   "G01 X%s Y%s" % [ NumString.new( 34.298398 ).to_s, NumString.new( 34.00500 ).to_s ]   =>  "G01 X34.3 Y34.01"
  #   "G01 X%s Y%s" % [ NumString.new( 34 ).to_s, NumString.new( 34.00400 ).to_s ]          =>  "G01 X34 Y34"
  #
  class NumString
    @terse = false
    @precision = 3

    class << self
      attr_accessor :terse, :precision
    end

    attr_reader :num

    def initialize( n, precision = nil, terse = nil )
      @num = n
      if ! terse.nil?
        NumString.terse = terse
      end
      if ! precision.nil?
        NumString.precision = precision
      end
    end

    def to_s
      str = "%.0#{NumString.precision}f" % @num
      if NumString.terse
        str.gsub!(/0+$/, '') && str.gsub!( /\.$/, '')  # remove trailing zeros and decimal points
        str.gsub!( /^-([.0 ]+)$/, '\1' )               # convert negative zero into plain zero
      end
      str
    end
  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class NumString


  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB module_function VectorToNumStringArray
  # usage:  VectorToNumStringArray(  Vector v , int size )
  # return array of size 'size' containing string versions v's numeric elements
  def VectorToNumStringArray( v, size=3 )
    a = []
    if size == 3 
      a = [ NumString.new( v[0] ).to_s,  NumString.new( v[1] ).to_s, NumString.new( v[2] ).to_s ]
    else
      raise ArgumentError.new("mismatched sizes") if size > v.size
      size.times { |i| a << NumString.new( v[i] ).to_s }
    end
    a
  end
  module_function :VectorToNumStringArray
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE module_function VectorToNumStringArray


  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class CommandArgs
  class CommandArgs
    attr_accessor :from, :to, :clr, :sweep, :largearc, :center, :radius

    def initialize( point_to=Vector[0,0,0], args={} )
      @to       = point_to
      @from     = args[:point_from]
      @center   = args[:point_center]
      @clr      = args[:clr]
      @sweep    = args[:sweep]
      @largearc = args[:largearc]
      @radius   = args[:radius]
    end
  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class CommandArgs

  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class Command
  class Command
#    attr_accessor :sox, :tox, :soy, :toy, :cmd, :clr, :sweep, :largearc, :cx, :cy, :radius
    attr_reader :start, :end
    attr_accessor :color

    def initialize( args=CommandArgs.new )
      @start  = Vector[ args.from[0], args.from[1], 0 ]
      @end    = Vector[ args.to[0],   args.to[1],   0 ]

      @color  = args.clr
    end

    def render( renderer ) ; end # pure virtual. is this dumb?

    # translate part so that @start becomes equal to new_position
    def reposition( new_position )
      translate( new_position - @start )
    end

    # move part in the direction of the Vector argument: offset
    def translate( offset )
      @start  = @start + offset
      @end    = @end   + offset
      self
    end

    # general linear transform
    # arg: mat  ...  type 3x3 numeric Matrix
    def linear_transform( mat )
      @start  = mat * @start
      @end    = mat * @end
      self
    end

    # args:
    #   matrix_valued_function   a function that takes a 3D Vector and returns a 3x3 Matrix
    #   vector_valued_function   a function that takes a 3D Vector and returns a 3D Vector
    # returns:
    #   self   (ie 'this' Part)
    def position_dependent_transform( matrix_valued_function = nil, vector_valued_function =nil )
      stt_copy = @start
      end_copy = @end

      newstart = @start
      newend   = @end

      unless matrix_valued_function.nil?
        newstart = matrix_valued_function.call( stt_copy ) * stt_copy
        newend = matrix_valued_function.call( end_copy ) * end_copy
      end

      unless vector_valued_function.nil?
        newstart = newstart + vector_valued_function.call( stt_copy )
        newend   = newend   + vector_valued_function.call( end_copy )
      end

      @start = newstart
      @end   = newend
      self
    end

  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class Command

  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class Arc < Command
  class Arc < Command
    attr_reader :center
    attr_accessor :sweep, :largearc, :radius

    TINY = 0.00000001

    def initialize( args=CommandArgs.new )
      super( args )
      @center = Vector[ args.cx,  args.cy, 0 ]
      @sweep    = args.sweep
      @largearc = args.largearc
      @radius   = args.radius

      if (((@center[0].nil? ? 0 : 1) + (@center[1].nil? ? 0 : 1)) % 2) != 0
        raise ArgumentError.new("Arc Center Vector must be entirely nil or numeric. center == #{@center}")
      end

      if ! @center[0].nil? && ( ! @largearc.nil? || ! @sweep.nil? )

        # combined with the starting point, ending point, and sweep,
        # we can calculate the radius and largearc-flag
        #
        # w = (u x v)
        #
        # if( w[2] > 0 &&   sweep ) largearc-flag = 0;
        # if( w[2] > 0 && ! sweep ) largearc-flag = 1;
        # if( w[2] < 0 &&   sweep ) largearc-flag = 1;
        # if( w[2] < 0 && ! sweep ) largearc-flag = 0;

        u = @start - @center
        v = @end - @center

        #w = u.cross_product( v )
        w = Vector[ u[1]*v[2] - v[1]*v[2], -(u[0]*v[2] - v[0]*u[2]), u[0]*v[1] - v[0]*u[1]  ]

        if ! @sweep.nil? && @largearc.nil?
          if w[2] < -TINY
            @largearc = @sweep
          elsif w[2] > TINY
            @largearc = ! @sweep
          else
            @largearc = 1
          end
        elsif @sweep.nil? && ! @largearc.nil?
          if w[2] < -TINY
             @sweep = @largearc
          elsif w[2] > TINY
            @sweep = ! @largearc
          else
            raise RuntimeError.new( "sweep is ambiguous with 180-degree arc. sweep must be defined" )
          end
        else
          # verify that the everything is in agreement
          if w[2] < -TINY && @sweep != @largearc then raise RuntimeError.new( "sweep disagrees with largearc given start,stop,center" ) ; end
          if w[2] >  TINY && @sweep == @largearc then raise RuntimeError.new( "sweep disagrees with largearc given start,stop,center" ) ; end
        end

        # assert( at this point, largearc and sweep are defined and agree )
        if (u.norm - v.norm).abs > TINY
          raise RuntimeError.new( "radius calculation failure. u.norm:#{u.norm} != v.norm:#{v.norm} diff: #{(u.norm - v.norm).abs}" )
        end

        if ! @radius.nil?
          if (@radius - v.norm).abs > TINY
            raise RuntimeError.new( "the provided radius(#{@radius}) and calculated(#{v.norm}) radii differ(#{(@radius - v.norm).abs}) by too much." )
          end
          # if no error then radius is defined and correct.
        else
          @radius = (u.norm == v.norm ? u.norm : 0.5*(u.norm + v.norm))
        end
      end


    end

    def render( renderer )
      renderer.arcto( self )
    end

    # move part in the direction of the Vector argument: offset
    def translate( offset )
      super( offset )
      @center = @center + offset
    end

    # general linear transform
    def linear_transform( mat )
      super( mat )
      @center = mat * @center
      @radius = (@start - @center).norm
    end

    # method: position_dependent_transform
    # args:
    #   matrix_valued_function   a function that takes a 3D Vector and returns a 3x3 Matrix
    #   vector_valued_function   a function that takes a 3D Vector and returns a 3D Vector
    # returns:
    #   self   (ie 'this' Part)
    def position_dependent_transform( matrix_valued_function = nil, vector_valued_function =nil )
      super( matrix_valued_function, vector_valued_function )

      cpyc = @center
      newc = @center

      unless matrix_valued_function.nil?
        newc = matrix_valued_function.call( cpyc ) * cpyc
      end

      unless vector_valued_function.nil?
        newc = newc + vector_valued_function.call( cpyc )
      end
      @center = newc

      self
    end


  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class Arc < Command

  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class Line < Command
  class Line < Command
    def initialize( args=CommandArgs.new )
      super( args )
    end

    def render( renderer )
      renderer.lineto( self )
    end
  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class Line < Command

  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class Move < Command
  class Move < Command
    def initialize( args=CommandArgs.new )
      super( args )
    end

    def render( renderer )
      renderer.moveto( self )
    end
  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class Move < Command

  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class Part
  class Part
    attr_reader :commands, :boundingbox, :start, :curpos

    def initialize( startpoint=nil )
      if startpoint.nil? || (startpoint.class == Vector && startpoint.size < 2)
        @start = Vector[0,0,0]

      elsif startpoint.class != Vector
        raise ArgumentError.new("Part requires a Vector")

      elsif startpoint.size == 2 
        @start = Vector[ startpoint[0], startpoint[1], 0 ]
      else
        @start = startpoint
      end

      @curpos = @start

      @commands = Array.new
      @boundingbox = { :min => @start, :max => @start }
    end

    def update_position( newpos )
      if @curpos != newpos
        @curpos = newpos
        if @commands.size == 0
          @start = @curpos 
        end
        update_bounds( @curpos )
      end
      self
    end

    def update_bounds( v )
      mn = @boundingbox[:min]
      mx = @boundingbox[:max]
      @boundingbox[:min] = Vector[ mn[0] < v[0] ? mn[0] : v[0], mn[1] < v[1] ? mn[1] : v[1], 0 ]
      @boundingbox[:max] = Vector[ mx[0] > v[0] ? mx[0] : v[0], mx[1] > v[1] ? mx[1] : v[1], 0 ]
    end
    private :update_bounds  #callable by self and ihereters

    def render( renderer )
      @commands.each { |cmd| cmd.render( renderer ) }
      self
    end

    # #######################################################################
    # ######  Use these {arcto, lineto, moveto} to add commands
    # #######################################################################
    def arcto( args )
      args.from = @curpos
      @commands << Arc.new( args )
      update_position( args.to )
    end

    def lineto( args )
      args.from = @curpos
      @commands << Line.new( args )
      update_position( args.to )
    end

    def moveto( args )
      args.from = @curpos
      @commands << Move.new( args )
      update_position( args.to )
    end

    # #######################################################################
    # ######  Use these to transform the part
    # #######################################################################

    # translate part so that @start becomes equal to new_position
    def reposition( new_position )
      @commands.each { |cmd| cmd.reposition( new_position ) }
      self
    end

    # move part in the direction of the Vector argument: shift_direction
    def translate( offset )
      @commands.each { |cmd| cmd.translate( offset ) }
      self
    end


    def linear_transform( mat )
      @commands.each { |cmd| cmd.linear_transform( mat ) }
      self
    end

    def scale( multiple )
      mat = Matrix[ [multiple, 0, 0], [0, multiple, 0], [0, 0, multiple] ]
      linear_transform( mat )
      self
    end

    # args:
    #   matrix_valued_function   a function that takes a 3D Vector and returns a 3x3 Matrix
    #   vector_valued_function   a function that takes a 3D Vector and returns a 3D Vector
    # returns:
    #   self   (ie 'this' Part)
    def position_dependent_transform( matrix_valued_function = nil, vector_valued_function =nil )
      start_copy = @start
      cur_copy = @curpos

      newstart = @start
      newcur = @curpos

      unless matrix_valued_function.nil?
        newstart = matrix_valued_function.call( start_copy ) * start_copy
        newcur   = matrix_valued_function.call( cur_copy )   * cur_copy
      end

      unless vector_valued_function.nil?
        newstart = newstart + vector_valued_function.call( start_copy )
        newcur   = newcur   + vector_valued_function.call( cur_copy )
      end

      @start = newstart
      @curpos = newcur

      @commands .each { |c| c.position_dependent_transform( matrix_valued_function, vector_valued_function ) }
      self
    end

    # #######################################################################
    # ######  Maintenance
    # #######################################################################

    def remake_boundingbox()
      @boundingbox[:min] = Vector[ commands[0].start[0], commands[0].start[1], 0 ]
      @boundingbox[:max] = @boundingbox[:min]
      @commands.each { |c| update_bounds( c.start ); update_bounds( c.end ) } # replay bounding-box creation
    end
  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class Part


  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class PartContext
  class PartContext
    attr_reader :partlist, :boundingbox, :firstpoint

    def initialize( startpoint=nil )
      @firstpoint = startpoint.nil? ? Vector[0,0,0] : Vector[ startpoint[0], startpoint[1], 0 ]

      @boundingbox = { :min => @firstpoint, :max => @firstpoint }
      @partlist = []
    end

    def add_part( prt )

      if @partlist.size > 0
        # if there's a position gap between the last part added and the new part being added then, at least
        # for g-code, there must be a command that bridges the gap. assume 'moveto' is that implicit command
        # and make it explicit by adding an additional one-command part before we follow through with the
        # requested Part addition.  XXX  This perhaps should be optional, so maybe add an option.
        unless @partlist[-1].curpos == prt.start
          p = Part.new( @partlist[-1].curpos )     # make a new part starting at last of most recent pushed
          p.moveto( CommandArgs.new( prt.start ) ) # containing a single moveto command ending at start of next
          @partlist << p
        end
      end

      @partlist << prt
      update_bbox( prt )
    end

    def update_bbox( prt )
      @boundingbox[:min] = Vector[ @boundingbox[:min][0] < prt.boundingbox[:min][0] ? @boundingbox[:min][0] : prt.boundingbox[:min][0],
                                   @boundingbox[:min][1] < prt.boundingbox[:min][1] ? @boundingbox[:min][1] : prt.boundingbox[:min][1], 0 ];
      @boundingbox[:max] = Vector[ @boundingbox[:max][0] > prt.boundingbox[:max][0] ? @boundingbox[:max][0] : prt.boundingbox[:max][0],
                                   @boundingbox[:max][1] > prt.boundingbox[:max][1] ? @boundingbox[:max][1] : prt.boundingbox[:max][1], 0 ];
    end

    def lastcoords
      if @partlist.size == 0 then @firstpoint else @partlist[-1].curpos end
    end

    def get_new_part
      Part.new( lastcoords )
    end

    # #######################################################################
    # ######  Use these to transform all parts
    # #######################################################################

    def reposition( new_position )
      @partlist.each { |p| p.reposition( new_position ) }
      @boundingbox[:max] = (@boundingbox[:max] - @boundingbox[:min]) + new_position 
      @boundingbox[:min] = new_position
      self
    end

    def translate( offset )
      @partlist.each { |p| p.translate( offset ) }
      @boundingbox[:min] = @boundingbox[:min] + offset
      @boundingbox[:max] = @boundingbox[:max] + offset
      self
    end

    def scale( multiple )
      mat = Matrix[ [multiple, 0, 0], [0, multiple, 0], [0, 0, multiple] ]
      @partlist.each { |p| p.linear_transform( mat ) }
      @boundingbox[:min] = mat * @boundingbox[:min]
      @boundingbox[:max] = mat * @boundingbox[:max]
      self
    end

    def linear_transform( mat )
      @partlist.each { |p| p.linear_transform( mat ) }
      @boundingbox[:min] = mat * @boundingbox[:min]
      @boundingbox[:max] = mat * @boundingbox[:max]
      self
    end

    def position_dependent_transform( matrix_valued_function = nil, vector_valued_function = nil )
      if ! ( matrix_valued_function.nil? && vector_valued_function.nil? )
        @partlist.each { |p| p.position_dependent_transform( matrix_valued_function, vector_valued_function ) }

        minnew = mincpy = @boundingbox[:min]
        maxnew = maxcpy = @boundingbox[:max]

        unless matrix_valued_function.nil?
          minnew = matrix_valued_function.call( mincpy ) * mincpy
          maxnew = matrix_valued_function.call( maxcpy ) * maxcpy
        end

        unless vector_valued_function.nil?
          minnew = minnew + vector_valued_function.call( mincpy )
          maxnew = maxnew + vector_valued_function.call( maxcpy )
        end

        @boundingbox[:min] = minnew
        @boundingbox[:max] = maxnew

      end
      self
    end

    def remake_boundingbox()
      # remake bounding-box
      partlist[0].remake_boundingbox()
      
      @boundingbox[:min] = Vector[ partlist[0].start[0], partlist[0].start[1], 0 ]
      @boundingbox[:max] = @boundingbox[:min]

      @partlist.each { |p| p.remake_boundingbox(); update_bbox( p ) }

    end

  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class PartContext


  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class Renderer
  class Renderer
    attr_reader :boundingbox, :offset
    attr_accessor :eol, :mult

    def initialize( startpoint=Vector[0,0,0], scaling=1, eol="\r\n", offset=Vector[0,0,0] )

      @start = startpoint
      @offset = offset
      raise ArgumentError.new("vector must be 3D") if @start.size  != 3
      raise ArgumentError.new("vector must be 3D") if @offset.size != 3

      @mult = scaling
      @eol = eol

      @boundingbox = { :min => @start, :max => @start }
    end

    def offset=( os )
      raise ArgumentError.new("offset vector must be 3D") if os.size != 3
      @offset = os
    end

    def set_line_end( chr )
      @eol = chr
    end

    def p( string )
      printf '%s%s', string, @eol
    end

    # this is the interface that should be overridden by subclasses.
    # in well-made Ruby code, are methods overridden? Or are mixins prefered?
    def print_start( boundingbox=nil ) ; end
    def print_end   ; end

    def render_all( parts )
      print_start( parts.boundingbox )
      parts.partlist.each { |p| p.render( self ) }
      print_end
    end

    def render( part )
      part.render( self )
    end

  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class Renderer

  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class GCode < Renderer
  class GCode < Renderer

    def initialize( start=Vector[0,0,0], scaling=1, eol="\r\n", offset=Vector[0,0,0], vbose=false, add_cutting=true )
      super( start, scaling, eol, offset )
      @cutting_on = false
      @verbose = vbose
      @add_cutting = add_cutting
      @last_command = ''
    end


    def print_start( boundingbox=nil )    # codes to basically turn the cutting heads off and stuff lke that.
      p( '*' )
      p( 'M08' )
      p( 'M16' )
      p( 'G20' )
      p( 'G40' )
      p( 'G90' )
    end

    def print_end
      cutting_off
      p( 'M30' )
    end

    def cutting_off
      if @cutting_on 
        p( 'M16' )
        @cutting_on = false;
      end
    end

    def cutting_on
      if ! @cutting_on 
        p( 'M15' )
        @cutting_on = true;
      end
    end

    def prep_coords( cmd )

      x_str, y_str = BWCNC.VectorToNumStringArray( (cmd.end + @offset)*@mult, 2 ) 
      i_str, j_str  = [ nil, nil ]
      if cmd.class == Arc
        i_str, j_str = BWCNC.VectorToNumStringArray( (cmd.center - cmd.start) * @mult, 2 ) 
      end

      [x_str, y_str, i_str, j_str]

    end

    def prep_cmd_start( gcode )

      if @add_cutting
        if gcode.match( /^g0+$/i )
          cutting_off
        elsif gcode.match( /^g0*[123]$/i )
          cutting_on
        end
      end


      # data transfer to the the Koike is very slow, so reduce
      # characters whenever it makes sense. (or when possible.)
      if @last_command == gcode && ! @verbose
        gcode = ''
      elsif @verbose                   # assert(@last_command != gcode || @verbose || (@last_command != gcode && @verbose))
        @last_command = gcode
        gcode += ' '
      else
        @last_command = gcode
      end

      gcode
    end


    def linear_motion( cmd, gcode )
      s = prep_cmd_start( gcode )
      x_str, y_str, i_str, j_str = prep_coords( cmd )
      p( sprintf( '%sX%sY%s', s, x_str, y_str ) )
    end

    def lineto( cmd )
      linear_motion( cmd, 'G01' )
    end

    def moveto( cmd )
      linear_motion( cmd, 'G01' )
    end

    def arcto( cmd )

      # In SVG, 'sweep' is a positive angle, but rotates clockwise.
      # In Koike gcode we decide that sweep is still positive, but counter-clockwise.
      # This correctly implements the mirror image flip around the x-axis

      gcode = prep_cmd_start( @sweep ? 'G03' : 'G02' )
      x_str, y_str, i_str, j_str = prep_coords( cmd )
      p( sprintf( '%sX%.03fY%.03fI%.03fJ%.03f', gcode, x_str, y_str, i_str, j_str ) )

    end

  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class GCode < Renderer

  #BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB class SVG < Renderer
  class SVG < Renderer
    @moveto_color = '#0000ff'
    @lineto_color = '#ff0000'
    @stroke_width = 1

    class << self
      attr_accessor :moveto_color, :lineto_color, :stroke_width
    end


    def initialize( start=Vector[0,0,0], scaling=1, eol="\r\n", offset=Vector[0,0,0] )
      super( start, scaling, eol, offset )
    end

    def set_options( opts )
      option_names = []
      opts
    end

    def print_start( boundingbox )
      widt, heit = [ boundingbox[:max][0].ceil + 1, boundingbox[:max][1].ceil + 1 ]
      p( '<?xml version="1.0" encoding="UTF-8" standalone="no"?>' )
      p( "<svg xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns=\"http://www.w3.org/2000/svg\" height=\"#{heit}\" width=\"#{widt}\">" )
    end

    def print_end
      p( "</svg>" )
    end

    def drawline( cmd, color )

      begp = BWCNC.VectorToNumStringArray( (cmd.start + @offset) * @mult, 2 )
      endp = BWCNC.VectorToNumStringArray( (cmd.end   + @offset) * @mult, 2 )

      printf '<line x1="%s" y1="%s" x2="%s" y2="%s" style="stroke:%s;stroke-width:%s" />%s',
        begp[0], begp[1], endp[0], endp[1], color, SVG.stroke_width, eol

    end


    def lineto( cmd )
      return if SVG.lineto_color == 'none'
      drawline( cmd, SVG.lineto_color )
    end

    def moveto( cmd )
      return if SVG.moveto_color == 'none'
      drawline( cmd, SVG.moveto_color )
    end


    def arcto( cmd )
      return if SVG.lineto_color == 'none'

      begp = BWCNC.VectorToNumStringArray( (cmd.start  + @offset) * @mult, 2 )
      endp = BWCNC.VectorToNumStringArray( (cmd.end    + @offset) * @mult, 2 )
      rdus = cmd.radius * @mult

      printf '<path d="M%s %s A%s,%s 0 %d,%d %s,%s" style="stroke:%s;stroke-width:%s;fill:none" />%s',
        begp[0], begp[1],
        NumString.new(rdus), NumString.new(rdus),
        (cmd.largearc ? 1 : 0), (cmd.sweep ? 1 : 0),
        endp[0], endp[1],
        SVG.lineto_color, SVG.stroke_width, eol

    end

  end
  #EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE class SVG < Renderer

end
