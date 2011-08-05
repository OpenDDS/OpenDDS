
require 'rubygems'
require 'uuid'

class NullMigrator
  def process line
    puts line
  end
end

class ModelMigrator 
  def initialize
    @transport_id_re = / transportId=(['"][^'"]*['"])/
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
        line.sub(@transport_id_re, " transportConfig=#{transport_id[1]}")
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
    @xmi_id = UUID.new.generate
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
    puts "    <transport type=\"generator:#{@type}\" xmi:id=\"#{xmi_id}\">"
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
    @transport_open_re = /<transport transportIndex=['"]([^'"])*['"]/
    @transport_close_re = /<\/transport>/
    @tcp_open_re = /<TCPTransport>/
    @tcp_close_re = /<\/TCPTransport>/
    @udp_open_re = /<UDPTransport>/
    @udp_close_re = /<\/UDPTransport>/
    @mc_open_re = /<MulticastTransport>/
    @mc_close_re = /<\/MulticastTransport>/
    @generator_close_re = /<\/generator:CodeGen>/
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
    puts "      </config>"
  end

  def process line
    if line =~ @transport_offset_re
      # do nothing
    elsif results = @transport_open_re.match(line)
      @current_transport_id = results[1]
    elsif line =~ @transport_close_re
      @current_transport_id = nil
    elsif line =~ @tcp_open_re
      transport 'TcpTransport'
    elsif line =~ @udp_open_re
      transport 'UdpTransport'
    elsif line =~ @mc_open_re
      transport 'MulticastTransport'
    elsif line =~ @tcp_close_re || line =~ @udp_close_re || line =~ @mc_close_re
      @current_transport = nil
    elsif @current_transport
      @current_transport << line
    elsif @generator_close_re =~ line
      output_transports
      puts line
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

