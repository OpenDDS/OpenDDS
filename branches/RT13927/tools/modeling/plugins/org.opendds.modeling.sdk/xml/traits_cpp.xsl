<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:lut='http://www.opendds.org/modeling/schemas/Lut/1.0'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.org/modeling/schemas/Generator/1.0'>
  <!--
    ** $Id$
    **
    ** Generate C++ implementation code.
    **
    ** @TODO - check string management.
    ** @TODO - determine how to set the transport addresses.
    **
    -->
<xsl:include href="common.xsl"/>

<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "document(/generator:CodeGen/source/@name)/opendds:OpenDDSModel/@name"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">
  <xsl:value-of select="concat('#include &quot;', $modelname, 'Traits.h&quot;', 
                               $newline)"/>
  <xsl:text>
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/multicast/MulticastConfiguration.h"
#include "dds/DCPS/transport/udp/UdpConfiguration.h"
#include &lt;stdexcept&gt;
</xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="//instance">
  <xsl:variable name="Instname">
    <xsl:call-template name="capitalize">
      <xsl:with-param name="value" select="@name"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="classname" select="concat($Instname, $modelname, 'Traits')"/>
  <xsl:value-of select="concat('void ', $classname)"/>
  <xsl:text>::transport_config(OpenDDS::DCPS::TransportIdType id) {
  OpenDDS::DCPS::TransportConfiguration_rch config;
  ACE_TString transport_type;

  try {
    config = TheTransportFactory->get_configuration(id);
  } catch (OpenDDS::DCPS::Transport::NotConfigured&amp; nc) {
    // Create configuration for this transport ID
    switch (id) {
</xsl:text>
<xsl:apply-templates/>
<xsl:text>      default:
        throw std::runtime_error("Invalid transport ID in configuration");
    };
  }

  // Create the impl
  OpenDDS::DCPS::TransportImpl_rch impl = TheTransportFactory->obtain(id);
  if (!impl.in()) {
    impl = TheTransportFactory->create_transport_impl(id, true);
  }
}
</xsl:text>

</xsl:template>

<xsl:template match="transport">
  <xsl:variable name="type" select="*/@transport_type"/>
  <xsl:variable name="label" select="../transportOffset/@value + @transportIndex"/>
  <xsl:value-of select="concat('      case ', $label, ':', $newline)"/>
  <xsl:value-of select="concat('        transport_type = &quot;', $type, '&quot;;', $newline)"/>
  <xsl:text>        config = TheTransportFactory->create_configuration(id, transport_type);
</xsl:text>
  <xsl:apply-templates/>
  <xsl:value-of select="concat('        break;', $newline, $newline)"/>
</xsl:template>

<xsl:template match="swap_bytes">
  <xsl:value-of select="concat('          config->swap_bytes_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="queue_messages_per_pool 
                   | queue_initial_pools
                   | max_packet_size
                   | max_samples_per_packet
                   | optimum_packet_size
                   | thread_per_connection
                   | datalink_release_delay
                   | datalink_control_chunks">
  <xsl:value-of select="concat('          config->', name(), '_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="TCPTransport">
  <xsl:text>        {
          OpenDDS::DCPS::SimpleTcpConfiguration* specific_config =
              (OpenDDS::DCPS::SimpleTcpConfiguration*) config.in();
</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>        }
</xsl:text>
</xsl:template>

<xsl:template match="MulticastTransport">
  <xsl:text>        {
          OpenDDS::DCPS::MulticastConfiguration* specific_config =
              (OpenDDS::DCPS::MulticastConfiguration*) config.in();
</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>        }
</xsl:text>
</xsl:template>

<xsl:template match="opendds:udpConfig">
  <xsl:text>        {
          OpenDDS::DCPS::UdpConfiguration* specific_config =
              (OpenDDS::DCPS::UdpConfiguration*) config.in();
</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>        }
</xsl:text>
</xsl:template>

<xsl:template match="local_address_str">
  <xsl:variable name="value">
    <xsl:call-template name="str-value"/>
  </xsl:variable>
  <xsl:value-of select="concat('          specific_config->local_address_ = ',
                               'ACE_INET_Addr(&quot;', $value, '&quot;)',
                               ';', $newline)"/>
</xsl:template>

<xsl:template match="enable_nagle_algorithm
                   | conn_retry_initial_delay
                   | conn_retry_backoff_multiplier
                   | conn_retry_attempts
                   | max_output_pause_period
                   | passive_reconnect_duration
                   | passive_connect_duration
                   | default_to_ipv6
                   | port_offset
                   | reliable
                   | syn_backoff
                   | syn_interval
                   | syn_timeout
                   | nak_depth
                   | ttl
                   | rcv_buffer_size
                   | nak_interval
                   | nak_timeout">

  <xsl:value-of select="concat('          specific_config->', name(),  '_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="group_address">
  <xsl:variable name="value">
    <xsl:call-template name="str-value"/>
  </xsl:variable>
  <xsl:value-of select="concat('          specific_config->group_address_ = ',
                               'ACE_INET_Addr(&quot;', $value, '&quot;)',
                               ';', $newline)"/>
</xsl:template>

<xsl:template name="str-value">
  <xsl:param name="value" select="@value"/>
  <xsl:choose>
    <xsl:when test="starts-with($value, '&quot;')">
      <xsl:value-of select="substring-before(substring-after($value, '&quot;'), '&quot;')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$value"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
</xsl:stylesheet>
