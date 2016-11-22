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
#include "dds/DCPS/PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
        const DDS::DomainParticipantQos& participantQos,
        DDS::StatusMask           mask,
        const OPENDDS_STRING&     transportConfig
      );

      DDS::Topic*
      createTopic(
        DDS::DomainParticipant* participant,
        const OPENDDS_STRING&   topicName,
        const OPENDDS_STRING&   typeName,
        const DDS::TopicQos&    topicQos,
        DDS::StatusMask         mask
      );

      DDS::Publisher*
      createPublisher(
        DDS::DomainParticipant*       participant,
        const DDS::PublisherQos&      publisherQos,
        DDS::StatusMask               mask,
        const OPENDDS_STRING&         transportConfig
      );

      DDS::Subscriber*
      createSubscriber(
        DDS::DomainParticipant*       participant,
        const DDS::SubscriberQos&     subscriberQos,
        DDS::StatusMask               mask,
        const OPENDDS_STRING&         transportConfig
      );

      DDS::DataWriter*
      createPublication(
        unsigned int              which,
        DDS::Publisher*           publisher,
        DDS::Topic*               topic,
        const DDS::DataWriterQos& writerQos,
        DDS::StatusMask           mask,
        const OPENDDS_STRING&     transportConfig,
        bool               copyQosFromTopic
      );

      DDS::DataWriter*
      createWriter(
        DDS::Publisher*        publisher,
        DDS::Topic*            topic,
        const DDS::DataWriterQos& writerQos,
        DDS::StatusMask        mask,
        const OPENDDS_STRING&  transportConfig
      );

      DDS::DataReader*
      createSubscription(
        unsigned int           which,
        DDS::Subscriber*       subscriber,
        DDS::TopicDescription* topic,
        const DDS::DataReaderQos& readerQos,
        DDS::StatusMask        mask,
        const OPENDDS_STRING&  transportConfig,
        bool                   copyQosFromTopic
      );

      DDS::DataReader*
      createReader(
        DDS::Subscriber*       subscriber,
        DDS::TopicDescription* topic,
        const DDS::DataReaderQos& readerQos,
        DDS::StatusMask        mask,
        const OPENDDS_STRING&  transportConfig
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* DELEGATE_H */

