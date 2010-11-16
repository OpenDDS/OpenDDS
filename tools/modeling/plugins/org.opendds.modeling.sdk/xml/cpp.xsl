<xsl:stylesheet version='1.0'
     xmlns:xmi='http://www.omg.org/XMI'
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

<!-- Documents -->
<xsl:variable name="lut" select="document('')/*/lut:tables"/>

<!-- Node sets -->
<xsl:variable name="typelib"     select="//generator:typelib"/>
<xsl:variable name="reader"      select="//readers"/>
<xsl:variable name="writer"      select="//writers"/>
<xsl:variable name="domain"      select="//domains"/>
<xsl:variable name="participant" select="//participants"/>
<xsl:variable name="publisher"   select="//publishers"/>
<xsl:variable name="subscriber"  select="//subscribers"/>
<xsl:variable name="topic"       select="//topics"/>
<xsl:variable name="types"       select="//types"/>
<xsl:variable name="transport"   select="//opendds:transport"/>

<!-- Indices (lookup tables are at the bottom of this document) -->
<xsl:key
     name  = "policies"
     match = "//opendds:policy"
     use   = "@name"/>

<xsl:key
     name  = "lut-qos-field"
     match = "qos-field"
     use   = "@type"/>

<xsl:key
     name  = "lut-transport"
     match = "transport"
     use   = "@type"/>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "/opendds:OpenDDSModel/@name"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">
  <xsl:text>
#include "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_T.h"

#include "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>TypeSupportImpl.h"</xsl:text>
  <xsl:value-of select="$newline"/>

  <!-- '#include "(//typelib/@name)TypeSupport.h"\n' -->
  <xsl:for-each select="$typelib">
    <!-- These are commented out for now. -->
    <xsl:text>// #include "</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>TypeSupportImpl.h"</xsl:text>
  </xsl:for-each>

  <xsl:text>

// For transport configuration
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/udp/UdpConfiguration.h"
#include "dds/DCPS/transport/multicast/MulticastConfiguration.h"

#include "dds/DCPS/Service_Participant.h"
#include "model/Utilities.h"

namespace OpenDDS { namespace Model { namespace </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text> {

template&lt; class InstanceTraits&gt;
inline
Elements::Data&lt;InstanceTraits&gt;::Data()
{
  for( int index = 0;
       index &lt; OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Types::LAST_INDEX;
       ++index
  ) {
    this->typeNames_[ index] = 0;
  }

  for (int index = 0;
       index &lt; OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Transports::LAST_INDEX;
       ++index) {
    this->transportConfigs_[index] = 0;
  }

  this->loadMasks();
  this->loadDomains();
  this->loadTopics();
  this->loadTransports();
  this->loadMaps(); /// MUST precede the QoS loading.

  this->buildParticipantsQos();
  this->buildTopicsQos();
  this->buildPublishersQos();
  this->buildSubscribersQos();
  this->buildPublicationsQos();
  this->buildSubscriptionsQos();
}

template&lt; class InstanceTraits&gt;
inline
Elements::Data&lt;InstanceTraits&gt;::~Data()
{
  for( int index = 0;
       index &lt; OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Types::LAST_INDEX;
       ++index
  ) {
    if( this->typeNames_[ index]) {
      free( this->typeNames_[ index]); // Was created by CORBA::string_dup()
      this->typeNames_[ index] = 0;
    }
  }
}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::registerType(
  Types::Values      type,
  DomainParticipant* participant
)
{
  switch( type) {
</xsl:text>
  <!-- '  case Types::(type/@name):\n ... \n  break;\n' -->
  <xsl:variable name="defined-types" select="$topic/opendds:datatype"/>
  <xsl:for-each select="$defined-types">
    <!-- Don't sort this without sorting the prior list as well. -->

    <!-- Only generate type code once for each type. -->
    <xsl:variable name="curpos" select="position()"/>
    <xsl:variable name="priors" select="$defined-types[ position() &lt; $curpos]"/>
    <xsl:if test="count( $priors[@name = current()/@name]) = 0">
      <xsl:text>    case Types::</xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text>:
      {
        typedef ::</xsl:text>
      <xsl:value-of select="$modelname"/>
      <xsl:text>::</xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text>TypeSupportImpl TypeSupport;

        TypeSupport* typeSupport = new TypeSupport();
        if( RETCODE_OK != typeSupport->register_type( participant, 0)) {
          throw BadRegisterException();
        }

        if( this->typeNames_[ type]) {
          free( this->typeNames_[ type]); // Was created by CORBA::string_dup()
        }
        this->typeNames_[ type] = typeSupport->get_type_name();
      }
      break;

</xsl:text>
    </xsl:if>
  </xsl:for-each>
  <xsl:text>    default:
      throw NoTypeException();
      break;
  }
}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::loadMasks()
{
</xsl:text>
  <!-- '  this->participantMasks_[ Participants::(domainParticipant/@name)] = (domainParticipant/@mask);\n' -->
  <xsl:for-each select="$participant/@mask">
    <xsl:text>  this->participantMasks_[ Participants::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>] = </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->publisherMasks_[ Publishers::(publisher/@name)] = (publisher/@mask);\n' -->
  <xsl:for-each select="$publisher/@mask">
    <xsl:text>  this->publisherMasks_[ Publishers::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>] = </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->subscriberMasks_[ Subscribers::(subscriber/@name)] = (subscriber/@mask);\n' -->
  <xsl:for-each select="$subscriber/@mask">
    <xsl:text>  this->subscriberMasks_[ Subscribers::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>] = </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->topicMasks_[ Topics::(topic/@name)] = (topic/@mask);\n' -->
  <xsl:for-each select="$topic/@mask">
    <xsl:text>  this->topicMasks_[ Topics::</xsl:text>
    <xsl:value-of select="translate(../@name,' ','_')"/>
    <xsl:text>] = </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->writerMasks_[ DataWriters::(dataWriters/@name)] = (dataWriters/@mask);\n' -->
  <xsl:for-each select="$writer/@mask">
    <xsl:text>  this->writerMasks_[ DataWriters::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>] = </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->readerMasks_[ DataReaders::(dataReaders/@name)] = (dataReaders/@mask);\n' -->
  <xsl:for-each select="$reader/@mask">
    <xsl:text>  this->readerMasks_[ DataReaders::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>] = </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::loadDomains()
{
</xsl:text>
  <!-- '  this->domains_[ Participants::(domainParticipant/@name)] = (domainParticipant/@domain);\n' -->
  <xsl:for-each select="$participant/@domain">
    <xsl:text>  this->domains_[ Participants::</xsl:text>
    <xsl:value-of select="../@name"/>
      <xsl:text>] = </xsl:text>
      <xsl:value-of select="$domain[@xmi:id = current()]/@domainId"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::loadTopics()
{
  /// @TODO verify how we manage the model strings.
</xsl:text>
  <!-- '  this->topicNames_[ Topics::(topic/@name)] = "(topic/@name)";\n' -->
  <xsl:for-each select="$topic/@name">
    <xsl:text>  this->topicNames_[ Topics::</xsl:text>
    <xsl:value-of select="translate(.,' ','_')"/>
    <xsl:text>] = "</xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>";</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::loadTransports()
{
  /// @TODO verify how we manage the model strings.
</xsl:text>
  <!-- '  this->transportKinds_[ Transports::(transport/@name)] = "(.../transport_type/@value)";\n' -->
  <xsl:for-each select="$transport/@key">
    <xsl:text>  this->transportKinds_[ Transports::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>] = "</xsl:text>
    <xsl:value-of select="../opendds:commonConfig/opendds:transport_type/@value"/>
    <xsl:text>";</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->transportKeys_[ Transports::(transport/@name)] = (transport/@key);\n' -->
  <xsl:for-each select="$transport/@key">
    <xsl:text>  this->transportKeys_[ Transports::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>] = InstanceTraits::transport_key_base + </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::loadMaps()
{
</xsl:text>
  <!-- '  this->publisherParticipants_[ Publishers::(publisher/@name)] = Participants::(publisher/../@name);\n' -->
  <xsl:for-each select="$publisher">
    <xsl:text>  this->publisherParticipants_[ Publishers::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>] = Participants::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->subscriberParticipants_[ Subscribers::(subscriber/@name)] = Participants::(subscriber/../@name);\n' -->
  <xsl:for-each select="$subscriber">
    <xsl:text>  this->subscriberParticipants_[ Subscribers::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>] = Participants::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->types_[ Topics::(topic/@name)] = Types::(topic/datatype/@name);\n' -->
  <xsl:for-each select="$topic">
    <xsl:text>  this->types_[ Topics::</xsl:text>
    <xsl:value-of select="translate(@name,' ','_')"/>
    <xsl:text>] = Types::</xsl:text>
    <xsl:value-of select="$types[@xmi:id = current()/@type]/@name"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->writerTopics[ DataWriters::(dataWriter/@name)] = Topics::(dataWriter/@topic);\n' -->
  <xsl:for-each select="$writer">
    <xsl:text>  this->writerTopics_[ DataWriters::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>] = Topics::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="$topic[@xmi:id = current()/@topic]/@name"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->readerTopics[ DataReaders::(dataReader/@name)] = Topics::(dataReader/@topic);\n' -->
  <xsl:for-each select="$reader">
    <xsl:text>  this->readerTopics_[ DataReaders::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>] = Topics::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="$topic[@xmi:id = current()/@topic]/@name"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->publishers_[ DataWriters::(dataWriter/@name)] = Publishers::(dataWriter/../@name);\n' -->
  <xsl:for-each select="$writer">
    <xsl:text>  this->publishers_[ DataWriters::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>] = Publishers::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->subscribers_[ DataReaders::(dataReader/@name)] = Subscribers::(dataReader/../@name);\n' -->
  <xsl:for-each select="$reader">
    <xsl:text>  this->subscribers_[ DataReaders::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>] = Subscribers::</xsl:text>
    <xsl:value-of select="../@name"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->publisherTransports_[ Publishers::(publisher/@name)] = Transports::(publisher/@transport);\n' -->
  <xsl:for-each select="$publisher">
    <xsl:text>  // this->publisherTransports_[ Publishers::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>] = Transports::</xsl:text>
    <xsl:value-of select="@transport"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->subscriberTransports_[ Subscribers::(subscriber/@name)] = Transports::(subscriber/@transport);\n' -->
  <xsl:for-each select="$subscriber">
    <xsl:text>  // this->subscriberTransports_[ Subscribers::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>] = Transports::</xsl:text>
    <xsl:value-of select="@transport"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::buildParticipantsQos()
{
  DomainParticipantQos participantQos;
  Participants::Values participant;
</xsl:text>
  <xsl:for-each select="$participant">
    <xsl:value-of select="$newline"/>
    <xsl:text>  participant = Participants::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>;
  participantQos = TheServiceParticipant->initial_DomainParticipantQos();
</xsl:text>
    <!-- '  participantQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  participantQos.'"/>
    </xsl:call-template>

    <xsl:text>  this->participantsQos_[ participant] = participantQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::buildTopicsQos()
{
  TopicQos       topicQos;
  Topics::Values topic;
</xsl:text>
  <xsl:for-each select="$topic">
    <xsl:value-of select="$newline"/>
    <xsl:text>  topic    = Topics::</xsl:text>
    <xsl:value-of select="translate(@name,' ','_')"/>
    <xsl:text>;
  topicQos = TheServiceParticipant->initial_TopicQos();
    
</xsl:text>
    <!-- '  topicQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  topicQos.'"/>
    </xsl:call-template>

    <xsl:text>  this->topicsQos_[ topic] = topicQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::buildPublishersQos()
{
  PublisherQos       publisherQos;
  Publishers::Values publisher;
</xsl:text>
  <xsl:for-each select="$publisher">
    <xsl:value-of select="$newline"/>
    <xsl:text>  publisher    = Publishers::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>;
  publisherQos = TheServiceParticipant->initial_PublisherQos();
</xsl:text>
    <!-- '  publisherQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  publisherQos.'"/>
    </xsl:call-template>

    <xsl:text>  this->publishersQos_[ publisher] = publisherQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::buildSubscribersQos()
{
  SubscriberQos       subscriberQos;
  Subscribers::Values subscriber;
</xsl:text>
  <xsl:for-each select="$subscriber">
    <xsl:value-of select="$newline"/>
    <xsl:text>  subscriber    = Subscribers::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>;
  subscriberQos = TheServiceParticipant->initial_SubscriberQos();
</xsl:text>
    <!-- '  subscriberQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  subscriberQos.'"/>
    </xsl:call-template>

    <xsl:text>  this->subscribersQos_[ subscriber] = subscriberQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::buildPublicationsQos()
{
  DataWriters::Values  writer;
  DataWriterQos        writerQos;
</xsl:text>
  <xsl:for-each select="$writer">
    <xsl:value-of select="$newline"/>
    <xsl:text>  writer    = DataWriters::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>;
  writerQos = TheServiceParticipant->initial_DataWriterQos();
</xsl:text>
    <!-- '  writerQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  writerQos.'"/>
    </xsl:call-template>

    <xsl:text>  this->writersQos_[ writer] = writerQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::buildSubscriptionsQos()
{
  DataReaders::Values  reader;
  DataReaderQos        readerQos;
</xsl:text>
  <xsl:for-each select="$reader">
    <xsl:value-of select="$newline"/>
    <xsl:text>  reader    = DataReaders::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>;
  readerQos = TheServiceParticipant->initial_DataReaderQos();
</xsl:text>
    <!-- '  readerQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  readerQos.'"/>
    </xsl:call-template>

    <xsl:text>  this->readersQos_[ reader] = readerQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

template&lt; class InstanceTraits&gt;
inline
OpenDDS::DCPS::TransportConfiguration*
Elements::Data&lt;InstanceTraits&gt;::transportConfig(Transports::Values which)
{
  if (which &lt; 0 || which &gt;= Transports::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  if (this->transportConfigs_[which]) {
    return this->transportConfigs_[which];
  }

  switch (which) {
</xsl:text>
  <xsl:for-each select="$transport">
    <!-- Lookup the configuration data type for this transport. -->
    <xsl:variable name="type" select="opendds:commonConfig/opendds:transport_type/@value"/>
    <xsl:variable name="config-type">
      <xsl:for-each select="$lut"> <!-- Change context for the lookup -->
        <xsl:value-of select="key('lut-transport', $type)/@configtype"/>
      </xsl:for-each>
    </xsl:variable>

    <xsl:text>  case Transports::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>:
    {
      unsigned long key = this->transportKey(which);
      const char* kind = this->transportKind(which);
      typedef </xsl:text>
    <xsl:value-of select="$config-type"/>
    <xsl:text> ConfigType;
      TransportConfiguration_rch baseconfig =
        TheTransportFactory->create_configuration(key, kind);
      ConfigType* config = static_cast&lt;ConfigType*&gt;(baseconfig.in());
      if (!config) {
        throw BadCastException();
      }
</xsl:text>
    <xsl:for-each select="./*">
      <xsl:value-of select="$newline"/>
      <xsl:text>      // </xsl:text>
      <xsl:value-of select="substring-after( name(), ':')"/>
      <xsl:value-of select="$newline"/>

      <!-- '  config->(configfield) = (value);\n' -->
      <xsl:call-template name="process-transports">
        <xsl:with-param name="config" select="."/>
        <xsl:with-param name="indent" select="'      '"/>
      </xsl:call-template>
    </xsl:for-each>

<xsl:text>
      return this->transportConfigs_[which] = baseconfig._retn();
    }
</xsl:text>
  </xsl:for-each>
  <xsl:text>  default:
    return 0;
  }
}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::copyPublicationQos(
  DataWriters::Values which,
  DataWriterQos&amp;  writerQos
)
{
  do{}while(&amp;writerQos==0); // In case we define no properties.

  switch( which) {
</xsl:text>
  <xsl:for-each select="$writer">
    <xsl:text>    case DataWriters::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>:</xsl:text>
    <xsl:value-of select="$newline"/>

    <!-- '  writerQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'      writerQos.'"/>
    </xsl:call-template>

    <xsl:text>      break;</xsl:text>
    <xsl:value-of select="$newline"/>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
<xsl:text>    default:
      throw NoWriterException();
  }
}

template&lt; class InstanceTraits&gt;
inline
void
Elements::Data&lt;InstanceTraits&gt;::copySubscriptionQos(
  DataReaders::Values which,
  DataReaderQos&amp;  readerQos
)
{
  do{}while(&amp;readerQos==0); // In case we define no properties.

  switch( which) {
</xsl:text>
  <xsl:for-each select="$reader">
    <xsl:text>    case DataReaders::</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>:</xsl:text>
    <xsl:value-of select="$newline"/>

    <!-- '  readerQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'      readerQos.'"/>
    </xsl:call-template>

    <xsl:text>      break;</xsl:text>
    <xsl:value-of select="$newline"/>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
<xsl:text>    default:
      throw NoReaderException();
  }
}

} } } // End of namespace OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>

</xsl:text>
</xsl:template>
<!-- End of main processing template. -->

<!-- Process QoS policy names into value assignments. -->
<xsl:template name="process-qos">
  <xsl:param name = "entity"/>
  <xsl:param name = "base"/>

  <!-- process each policy of the entity -->
  <xsl:for-each select="$entity/opendds:qosPolicy">
    <!-- the policy lib node for this policy name. -->
    <xsl:variable name="policy" select="key( 'policies', @name)"/>

    <!-- lookup the field name for the current policy type. -->
    <xsl:variable name="field">
      <xsl:for-each select="$lut"> <!-- Change context for the lookup -->
        <xsl:value-of select="key('lut-qos-field', $policy/@type)/text()"/>
      </xsl:for-each>
    </xsl:variable>

    <!-- lookup whether to quote the value. -->
    <xsl:variable name="should-quote">
      <xsl:for-each select="$lut"> <!-- Change context for the lookup -->
        <xsl:value-of select="key('lut-qos-field', $policy/@type)/@quote"/>
      </xsl:for-each>
    </xsl:variable>

    <!-- process all of the policy attributes -->
    <xsl:for-each select="$policy/@*">
      <xsl:choose>
        <!-- ignore the 'name' and 'type' attributes of the policy. -->
        <xsl:when test="name() = 'name' or name() = 'type'"/>

        <!-- OpenDDS::Model::stringToByteSeq( (base)(field).(name), (value)); -->
        <xsl:when test="../@type = 'opendds:udQosPolicy'
                     or ../@type = 'opendds:tdQosPolicy'
                     or ../@type = 'opendds:gdQosPolicy'">
          <xsl:text>  OpenDDS::Model::stringToByteSeq( </xsl:text>
          <xsl:value-of select="$base"/>
          <xsl:value-of select="$field"/>
          <xsl:text>.</xsl:text>
          <xsl:value-of select="name()"/>
          <xsl:text>, </xsl:text>

          <!-- quote the value if specified in the lookup table. -->
          <xsl:choose>
            <xsl:when test="$should-quote = 'true'">
              <xsl:text>"</xsl:text>
              <xsl:value-of select="."/>
              <xsl:text>"</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="."/>
            </xsl:otherwise>
          </xsl:choose>

          <xsl:text>);</xsl:text>
          <xsl:value-of select="$newline"/>
        </xsl:when>

        <!-- (base)(field).(name) = (value); -->
        <xsl:otherwise>
          <xsl:value-of select="$base"/>
          <xsl:value-of select="$field"/>
          <xsl:text>.</xsl:text>
          <xsl:value-of select="name()"/>
          <xsl:text> = </xsl:text>

          <!-- quote the value if specified in the lookup table. -->
          <xsl:choose>
            <xsl:when test="$should-quote = 'true'">
              <xsl:text>"</xsl:text>
              <xsl:value-of select="."/>
              <xsl:text>";</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="."/>
              <xsl:text>;</xsl:text>
            </xsl:otherwise>
          </xsl:choose>

          <xsl:value-of select="$newline"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>

    <!-- check for any contained elements for the current policy. -->
    <xsl:if test="count($policy/*)">
      <xsl:choose>
        <!-- the contained elements have attributes - they are self contained -->
        <xsl:when test="count( $policy/*/@*) > 0">
          <xsl:for-each select="$policy/*">
            <!-- strip the namespace - this may need to be tailored if the
                 namespace is not consistently present as the substring-after()
                 call will return an empty string in the case where no
                 namepsace is present.
              -->
            <xsl:variable name="subfield" select="substring-after( name(), ':')"/>

            <!-- process each attribute of the current element. -->
            <xsl:for-each select="./@*">
              <!-- '  (base)(field).(subfield).(name) = (value);\n' -->
              <xsl:value-of select="$base"/>
              <xsl:value-of select="$field"/>
              <xsl:text>.</xsl:text>
              <xsl:value-of select="$subfield"/>
              <xsl:text>.</xsl:text>
              <xsl:value-of select="name()"/>
              <xsl:text> = </xsl:text>
              <xsl:value-of select="."/>
              <xsl:text>;</xsl:text>
              <xsl:value-of select="$newline"/>
            </xsl:for-each>
          </xsl:for-each>
        </xsl:when>

        <!-- the contained elements have no attributes, they are part of a sequence -->
        <xsl:otherwise>
          <!-- Size of the sequence. -->
          <xsl:variable name="length" select="count($policy/*)"/>

          <!-- process each contained element. -->
          <xsl:for-each select="$policy/*">
            <!-- strip the namespace - this may need to be tailored if the
                 namespace is not consistently present as the substring-after()
                 call will return an empty string in the case where no
                 namespace is present.
              -->
            <xsl:variable name="subfield" select="substring-after(name(), ':')"/>

            <!-- First time in, set the length. -->
            <!-- N.B. If we encounter multiple sequences in the same
                      scope, we will need to parse this further.
              -->
            <xsl:if test="position() = 1">
              <!-- '  (base)(field).length( (count) );\n' -->
              <xsl:value-of select="$base"/>
              <xsl:value-of select="$field"/>
              <xsl:text>.</xsl:text>
              <xsl:value-of select="$subfield"/>
              <xsl:text>.length(</xsl:text>
              <xsl:value-of select="$length"/>
              <xsl:text>);</xsl:text>
              <xsl:value-of select="$newline"/>
            </xsl:if>

            <!-- '  (base)(field).(subfield)[ (position)-1 ] = (value);\n' -->
            <xsl:value-of select="$base"/>
            <xsl:value-of select="$field"/>
            <xsl:text>.</xsl:text>
            <xsl:value-of select="$subfield"/>
            <xsl:text>[</xsl:text>
            <xsl:value-of select="position()-1"/>
            <xsl:text>] = </xsl:text>

            <!-- quote the value if specified in the lookup table. -->
            <xsl:choose>
              <xsl:when test="$should-quote = 'true'">
                <xsl:text>"</xsl:text>
                <xsl:value-of select="."/>
                <xsl:text>";</xsl:text>
                </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="."/>
                <xsl:text>;</xsl:text>
                </xsl:otherwise>
            </xsl:choose>

            <xsl:value-of select="$newline"/>
          </xsl:for-each>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<!-- Process Transport configuration data into value assignments. -->
<xsl:template name="process-transports">
  <xsl:param name="config"/>
  <xsl:param name="indent"/>

  <!-- process each configuration specification in the node-set. -->
  <xsl:for-each select="$config/*">
    <!-- strip the namespace - this may need to be tailored if the namespace
         is not consistently present as the substring-after() call will return
         an empty string in the case where no namepsace is present.
      -->
    <xsl:variable name="configname" select="substring-after( name(), ':')"/>
    <xsl:choose>
      <!-- local_address is loaded in a unique fashion. -->
      <xsl:when test="$configname = 'local_address_str'">
        <xsl:value-of select="$indent"/>
        <xsl:text>// config-></xsl:text>
        <xsl:value-of select="$configname"/>
        <xsl:text>_ = </xsl:text>
        <xsl:value-of select="@value"/>
        <xsl:text>;</xsl:text>
        <xsl:value-of select="$newline"/>
      </xsl:when>

      <!-- group_address is loaded in a unique fashion. -->
      <xsl:when test="$configname = 'group_address'">
        <xsl:value-of select="$indent"/>
        <xsl:text>// config-></xsl:text>
        <xsl:value-of select="$configname"/>
        <xsl:text>_ = </xsl:text>
        <xsl:value-of select="@value"/>
        <xsl:text>;</xsl:text>
        <xsl:value-of select="$newline"/>
      </xsl:when>

      <!-- transport_type uses the already established value in the code. -->
      <xsl:when test="$configname = 'transport_type'">
        <xsl:value-of select="$indent"/>
        <xsl:text>config->transport_type_ = kind;</xsl:text>
        <xsl:value-of select="$newline"/>
      </xsl:when>

      <!-- config->(name)_ = (value); -->
      <xsl:otherwise>
        <xsl:value-of select="$indent"/>
        <xsl:text>config-></xsl:text>
        <xsl:value-of select="$configname"/>
        <xsl:text>_ = </xsl:text>
        <xsl:value-of select="@value"/>
        <xsl:text>;</xsl:text>
        <xsl:value-of select="$newline"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<xsl:template name="normalize-identifier">
  <xsl:param name="identifier" select="."/>
  <xsl:value-of select="translate($identifier, ' -', '__')"/>
</xsl:template>

<!-- Lookup Table Magic. -->
<lut:tables>
  <qos-field type="opendds:udQosPolicy" quote="true">user_data</qos-field>
  <qos-field type="opendds:tdQosPolicy" quote="true">topic_data</qos-field>
  <qos-field type="opendds:gdQosPolicy" quote="true">group_data</qos-field>
  <qos-field type="opendds:efQosPolicy">entity_factory</qos-field>
  <qos-field type="opendds:durabilityQosPolicy">durability</qos-field>
  <qos-field type="opendds:dsQosPolicy">durability_service</qos-field>
  <qos-field type="opendds:deadlineQosPolicy">deadline</qos-field>
  <qos-field type="opendds:lbQosPolicy">latency_budget</qos-field>
  <qos-field type="opendds:livelinessQosPolicy">liveliness</qos-field>
  <qos-field type="opendds:reliabilityQosPolicy">reliability</qos-field>
  <qos-field type="opendds:destinationOrderQosPolicy">destination_order</qos-field>
  <qos-field type="opendds:historyQosPolicy">history</qos-field>
  <qos-field type="opendds:rlQosPolicy">resource_limits</qos-field>
  <qos-field type="opendds:tpQosPolicy">transport_priority</qos-field>
  <qos-field type="opendds:lifespanQosPolicy">lifespan</qos-field>
  <qos-field type="opendds:ownershipQosPolicy">ownership</qos-field>
  <qos-field type="opendds:osQosPolicy">ownership_strength</qos-field>
  <qos-field type="opendds:presentationQosPolicy">presentation</qos-field>
  <qos-field type="opendds:partitionQosPolicy" quote="true">partition</qos-field>
  <qos-field type="opendds:tbfQosPolicy">time_based_filter</qos-field>
  <qos-field type="opendds:wdlQosPolicy">writer_data_lifecycle</qos-field>
  <qos-field type="opendds:rdlQosPolicy">reader_data_lifecycle</qos-field>

  <transport type="SimpleTcp" configtype="SimpleTcpConfiguration"/>
  <transport type="multicast" configtype="MulticastConfiguration"/>
  <transport type="udp" configtype="UdpConfiguration"/>
</lut:tables>
<!-- ................... -->

</xsl:stylesheet>

