
require 'rubygems'
require 'uuid'

class NullMigrator
  def process line
    puts line
  end
end

class ModelMigrator 
  def initialize
    @transport_id_re = / transportId=['"]([^'"]*)['"]/
    @transport_config_re = / transportConfig=['"][^'"]*['"]/
  end

  def process line
    puts replaceTransportIdWithTransportConfig line
  end

  def replaceTransportIdWithTransportConfig line
    if transport_id = @transport_id_re.match(line)
      # if line has transport config attribute, just remove transport id
      if line =~ @transport_config_re
        line.sub @transport_id_re, ''
      # else line has no transport config attribute, 
      # replace transport id with transport config
      else
        line.sub(@transport_id_re, " transportConfig=\"trans#{transport_id[1]}\"")
      end
    else
      line
    end
  end
end

class Transport
  attr_accessor :transport_id
  def initialize id, type 
    @transport_id = id
    @xmi_id = "_" + UUID.new.generate
    @text = []
    @type = type
  end

  def xmi_id
    @xmi_id
  end

  def <<(s)
    @text << s
  end

  def output
    puts "    <transport xsi:type=\"generator:#{@type}\" xmi:id=\"#{xmi_id}\" name=\"#{@transport_id}\">"
    puts @text.map {|l| l.sub(/^ */, '      ')}
    puts "    </transport>"
  end
end

class GeneratorMigrator
  
  def initialize
    # transports is hash from trasnsport ID to 
    # array of [uuid, xml-string]
    @transports = []

    @transport_offset_re = /<transportOffset/
    @transport_open_re = /<transport transportIndex=['"]([^'"]*)['"]/
    @transport_close_re = /<\/transport>/
    @new_transport_open_re = /<transport .*xmi:id=['"]([^'"]*)['"]/
    @new_transport_ref_re = /<transportRef.*transport=['"]([^'"]*)['"]/
    @tcp_open_re = /<TCPTransport>/
    @tcp_close_re = /<\/TCPTransport>/
    @tcp_open_close_re = /<TCPTransport\/>/
    @udp_open_re = /<UDPTransport>/
    @udp_close_re = /<\/UDPTransport>/
    @udp_open_close_re = /<UDPTransport\/>/
    @mc_open_re = /<MulticastTransport>/
    @mc_close_re = /<\/MulticastTransport>/
    @mc_open_close_re = /<MulticastTransport\/>/
    @generator_open_re = /<\generator:CodeGen (.*)>/
    @generator_close_re = /<\/generator:CodeGen>/
    @transports_re = /<transports>/
    @swap_bytes_re = /<swap_bytes *value=['"]([^'"]*)['"]/
    @passive_connect_duration_re = /<passive_connect_duration *value=['"]([^'"]*)['"]/
  end

  def output_transports
    puts "  <transports>"
    @transports.each {|trns| trns.output }
    puts "  </transports>"
  end

  def transport type
    @current_transport = Transport.new(@current_transport_id, type)
    @transports << @current_transport
    puts "      <config name=\"#{@current_transport_id}\">"
    puts "        <transportRef transport=\"#{@current_transport.xmi_id}\"/>"
  end

  def close_transport
    @current_transport = nil
    @current_transport_id = nil
    puts "      </config>"
  end

  def process line
    if line =~ @transport_offset_re
      # do nothing - removed from model
    elsif results = @transport_open_re.match(line)
      @current_transport_id = "trans" + results[1] # remember for later
      $stderr.puts "got name #{ @current_transport_id} from #{results[0]}"
    elsif line =~ @transport_close_re
      puts line if @transports.empty? # already migrated
    elsif line =~ @swap_bytes_re || line =~ @passive_connect_duration_re
      if @current_transport
        puts line.sub(/^ */, '        ') # change indent
      else
        puts line # already migrated
      end
    elsif line =~ @tcp_open_re
      transport 'TcpTransport' # output config now, transport later
    elsif line =~ @udp_open_re
      transport 'UdpTransport' # output config now, transport later
    elsif line =~ @mc_open_re
      transport 'MulticastTransport' # output config now, transport later
    elsif line =~ @tcp_close_re || line =~ @udp_close_re || line =~ @mc_close_re
      close_transport # finish config output
    elsif line =~ @tcp_open_close_re
      transport 'TcpTransport' # output config now, transport later
      close_transport # finish config output
    elsif line =~ @udp_open_close_re
      transport 'UdpTransport' # output config now, transport later
      close_transport # finish config output
    elsif line =~ @mc_open_close_re
      transport 'MulticastTransport' # output config now, transport later
      close_transport # finish config output
    elsif line =~ @transports_re
      @has_transports = true # remember in case rerunning file
      puts line
    elsif results = @generator_open_re.match(line)
      if line =~ /xmlns:xsi=/
        puts line # rerunning file
      else
        puts "<generator:CodeGen #{results[1]} xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
      end
    elsif line =~ @generator_close_re
      output_transports unless @has_transports # output transportInsts
      puts line
    elsif results = @new_transport_open_re.match(line)
      if results[1][0,1] == '_'
        puts line # line is fine, leading underscore in NCName
      else
        puts line.sub(/xmi:id=['"][^'"]*['"]/, "xmi:id=\"_#{results[1]}\"")
      end
    elsif results = @new_transport_ref_re.match(line)
      if results[1][0,1] == '_'
        puts line # line is fine, leading underscore in NCName
      else
        puts line.sub(/transport=['"][^'"]*['"]/, "transport=\"_#{results[1]}\"")
      end
      
    elsif @current_transport
      @current_transport << line # save text for later output
    else
      puts line
    end
  end
end

if ARGV.empty?
   puts "missing filename argument"
   puts "  NOTE: both model and codegen accepted"
   exit
end


File.open(ARGV[0]) do |file|
  @model_re = /<opendds:OpenDDSModel /
  @generator_re = /<generator:CodeGen /
  @migrator = nil

  def get_migrator line
    if line =~ @model_re
      @migrator = ModelMigrator.new
    elsif line =~ @generator_re
      @migrator = GeneratorMigrator.new
    else
      NullMigrator.new
    end
  end

  file.each_line do |file|
    file.each_line do |line|
      (@migrator || get_migrator(line)).process(line)
    end
  end
end

