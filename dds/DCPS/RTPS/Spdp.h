/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SPDP_H
#define OPENDDS_RTPS_SPDP_H
#ifndef DDS_HAS_MINIMUM_BIT

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Definitions.h"

#include "RtpsMessageTypesC.h"
#include "Sedp.h"

#include "ace/Atomic_Op.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Condition_Thread_Mutex.h"

#include <map>
#include <set>
#include "dds/DCPS/PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace DDS {
  class ParticipantBuiltinTopicDataDataReaderImpl;
}

namespace OpenDDS {
namespace RTPS {

class RtpsDiscovery;

/// Each instance of class Spdp represents the implementation of the RTPS
/// Simple Participant Discovery Protocol for a single local DomainParticipant.
class Spdp : public DCPS::RcObject<ACE_SYNCH_MUTEX> {
public:
  Spdp(DDS::DomainId_t domain, DCPS::RepoId& guid,
       const DDS::DomainParticipantQos& qos, RtpsDiscovery* disco);
  ~Spdp();

  // Participant
  void ignore_domain_participant(const DCPS::RepoId& ignoreId);
  bool update_domain_participant_qos(const DDS::DomainParticipantQos& qos);
  void init_bit(const DDS::Subscriber_var& bit_subscriber);
  void fini_bit();
  DDS::Subscriber_var bit_subscriber() const { return bit_subscriber_; }
  bool get_default_locators(const DCPS::RepoId& part_id,
                            LocatorSeq& target,
                            bool& inlineQos);

  // Topic
  DCPS::TopicStatus assert_topic(DCPS::RepoId_out topicId,
                                 const char* topicName,
                                 const char* dataTypeName,
                                 const DDS::TopicQos& qos,
                                 bool hasDcpsKey);
  DCPS::TopicStatus remove_topic(const DCPS::RepoId& topicId,
                                 OPENDDS_STRING& name);
  void ignore_topic(const DCPS::RepoId& ignoreId);
  bool update_topic_qos(const DCPS::RepoId& topicId, const DDS::TopicQos& qos,
                        OPENDDS_STRING& name);

  // Publication
  DCPS::RepoId add_publication(const DCPS::RepoId& topicId,
                               DCPS::DataWriterCallbacks* publication,
                               const DDS::DataWriterQos& qos,
                               const DCPS::TransportLocatorSeq& transInfo,
                               const DDS::PublisherQos& publisherQos);
  void remove_publication(const DCPS::RepoId& publicationId);
  void ignore_publication(const DCPS::RepoId& ignoreId);
  bool update_publication_qos(const DCPS::RepoId& publicationId,
                              const DDS::DataWriterQos& qos,
                              const DDS::PublisherQos& publisherQos);

  // Subscription
  DCPS::RepoId add_subscription(const DCPS::RepoId& topicId,
                                DCPS::DataReaderCallbacks* subscription,
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

  // Managing reader/writer associations
  void association_complete(const DCPS::RepoId& localId,
                            const DCPS::RepoId& remoteId);

  void signal_liveliness(DDS::LivelinessQosPolicyKind kind);

  DCPS::RepoId bit_key_to_repo_id(const char* bit_topic_name,
                                  const DDS::BuiltinTopicKey_t& key);

  // Is Spdp shutting down?
  bool shutting_down() { return shutdown_flag_.value(); }

  bool associated() const;
  bool has_discovered_participant(const DCPS::RepoId& guid);

  WaitForAcks& wait_for_acks();

#ifndef ACE_LYNXOS_MAJOR
private:
#endif

  ACE_Reactor* reactor() const;

  ACE_Thread_Mutex lock_;
  RtpsDiscovery* disco_;

  // Participant:
  const DDS::DomainId_t domain_;
  DCPS::RepoId guid_;
  DDS::DomainParticipantQos qos_;
  DDS::Subscriber_var bit_subscriber_;
  LocatorSeq sedp_unicast_, sedp_multicast_;

  void data_received(const DataSubmessage& data, const ParameterList& plist);

  DDS::ParticipantBuiltinTopicDataDataReaderImpl* part_bit();

  struct SpdpTransport : ACE_Event_Handler {
    explicit SpdpTransport(Spdp* outer);
    ~SpdpTransport();

    virtual int handle_timeout(const ACE_Time_Value&, const void*);
    virtual int handle_input(ACE_HANDLE h);
    virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE);

    void open();
    void write();
    void write_i();
    void close();
    void dispose_unregister();
    bool open_unicast_socket(u_short port_common, u_short participant_id);
    void acknowledge();

    Spdp* outer_;
    Header hdr_;
    DataSubmessage data_;
    DCPS::SequenceNumber seq_;
    ACE_Time_Value lease_duration_;
    ACE_SOCK_Dgram unicast_socket_;
    ACE_SOCK_Dgram_Mcast multicast_socket_;
    std::set<ACE_INET_Addr> send_addrs_;
    ACE_Message_Block buff_, wbuff_;

  } *tport_;

  ACE_Event_Handler_var eh_; // manages our refcount on tport_
  bool eh_shutdown_;
  ACE_Condition_Thread_Mutex shutdown_cond_;
  ACE_Atomic_Op<ACE_Thread_Mutex, long> shutdown_flag_; // Spdp shutting down

  struct DiscoveredParticipant {
    DiscoveredParticipant() : bit_ih_(0) {}
    DiscoveredParticipant(const SPDPdiscoveredParticipantData& p,
                          const ACE_Time_Value& t)
      : pdata_(p), last_seen_(t), bit_ih_(DDS::HANDLE_NIL) {}

    SPDPdiscoveredParticipantData pdata_;
    ACE_Time_Value last_seen_;
    DDS::InstanceHandle_t bit_ih_;
  };
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, DiscoveredParticipant,
                   DCPS::GUID_tKeyLessThan) DiscoveredParticipantMap;
  typedef DiscoveredParticipantMap::iterator DiscoveredParticipantIter;
  DiscoveredParticipantMap participants_;

  void remove_discovered_participant(DiscoveredParticipantIter iter);
  void remove_expired_participants();
  void get_discovered_participant_ids(RepoIdSet& results) const;

  Sedp sedp_;
  // wait for acknowledgments from SpdpTransport and Sedp::Task
  // when BIT is being removed (fini_bit)
  WaitForAcks wait_for_acks_;
};

}
}

#endif // DDS_HAS_MINIMUM_BIT
#endif // OPENDDS_RTPS_SPDP_H
