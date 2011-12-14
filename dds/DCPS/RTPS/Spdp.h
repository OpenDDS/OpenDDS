/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SPDP_H
#define OPENDDS_RTPS_SPDP_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfoC.h"

#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Definitions.h"

#include "RtpsMessageTypesC.h"

#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Condition_Thread_Mutex.h"

#include <map>
#include <set>
#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace DDS {
  class ParticipantBuiltinTopicDataDataReaderImpl;
  class TopicBuiltinTopicDataDataReaderImpl;
  class PublicationBuiltinTopicDataDataReaderImpl;
  class SubscriptionBuiltinTopicDataDataReaderImpl;
}

namespace OpenDDS {
namespace RTPS {

class RtpsDiscovery;

class SepdBuiltinPublicationsWriter;
class SepdBuiltinPublicationsReader;
class SepdBuiltinSubscriptionsWriter;
class SepdBuiltinSubscriptionsReader;

/// Each instance of class Spdp represents the implementation of the RTPS
/// Simple Participant Discovery Protocol for a single local DomainParticipant.
class Spdp : public DCPS::RcObject<ACE_SYNCH_MUTEX> {
public:
  Spdp(DDS::DomainId_t domain, const DCPS::RepoId& guid,
       const DDS::DomainParticipantQos& qos, RtpsDiscovery* disco);
  ~Spdp();

  // Participant
  void ignore_domain_participant(const DCPS::RepoId& ignoreId);
  bool update_domain_participant_qos(const DDS::DomainParticipantQos& qos);
  void bit_subscriber(const DDS::Subscriber_var& bit_subscriber);

  // Topic
  DCPS::TopicStatus assert_topic(DCPS::RepoId_out topicId,
                                 const char* topicName,
                                 const char* dataTypeName,
                                 const DDS::TopicQos& qos);
  DCPS::TopicStatus remove_topic(const DCPS::RepoId& topicId,
                                 std::string& name);
  void ignore_topic(const DCPS::RepoId& ignoreId);
  bool update_topic_qos(const DCPS::RepoId& topicId, const DDS::TopicQos& qos,
                        std::string& name);
  struct TopicDetails {
    std::string data_type_;
    DDS::TopicQos qos_;
    DCPS::RepoId repo_id_;
  };

  // Publication
  DCPS::RepoId add_publication(const DCPS::RepoId& topicId,
                               DCPS::DataWriterRemote_ptr publication,
                               const DDS::DataWriterQos& qos,
                               const DCPS::TransportLocatorSeq& transInfo,
                               const DDS::PublisherQos& publisherQos);
  void remove_publication(const DCPS::RepoId& publicationId);
  void ignore_publication(const DCPS::RepoId& ignoreId);
  bool update_publication_qos(const DCPS::RepoId& publicationId,
                              const DDS::DataWriterQos& qos,
                              const DDS::PublisherQos& publisherQos);

  struct PublisherDetails {
    DCPS::RepoId topic_id_;
    DCPS::DataWriterRemote_ptr publication_;
    DDS::DataWriterQos qos_;
    DCPS::TransportLocatorSeq trans_info_;
    DDS::PublisherQos publisher_qos_;
    DCPS::RepoId repo_id_;
  };

  // Subscription
  DCPS::RepoId add_subscription(const DCPS::RepoId& topicId,
                                DCPS::DataReaderRemote_ptr subscription,
                                const DDS::DataReaderQos& qos,
                                const DCPS::TransportLocatorSeq& transInfo,
                                const DDS::SubscriberQos& subscriberQos,
                                const char* filterExpr,
                                const DDS::StringSeq& params);
  void remove_subscription(const DCPS::RepoId& subscriptionId);
  void ignore_subscription(const DCPS::RepoId& ignoreId);
  bool update_subscription_qos(const DCPS::RepoId& subscriptionId,
                               const DDS::DataReaderQos& qos,
                               const DDS::SubscriberQos& subscriberQos);
  bool update_subscription_params(const DCPS::RepoId& subId,
                                  const DDS::StringSeq& params);

  struct SubscriberDetails {
    DCPS::RepoId topic_id_;
    DCPS::DataReaderRemote_ptr subscription_;
    DDS::DataReaderQos qos_;
    DCPS::TransportLocatorSeq trans_info_;
    DDS::SubscriberQos subscriber_qos_;
    DDS::StringSeq params_;
    DCPS::RepoId repo_id_;
  };

  // Managing reader/writer associations
  void association_complete(const DCPS::RepoId& localId,
                            const DCPS::RepoId& remoteId);
  void disassociate_participant(const DCPS::RepoId& remoteId);
  void disassociate_publication(const DCPS::RepoId& localId,
                                const DCPS::RepoId& remoteId);
  void disassociate_subscription(const DCPS::RepoId& localId,
                                 const DCPS::RepoId& remoteId);

private:
  ACE_Reactor* reactor() const;

  ACE_Thread_Mutex lock_;
  RtpsDiscovery* disco_;

  // Participant:
  const DDS::DomainId_t domain_;
  const DCPS::RepoId guid_;
  DDS::DomainParticipantQos qos_;
  DDS::Subscriber_var bit_subscriber_;
  LocatorSeq sedp_unicast_, sedp_multicast_;

  void data_received(const Header& header, const DataSubmessage& data,
                     const ParameterList& plist);

  DDS::ParticipantBuiltinTopicDataDataReaderImpl* part_bit();
  DDS::TopicBuiltinTopicDataDataReaderImpl* topic_bit();
  DDS::PublicationBuiltinTopicDataDataReaderImpl* pub_bit();
  DDS::SubscriptionBuiltinTopicDataDataReaderImpl* sub_bit();

  struct SpdpTransport : ACE_Event_Handler {
    explicit SpdpTransport(Spdp* outer);
    ~SpdpTransport();

    int handle_timeout(const ACE_Time_Value&, const void*);
    int handle_input(ACE_HANDLE h);

    void open();
    void write();
    void close();
    void dispose_unregister();

    Spdp* outer_;
    Header hdr_;
    DataSubmessage data_;
    DCPS::SequenceNumber seq_;
    ACE_Time_Value lease_duration_;
    ACE_SOCK_Dgram unicast_socket_;
    ACE_SOCK_Dgram_Mcast multicast_socket_;
    std::set<ACE_INET_Addr> send_addrs_;
    ACE_Message_Block buff_;

  } *tport_;

  SepdBuiltinPublicationsWriter* sepd_pub_bit_writer();
  SepdBuiltinPublicationsReader* sepd_pub_bit_reader();

  SepdBuiltinSubscriptionsWriter* sepd_sub_bit_writer();
  SepdBuiltinSubscriptionsReader* sepd_sub_bit_reader();

  ACE_Event_Handler_var eh_; // manages our refcount on tport_
  bool eh_shutdown_;
  ACE_Condition_Thread_Mutex shutdown_cond_;

  struct ParticipantDetails {
    ParticipantDetails() {}
    ParticipantDetails(const SPDPdiscoveredParticipantData& p,
                       const ACE_Time_Value& t)
      : pdata_(p), last_seen_(t) {}

    SPDPdiscoveredParticipantData pdata_;
    ACE_Time_Value last_seen_;
    DDS::InstanceHandle_t bit_ih_;
  };
  typedef std::map<DCPS::RepoId, ParticipantDetails,
                   DCPS::GUID_tKeyLessThan> ParticipantMap;
  typedef ParticipantMap::iterator ParticipantIter;
  ParticipantMap participants_;
  void remove_discovered_participant(ParticipantIter iter);
  void remove_expired_participants();

  // Endpoints:

  std::map<DCPS::RepoId, PublisherDetails, DCPS::GUID_tKeyLessThan> publishers_;
  std::map<DCPS::RepoId, SubscriberDetails, DCPS::GUID_tKeyLessThan> subscribers_;
  unsigned int endpoint_counter_;

  // Topic:
  std::map<std::string, TopicDetails> topics_;
  std::map<DCPS::RepoId, std::string, DCPS::GUID_tKeyLessThan> topic_names_;
  unsigned int topic_counter_;
};

}
}

#endif // OPENDDS_RTPS_SPDP_H
