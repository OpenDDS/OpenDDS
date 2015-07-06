<xsl:stylesheet version='1.0'
     xmlns:xmi='http://www.omg.org/XMI'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:lut='http://www.opendds.org/modeling/schemas/Lut/1.0'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.org/modeling/schemas/Generator/1.0'>
  <!--
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

<!-- Documents -->
<xsl:variable name="lut-policies" select="document('lut.xml')/*/lut:policies"/>

<!-- Node sets -->
<xsl:variable name="types"        select="//types"/>
<xsl:variable name="topics"       select="//topicDescriptions"/>
<xsl:variable name="cf-topics"    select="$topics[@xsi:type='topics:ContentFilteredTopic']"/>
<xsl:variable name="multitopics"  select="$topics[@xsi:type='topics:MultiTopic']"/>
<xsl:variable name="policies"     select="//policies"/>

<xsl:key
     name  = "lut-qos-field"
     match = "qos-field"
     use   = "@type"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">
  <xsl:variable name = "modelname" select = "/opendds:OpenDDSModel/@name"/>
  <xsl:text>
#include "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_T.h"

#include "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>TypeSupportImpl.h"</xsl:text>
  <xsl:value-of select="$newline"/>

  <xsl:call-template name="include-referenced-data-models">
    <xsl:with-param name="models" select="//types/@model"/>
  </xsl:call-template>

  <xsl:text>

// For transport configuration
#include "dds/DCPS/transport/tcp/TcpInst.h"
#include "dds/DCPS/transport/udp/UdpInst.h"
#include "dds/DCPS/transport/multicast/MulticastInst.h"

#include "dds/DCPS/Service_Participant.h"
#include "model/Utilities.h"

namespace OpenDDS { namespace Model { 
</xsl:text>
  <xsl:apply-templates/>
<xsl:text>
} // End namespace Model
} // End namespace OpenDDS
</xsl:text>
</xsl:template>
<!-- End of main processing template. -->

<!-- Output a namespace for a package containing a DCPSLib -->
<xsl:template match="packages[.//libs[@xsi:type='opendds:DcpsLib']]">
  <xsl:value-of select="concat('namespace ', @name, ' {', $newline)"/>
  <xsl:apply-templates/>
  <xsl:value-of select="concat('} // End namespace ', @name, $newline)"/>
</xsl:template>

<!-- Output a namespace for a DCPSLib -->
<xsl:template match="libs[@xsi:type='opendds:DcpsLib']">
  <xsl:variable name="lib-readers"      select=".//readers"/>
  <xsl:variable name="lib-writers"      select=".//writers"/>
  <xsl:variable name="lib-domains"      select=".//domains"/>
  <xsl:variable name="lib-participants" select=".//participants"/>
  <xsl:variable name="lib-publishers"   select=".//publishers"/>
  <xsl:variable name="lib-subscribers"  select=".//subscribers"/>
  <xsl:variable name="defined-types"    select="$types[@xmi:id = $topics/@datatype]"/>
  <xsl:value-of select="concat('namespace ', @name, ' {', $newline)"/>
  <xsl:text>
inline
Elements::Data::Data()
{ </xsl:text>
<xsl:if test="$topics">
  <xsl:text>
  for( int index = 0;
       index &lt; Elements::Types::LAST_INDEX;
       ++index) {
    this->typeNames_[index] = 0;
  }</xsl:text>
</xsl:if>
<xsl:if test="$cf-topics">
  <xsl:text>
  for( int index = 0;
       index &lt; Elements::ContentFilteredTopics::LAST_INDEX;
       ++index) {
    this->filterExpressions_[index] = 0;
  }</xsl:text>
</xsl:if>
<xsl:if test="$multitopics">
  <xsl:text>
  for( int index = 0;
       index &lt; Elements::MultiTopics::LAST_INDEX;
       ++index) {
    this->topicExpressions_[index] = 0;
  }</xsl:text>
</xsl:if>
  <xsl:text>
  this->loadDomains();
  this->loadTopics();
  this->loadMaps(); /// MUST precede the QoS loading.
  this->loadTransportConfigNames();

  this->buildParticipantsQos();
  this->buildTopicsQos();
  this->buildPublishersQos();
  this->buildSubscribersQos();
  this->buildPublicationsQos();
  this->buildSubscriptionsQos();
}

inline
Elements::Data::~Data()
{</xsl:text>
<xsl:if test="$topics">
  <xsl:text>
  for(int index = 0;
      index &lt; Elements::Types::LAST_INDEX;
      ++index) {
    if(this->typeNames_[index]) {
      CORBA::string_free(this->typeNames_[index]); // Created by CORBA::string_dup()
      this->typeNames_[index] = 0;
    }
  }
</xsl:text>
</xsl:if>
<xsl:text>}

</xsl:text>
<xsl:choose>
  <xsl:when test="$defined-types">
    <xsl:text>inline
void
Elements::Data::registerType(
  Types::Values      type,
  DomainParticipant* participant)
{
  switch(type) {
</xsl:text>
    <xsl:for-each select="$defined-types">
      <xsl:call-template name="output-registerType-case"/>
    </xsl:for-each>
    <xsl:text>  default:
    throw NoTypeException();
    break;
  }
}</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>inline
void
Elements::Data::registerType(
  Types::Values,
  DomainParticipant*)
{
  throw NoTypeException();  // No DCPS data types defined in model
}</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>

inline
void
Elements::Data::loadDomains()
{
</xsl:text>
  <!-- '  this->domains_[ Participants::(domainParticipant/@name)] = (domainParticipant/@domain);\n' -->
  <xsl:for-each select="$lib-participants/@domain">
    <xsl:text>  this->domains_[ Participants::</xsl:text>
    <xsl:call-template name="normalize-identifier">
      <xsl:with-param name="identifier" select="../@name"/>
    </xsl:call-template>
    <xsl:text>] = </xsl:text>
    <xsl:value-of select="$lib-domains[@xmi:id = current()]/@domainId"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

inline
void
Elements::Data::loadTopics()
{
</xsl:text>
  <xsl:for-each select="$topics">
    <xsl:variable name="enum">
      <xsl:call-template name="topic-enum"/>
    </xsl:variable>
    <xsl:text>  this->topicNames_[Topics::</xsl:text>
    <xsl:value-of select="concat($enum, '] = &quot;', @name, '&quot;;', $newline)"/>
    <xsl:choose>
      <xsl:when test="$cf-topics[@xmi:id = current()/@xmi:id]">
        <xsl:text>  this->cfTopics_[</xsl:text>
        <xsl:value-of select="concat('Topics::', $enum, 
                                     '] = ContentFilteredTopics::', $enum,
                                     ';', $newline)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>  this->cfTopics_[</xsl:text>
        <xsl:value-of select="concat('Topics::', $enum, 
                                     '] = ContentFilteredTopics::LAST_INDEX;',
                                     $newline)"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="$multitopics[@xmi:id = current()/@xmi:id]">
        <xsl:text>  this->multiTopics_[</xsl:text>
        <xsl:value-of select="concat('Topics::', $enum, 
                                     '] = MultiTopics::', $enum,
                                     ';', $newline)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>  this->multiTopics_[</xsl:text>
        <xsl:value-of select="concat('Topics::', $enum, 
                                     '] = MultiTopics::LAST_INDEX;',
                                     $newline)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
  <xsl:for-each select="$cf-topics">
    <xsl:variable name="enum">
      <xsl:call-template name="topic-enum"/>
    </xsl:variable>
    <xsl:value-of select="concat('  this->filterExpressions_[',
                                 'ContentFilteredTopics::',
                                 $enum, '] = &quot;',
                                 @filter_expression, '&quot;;', $newline)"/>
  </xsl:for-each>
  <xsl:for-each select="$multitopics">
    <xsl:variable name="enum">
      <xsl:call-template name="topic-enum"/>
    </xsl:variable>
    <xsl:value-of select="concat('  this->topicExpressions_[',
                                 'MultiTopics::',
                                 $enum, '] = &quot;',
                                 @subscription_expression, '&quot;;', $newline)"/>
  </xsl:for-each>
  <xsl:text>}

inline
std::string
</xsl:text>
  <xsl:choose>
    <xsl:when test="$lib-participants">
      <xsl:text>Elements::Data::transportConfigName(Participants::Values which)
{
  return participantTxCfgNames_[which];</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>Elements::Data::transportConfigName(Participants::Values)
{
  return "";</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
<xsl:text>
}

inline
std::string
</xsl:text>
  <xsl:choose>
    <xsl:when test="$lib-publishers">
      <xsl:text>Elements::Data::transportConfigName(Publishers::Values which)
{
  return publisherTxCfgNames_[which];</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>Elements::Data::transportConfigName(Publishers::Values)
{
  return "";</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
<xsl:text>
}

inline
std::string
</xsl:text>
  <xsl:choose>
    <xsl:when test="$lib-subscribers">
      <xsl:text>Elements::Data::transportConfigName(Subscribers::Values which)
{
  return subscriberTxCfgNames_[which];</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>Elements::Data::transportConfigName(Subscribers::Values)
{
  return "";</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
<xsl:text>
}

inline
std::string
</xsl:text>
  <xsl:choose>
    <xsl:when test="$lib-writers">
      <xsl:text>Elements::Data::transportConfigName(DataWriters::Values which)
{
  return writerTxCfgNames_[which];</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>Elements::Data::transportConfigName(DataWriters::Values)
{
  return "";</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
<xsl:text>
}

inline
std::string
</xsl:text>
  <xsl:choose>
    <xsl:when test="$lib-readers">
      <xsl:text>Elements::Data::transportConfigName(DataReaders::Values which)
{
  return readerTxCfgNames_[which];</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>Elements::Data::transportConfigName(DataReaders::Values)
{
  return "";</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
<xsl:text>
}

inline
void
Elements::Data::loadTransportConfigNames()
{
</xsl:text>
  <xsl:for-each select="$lib-participants[@transportConfig]">
    <xsl:variable name="enum"> 
      <xsl:call-template name="normalize-identifier"/>
    </xsl:variable>
    <xsl:value-of select="concat('  participantTxCfgNames_[Participants::',
                                 $enum, '] = &quot;', @transportConfig, 
                                 '&quot;;', $newline)"/>
  </xsl:for-each>
  <xsl:for-each select="$lib-publishers[@transportConfig]">
    <xsl:variable name="enum"> 
      <xsl:call-template name="normalize-identifier"/>
    </xsl:variable>
    <xsl:value-of select="concat('  publisherTxCfgNames_[Publishers::',
                                 $enum, '] = &quot;', @transportConfig, 
                                 '&quot;;', $newline)"/>
  </xsl:for-each>
  <xsl:for-each select="$lib-subscribers[@transportConfig]">
    <xsl:variable name="enum"> 
      <xsl:call-template name="normalize-identifier"/>
    </xsl:variable>
    <xsl:value-of select="concat('  subscriberTxCfgNames_[Subscribers::',
                                 $enum, '] = &quot;', @transportConfig, 
                                 '&quot;;', $newline)"/>
  </xsl:for-each>
  <xsl:for-each select="$lib-writers[@transportConfig]">
    <xsl:variable name="enum"> 
      <xsl:call-template name="normalize-identifier"/>
    </xsl:variable>
    <xsl:value-of select="concat('  writerTxCfgNames_[DataWriters::',
                                 $enum, '] = &quot;', @transportConfig, 
                                 '&quot;;', $newline)"/>
  </xsl:for-each>
  <xsl:for-each select="$lib-readers[@transportConfig]">
    <xsl:variable name="enum"> 
      <xsl:call-template name="normalize-identifier"/>
    </xsl:variable>
    <xsl:value-of select="concat('  readerTxCfgNames_[DataReaders::',
                                 $enum, '] = &quot;', @transportConfig, 
                                 '&quot;;', $newline)"/>
  </xsl:for-each>
<xsl:text>}

inline
void
Elements::Data::loadMaps()
{
</xsl:text>
  <!-- '  this->publisherParticipants_[ Publishers::(publisher/@name)] = Participants::(publisher/../@name);\n' -->
  <xsl:for-each select="$lib-publishers">
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
  <xsl:for-each select="$lib-subscribers">
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
    <xsl:call-template name="topic-enum"/>
    <xsl:text>] = Types::</xsl:text>
    <xsl:call-template name="type-enum">
      <xsl:with-param name="type" select="$types[@xmi:id = current()/@datatype]"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:for-each select="$cf-topics">
    <xsl:variable name="enum">
      <xsl:call-template name="topic-enum"/>
    </xsl:variable>
    <xsl:value-of select="concat('  this->relatedTopics_[',
                                 'ContentFilteredTopics::',
                                 $enum, '] = Topics::')"/>
    <xsl:call-template name="topic-enum">
      <xsl:with-param name="topic" select="$topics[@xmi:id = current()/@related_topic]"/>
    </xsl:call-template>
    <xsl:text>;
</xsl:text>
  </xsl:for-each>

  <xsl:value-of select="$newline"/>

  <!-- '  this->writerTopics[ DataWriters::(dataWriter/@name)] = Topics::(dataWriter/@topic);\n' -->
  <xsl:for-each select="$lib-writers">
    <xsl:text>  this->writerTopics_[ DataWriters::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>] = Topics::</xsl:text>
    <xsl:call-template name="topic-enum">
      <xsl:with-param name="topic" select="$topics[@xmi:id = current()/@topic]"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->readerTopics[ DataReaders::(dataReader/@name)] = Topics::(dataReader/@topic);\n' -->
  <xsl:for-each select="$lib-readers">
    <xsl:text>  this->readerTopics_[ DataReaders::</xsl:text>
    <xsl:call-template name="normalize-identifier"/>
    <xsl:text>] = Topics::</xsl:text>
    <xsl:call-template name="topic-enum">
      <xsl:with-param name="topic" select="$topics[@xmi:id = current()/@topic]"/>
    </xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:value-of select="$newline"/>

  <!-- '  this->publishers_[ DataWriters::(dataWriter/@name)] = Publishers::(dataWriter/../@name);\n' -->
  <xsl:for-each select="$lib-writers">
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
  <xsl:for-each select="$lib-readers">
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
  <xsl:text>}

inline
void
Elements::Data::buildParticipantsQos()
{
</xsl:text>
  <xsl:if test="$lib-participants">
    <xsl:text>  DomainParticipantQos participantQos;
  Participants::Values participant;
    </xsl:text>
  </xsl:if>
  <xsl:for-each select="$lib-participants">
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
    
    <xsl:text>  this->participantsQos_[ participant] = participantQos;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>}

inline
void
Elements::Data::buildTopicsQos()
{
</xsl:text>
  <xsl:if test="$topics">
    <xsl:text>  TopicQos       topicQos;
  Topics::Values topic;
    </xsl:text>
  </xsl:if>
  <xsl:for-each select="$topics">
    <xsl:value-of select="$newline"/>
    <xsl:text>  topic    = Topics::</xsl:text>
    <xsl:call-template name="topic-enum"/>
    <xsl:text>;
  topicQos = TheServiceParticipant->initial_TopicQos();
    
</xsl:text>
    <!-- '  topicQos.(policyfield) = (value);\n' -->
    <xsl:call-template name="process-policies">
      <xsl:with-param name="base"   select="'topicQos.'"/>
    </xsl:call-template>

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
  <xsl:when test="$lib-publishers">
    <xsl:text>  PublisherQos       publisherQos;
  Publishers::Values publisher;
</xsl:text>
  <xsl:for-each select="$lib-publishers">
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
    <xsl:when test="$lib-subscribers">

      <xsl:text>  SubscriberQos       subscriberQos;
  Subscribers::Values subscriber;
</xsl:text>
      <xsl:for-each select="$lib-subscribers">
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
  <xsl:when test="$lib-writers">
    <xsl:text>  DataWriters::Values  writer;
  DataWriterQos        writerQos;
</xsl:text>
    <xsl:for-each select="$lib-writers">
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
</xsl:text>
<xsl:choose>
  <xsl:when test="$lib-readers">
    <xsl:text>  DataReaders::Values  reader;
  DataReaderQos        readerQos;
</xsl:text>
    <xsl:for-each select="$lib-readers">
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
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  // No data readers were defined by this model
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

</xsl:text>
<xsl:choose>
  <xsl:when test="$lib-writers">
    <xsl:variable name="param-name">
      <xsl:if test="$policies[@xmi:id = $lib-writers/@*]">writerQos</xsl:if>
    </xsl:variable>
    <xsl:text>
inline
void
Elements::Data::copyPublicationQos(
  DataWriters::Values which,
  DataWriterQos&amp;  </xsl:text><xsl:value-of select="$param-name"/><xsl:text>
)
{
  switch(which) {
</xsl:text>
    <xsl:for-each select="$lib-writers">
      <xsl:text>    case DataWriters::</xsl:text>
      <xsl:call-template name="normalize-identifier"/>
      <xsl:text>:</xsl:text>
      <xsl:value-of select="$newline"/>

      <!-- '  writerQos.(policyfield) = (value);\n' -->
      <xsl:call-template name="process-policies">
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
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>
inline
void
Elements::Data::copyPublicationQos(
  DataWriters::Values,
  DataWriterQos&amp;
)
{
  throw NoWriterException();
}
</xsl:text>
  </xsl:otherwise>
</xsl:choose>

<xsl:choose>
  <xsl:when test="$lib-readers">
    <xsl:variable name="param-name">
      <xsl:if test="$policies[@xmi:id = $lib-readers/@*]">readerQos</xsl:if>
    </xsl:variable>
    <xsl:text>
inline
void
Elements::Data::copySubscriptionQos(
  DataReaders::Values which,
  DataReaderQos&amp;  </xsl:text><xsl:value-of select="$param-name"/><xsl:text>
)
{
  switch(which) {
</xsl:text>
  <xsl:for-each select="$lib-readers">
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
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>
inline
void
Elements::Data::copySubscriptionQos(
  DataReaders::Values,
  DataReaderQos&amp;
)
{
  throw NoReaderException();
}
    </xsl:text>
  </xsl:otherwise>
</xsl:choose>

  <xsl:value-of select="concat('} // End namespace ', @name, $newline)"/>
</xsl:template>

<!-- Process all the policies referenced by base -->
<xsl:template name="process-policies">
  <xsl:param name="base"/>

  <!-- direct references -->
  <xsl:variable name="reffed-policies" select="$policies[@xmi:id = current()/@*]"/>

  <xsl:for-each select="$reffed-policies">
    <xsl:call-template name="process-policy">
      <xsl:with-param name="base" select="$base"/>
    </xsl:call-template>
  </xsl:for-each>
</xsl:template>

<!-- Process an individual policy referenced by base -->
<xsl:template name="process-policy">
  <xsl:param name="base"/>
  <xsl:variable name="policy-type" select="@xsi:type"/>

  <!-- lookup the field name for the current policy type. -->
  <xsl:variable name="field">
    <xsl:for-each select="$lut-policies"> <!-- Change context for lookup -->
      <xsl:value-of select="key('lut-qos-field', $policy-type)/text()"/>
    </xsl:for-each>
  </xsl:variable>

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
                      name() = 'model' or
                      name() = 'xsi:type' or 
                      name() = 'xmi:id'"/>

      <xsl:when test="../@xsi:type = 'opendds:udQosPolicy'
                   or ../@xsi:type = 'opendds:tdQosPolicy'
                   or ../@xsi:type = 'opendds:gdQosPolicy'">
        <xsl:variable name="value" select="concat($base, $field, '.value')"/>
        <xsl:value-of select="concat('  ', $value, '.replace(', $newline, 
                                '      ', string-length(.) + 1, ',', $newline,
                                '      ', string-length(.) + 1, ',', $newline,
                                '      reinterpret_cast&lt;CORBA::Octet*&gt;(const_cast&lt;char *&gt;(&quot;', ., '&quot;)));', $newline)"/>

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

<!-- For each referenced data model, add #include statement -->
<xsl:template name="include-referenced-data-models">
  <xsl:param name="models"/>
  <xsl:param name="completed-models" select="' '"/>
  
  <xsl:if test="$models">
    <xsl:variable name="model" select="$models[1]"/>

    <xsl:if test="not(contains($completed-models, $model))">
      <xsl:value-of select="concat('#include &quot;', $model, 'TypeSupportImpl.h&quot;', $newline)"/>
    </xsl:if>
    <xsl:call-template name="include-referenced-data-models">
      <xsl:with-param name="models" select="$models[position() &gt; 1]"/>
      <xsl:with-param name="completed-models" select="concat(' ', $model, ' ')"/>
    </xsl:call-template>
  </xsl:if>
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
  <xsl:value-of select="concat('::', $scopename, $type/@name)"/>
  <xsl:text>TypeSupportImpl TypeSupport;

        TypeSupport* typeSupport = new TypeSupport();
        if( RETCODE_OK != typeSupport->register_type( participant, 0)) {
          throw BadRegisterException();
        }

        if( this->typeNames_[ type]) {
          CORBA::string_free( this->typeNames_[ type]); // Was created by CORBA::string_dup()
        }
        this->typeNames_[ type] = typeSupport->get_type_name();
      }
      break;

</xsl:text>
</xsl:template>

<!-- Ignore text -->
<xsl:template match="text()"/>

</xsl:stylesheet>

