/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_STATICDISCOVERY_STATICDISCOVERY_H
#define OPENDDS_STATICDISCOVERY_STATICDISCOVERY_H

#ifndef DDS_HAS_MINIMUM_BIT

#include "dds/DCPS/DiscoveryBase.h"
#include "dds/DCPS/Service_Participant.h"
#include "dcps_export.h"

#include "ace/Configuration.h"

#include "dds/DCPS/PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
  namespace DCPS {

    class StaticDiscovery;

    typedef OpenDDS::DCPS::RcHandle<StaticDiscovery> StaticDiscovery_rch;

    class OpenDDS_Dcps_Export EndpointRegistry {
    public:
      struct Topic {
        OPENDDS_STRING name;
        OPENDDS_STRING type_name;
        Topic() { }
      };
      typedef OPENDDS_MAP(OPENDDS_STRING, Topic) TopicMapType;
      TopicMapType topic_map;

      typedef OPENDDS_MAP(OPENDDS_STRING, DDS::DataReaderQos) DataReaderQosMapType;
      DataReaderQosMapType datareaderqos_map;

      typedef OPENDDS_MAP(OPENDDS_STRING, DDS::DataWriterQos) DataWriterQosMapType;
      DataWriterQosMapType datawriterqos_map;

      typedef OPENDDS_MAP(OPENDDS_STRING, DDS::SubscriberQos) SubscriberQosMapType;
      SubscriberQosMapType subscriberqos_map;

      typedef OPENDDS_MAP(OPENDDS_STRING, DDS::PublisherQos) PublisherQosMapType;
      PublisherQosMapType publisherqos_map;

      typedef OPENDDS_SET_CMP(RepoId, DCPS::GUID_tKeyLessThan) RepoIdSetType;
      struct Reader {
        OPENDDS_STRING topic_name;
        DDS::DataReaderQos qos;
        DDS::SubscriberQos subscriber_qos;
        DCPS::TransportLocatorSeq trans_info;
        RepoIdSetType best_effort_writers;
        RepoIdSetType reliable_writers;
        Reader(const OPENDDS_STRING& tn,
               const DDS::DataReaderQos& q,
               const DDS::SubscriberQos& sq,
               const DCPS::TransportLocatorSeq& ti)
        : topic_name(tn)
        , qos(q)
        , subscriber_qos(sq)
        , trans_info(ti)
        { }
      };
      typedef OPENDDS_MAP_CMP(RepoId, Reader, DCPS::GUID_tKeyLessThan) ReaderMapType;
      ReaderMapType reader_map;

      struct Writer {
        OPENDDS_STRING topic_name;
        DDS::DataWriterQos qos;
        DDS::PublisherQos publisher_qos;
        DCPS::TransportLocatorSeq trans_info;
        RepoIdSetType best_effort_readers;
        RepoIdSetType reliable_readers;
        Writer(const OPENDDS_STRING& tn,
               const DDS::DataWriterQos& q,
               const DDS::PublisherQos& pq,
               const DCPS::TransportLocatorSeq& ti)
        : topic_name(tn)
        , qos(q)
        , publisher_qos(pq)
        , trans_info(ti)
        { }
      };
      typedef OPENDDS_MAP_CMP(RepoId, Writer, DCPS::GUID_tKeyLessThan) WriterMapType;
      WriterMapType writer_map;

      void match();

      static EntityId_t
      build_id(const unsigned char* entity_key /* length of 3 */,
               const unsigned char entity_kind);

      static RepoId
      build_id(DDS::DomainId_t domain,
               const unsigned char* participant_id /* length of 6 */,
               const EntityId_t& entity_id);
    };

    struct StaticDiscoveredParticipantData { };
    class StaticParticipant;

    class StaticEndpointManager
      : public EndpointManager<StaticDiscoveredParticipantData>
      , public DiscoveryListener {
    public:
      StaticEndpointManager(const RepoId& participant_id,
                            ACE_Thread_Mutex& lock,
                            const EndpointRegistry& registry,
                            StaticParticipant& participant);

      void init_bit();

      virtual void assign_publication_key(DCPS::RepoId& rid,
                                          const RepoId& topicId,
                                          const DDS::DataWriterQos& qos);
      virtual void assign_subscription_key(DCPS::RepoId& rid,
                                           const RepoId& topicId,
                                           const DDS::DataReaderQos& qos);

      virtual bool update_topic_qos(const DCPS::RepoId& /*topicId*/,
                                    const DDS::TopicQos& /*qos*/,
                                    OPENDDS_STRING& /*name*/);

      virtual bool update_publication_qos(const DCPS::RepoId& /*publicationId*/,
                                          const DDS::DataWriterQos& /*qos*/,
                                          const DDS::PublisherQos& /*publisherQos*/);

      virtual bool update_subscription_qos(const DCPS::RepoId& /*subscriptionId*/,
                                           const DDS::DataReaderQos& /*qos*/,
                                           const DDS::SubscriberQos& /*subscriberQos*/);

      virtual bool update_subscription_params(const DCPS::RepoId& /*subId*/,
                                              const DDS::StringSeq& /*params*/);

      virtual void association_complete(const DCPS::RepoId& /*localId*/,
                                        const DCPS::RepoId& /*remoteId*/);

      virtual bool disassociate(const StaticDiscoveredParticipantData& /*pdata*/);

      virtual DDS::ReturnCode_t add_publication_i(const DCPS::RepoId& /*rid*/,
                                                  LocalPublication& /*pub*/);

      virtual DDS::ReturnCode_t remove_publication_i(const RepoId& /*publicationId*/);

      virtual DDS::ReturnCode_t add_subscription_i(const DCPS::RepoId& /*rid*/,
                                                   LocalSubscription& /*pub*/);

      virtual DDS::ReturnCode_t remove_subscription_i(const RepoId& /*subscriptionId*/);

      virtual bool shutting_down() const;

      virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& /*tls*/,
                                                       DiscoveredSubscriptionIter& /*iter*/,
                                                       const RepoId& /*reader*/);

      virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& /*tls*/,
                                                       DiscoveredPublicationIter& /*iter*/,
                                                       const RepoId& /*reader*/);

      virtual bool defer_writer(const RepoId& /*writer*/,
                                const RepoId& /*writer_participant*/);

      virtual bool defer_reader(const RepoId& /*writer*/,
                                const RepoId& /*writer_participant*/);

      virtual void reader_exists(const RepoId& readerid, const RepoId& writerid);
      virtual void reader_does_not_exist(const RepoId& readerid, const RepoId& writerid);
      virtual void writer_exists(const RepoId& writerid, const RepoId& readerid);
      virtual void writer_does_not_exist(const RepoId& writerid, const RepoId& readerid);

      DDS::PublicationBuiltinTopicDataDataReaderImpl* pub_bit();
      DDS::SubscriptionBuiltinTopicDataDataReaderImpl* sub_bit();

    private:
      const EndpointRegistry& registry_;
      StaticParticipant& participant_;
    };

    class StaticParticipant : public LocalParticipant<StaticEndpointManager> {
    public:
      StaticParticipant(RepoId& guid,
                        const DDS::DomainParticipantQos& qos,
                        const EndpointRegistry& registry)
        : LocalParticipant<StaticEndpointManager>(qos)
        , endpoint_manager_(guid, lock_, registry, *this)
      { }

      void init_bit(const DDS::Subscriber_var& bit_subscriber)
      {
        bit_subscriber_ = bit_subscriber;
        endpoint_manager_.init_bit();
      }

      void fini_bit() {
        bit_subscriber_ = 0;
      }

    private:
      virtual StaticEndpointManager& endpoint_manager() { return endpoint_manager_; }

      StaticEndpointManager endpoint_manager_;
    };

    class OpenDDS_Dcps_Export StaticDiscovery
      : public OpenDDS::DCPS::PeerDiscovery<StaticParticipant> {
    public:
      StaticDiscovery(const RepoKey& key);

      int load_configuration(ACE_Configuration_Heap& config);

      virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant(
                                                                    DDS::DomainId_t domain,
                                                                    const DDS::DomainParticipantQos& qos);

      EndpointRegistry registry;

      static StaticDiscovery_rch instance() { return instance_; }

    private:
      int parse_topics(ACE_Configuration_Heap& cf);
      int parse_datawriterqos(ACE_Configuration_Heap& cf);
      int parse_datareaderqos(ACE_Configuration_Heap& cf);
      int parse_publisherqos(ACE_Configuration_Heap& cf);
      int parse_subscriberqos(ACE_Configuration_Heap& cf);
      int parse_endpoints(ACE_Configuration_Heap& cf);

      static StaticDiscovery_rch instance_;
    };
  }
}

#endif /* DDS_HAS_MINIMUM_BIT */

#endif /* OPENDDS_STATICDISCOVERY_STATICDISCOVERY_H */
