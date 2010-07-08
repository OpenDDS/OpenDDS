#ifndef DELEGATE_H
#define DELEGATE_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "model_export.h"
#include "CopyQos.h"

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
      Delegate();

      void init( int argc, char** argv);
      void fini();

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
        unsigned int       which,
        DDS::DataWriter*&  writer,
        DDS::Publisher*    publisher,
        DDS::Topic*        topic,
        DDS::DataWriterQos writerQos,
        DDS::StatusMask    mask
      );

      void
      createSubscription(
        unsigned int       which,
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

      CopyQos*& service();

    private:
      CopyQos* service_;
  };

} } // End of namespace OpenDDS::Model

#include /**/ "ace/post.h"

#endif /* DELEGATE_H */

