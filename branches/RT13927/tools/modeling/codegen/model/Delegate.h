#ifndef DELEGATE_H
#define DELEGATE_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "model_export.h" // FROM GENERATOR CONFIGURATION SPEC

#include "dds/DdsDcpsInfrastructureC.h" // For QoS Policy types.

namespace DDS {
  class DomainParticipant;
  class Topic;
  class Publisher;
  class Subscriber;
  class DataWriter;
  class DataReader;
} // End of namespace DDS

namespace OpenDDS { namespace DCPS {
  class TransportConfiguration;
  class TransportImpl;
} } // End of namespace OpenDDS::DCPS

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export Delegate {
    public:
      void init( int argc, char** argv);

      void
      createParticipant(
        DDS::DomainParticipant*&  participant,
        unsigned long             domain,
        DDS::DomainParticipantQos participantQos,
        DDS::StatusMask           mask
      );

      void
      createTopic(
        DDS::Topic*&            topic,
        DDS::DomainParticipant* participant,
        const char*             topicName,
        const char*             typeName,
        DDS::TopicQos           topicQos,
        DDS::StatusMask         mask
      );

      void
      createPublisher(
        DDS::Publisher*&              publisher,
        DDS::DomainParticipant*       participant,
        DDS::PublisherQos             publisherQos,
        DDS::StatusMask               mask,
        OpenDDS::DCPS::TransportImpl* transport
      );

      void
      createSubscriber(
        DDS::Subscriber*&             subscriber,
        DDS::DomainParticipant*       participant,
        DDS::SubscriberQos            subscriberQos,
        DDS::StatusMask               mask,
        OpenDDS::DCPS::TransportImpl* transport
      );

      void
      createPublication(
        DDS::DataWriter*&  writer,
        DDS::Publisher*    publisher,
        DDS::Topic*        topic,
        DDS::DataWriterQos writerQos,
        DDS::StatusMask    mask
      );

      void
      createSubscription(
        DDS::DataReader*&  reader,
        DDS::Subscriber*   subscriber,
        DDS::Topic*        topic,
        DDS::DataReaderQos readerQos,
        DDS::StatusMask    mask
      );

      void
      createTransport(
        OpenDDS::DCPS::TransportImpl*&         transport,
        unsigned long                          key,
        const char*                            kind,
        OpenDDS::DCPS::TransportConfiguration* config
      );
  };

} } // End of namespace OpenDDS::Model

#include /**/ "ace/post.h"

#endif /* DELEGATE_H */

