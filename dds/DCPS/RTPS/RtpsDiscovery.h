/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_RTPSDISCOVERY_H
#define OPENDDS_DCPS_RTPS_RTPSDISCOVERY_H

#include "RtpsDiscoveryConfig.h"
#include "GuidGenerator.h"
#include "Spdp.h"
#include "rtps_export.h"

#include <dds/DCPS/AtomicBool.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/debug.h>

#include <dds/OpenDDSConfigWrapper.h>

#include <ace/Configuration.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::log_level;
using DCPS::LogLevel;
using DCPS::AtomicBool;

typedef RcHandle<Spdp> ParticipantHandle;
typedef OPENDDS_MAP_CMP(GUID_t, ParticipantHandle, GUID_tKeyLessThan) ParticipantMap;
typedef OPENDDS_MAP(DDS::DomainId_t, ParticipantMap) DomainParticipantMap;
typedef OpenDDS::DCPS::RcHandle<RtpsDiscoveryConfig> RtpsDiscoveryConfig_rch;

const char RTPS_DISCOVERY_ENDPOINT_ANNOUNCEMENTS[] = "OpenDDS.RtpsDiscovery.EndpointAnnouncements";
const char RTPS_DISCOVERY_TYPE_LOOKUP_SERVICE[] = "OpenDDS.RtpsDiscovery.TypeLookupService";
const char RTPS_RELAY_APPLICATION_PARTICIPANT[] = "OpenDDS.Rtps.RelayApplicationParticipant";
const char RTPS_REFLECT_HEARTBEAT_COUNT[] = "OpenDDS.Rtps.ReflectHeartbeatCount";


/**
 * @class RtpsDiscovery
 *
 * @brief Discovery Strategy class that implements RTPS discovery
 *
 * This class implements the Discovery interface for Rtps-based
 * discovery.
 *
 */
class OpenDDS_Rtps_Export RtpsDiscovery : public DCPS::Discovery {
public:
  explicit RtpsDiscovery(const RepoKey& key);
  ~RtpsDiscovery();

  virtual RepoKey key() const { return key_; }

  virtual OpenDDS::DCPS::GUID_t generate_participant_guid();

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls);

#if OPENDDS_CONFIG_SECURITY
#  if defined __GNUC__ && ((__GNUC__ == 5 && __GNUC_MINOR__ < 3) || __GNUC__ < 5) && ! defined __clang__
#    define OPENDDS_GCC_PRE53_DISABLE_OPTIMIZATION __attribute__((optimize("-O0")))
#  else
#    define OPENDDS_GCC_PRE53_DISABLE_OPTIMIZATION
#  endif

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls,
    const OpenDDS::DCPS::GUID_t& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto)
    OPENDDS_GCC_PRE53_DISABLE_OPTIMIZATION;
#endif

  virtual bool supports_liveliness() const { return true; }

  virtual void signal_liveliness(const DDS::DomainId_t domain_id,
                                 const OpenDDS::DCPS::GUID_t& part_id,
                                 DDS::LivelinessQosPolicyKind kind);

  // configuration parameters:

  DCPS::TimeDuration resend_period() const { return config_->resend_period(); }
  void resend_period(const DCPS::TimeDuration& period) { config_->resend_period(period); }

  DCPS::TimeDuration lease_duration() const { return config_->lease_duration(); }
  void lease_duration(const DCPS::TimeDuration& period) { config_->lease_duration(period); }

  DCPS::TimeDuration lease_extension() const { return config_->lease_extension(); }
  void lease_extension(const DCPS::TimeDuration& period) { config_->lease_extension(period); }

  u_short pb() const { return config_->pb(); }
  void pb(u_short port_base) { config_->pb(port_base); }

  u_short dg() const { return config_->dg(); }
  void dg(u_short domain_gain) { config_->dg(domain_gain); }

  u_short pg() const { return config_->pg(); }
  void pg(u_short participant_gain) { config_->pg(participant_gain); }

  u_short d0() const { return config_->d0(); }
  void d0(u_short offset_zero) { config_->d0(offset_zero); }

  u_short d1() const { return config_->d1(); }
  void d1(u_short offset_one) { config_->d1(offset_one); }

  u_short dx() const { return config_->dx(); }
  void dx(u_short offset_two) { config_->dx(offset_two); }

  unsigned char ttl() const { return config_->ttl(); }
  void ttl(unsigned char time_to_live) { config_->ttl(time_to_live); }

  DCPS::NetworkAddress sedp_local_address() const { return config_->sedp_local_address(); }
  void sedp_local_address(const DCPS::NetworkAddress& mi) { config_->sedp_local_address(mi); }

  DCPS::NetworkAddress spdp_local_address() const { return config_->spdp_local_address(); }
  void spdp_local_address(const DCPS::NetworkAddress& mi) { config_->spdp_local_address(mi); }

  bool sedp_multicast() const { return config_->sedp_multicast(); }
  void sedp_multicast(bool sm) { config_->sedp_multicast(sm); }

  OPENDDS_STRING multicast_interface() const { return config_->multicast_interface(); }
  void multicast_interface(const OPENDDS_STRING& mi) { config_->multicast_interface(mi); }

  DCPS::NetworkAddress default_multicast_group(DDS::DomainId_t domain) const { return config_->default_multicast_group(domain); }
  void default_multicast_group(const DCPS::NetworkAddress& group) { config_->default_multicast_group(group); }

  DCPS::NetworkAddressSet spdp_send_addrs() const { return config_->spdp_send_addrs(); }
  void spdp_send_addrs(const DCPS::NetworkAddressSet& addrs) { return config_->spdp_send_addrs(addrs); }

  OPENDDS_STRING guid_interface() const { return config_->guid_interface(); }
  void guid_interface(const OPENDDS_STRING& gi) { config_->guid_interface(gi); }

  DCPS::TimeDuration max_auth_time() const { return config_->max_auth_time(); }
  void max_auth_time(const DCPS::TimeDuration& x) { config_->max_auth_time(x); }

  DCPS::TimeDuration auth_resend_period() const { return config_->auth_resend_period(); }
  void auth_resend_period(const DCPS::TimeDuration& x) { config_->auth_resend_period(x); }

  u_short max_spdp_sequence_msg_reset_check() const { return config_->max_spdp_sequence_msg_reset_check(); }
  void max_spdp_sequence_msg_reset_check(u_short reset_value) { config_->max_spdp_sequence_msg_reset_check(reset_value); }

  bool rtps_relay_only() const { return config_->rtps_relay_only(); }
  void rtps_relay_only_now(bool f);

  bool use_rtps_relay() const { return config_->use_rtps_relay(); }
  void use_rtps_relay_now(bool f);

#if OPENDDS_CONFIG_SECURITY
  bool use_ice() const { return config_->use_ice(); }
  void use_ice_now(bool f);
#endif

  bool secure_participant_user_data() const
  {
    return config_->secure_participant_user_data();
  }
  void secure_participant_user_data(bool value)
  {
    config_->secure_participant_user_data(value);
  }

  bool use_xtypes() const { return config_->use_xtypes(); }
  void use_xtypes(RtpsDiscoveryConfig::UseXTypes val) { return config_->use_xtypes(val); }
  bool use_xtypes_complete() const { return config_->use_xtypes_complete(); }

  RtpsDiscoveryConfig_rch config() const { return config_; }

#if OPENDDS_CONFIG_SECURITY
  DDS::Security::ParticipantCryptoHandle get_crypto_handle(DDS::DomainId_t domain,
                                                           const DCPS::GUID_t& local_participant,
                                                           const DCPS::GUID_t& remote_participant = GUID_UNKNOWN) const;
#endif

  u_short get_spdp_port(DDS::DomainId_t domain,
                        const DCPS::GUID_t& local_participant) const;
  u_short get_sedp_port(DDS::DomainId_t domain,
                        const DCPS::GUID_t& local_participant) const;
#ifdef ACE_HAS_IPV6
  u_short get_ipv6_spdp_port(DDS::DomainId_t domain,
                             const DCPS::GUID_t& local_participant) const;
  u_short get_ipv6_sedp_port(DDS::DomainId_t domain,
                             const DCPS::GUID_t& local_participant) const;
#endif
  void spdp_rtps_relay_address(const DCPS::NetworkAddress& address);
  void sedp_rtps_relay_address(const DCPS::NetworkAddress& address);
  void spdp_stun_server_address(const DCPS::NetworkAddress& address);
  void sedp_stun_server_address(const DCPS::NetworkAddress& address);

  void append_transport_statistics(DDS::DomainId_t domain,
                                   const DCPS::GUID_t& local_participant,
                                   DCPS::TransportStatisticsSequence& seq);

  RcHandle<DCPS::BitSubscriber> init_bit(DCPS::DomainParticipantImpl* participant);

  void fini_bit(DCPS::DomainParticipantImpl* participant);

  bool attach_participant(DDS::DomainId_t domainId, const GUID_t& participantId);

  bool remove_domain_participant(DDS::DomainId_t domain_id, const GUID_t& participantId);

  bool ignore_domain_participant(DDS::DomainId_t domain, const GUID_t& myParticipantId,
    const GUID_t& ignoreId);

  bool remove_domain_participant(DDS::DomainId_t domain, const GUID_t& myParticipantId,
    const GUID_t& removeId);

  bool update_domain_participant_qos(DDS::DomainId_t domain, const GUID_t& participant,
    const DDS::DomainParticipantQos& qos);

  bool has_domain_participant(DDS::DomainId_t domain, const GUID_t& local, const GUID_t& remote) const;

  DCPS::TopicStatus assert_topic(
    GUID_t& topicId,
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey,
    DCPS::TopicCallbacks* topic_callbacks);

  DCPS::TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    GUID_t& topicId);

  DCPS::TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId);

  bool ignore_topic(DDS::DomainId_t domainId,
    const GUID_t& myParticipantId, const GUID_t& ignoreId);

  bool update_topic_qos(const GUID_t& topicId, DDS::DomainId_t domainId,
    const GUID_t& participantId, const DDS::TopicQos& qos);

  bool add_publication(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId,
    DCPS::DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const XTypes::TypeInformation& type_info);

  bool remove_publication(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& publicationId);

  bool ignore_publication(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& ignoreId);

  bool update_publication_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos);

  void update_publication_locators(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const DCPS::TransportLocatorSeq& transInfo);

  bool add_subscription(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId,
    DCPS::DataReaderCallbacks_rch subscription,
    const DDS::DataReaderQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpr,
    const DDS::StringSeq& params,
    const XTypes::TypeInformation& type_info);

  bool remove_subscription(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& subscriptionId);

  bool ignore_subscription(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& ignoreId);

  bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subQos);

  bool update_subscription_params(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& subId,
    const DDS::StringSeq& params);

  void update_subscription_locators(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& subId,
    const DCPS::TransportLocatorSeq& transInfo);

  RcHandle<DCPS::TransportInst> sedp_transport_inst(DDS::DomainId_t domainId,
                                                    const GUID_t& partId) const;

  void request_remote_complete_type_objects(
    DDS::DomainId_t domain, const GUID_t& local_participant,
    const GUID_t& remote_entity, const XTypes::TypeInformation& remote_type_info,
    DCPS::TypeObjReqCond& cond);

private:

  // This mutex only protects the participants map
  mutable ACE_Thread_Mutex participants_lock_;

  DomainParticipantMap participants_;

  ParticipantHandle get_part(const DDS::DomainId_t domain_id, const GUID_t& part_id) const;

  const RepoKey key_;

  // This mutex protects everything else
  mutable ACE_Thread_Mutex lock_;

  RtpsDiscoveryConfig_rch config_;

  RtpsDiscoveryConfig_rch get_config() const;

  /// Guids will be unique within this RTPS configuration
  GuidGenerator guid_gen_;

  void create_bit_dr(DDS::TopicDescription_ptr topic, const char* type,
                     DCPS::SubscriberImpl* sub,
                     const DDS::DataReaderQos& qos);

public:
  class Config : public Discovery::Config {
  public:
    int discovery_config();
  };

  class OpenDDS_Rtps_Export StaticInitializer {
  public:
    StaticInitializer();
  };

private:
  friend class ::DDS_TEST;
};

static RtpsDiscovery::StaticInitializer initialize_rtps;

typedef OpenDDS::DCPS::RcHandle<RtpsDiscovery> RtpsDiscovery_rch;

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_RTPSDISCOVERY_H  */
