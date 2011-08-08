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
#include "dds/DCPS/transport/framework/TransportDefs.h"

namespace DDS {
  class DomainParticipant;
  class Topic;
  class TopicDescription;
  class Publisher;
  class Subscriber;
  class DataWriter;
  class DataReader;
} // End of namespace DDS

namespace OpenDDS { namespace DCPS {
  class TransportInst;
  class TransportImpl;
} } // End of namespace OpenDDS::DCPS

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export Delegate {
    public:
      Delegate();

      DDS::DomainParticipant*
      createParticipant(
        unsigned long             domain,
        DDS::DomainParticipantQos participantQos,
        DDS::StatusMask           mask,
        const std::string&        transportConfig
      );

      DDS::Topic*
      createTopic(
        DDS::DomainParticipant* participant,
        const std::string&      topicName,
        const std::string&      typeName,
        DDS::TopicQos           topicQos,
        DDS::StatusMask         mask
      );

      DDS::Publisher*
      createPublisher(
        DDS::DomainParticipant*       participant,
        DDS::PublisherQos             publisherQos,
        DDS::StatusMask               mask,
        const std::string&            transportConfig
      );

      DDS::Subscriber*
      createSubscriber(
        DDS::DomainParticipant*       participant,
        DDS::SubscriberQos            subscriberQos,
        DDS::StatusMask               mask,
        const std::string&            transportConfig
      );

      DDS::DataWriter*
      createPublication(
        unsigned int       which,
        DDS::Publisher*    publisher,
        DDS::Topic*        topic,
        DDS::DataWriterQos writerQos,
        DDS::StatusMask    mask,
        const std::string& transportConfig,
        bool               copyQosFromTopic
      );

      DDS::DataWriter*
      createWriter(
        DDS::Publisher*        publisher,
        DDS::Topic*            topic,
        DDS::DataWriterQos     writerQos,
        DDS::StatusMask        mask,
        const std::string&     transportConfig
      );

      DDS::DataReader*
      createSubscription(
        unsigned int           which,
        DDS::Subscriber*       subscriber,
        DDS::TopicDescription* topic,
        DDS::DataReaderQos     readerQos,
        DDS::StatusMask        mask,
        const std::string&     transportConfig,
        bool                   copyQosFromTopic
      );

      DDS::DataReader*
      createReader(
        DDS::Subscriber*       subscriber,
        DDS::TopicDescription* topic,
        DDS::DataReaderQos     readerQos,
        DDS::StatusMask        mask,
        const std::string&     transportConfig
      );

      CopyQos*& service();

    private:
      CopyQos* service_;

      bool override_autoenabled_qos(DDS::Publisher* publisher);
      bool override_autoenabled_qos(DDS::Subscriber* subscriber);

      void restore_autoenabled_qos(DDS::Publisher* publisher);
      void restore_autoenabled_qos(DDS::Subscriber* subscriber);
  };

} } // End of namespace OpenDDS::Model

#include /**/ "ace/post.h"

#endif /* DELEGATE_H */

