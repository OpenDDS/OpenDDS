/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_RTPSDISCOVERY_H
#define OPENDDS_RTPS_RTPSDISCOVERY_H

#ifndef DDS_HAS_MINIMUM_BIT

#include "dds/DCPS/Discovery.h"
#include "dds/DCPS/RTPS/GuidGenerator.h"
#include "dds/DCPS/RTPS/Spdp.h"
#include "dds/DCPS/Service_Participant.h"
#include "rtps_export.h"

#include "ace/Configuration.h"

#include "dds/DCPS/PoolAllocator.h"
#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

namespace OpenDDS {
namespace RTPS {

/**
 * @class RtpsDiscovery
 *
 * @brief Discovery Strategy class that implements RTPS discovery
 *
 * This class implements the Discovery interface for Rtps-based
 * discovery.
 *
 */
class OpenDDS_Rtps_Export RtpsDiscovery : public OpenDDS::DCPS::Discovery {
public:
  explicit RtpsDiscovery(const RepoKey& key);
  ~RtpsDiscovery();

  virtual DDS::Subscriber_ptr init_bit(DCPS::DomainParticipantImpl* dpi);

  virtual void fini_bit(DCPS::DomainParticipantImpl* participant);

  virtual DCPS::RepoId bit_key_to_repo_id(DCPS::DomainParticipantImpl* part,
                                          const char* bit_topic_name,
                                          const DDS::BuiltinTopicKey_t& key) const;

  virtual bool attach_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId);

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos);

  virtual bool remove_domain_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId);

  virtual bool ignore_domain_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_domain_participant_qos(
    DDS::DomainId_t domain,
    const OpenDDS::DCPS::RepoId& participantId,
    const DDS::DomainParticipantQos& qos);


  // Topic operations:

  virtual OpenDDS::DCPS::TopicStatus assert_topic(
    OpenDDS::DCPS::RepoId_out topicId,
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey);

  virtual OpenDDS::DCPS::TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    OpenDDS::DCPS::RepoId_out topicId);

  virtual OpenDDS::DCPS::TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId);

  virtual bool ignore_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_topic_qos(
    const OpenDDS::DCPS::RepoId& topicId,
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const DDS::TopicQos& qos);


  // Publication operations:

  virtual OpenDDS::DCPS::RepoId add_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataWriterCallbacks* publication,
    const DDS::DataWriterQos& qos,
    const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos);

  virtual bool remove_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& publicationId);

  virtual bool ignore_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_publication_qos(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& partId,
    const OpenDDS::DCPS::RepoId& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos);


  // Subscription operations:

  virtual OpenDDS::DCPS::RepoId add_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataReaderCallbacks* subscription,
    const DDS::DataReaderQos& qos,
    const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterExpression,
    const DDS::StringSeq& exprParams);

  virtual bool remove_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId);

  virtual bool ignore_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& partId,
    const OpenDDS::DCPS::RepoId& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos);

  virtual bool update_subscription_params(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId,
    const DDS::StringSeq& params);


  // Managing reader/writer associations:

  virtual void association_complete(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& localId,
    const OpenDDS::DCPS::RepoId& remoteId);

  virtual bool supports_liveliness() const { return true; }

  virtual void signal_liveliness(const DDS::DomainId_t domain_id,
                                 const OpenDDS::DCPS::RepoId& part_id,
                                 DDS::LivelinessQosPolicyKind kind);

  // configuration parameters:

  ACE_Time_Value resend_period() const { return resend_period_; }
  void resend_period(const ACE_Time_Value& period) {
    resend_period_ = period;
  }

  u_short pb() const { return pb_; }
  void pb(u_short port_base) {
    pb_ = port_base;
  }

  u_short dg() const { return dg_; }
  void dg(u_short domain_gain) {
    dg_ = domain_gain;
  }

  u_short pg() const { return pg_; }
  void pg(u_short participant_gain) {
    pg_ = participant_gain;
  }

  u_short d0() const { return d0_; }
  void d0(u_short offset_zero) {
    d0_ = offset_zero;
  }

  u_short d1() const { return d1_; }
  void d1(u_short offset_one) {
    d1_ = offset_one;
  }

  u_short dx() const { return dx_; }
  void dx(u_short offset_two) {
    dx_ = offset_two;
  }

  u_short ttl() const { return ttl_; }
  void ttl(u_short time_to_live) {
    ttl_ = time_to_live;
  }

  bool sedp_multicast() const { return sedp_multicast_; }
  void sedp_multicast(bool sm) {
    sedp_multicast_ = sm;
  }

  OPENDDS_STRING multicast_interface() const { return multicast_interface_; }
  void multicast_interface(const OPENDDS_STRING& mi) {
    multicast_interface_ = mi;
  }

  OPENDDS_STRING default_multicast_group() const { return default_multicast_group_; }
  void default_multicast_group(const OPENDDS_STRING& group) {
    default_multicast_group_ = group;
  }

  typedef std::vector<OPENDDS_STRING> AddrVec;
  const AddrVec& spdp_send_addrs() const { return spdp_send_addrs_; }
  AddrVec& spdp_send_addrs() { return spdp_send_addrs_; }

  ACE_Reactor* reactor();

private:
  // Retrieve the Spdp representing the participant
  DCPS::RcHandle<Spdp> get_part(const DDS::DomainId_t domain_id,
                                const DCPS::RepoId& part_id) const;

  ACE_Time_Value resend_period_;
  u_short pb_, dg_, pg_, d0_, d1_, dx_, ttl_;
  bool sedp_multicast_;
  OPENDDS_STRING multicast_interface_;
  OPENDDS_STRING default_multicast_group_;  /// FUTURE: handle > 1 group.
  AddrVec spdp_send_addrs_;

  struct ReactorRunner : ACE_Task_Base {
    ReactorRunner() : reactor_(0) {}
    ~ReactorRunner();
    int svc();
    void end();

    ACE_Reactor* reactor_;
    ACE_Thread_Mutex mtx_;
  } reactor_runner_;

  typedef OPENDDS_MAP_CMP(DCPS::RepoId, DCPS::RcHandle<Spdp>, DCPS::GUID_tKeyLessThan) ParticipantMap;
  typedef OPENDDS_MAP(DDS::DomainId_t, ParticipantMap) DomainParticipantMap;

  DomainParticipantMap participants_;

  OPENDDS_MAP(DDS::DomainId_t, OPENDDS_MAP(OPENDDS_STRING, Sedp::TopicDetails) ) topics_;
  OPENDDS_MAP(DDS::DomainId_t, OPENDDS_MAP(OPENDDS_STRING, unsigned int) ) topic_use_;

  /// Guids will be unique within this RTPS configuration
  GuidGenerator guid_gen_;
  mutable ACE_Thread_Mutex lock_;

public:
  class Config : public DCPS::Discovery::Config {
  public:
    int discovery_config(ACE_Configuration_Heap& cf);
  };

  class OpenDDS_Rtps_Export StaticInitializer {
  public:
    StaticInitializer();
  };

private:
  friend class ::DDS_TEST;

  void set_part_bit_subscriber(const DDS::DomainId_t domain_id,
                               const DCPS::RepoId& part_id,
                               const DDS::Subscriber_var& bit_subscriber);
};

static RtpsDiscovery::StaticInitializer initialize_rtps;

typedef OpenDDS::DCPS::RcHandle<RtpsDiscovery> RtpsDiscovery_rch;

} // namespace RTPS
} // namespace OpenDDS

#endif /* DDS_HAS_MINIMUM_BIT */
#endif /* OPENDDS_RTPS_RTPSDISCOVERY_H  */
