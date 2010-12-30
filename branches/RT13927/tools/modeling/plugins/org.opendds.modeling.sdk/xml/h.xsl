<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xmi='http://www.omg.org/XMI'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'>
  <!--
    ** $Id$
    **
    ** Generate C++ header code.
    **
    -->
<xsl:include href="common.xsl"/>

<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- blech -->
<xsl:variable name="lowercase" select="'abcdefghijklmnopqrstuvwxyz'"/>
<xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>

<!-- Node sets -->
<xsl:variable name="participants" select="//participants"/>
<xsl:variable name="topics"       select="//topics"/>
<xsl:variable name="types"        select="//dataLib/types"/>
<xsl:variable name="publishers"   select="//publishers"/>
<xsl:variable name="subscribers"  select="//subscribers"/>
<xsl:variable name="writers"      select="//writers"/>
<xsl:variable name="readers"      select="//readers"/>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "/opendds:OpenDDSModel/@name"/>
<xsl:variable name = "MODELNAME" select = "translate( $modelname, $lowercase, $uppercase)"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">

  <xsl:text>#ifndef </xsl:text>
  <xsl:value-of select="$MODELNAME"/>
  <xsl:text>_T_H
#define </xsl:text>
  <xsl:value-of select="$MODELNAME"/>
  <xsl:text>_T_H

#include "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_export.h"

#include "model/Exceptions.h"

#include "dds/DdsDcpsInfrastructureC.h" // For QoS Policy types.
#include "dds/DCPS/transport/framework/TransportDefs.h"

namespace DDS {
  class DomainParticipant; // For type registration
} // End of namespace DDS

namespace OpenDDS { namespace DCPS {
  class TransportConfiguration;
} } // End of namespace OpenDDS::DCPS

namespace OpenDDS { namespace Model { namespace </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text> {

  using namespace OpenDDS::DCPS;
  using namespace DDS;

  class Elements {
    public:
</xsl:text>

  <xsl:call-template name="generate-enum">
    <xsl:with-param name="class" select="'Participants'"/>
    <xsl:with-param name="values" select="$participants"/>
  </xsl:call-template>

  <xsl:value-of select="concat('      class Types {', $newline)"/>

  <xsl:value-of select="concat('        public: enum Values {', $newline)"/>
  <xsl:for-each select="$types[@xmi:id = $topics/@datatype]">
    <xsl:variable name="enum">
      <xsl:call-template name="type-enum"/>
    </xsl:variable>
    <xsl:value-of select="concat('          ', $enum, ',', $newline)"/>
  </xsl:for-each>

  <xsl:value-of select="concat('          LAST_INDEX', $newline)"/>

  <xsl:value-of select="concat('        };', $newline)"/>
  <xsl:value-of select="concat('      };', $newline, $newline)"/>

  <xsl:call-template name="generate-enum">
    <xsl:with-param name="class" select="'Topics'"/>
    <xsl:with-param name="values" select="$topics"/>
  </xsl:call-template>

  <xsl:call-template name="generate-enum">
    <xsl:with-param name="class" select="'Publishers'"/>
    <xsl:with-param name="values" select="$publishers"/>
  </xsl:call-template>

  <xsl:call-template name="generate-enum">
    <xsl:with-param name="class" select="'Subscribers'"/>
    <xsl:with-param name="values" select="$subscribers"/>
  </xsl:call-template>

  <xsl:call-template name="generate-enum">
    <xsl:with-param name="class" select="'DataWriters'"/>
    <xsl:with-param name="values" select="$writers"/>
  </xsl:call-template>

  <xsl:call-template name="generate-enum">
    <xsl:with-param name="class" select="'DataReaders'"/>
    <xsl:with-param name="values" select="$readers"/>
  </xsl:call-template>

<xsl:text>
      class Data {
        public:
          Data();
          ~Data();

          ///{ @name Qos Policy values
          DDS::DomainParticipantQos qos( Participants::Values which);
          DDS::TopicQos             qos( Topics::Values which);
          DDS::PublisherQos         qos( Publishers::Values which);
          DDS::SubscriberQos        qos( Subscribers::Values which);
          DDS::DataWriterQos        qos( DataWriters::Values which);
          DDS::DataReaderQos        qos( DataReaders::Values which);

          bool copyTopicQos(DataWriters::Values which);
          bool copyTopicQos(DataReaders::Values which);

          void copyPublicationQos(  DataWriters::Values which, DDS::DataWriterQos&amp; writerQos);
          void copySubscriptionQos( DataReaders::Values which, DDS::DataReaderQos&amp; readerQos);
          ///}

          ///{ @name Other configuration values
          long        mask( Participants::Values which);
          long        mask( Publishers::Values which);
          long        mask( Subscribers::Values which);
          long        mask( Topics::Values which);
          long        mask( DataWriters::Values which);
          long        mask( DataReaders::Values which);
          long        domain( Participants::Values which);
          const char* typeName( Types::Values which);
          const char* topicName( Topics::Values which);
          ///}

          /// Type registration.
          void registerType( Types::Values type, DDS::DomainParticipant* participant);

          ///{ @name Containment Relationships
          Participants::Values participant( Publishers::Values which);
          Participants::Values participant( Subscribers::Values which);
          Types::Values        type( Topics::Values which);
          Topics::Values       topic( DataWriters::Values which);
          Topics::Values       topic( DataReaders::Values which);
          Publishers::Values   publisher( DataWriters::Values which);
          Subscribers::Values  subscriber( DataReaders::Values which);
          OpenDDS::DCPS::TransportIdType transport( Publishers::Values which);
          OpenDDS::DCPS::TransportIdType transport( Subscribers::Values which);
          ///}

        private:

          // Initialization.
          void loadDomains();
          void loadTopics();
          void loadMaps();

          void buildParticipantsQos();
          void buildTopicsQos();
          void buildPublishersQos();
          void buildSubscribersQos();
          void buildPublicationsQos();
          void buildSubscriptionsQos();

          // Basic array containers since we only allow access using the
          // defined enumeration values.
</xsl:text>
<xsl:if test="$participants">
  <xsl:text>
          unsigned long             domains_[                Participants::LAST_INDEX];
          DDS::DomainParticipantQos participantsQos_[        Participants::LAST_INDEX];
</xsl:text>
</xsl:if>
<xsl:if test="$publishers">
  <xsl:text>
          Participants::Values      publisherParticipants_[   Publishers::LAST_INDEX];
          DDS::PublisherQos         publishersQos_[           Publishers::LAST_INDEX];
          OpenDDS::DCPS::TransportIdType publisherTransports_[Publishers::LAST_INDEX];
</xsl:text>
</xsl:if>
<xsl:if test="$subscribers">
  <xsl:text>
          Participants::Values      subscriberParticipants_[   Subscribers::LAST_INDEX];
          DDS::SubscriberQos        subscribersQos_[           Subscribers::LAST_INDEX];
          OpenDDS::DCPS::TransportIdType subscriberTransports_[Subscribers::LAST_INDEX];
</xsl:text>
</xsl:if>
<xsl:if test="$writers">
  <xsl:text>
          Topics::Values            writerTopics_[           DataWriters::LAST_INDEX];
          Publishers::Values        publishers_[             DataWriters::LAST_INDEX];
          DDS::DataWriterQos        writersQos_[             DataWriters::LAST_INDEX];
          bool                      writerCopyTopicQos_[     DataWriters::LAST_INDEX];
</xsl:text>
</xsl:if>
<xsl:if test="$readers">
<xsl:text>
          Topics::Values            readerTopics_[           DataReaders::LAST_INDEX];
          Subscribers::Values       subscribers_[            DataReaders::LAST_INDEX];
          DDS::DataReaderQos        readersQos_[             DataReaders::LAST_INDEX];
          bool                      readerCopyTopicQos_[     DataReaders::LAST_INDEX];
</xsl:text>
</xsl:if>
<xsl:if test="$topics">
<xsl:text>
          Types::Values             types_[                  Topics::LAST_INDEX];
          const char*               topicNames_[             Topics::LAST_INDEX];
          DDS::TopicQos             topicsQos_[              Topics::LAST_INDEX];
          char*                     typeNames_[              Types::LAST_INDEX];
</xsl:text>
</xsl:if>
<xsl:text>

      };
  };
} } } // End of namespace OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>

inline
DDS::DomainParticipantQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( Participants::Values which)
{
  if( which &lt; 0 || which >= Participants::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$participants">
    <xsl:text>  return this->participantsQos_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return DDS::DomainParticipantQos(); // not valid when no domain participants defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
DDS::TopicQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( Topics::Values which)
{
  if( which &lt; 0 || which >= Topics::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$topics">
    <xsl:text>  return this->topicsQos_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return DDS::TopicQos(); // not valid when no topics defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
DDS::PublisherQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( Publishers::Values which)
{
  if( which &lt; 0 || which >= Publishers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$publishers">
    <xsl:text>  return this->publishersQos_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return PublisherQos();  // not valid when no publishers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
DDS::SubscriberQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( Subscribers::Values which)
{
  if( which &lt; 0 || which >= Subscribers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$subscribers">
    <xsl:text>  return this->subscribersQos_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return DDS::SubscriberQos(); // not valid when no subscribers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
DDS::DataWriterQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( DataWriters::Values which)
{
  if( which &lt; 0 || which >= DataWriters::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$writers">
    <xsl:text>  return this->writersQos_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return DDS::DataWriterQos(); // not valid when no data writers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
DDS::DataReaderQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( DataReaders::Values which)
{
  if( which &lt; 0 || which >= DataReaders::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$readers">
    <xsl:text>  return this->readersQos_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return DDS::DataReaderQos(); // not valid when no data readers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
bool
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::copyTopicQos(DataWriters::Values which)
{
  if (which &lt; 0 || which >= DataWriters::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$writers">
    <xsl:text>  return this->writerCopyTopicQos_[which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return false; // not valid when no data writers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
bool
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::copyTopicQos(DataReaders::Values which)
{
  if (which &lt; 0 || which >= DataReaders::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$readers">
    <xsl:text>  return this->readerCopyTopicQos_[which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return false; // not valid when no data readers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::mask( Participants::Values which)
{
  if( which &lt; 0 || which >= Participants::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return DEFAULT_STATUS_MASK;
}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::mask( Publishers::Values which)
{
  if( which &lt; 0 || which >= Publishers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return DEFAULT_STATUS_MASK;
}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::mask( Subscribers::Values which)
{
  if( which &lt; 0 || which >= Subscribers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return DEFAULT_STATUS_MASK;
}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::mask( Topics::Values which)
{
  if( which &lt; 0 || which >= Topics::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return DEFAULT_STATUS_MASK;
}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::mask( DataWriters::Values which)
{
  if( which &lt; 0 || which >= DataWriters::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return DEFAULT_STATUS_MASK;
}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::mask( DataReaders::Values which)
{
  if( which &lt; 0 || which >= DataReaders::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return DEFAULT_STATUS_MASK;
}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::domain( Participants::Values which)
{
  if( which &lt; 0 || which >= Participants::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$participants">
    <xsl:text>  return this->domains_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return 0; // not valid when no domain participants defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
const char*
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::typeName( Types::Values which)
{
  if( which &lt; 0 || which >= Types::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$topics">
    <xsl:text>  return this->typeNames_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return (const char*)NULL; // not valid when no topics defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
const char*
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::topicName( Topics::Values which)
{
  if( which &lt; 0 || which >= Topics::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$topics">
    <xsl:text>  return this->topicNames_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return (const char*)NULL;  // not valid when no topics defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Participants::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::participant( Publishers::Values which)
{
  if( which &lt; 0 || which >= Publishers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$publishers">
    <xsl:text>  return this->publisherParticipants_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return Participants::LAST_INDEX; // not valid when no publishers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Participants::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::participant( Subscribers::Values which)
{
  if( which &lt; 0 || which >= Subscribers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$subscribers">
    <xsl:text>  return this->subscriberParticipants_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return Participants::LAST_INDEX; // Not valid when no subscribers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Types::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::type( Topics::Values which)
{
  if( which &lt; 0 || which >= Topics::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$topics">
    <xsl:text>  return this->types_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return Types::LAST_INDEX; // not valid when no types defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Topics::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::topic( DataWriters::Values which)
{
  if( which &lt; 0 || which >= DataWriters::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$writers">
    <xsl:text>  return this->writerTopics_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return Topics::LAST_INDEX; // not valid when no data writers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Topics::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::topic( DataReaders::Values which)
{
  if( which &lt; 0 || which >= DataReaders::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$readers">
    <xsl:text>  return this->readerTopics_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return Topics::LAST_INDEX; // not valid when no data readers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Publishers::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::publisher( DataWriters::Values which)
{
  if( which &lt; 0 || which >= DataWriters::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$writers">
    <xsl:text>  return this->publishers_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text> return Publishers::LAST_INDEX; 
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Subscribers::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::subscriber( DataReaders::Values which)
{
  if( which &lt; 0 || which >= DataReaders::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$readers">
    <xsl:text>  return this->subscribers_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return Subscribers::LAST_INDEX; // not valid when no data readers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>}

inline
OpenDDS::DCPS::TransportIdType
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::transport( Publishers::Values which)
{
  if( which &lt; 0 || which >= Publishers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$publishers">
    <xsl:text>  return this->publisherTransports_[ which];
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return 0; // not valid when no publishers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>

}

inline
OpenDDS::DCPS::TransportIdType
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::transport( Subscribers::Values which)
{
  if( which &lt; 0 || which >= Subscribers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
</xsl:text>
<xsl:choose>
  <xsl:when test="$subscribers">  return this->subscriberTransports_[ which];
    <xsl:text>
</xsl:text>
  </xsl:when>
  <xsl:otherwise>
    <xsl:text>  return 0;  //  not valid when no subscribers defined
</xsl:text>
  </xsl:otherwise>
</xsl:choose>
<xsl:text>
}

// The template implementation.

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

// Establish the model interfaces for use by the application.

</xsl:text>
  <xsl:value-of select="concat('#include &quot;', $modelname, 'TypeSupportImpl.h&quot;', $newline)"/>
<xsl:text>
#include "model/DefaultInstanceTraits.h"
#include "model/Service_T.h"

#endif /* </xsl:text>
  <xsl:value-of select="$MODELNAME"/>
  <xsl:text>_T_H */

</xsl:text>
</xsl:template>
<!-- End of main processing template. -->

<xsl:template name="generate-enum">
  <xsl:param name="class" />
  <xsl:param name="values" />

  <xsl:value-of select="concat('      class ',$class, ' {')"/>
  <xsl:call-template name="generate-enum-values">
    <xsl:with-param name="values" select="$values"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="generate-enum-values">
  <xsl:param name="values" />
    <xsl:text>
        public: enum Values {
</xsl:text>
    <xsl:for-each select="$values">
      <xsl:text>          </xsl:text>
      <xsl:call-template name="normalize-identifier"/>
      <xsl:value-of select="concat(',', $newline)"/>
    </xsl:for-each>
    <xsl:text>          LAST_INDEX
        };
      };

</xsl:text>
</xsl:template>

</xsl:stylesheet>

