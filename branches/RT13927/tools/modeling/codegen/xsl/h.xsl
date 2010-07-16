<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:lut='http://www.opendds.com/modeling/schemas/Lut/1.0'
     xmlns:opendds='http://www.opendds.com/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.com/modeling/schemas/Generator/1.0'>
  <!--
    ** $Id$
    **
    ** Generate C++ header code.
    **
    -->
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
<xsl:variable name="participant" select="//opendds:domainParticipant"/>
<xsl:variable name="topic"       select="//opendds:topic"/>
<xsl:variable name="publisher"   select="//opendds:publisher"/>
<xsl:variable name="subscriber"  select="//opendds:subscriber"/>
<xsl:variable name="writer"      select="//opendds:dataWriter"/>
<xsl:variable name="reader"      select="//opendds:dataReader"/>
<xsl:variable name="transport"   select="//opendds:transport"/>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "/generator:model/@name"/>
<xsl:variable name = "MODELNAME" select = "translate( $modelname, $lowercase, $uppercase)"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">

  <xsl:text>#ifndef </xsl:text>
  <xsl:value-of select="$MODELNAME"/>
  <xsl:text>_H
#define </xsl:text>
  <xsl:value-of select="$MODELNAME"/>
  <xsl:text>_H

#include "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_export.h"

#include "model/Exceptions.h"

#include "dds/DdsDcpsInfrastructureC.h" // For QoS Policy types.

namespace DDS {
  class DomainParticipant; // For type registration
} // End of namespace DDS

namespace OpenDDS { namespace DCPS {
  class TransportConfiguration;
} } // End of namespace OpenDDS::DCPS

namespace OpenDDS { namespace Model { namespace </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text> {
  class Elements {
    public:
      class Participants {
        public: enum Values {
</xsl:text>
  <!-- '          (//domainParticipant/@name),\n' -->
  <xsl:for-each select="$participant">
    <xsl:value-of select="'          '"/>
    <xsl:value-of select="@name"/>
    <xsl:text>,</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>          LAST_INDEX
          };
      };

      class Types {
        public: enum Values {
</xsl:text>
  <!-- '          (//type/@name),\n' -->
  <xsl:variable name="defined-types" select="$topic/opendds:datatype"/>
  <xsl:for-each select="$defined-types">
    <!-- Don't sort this without sorting the prior list as well. -->

    <!-- Only generate type code once for each type. -->
    <xsl:variable name="curpos" select="position()"/>
    <xsl:variable name="priors" select="$defined-types[ position() &lt; $curpos]"/>
    <xsl:if test="count( $priors[@name = current()/@name]) = 0">
      <xsl:value-of select="'          '"/>
      <xsl:value-of select="@name"/>
      <xsl:text>,</xsl:text>
      <xsl:value-of select="$newline"/>
    </xsl:if>
  </xsl:for-each>
  <xsl:text>          LAST_INDEX
        };
      };

      class Topics {
        public: enum Values {
</xsl:text>
  <!-- '          (//topic/@name),\n' -->
  <xsl:for-each select="$topic">
    <xsl:value-of select="'          '"/>
    <xsl:value-of select="translate(@name,' ','_')"/>
    <xsl:text>,</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>          LAST_INDEX
        };
      };

      class Publishers {
        public: enum Values {
</xsl:text>
  <!-- '          (//publisher/@name),\n' -->
  <xsl:for-each select="$publisher">
    <xsl:value-of select="'          '"/>
    <xsl:value-of select="@name"/>
    <xsl:text>,</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>          LAST_INDEX
        };
      };

      class Subscribers {
        public: enum Values {
</xsl:text>
  <!-- '          (//subscriber/@name),\n' -->
  <xsl:for-each select="$subscriber">
    <xsl:value-of select="'          '"/>
    <xsl:value-of select="@name"/>
    <xsl:text>,</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>          LAST_INDEX
        };
      };

      class DataWriters {
        public: enum Values {
</xsl:text>
  <!-- '          (//dataWriter/@name),\n' -->
  <xsl:for-each select="$writer">
    <xsl:value-of select="'          '"/>
    <xsl:value-of select="@name"/>
    <xsl:text>,</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>          LAST_INDEX
        };
      };

      class DataReaders {
        public: enum Values {
</xsl:text>
  <!-- '          (//dataReader/@name),\n' -->
  <xsl:for-each select="$reader">
    <xsl:value-of select="'          '"/>
    <xsl:value-of select="@name"/>
    <xsl:text>,</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>          LAST_INDEX
        };
      };

      class Transports {
        public: enum Values {
</xsl:text>
  <!-- '          (//transport/@name),\n' -->
  <xsl:for-each select="$transport">
    <xsl:value-of select="'          '"/>
    <xsl:value-of select="@name"/>
    <xsl:text>,</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:for-each>
  <xsl:text>          LAST_INDEX
        };
      };

      class </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_Export Data {
        public:
          Data();
          ~Data();

          /// Delayed initialization.
          void init();

          ///{ @name Qos Policy values
          DDS::DomainParticipantQos qos( Participants::Values which);
          DDS::TopicQos             qos( Topics::Values which);
          DDS::PublisherQos         qos( Publishers::Values which);
          DDS::SubscriberQos        qos( Subscribers::Values which);
          DDS::DataWriterQos        qos( DataWriters::Values which);
          DDS::DataReaderQos        qos( DataReaders::Values which);

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
          const char* transportKind( Transports::Values which);
          long        transportKey( Transports::Values which);
          OpenDDS::DCPS::TransportConfiguration* transportConfig( Transports::Values which);
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
          Transports::Values   transport( Publishers::Values which);
          Transports::Values   transport( Subscribers::Values which);
          ///}

        private:

          // Initialization.
          void loadMasks();
          void loadDomains();
          void loadTopics();
          void loadTransports();
          void loadMaps();

          void buildParticipantsQos();
          void buildTopicsQos();
          void buildPublishersQos();
          void buildSubscribersQos();
          void buildPublicationsQos();
          void buildSubscriptionsQos();

          void buildTransportsConfig();

          // Basic array containers since we only allow access using the
          // defined enumeration values.

          Participants::Values      publisherParticipants_[  Publishers::LAST_INDEX];
          Participants::Values      subscriberParticipants_[ Subscribers::LAST_INDEX];
          Types::Values             types_[                  Topics::LAST_INDEX];
          Topics::Values            writerTopics_[           DataWriters::LAST_INDEX];
          Topics::Values            readerTopics_[           DataReaders::LAST_INDEX];
          Publishers::Values        publishers_[             DataWriters::LAST_INDEX];
          Subscribers::Values       subscribers_[            DataReaders::LAST_INDEX];
          Transports::Values        publisherTransports_[    Publishers::LAST_INDEX];
          Transports::Values        subscriberTransports_[   Subscribers::LAST_INDEX];

          unsigned long             participantMasks_[       Participants::LAST_INDEX];
          unsigned long             publisherMasks_[         Publishers::LAST_INDEX];
          unsigned long             subscriberMasks_[        Subscribers::LAST_INDEX];
          unsigned long             topicMasks_[             Topics::LAST_INDEX];
          unsigned long             writerMasks_[            DataWriters::LAST_INDEX];
          unsigned long             readerMasks_[            DataReaders::LAST_INDEX];

          unsigned long             domains_[                Participants::LAST_INDEX];
          char*                     typeNames_[              Types::LAST_INDEX];
          const char*               topicNames_[             Topics::LAST_INDEX];
          const char*               transportKinds_[         Transports::LAST_INDEX];
          unsigned long             transportKeys_[          Transports::LAST_INDEX];

          OpenDDS::DCPS::TransportConfiguration* transportConfigs_[ Transports::LAST_INDEX];

          DDS::DomainParticipantQos participantsQos_[        Participants::LAST_INDEX];
          DDS::TopicQos             topicsQos_[              Topics::LAST_INDEX];
          DDS::PublisherQos         publishersQos_[          Publishers::LAST_INDEX];
          DDS::SubscriberQos        subscribersQos_[         Subscribers::LAST_INDEX];
          DDS::DataWriterQos        writersQos_[             DataWriters::LAST_INDEX];
          DDS::DataReaderQos        readersQos_[             DataReaders::LAST_INDEX];
      };
  };
} } } // End of namespace OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>

inline
void
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::init()
{
  this->buildTransportsConfig();
}

inline
DDS::DomainParticipantQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( Participants::Values which)
{
  if( which &lt; 0 || which >= Participants::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->participantsQos_[ which];
}

inline
DDS::TopicQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( Topics::Values which)
{
  if( which &lt; 0 || which >= Topics::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->topicsQos_[ which];
}

inline
DDS::PublisherQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( Publishers::Values which)
{
  if( which &lt; 0 || which >= Publishers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->publishersQos_[ which];
}

inline
DDS::SubscriberQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( Subscribers::Values which)
{
  if( which &lt; 0 || which >= Subscribers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->subscribersQos_[ which];
}

inline
DDS::DataWriterQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( DataWriters::Values which)
{
  if( which &lt; 0 || which >= DataWriters::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->writersQos_[ which];
}

inline
DDS::DataReaderQos
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::qos( DataReaders::Values which)
{
  if( which &lt; 0 || which >= DataReaders::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->readersQos_[ which];
}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::mask( Participants::Values which)
{
  if( which &lt; 0 || which >= Participants::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->participantMasks_[ which];
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
  return this->publisherMasks_[ which];
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
  return this->subscriberMasks_[ which];
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
  return this->topicMasks_[ which];
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
  return this->writerMasks_[ which];
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
  return this->readerMasks_[ which];
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
  return this->domains_[ which];
}

inline
const char*
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::typeName( Types::Values which)
{
  if( which &lt; 0 || which >= Types::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->typeNames_[ which];
}

inline
const char*
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::topicName( Topics::Values which)
{
  if( which &lt; 0 || which >= Topics::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->topicNames_[ which];
}

inline
const char*
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::transportKind( Transports::Values which)
{
  if( which &lt; 0 || which >= Transports::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->transportKinds_[ which];
}

inline
long
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::transportKey( Transports::Values which)
{
  if( which &lt; 0 || which >= Transports::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->transportKeys_[ which];
}

inline
OpenDDS::DCPS::TransportConfiguration*
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::transportConfig( Transports::Values which)
{
  if( which &lt; 0 || which >= Transports::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->transportConfigs_[ which];
}

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
  return this->publisherParticipants_[ which];
}

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
  return this->subscriberParticipants_[ which];
}

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
  return this->types_[ which];
}

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
  return this->writerTopics_[ which];
}

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
  return this->readerTopics_[ which];
}

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
  return this->publishers_[ which];
}

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
  return this->subscribers_[ which];
}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Transports::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::transport( Publishers::Values which)
{
  if( which &lt; 0 || which >= Publishers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->publisherTransports_[ which];
}

inline
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Transports::Values
OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements::Data::transport( Subscribers::Values which)
{
  if( which &lt; 0 || which >= Subscribers::LAST_INDEX) {
    throw OutOfBoundsException();
  }
  return this->subscriberTransports_[ which];
}

// Establish the model interfaces for use by the application.

#include "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>TypeSupportImpl.h"
#include "model/Service_T.h"

typedef OpenDDS::Model::Service&lt; OpenDDS::Model::</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::Elements&gt; </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>Type;

#endif /* </xsl:text>
  <xsl:value-of select="$MODELNAME"/>
  <xsl:text>_H */

</xsl:text>
</xsl:template>
<!-- End of main processing template. -->

</xsl:stylesheet>

