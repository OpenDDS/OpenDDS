<xsl:stylesheet version='1.0'
     xmlns:xmi='http://www.omg.org/XMI'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
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

<!-- Documents -->
<xsl:variable name="lut-policies" select="document('lut.xml')/*/lut:policies"/>

<!-- Node sets -->
<xsl:variable name="readers"      select="//readers"/>
<xsl:variable name="writers"      select="//writers"/>
<xsl:variable name="domains"      select="//domains"/>
<xsl:variable name="participants" select="//participants"/>
<xsl:variable name="publishers"   select="//publishers"/>
<xsl:variable name="policies"     select="//policies"/>
<xsl:variable name="subscribers"  select="//subscribers"/>
<xsl:variable name="topics"       select="//topics"/>
<xsl:variable name="types"        select="//types"/>

<!-- Indices (lookup tables are at the bottom of this document) -->
<xsl:key
     name  = "policies"
     match = "//opendds:policy"
     use   = "@xmi:id"/>

<xsl:key
     name  = "lut-qos-field"
     match = "qos-field"
     use   = "@type"/>

<!-- key table of remote data type references
<xsl:key name="remote-topic-types"
         match="datatype"
         use="@href"/>

<xsl:key name="remote-models"
         match="datatype"
         use="substring-before(@href,'#')"/>
 -->

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "/opendds:OpenDDSModel/@name"/>

<!-- A unique set of remote type hrefs, containted in datatype elements 
<xsl:variable name="uniq-type-refs" select="$topics/datatype[generate-id() = generate-id(key('remote-topic-types', @href)[1])]"/>

<xsl:variable name="uniq-model-refs" select="$uniq-type-refs[generate-id() = generate-id(key('remote-models',substring-before(@href,'#'))[1])]"/>
-->

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

  <xsl:for-each select="//dataLib/@model">
    <xsl:value-of select="concat('#include &quot;', ., 'TypeSupportImpl.h&quot;', $newline)"/>
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

inline
Elements::Data::Data()
{ </xsl:text>
<xsl:if test="$topics">
  <xsl:text>
  for( int index = 0;
       index &lt; OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Types::LAST_INDEX;
       ++index
  ) {
    this->typeNames_[ index] = 0;
  }
</xsl:text>
</xsl:if>
<xsl:text>

  this->loadDomains();
  this->loadTopics();
  this->loadMaps(); /// MUST precede the QoS loading.

  this->buildParticipantsQos();
  this->buildTopicsQos();
  this->buildPublishersQos();
  this->buildSubscribersQos();
  this->buildPublicationsQos();
  this->buildSubscriptionsQos();
}

inline
Elements::Data::~Data()
{ </xsl:text>
<xsl:if test="$topics">
  <xsl:text>
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
</xsl:text>
</xsl:if>
<xsl:text>}

inline
void
Elements::Data::registerType(
  Types::Values      type,
  DomainParticipant* participant
)
{
  switch( type) {
</xsl:text>
  <!-- handle internal datatypes -->
  <xsl:variable name="defined-types" select="$types[@xmi:id = $topics/@datatype]"/>

  <xsl:for-each select="$defined-types">
    <xsl:call-template name="output-registerType-case"/>
  </xsl:for-each>

  <!-- handle external datatypes
  <xsl:for-each select="$uniq-type-refs">
    <xsl:variable name="model-file" select="substring-before(@href, '#')"/>
    <xsl:variable name="model-id" select="substring-after(@href, '#')"/>
    <xsl:variable name="remote-type" select="document($model-file)//dataLib/types[@xmi:id = $model-id]"/>
    <xsl:call-template name="output-registerType-case">
      <xsl:with-param name="type" select="$remote-type"/>
    </xsl:call-template>
  </xsl:for-each> 
-->

  <xsl:text>    default:
      throw NoTypeException();
      break;
  }
}

inline
void
Elements::Data::loadDomains()
{
</xsl:text>
  <!-- '  this->domains_[ Participants::(domainParticipant/@name)] = (domainParticipant/@domain);\n' -->
  <xsl:for-each select="$participants/@domain">
    <xsl:text>  this->domains_[ Participants::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="../@name"/>
    </xsl:call-template>
    <xsl:text>] = </xsl:text>
    <xsl:value-of select="$domains[@xmi:id = current()]/@domainId"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

inline
void
Elements::Data::loadTopics()
{
  /// @TODO verify how we manage the model strings.
</xsl:text>
  <!-- '  this->topicNames_[ Topics::(topic/@name)] = "(topic/@name)";\n' -->
  <xsl:for-each select="$topics/@name">
    <xsl:text>  this->topicNames_[ Topics::</xsl:text>
    <xsl:value-of select="translate(.,' ','_')"/>
    <xsl:text>] = "</xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>";</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

inline
void
Elements::Data::loadMaps()
{
</xsl:text>
  <!-- '  this->publisherParticipants_[ Publishers::(publisher/@name)] = Participants::(publisher/../@name);\n' -->
  <xsl:for-each select="$publishers">
    <xsl:text>  this->publisherParticipants_[ Publishers::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>] = Participants::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="../@name"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->subscriberParticipants_[ Subscribers::(subscriber/@name)] = Participants::(subscriber/../@name);\n' -->
  <xsl:for-each select="$subscribers">
    <xsl:text>  this->subscriberParticipants_[ Subscribers::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>] = Participants::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="../@name"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- defined types -->
  <xsl:for-each select="$topics[@datatype]">
    <xsl:text>  this->types_[ Topics::</xsl:text>
    <xsl:value-of select="translate(@name,' ','_')"/>
    <xsl:text>] = Types::</xsl:text>
    <xsl:call-template name="type-enum">
      <xsl:with-param name="type" select="$types[@xmi:id = current()/@datatype]"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <!-- referenced types 
  <xsl:for-each select="$topics[datatype]">
    <xsl:variable name="model-file" select="substring-before(datatype/@href,'#')"/>
    <xsl:variable name="model-id" select="substring-after(datatype/@href,'#')"/>
    <xsl:variable name="referred-type" select="document($model-file)//dataLib/types[@xmi:id = $model-id]"/>

    <xsl:text>  this->types_[ Topics::</xsl:text>
    <xsl:value-of select="translate(@name,' ','_')"/>
    <xsl:text>] = Types::</xsl:text>
    <xsl:value-of select="concat($referred-type/../@name, '_', $referred-type/@name)"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
-->
  <xsl:value-of select="$newline"/>

  <!-- '  this->writerTopics[ DataWriters::(dataWriter/@name)] = Topics::(dataWriter/@topic);\n' -->
  <xsl:for-each select="$writers">
    <xsl:text>  this->writerTopics_[ DataWriters::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>] = Topics::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="$topics[@xmi:id = current()/@topic]/@name"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->readerTopics[ DataReaders::(dataReader/@name)] = Topics::(dataReader/@topic);\n' -->
  <xsl:for-each select="$readers">
    <xsl:text>  this->readerTopics_[ DataReaders::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>] = Topics::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="$topics[@xmi:id = current()/@topic]/@name"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->publishers_[ DataWriters::(dataWriter/@name)] = Publishers::(dataWriter/../@name);\n' -->
  <xsl:for-each select="$writers">
    <xsl:text>  this->publishers_[ DataWriters::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>] = Publishers::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="../@name"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->subscribers_[ DataReaders::(dataReader/@name)] = Subscribers::(dataReader/../@name);\n' -->
  <xsl:for-each select="$readers">
    <xsl:text>  this->subscribers_[ DataReaders::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>] = Subscribers::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="../@name"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- Assign Transport ID -->
  <!-- '  this->publisherTransports_[ Publishers::(publisher/@name)] = Transports::(publisher/@transport);\n' -->
  <xsl:for-each select="$publishers">
    <xsl:text>  this->publisherTransports_[ Publishers::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:value-of select="concat('] = ', @transportId, ';', $newline)"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- Assign Transport ID -->
  <!-- '  this->subscriberTransports_[ Subscribers::(subscriber/@name)] = Transports::(subscriber/@transport);\n' -->
  <xsl:for-each select="$subscribers">
    <xsl:text>  this->subscriberTransports_[ Subscribers::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:value-of select="concat('] = ', @transportId, ';', $newline)"/>
  </xsl:for-each>
  <xsl:text>}

inline
void
Elements::Data::buildParticipantsQos()
{
  DomainParticipantQos participantQos;
  Participants::Values participant;
</xsl:text>
  <xsl:for-each select="$participants">
    <xsl:value-of select="$newline"/>
    <xsl:text>  participant = Participants::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>;
  participantQos = TheServiceParticipant->initial_DomainParticipantQos();
</xsl:text>
    <!-- '  participantQos.(policyfield) = (value);\n' -->

    <xsl:call-template name="process-policies">
      <xsl:with-param name="base"   select="'participantQos.'"/>
    </xsl:call-template>
    
<!--
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  participantQos.'"/>
    </xsl:call-template>
-->

    <xsl:text>  this->participantsQos_[ participant] = participantQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

inline
void
Elements::Data::buildTopicsQos()
{
  TopicQos       topicQos;
  Topics::Values topic;
</xsl:text>
  <xsl:for-each select="$topics">
    <xsl:value-of select="$newline"/>
    <xsl:text>  topic    = Topics::</xsl:text>
    <xsl:value-of select="translate(@name,' ','_')"/>
    <xsl:text>;
  topicQos = TheServiceParticipant->initial_TopicQos();
    
</xsl:text>
    <!-- '  topicQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-policies">
      <xsl:with-param name="base"   select="'topicQos.'"/>
    </xsl:call-template>
<!--
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  topicQos.'"/>
    </xsl:call-template>
-->

    <xsl:text>  this->topicsQos_[ topic] = topicQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

inline
void
Elements::Data::buildPublishersQos()
{
</xsl:text>
<xsl:choose>
  <xsl:when test="$publishers">
    <xsl:text>  PublisherQos       publisherQos;
  Publishers::Values publisher;
</xsl:text>
  <xsl:for-each select="$publishers">
    <xsl:value-of select="$newline"/>
    <xsl:text>  publisher    = Publishers::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>;
  publisherQos = TheServiceParticipant->initial_PublisherQos();
</xsl:text>
    <!-- '  publisherQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-policies">
      <xsl:with-param name="base"   select="'publisherQos.'"/>
    </xsl:call-template>
<!--
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'  publisherQos.'"/>
    </xsl:call-template>
-->

    <xsl:text>  this->publishersQos_[ publisher] = publisherQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  // No publishers were defined by this model.
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
  <xsl:text>}

inline
void
Elements::Data::buildSubscribersQos()
{
</xsl:text>
  <xsl:choose>
    <xsl:when test="$subscribers">

      <xsl:text>  SubscriberQos       subscriberQos;
  Subscribers::Values subscriber;
</xsl:text>
      <xsl:for-each select="$subscribers">
        <xsl:value-of select="$newline"/>
        <xsl:text>  subscriber    = Subscribers::</xsl:text>
        <xsl:call-template name="normalize-identifier"/>
        <xsl:text>;
  subscriberQos = TheServiceParticipant->initial_SubscriberQos();
</xsl:text>
        <!-- '  subscriberQos.(policyfield) = (value);\n' -->
        <xsl:call-template name="process-policies">
          <xsl:with-param name="base"   select="'subscriberQos.'"/>
        </xsl:call-template>
<!--
        <xsl:call-template name="process-qos">
          <xsl:with-param name="entity" select="."/>
          <xsl:with-param name="base"   select="'  subscriberQos.'"/>
        </xsl:call-template>
-->

        <xsl:text>  this->subscribersQos_[ subscriber] = subscriberQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>  // No subscribers were defined by this model.
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:text>}

inline
void
Elements::Data::buildPublicationsQos()
{
</xsl:text>
<xsl:choose>
  <xsl:when test="$writers">
    <xsl:text>  DataWriters::Values  writer;
  DataWriterQos        writerQos;
</xsl:text>
    <xsl:for-each select="$writers">
      <xsl:value-of select="$newline"/>
      <xsl:text>  writer    = DataWriters::</xsl:text>
      <xsl:call-template name="normalize-identifier"/>
      <xsl:text>;
  writerQos = TheServiceParticipant->initial_DataWriterQos();
</xsl:text>
    <!-- '  writerQos.(policyfield) = (value);\n' -->
      <xsl:call-template name="process-policies">
        <xsl:with-param name="base"   select="'writerQos.'"/>
      </xsl:call-template>
<!--
      <xsl:call-template name="process-qos">
        <xsl:with-param name="entity" select="."/>
        <xsl:with-param name="base"   select="'  writerQos.'"/>
      </xsl:call-template>
-->
      <xsl:text>  this->writersQos_[ writer] = writerQos;
  this->writerCopyTopicQos_[writer] = </xsl:text>
      <xsl:value-of select="concat(@copyFromTopicQos, ';', $newline)"/>
    </xsl:for-each>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  // No data writers were defined by this model
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
  <xsl:text>}

inline
void
Elements::Data::buildSubscriptionsQos()
{
  DataReaders::Values  reader;
  DataReaderQos        readerQos;
</xsl:text>
  <xsl:for-each select="$readers">
    <xsl:value-of select="$newline"/>
    <xsl:text>  reader    = DataReaders::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>;
  readerQos = TheServiceParticipant->initial_DataReaderQos();
</xsl:text>
    <!-- '  readerQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-policies">
      <xsl:with-param name="base"   select="'readerQos.'"/>
    </xsl:call-template>

    <xsl:text>  this->readersQos_[ reader] = readerQos;
  this->readerCopyTopicQos_[reader] = </xsl:text>
    <xsl:value-of select="concat(@copyFromTopicQos, ';', $newline)"/>
  </xsl:for-each>
  <xsl:text>}

inline
void
Elements::Data::copyPublicationQos(
  DataWriters::Values which,
  DataWriterQos&amp;  writerQos
)
{
  do{}while(&amp;writerQos==0); // In case we define no properties.

  switch( which) {
</xsl:text>
  <xsl:for-each select="$writers">
    <xsl:text>    case DataWriters::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>:</xsl:text>
    <xsl:value-of select="$newline"/>

    <!-- '  writerQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-policies">
      <xsl:with-param name="base"   select="'      writerQos.'"/>
    </xsl:call-template>
<!--
    <xsl:call-template name="process-qos">
      <xsl:with-param name="entity" select="."/>
      <xsl:with-param name="base"   select="'      writerQos.'"/>
    </xsl:call-template>
-->
    <xsl:text>      break;</xsl:text>
    <xsl:value-of select="$newline"/>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
<xsl:text>    default:
      throw NoWriterException();
  }
}

inline
void
Elements::Data::copySubscriptionQos(
  DataReaders::Values which,
  DataReaderQos&amp;  readerQos
)
{
  do{}while(&amp;readerQos==0); // In case we define no properties.

  switch( which) {
</xsl:text>
  <xsl:for-each select="$readers">
    <xsl:text>    case DataReaders::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>:</xsl:text>
    <xsl:value-of select="$newline"/>

    <!-- '  readerQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-policies">
      <xsl:with-param name="base"   select="'readerQos.'"/>
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

<xsl:template name="process-policies">
  <xsl:param name="base"/>

  <!-- direct references -->
  <xsl:variable name="reffed-policies" select="$policies[@xmi:id = current()/@*]"/>

  <xsl:for-each select="$reffed-policies">
    <xsl:call-template name="process-policy">
      <xsl:with-param name="base" select="$base"/>
    </xsl:call-template>
  </xsl:for-each>

<!--
  <xsl:variable name="external-policy-refs" select="*[name() != 'datatype'][@href]"/>
  <xsl:for-each select="$external-policy-refs">
    <xsl:variable name="policy-file" select="substring-before(@href, '#')"/>
    <xsl:variable name="policy-id" select="substring-after(@href, '#')"/>
    <xsl:for-each select="document($policy-file)//*[@xmi:id = $policy-id]">
      <xsl:call-template name="process-policy">
        <xsl:with-param name="base" select="$base"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:for-each>
-->

</xsl:template>

<xsl:template name="process-policy">
  <xsl:param name="base"/>
  <xsl:variable name="policy-type" select="@xsi:type"/>

  <!-- lookup the field name for the current policy type. -->
  <xsl:variable name="field">
    <xsl:for-each select="$lut-policies"> <!-- Change context for lookup -->
      <xsl:value-of select="key('lut-qos-field', $policy-type)/text()"/>
    </xsl:for-each>
  </xsl:variable>

<!--
<xsl:message>process-policy field <xsl:value-of select="$field"/> </xsl:message>
-->
  <!-- lookup whether to quote the value. -->
  <xsl:variable name="should-quote">
    <xsl:for-each select="$lut-policies"> <!-- Change context for lookup -->
      <xsl:value-of select="key('lut-qos-field', $policy-type)/@quote"/>
    </xsl:for-each>
  </xsl:variable>

  <!-- process all of the policy attributes -->
  <xsl:for-each select="@*">
    <xsl:choose>
      <!-- ignore the 'name', 'type' and 'id' attributes of the policy. -->
      <xsl:when test="name() = 'name' or 
                      name() = 'xsi:type' or 
                      name() = 'xmi:id'"/>

      <xsl:when test="../@xsi:type = 'opendds:udQosPolicy'
                   or ../@xsi:type = 'opendds:tdQosPolicy'
                   or ../@xsi:type = 'opendds:gdQosPolicy'">
        <xsl:variable name="value" select="concat($base, $field, '.value')"/>
        <xsl:value-of select="concat('  ', $value, '.replace(', $newline, 
                                '      ', $value, '.length(),', $newline,
                                '      ', $value, '.length(),', $newline,
                                '      (CORBA::Octet*)&quot;', ., '&quot;);', $newline)"/>

      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="suffix-attr-name" select="concat(name(), '_suffix')"/>
        <xsl:variable name="suffix">
          <xsl:for-each select="$lut-policies"> <!-- Change context for lookup -->
            <xsl:value-of select="key('lut-qos-field', $policy-type)/@*[name() = $suffix-attr-name]"/>
          </xsl:for-each>
        </xsl:variable>
        
        <xsl:value-of select="concat('  ', $base, $field, '.', name(), ' = ')"/>

        <!-- quote the value if specified in the lookup table. -->
        <xsl:choose>
          <xsl:when test="$should-quote = 'true'">
            <xsl:value-of select="concat('&quot;', ., $suffix, '&quot;;')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat(., $suffix, ';')"/>
          </xsl:otherwise>
        </xsl:choose>

        <xsl:value-of select="$newline"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>

  <!-- special handling cases -->
  <xsl:choose>
    <xsl:when test="@xsi:type = 'opendds:partitionQosPolicy' and count(names) > 0">
      <xsl:value-of select="concat('  ', $base, $field, '.name.length(',
                                   count(names), ');', $newline)"/>
      <xsl:for-each select="names">
        <xsl:value-of select="concat('  ', $base, $field, '.name[', 
                                     position() - 1, '] = &quot;',
                                     ., '&quot;;', $newline)"/>
      </xsl:for-each>
    </xsl:when>
    <xsl:when test="*/@second | */@nanosecond">
      <xsl:for-each select="*">
        <xsl:variable name="sec">
          <xsl:choose>
            <xsl:when test="@second">
              <xsl:value-of select="@second"/>
            </xsl:when>
            <xsl:otherwise>0</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="nanosec">
          <xsl:choose>
            <xsl:when test="@nanosecond">
              <xsl:value-of select="@nanosecond"/>
            </xsl:when>
            <xsl:otherwise>0</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat('  ', $base, $field, '.', name(), 
                                     '.sec = ', $sec, ';', $newline)"/>
        <xsl:value-of select="concat('  ', $base, $field, '.', name(), 
                                     '.nanosec = ', $nanosec, ';', $newline)"/>
      </xsl:for-each>
    </xsl:when>
  </xsl:choose>
</xsl:template>

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
      <xsl:for-each select="$lut-policies"> <!-- Change context for the lookup -->
        <xsl:value-of select="key('lut-qos-field', $policy/@type)/text()"/>
      </xsl:for-each>
    </xsl:variable>

    <!-- lookup whether to quote the value. -->
    <xsl:variable name="should-quote">
      <xsl:for-each select="$lut-policies"> <!-- Change context for the lookup -->
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

<!-- will only be called if topics defined -->
<xsl:template name="output-registerType-case">
  <xsl:param name="type" select="."/>
  <xsl:variable name="typename" select="$type/@name"/>
  <xsl:variable name="type-modelname">
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="$type/../@model"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="scopename">
    <xsl:call-template name="scopename"/>
  </xsl:variable>

  <xsl:text>    case Types::</xsl:text>
  <xsl:call-template name="type-enum"/>
  <xsl:value-of select="concat(':', $newline, '      {', $newline, 
                               '        typedef ')"/>
  <xsl:value-of select="concat($scopename, $type/@name)"/>
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
</xsl:template>

<!-- Lookup Table Magic.
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
 -->
<!-- ................... -->

</xsl:stylesheet>

