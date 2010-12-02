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
<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "/generator:model/@name"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">
  <xsl:variable name="classname" select="concat($modelname, 'Traits')"/>
  <xsl:value-of select="concat('#include &quot;', $classname, '.h&quot;', $newline)"/>
  <xsl:text>
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/multicast/MulticastConfiguration.h"
#include "dds/DCPS/transport/udp/UdpConfiguration.h"
#include &lt;stdexcept&gt;

void
</xsl:text>
<xsl:value-of select="$classname"/>
<xsl:text>::transport_config(OpenDDS::DCPS::TransportIdType id) {
  OpenDDS::DCPS::TransportConfiguration_rch config;
  ACE_TString transport_type;
  try {
    config = TheTransportFactory->get_configuration(id);
  } catch (OpenDDS::DCPS::Transport::NotConfigured&amp; nc) {
    // Create configuration for this transport ID
    switch (id) {
</xsl:text>
<xsl:apply-templates select="//opendds:transport"/>
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

<xsl:template match="opendds:transport">
  <xsl:message>transport </xsl:message>
  <xsl:variable name="type" select="opendds:commonConfig/opendds:transport_type/@value"/>
  <xsl:value-of select="concat('      case ', @key, ':', $newline)"/>
  <xsl:value-of select="concat('        transport_type = &quot;', $type, '&quot;;', $newline)"/>
  <xsl:text>        config = TheTransportFactory->create_configuration(id, transport_type);
</xsl:text>
  <xsl:apply-templates select="*"/>
  <xsl:value-of select="concat('        break;', $newline, $newline)"/>
</xsl:template>

<xsl:template match="opendds:swap_bytes">
  <xsl:value-of select="concat('        config->swap_bytes_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:queue_messages_per_pool">
  <xsl:value-of select="concat('        config->queue_messages_per_pool_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:queue_initial_pools">
  <xsl:value-of select="concat('        config->queue_initial_pools_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:max_packet_size">
  <xsl:value-of select="concat('        config->max_packet_size_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:max_samples_per_packet">
  <xsl:value-of select="concat('        config->max_samples_per_packet_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:optimum_packet_size">
  <xsl:value-of select="concat('        config->optimum_packet_size_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:thread_per_connection">
  <xsl:value-of select="concat('        config->thread_per_connection_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:datalink_release_delay">
  <xsl:value-of select="concat('        config->datalink_release_delay_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:datalink_control_chunks">
  <xsl:value-of select="concat('        config->datalink_control_chunks_ = ', 
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:tcpConfig">
  <xsl:text>        {
          OpenDDS::DCPS::SimpleTcpConfiguration* specific_config =
              (OpenDDS::DCPS::SimpleTcpConfiguration*) config.in();
</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>        }
</xsl:text>
</xsl:template>

<xsl:template match="opendds:multicastConfig">
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


<xsl:template match="opendds:local_address_str">
  <xsl:value-of select="concat('          specific_config->local_address_ = ',
                               'ACE_INET_Addr(&quot;', @value, '&quot;)',
                               ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:enable_nagle_algorithm">
  <xsl:value-of select="concat('          specific_config->enable_nagle_algorithm_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:conn_retry_initial_delay">
  <xsl:value-of select="concat('          specific_config->conn_retry_initial_delay_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:conn_retry_backoff_multiplier">
  <xsl:value-of select="concat('          specific_config->conn_retry_backoff_multiplier_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:conn_retry_attempts">
  <xsl:value-of select="concat('          specific_config->conn_retry_attempts_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:max_output_pause_period">
  <xsl:value-of select="concat('          specific_config->max_output_pause_period_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:passive_reconnect_duration">
  <xsl:value-of select="concat('          specific_config->passive_reconnect_duration_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:passive_connect_duration">
  <xsl:value-of select="concat('          specific_config->passive_connect_duration_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:default_to_ipv6">
  <xsl:value-of select="concat('          specific_config->default_to_ipv6_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:port_offset">
  <xsl:value-of select="concat('          specific_config->port_offset_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:group_address">
  <xsl:value-of select="concat('          specific_config->group_address_ = ',
                               'ACE_INET_Addr(&quot;', @value, '&quot;)',
                               ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:reliable">
  <xsl:value-of select="concat('          specific_config->reliable_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:syn_backoff">
  <xsl:value-of select="concat('          specific_config->syn_backoff_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:syn_interval">
  <xsl:value-of select="concat('          specific_config->syn_interval_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:syn_timeout">
  <xsl:value-of select="concat('          specific_config->syn_timeout_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:nak_depth">
  <xsl:value-of select="concat('          specific_config->nak_depth_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:ttl">
  <xsl:value-of select="concat('          specific_config->ttl_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:rcv_buffer_size">
  <xsl:value-of select="concat('          specific_config->rcv_buffer_size_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:nak_interval">
  <xsl:value-of select="concat('          specific_config->nak_interval_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

<xsl:template match="opendds:nak_timeout">
  <xsl:value-of select="concat('          specific_config->nak_timeout_ = ',
                               @value, ';', $newline)"/>
</xsl:template>

</xsl:stylesheet>
