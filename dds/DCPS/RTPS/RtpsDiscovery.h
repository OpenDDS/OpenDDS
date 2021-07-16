/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_RTPSDISCOVERY_H
#define OPENDDS_DCPS_RTPS_RTPSDISCOVERY_H


#include "GuidGenerator.h"
#include "Spdp.h"
#include "rtps_export.h"

#include <dds/DCPS/DiscoveryBase.h>
#include <dds/DCPS/PoolAllocator.h>

#include <ace/Configuration.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

const char RTPS_DISCOVERY_ENDPOINT_ANNOUNCEMENTS[] = "OpenDDS.RtpsDiscovery.EndpointAnnouncements";
const char RTPS_DISCOVERY_TYPE_LOOKUP_SERVICE[] = "OpenDDS.RtpsDiscovery.TypeLookupService";
const char RTPS_RELAY_APPLICATION_PARTICIPANT[] = "OpenDDS.Rtps.RelayApplicationParticipant";
const char RTPS_REFLECT_HEARTBEAT_COUNT[] = "OpenDDS.Rtps.ReflectHeartbeatCount";

class OpenDDS_Rtps_Export RtpsDiscoveryConfig : public OpenDDS::DCPS::RcObject {
public:
  typedef OPENDDS_VECTOR(OPENDDS_STRING) AddrVec;

  RtpsDiscoveryConfig();

  DCPS::TimeDuration resend_period() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return resend_period_;
  }
  void resend_period(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    resend_period_ = period;
  }

  double quick_resend_ratio() const
  {
    ACE_Guard<ACE_Thread_Mutex> g(lock_);
    return quick_resend_ratio_;
  }
  void quick_resend_ratio(double ratio)
  {
    ACE_Guard<ACE_Thread_Mutex> g(lock_);
    quick_resend_ratio_ = ratio;
  }
  DCPS::TimeDuration min_resend_delay() const
  {
    ACE_Guard<ACE_Thread_Mutex> g(lock_);
    return min_resend_delay_;
  }
  void min_resend_delay(const DCPS::TimeDuration& delay)
  {
    ACE_Guard<ACE_Thread_Mutex> g(lock_);
    min_resend_delay_ = delay;
  }

  DCPS::TimeDuration lease_duration() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return lease_duration_;
  }
  void lease_duration(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    lease_duration_ = period;
  }

  u_short pb() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, u_short());
    return pb_;
  }
  void pb(u_short port_base)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    pb_ = port_base;
  }

  u_short dg() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, u_short());
    return dg_;
  }
  void dg(u_short domain_gain)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    dg_ = domain_gain;
  }

  u_short pg() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, u_short());
    return pg_;
  }
  void pg(u_short participant_gain)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    pg_ = participant_gain;
  }

  u_short d0() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, u_short());
    return d0_;
  }
  void d0(u_short offset_zero)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    d0_ = offset_zero;
  }

  u_short d1() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, u_short());
    return d1_;
  }
  void d1(u_short offset_one)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    d1_ = offset_one;
  }

  u_short dx() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, u_short());
    return dx_;
  }
  void dx(u_short offset_two)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    dx_ = offset_two;
  }

  unsigned char ttl() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
    return ttl_;
  }
  void ttl(unsigned char time_to_live)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    ttl_ = time_to_live;
  }

  ACE_INET_Addr sedp_local_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return sedp_local_address_;
  }
  void sedp_local_address(const ACE_INET_Addr& mi)
  {
    if (mi.get_type() == AF_INET) {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      sedp_local_address_ = mi;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::sedp_local_address set failed because address family is not AF_INET\n")));
    }
  }

  ACE_INET_Addr sedp_advertised_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return sedp_advertised_address_;
  }
  void sedp_advertised_address(const ACE_INET_Addr& mi)
  {
    if (mi.get_type() == AF_INET) {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      sedp_advertised_address_ = mi;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::sedp_local_address set failed because address family is not AF_INET\n")));
    }
  }

  ACE_INET_Addr spdp_local_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return spdp_local_address_;
  }
  void spdp_local_address(const ACE_INET_Addr& mi)
  {
    if (mi.get_type() == AF_INET) {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      spdp_local_address_ = mi;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::spdp_local_address set failed because address family is not AF_INET\n")));
    }
  }

  bool sedp_multicast() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return sedp_multicast_;
  }
  void sedp_multicast(bool sm)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_multicast_ = sm;
  }

  OPENDDS_STRING multicast_interface() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, "");
    return multicast_interface_;
  }
  void multicast_interface(const OPENDDS_STRING& mi)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    multicast_interface_ = mi;
  }

  ACE_INET_Addr default_multicast_group() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return default_multicast_group_;
  }
  void default_multicast_group(const ACE_INET_Addr& group)
  {
    if (group.get_type() == AF_INET) {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      default_multicast_group_ = group;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::default_multicast_group set failed because address family is not AF_INET\n")));
    }
  }

#ifdef ACE_HAS_IPV6
  ACE_INET_Addr ipv6_spdp_local_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return ipv6_spdp_local_address_;
  }
  void ipv6_spdp_local_address(const ACE_INET_Addr& mi)
  {
    if (mi.get_type() == AF_INET6) {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      ipv6_spdp_local_address_ = mi;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::ipv6_spdp_local_address set failed because address family is not AF_INET6\n")));
    }
  }

  ACE_INET_Addr ipv6_sedp_local_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return ipv6_sedp_local_address_;
  }
  void ipv6_sedp_local_address(const ACE_INET_Addr& mi)
  {
    if (mi.get_type() == AF_INET6) {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      ipv6_sedp_local_address_ = mi;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::ipv6_sedp_local_address set failed because address family is not AF_INET6\n")));
    }
  }

  ACE_INET_Addr ipv6_sedp_advertised_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return ipv6_sedp_advertised_address_;
  }
  void ipv6_sedp_advertised_address(const ACE_INET_Addr& mi)
  {
    if (mi.get_type() == AF_INET6) {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      ipv6_sedp_advertised_address_ = mi;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::ipv6_sedp_advertised_address set failed because address family is not AF_INET6\n")));
    }
  }

  ACE_INET_Addr ipv6_default_multicast_group() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return ipv6_default_multicast_group_;
  }
  void ipv6_default_multicast_group(const ACE_INET_Addr& group)
  {
    if (group.get_type() == AF_INET6) {
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      ipv6_default_multicast_group_ = group;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::ipv6_default_multicast_group set failed because address family is not AF_INET6\n")));
    }
  }
#endif

  AddrVec spdp_send_addrs() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, AddrVec());
    return spdp_send_addrs_;
  }
  void spdp_send_addrs(const AddrVec& addrs)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    spdp_send_addrs_ = addrs;
  }

  OPENDDS_STRING guid_interface() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, "");
    return guid_interface_;
  }
  void guid_interface(const OPENDDS_STRING& gi)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    guid_interface_ = gi;
  }

  DCPS::TimeDuration max_auth_time() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return max_auth_time_;
  }
  void max_auth_time(const DCPS::TimeDuration& x)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    max_auth_time_ = x;
  }

  DCPS::TimeDuration auth_resend_period() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return auth_resend_period_;
  }
  void auth_resend_period(const DCPS::TimeDuration& x)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    auth_resend_period_ = x;
  }

  u_short max_spdp_sequence_msg_reset_check() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, u_short());
    return max_spdp_sequence_msg_reset_check_;
  }
  void max_spdp_sequence_msg_reset_check(u_short reset_value)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    max_spdp_sequence_msg_reset_check_ = reset_value;
  }

  ACE_INET_Addr spdp_rtps_relay_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return spdp_rtps_relay_address_;
  }
  void spdp_rtps_relay_address(const ACE_INET_Addr& address)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    spdp_rtps_relay_address_ = address;
  }

  DCPS::TimeDuration spdp_rtps_relay_send_period() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return spdp_rtps_relay_send_period_;
  }
  void spdp_rtps_relay_send_period(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    spdp_rtps_relay_send_period_ = period;
  }

  ACE_INET_Addr sedp_rtps_relay_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return sedp_rtps_relay_address_;
  }
  void sedp_rtps_relay_address(const ACE_INET_Addr& address)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_rtps_relay_address_ = address;
  }

  bool use_rtps_relay() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return use_rtps_relay_;
  }
  void use_rtps_relay(bool f)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    use_rtps_relay_ = f;
  }

  bool rtps_relay_only() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return rtps_relay_only_;
  }
  void rtps_relay_only(bool f)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    rtps_relay_only_ = f;
  }

  ACE_INET_Addr spdp_stun_server_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return spdp_stun_server_address_;
  }
  void spdp_stun_server_address(const ACE_INET_Addr& address)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    spdp_stun_server_address_ = address;
  }

  ACE_INET_Addr sedp_stun_server_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());
    return sedp_stun_server_address_;
  }
  void sedp_stun_server_address(const ACE_INET_Addr& address)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_stun_server_address_ = address;
  }

  bool use_ice() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return use_ice_;
  }
  void use_ice(bool ui)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    use_ice_ = ui;
  }

  bool use_ncm() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return use_ncm_;
  }
  void use_ncm(bool ui)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    use_ncm_ = ui;
  }

  size_t sedp_max_message_size() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, size_t());
    return sedp_max_message_size_;
  }
  void sedp_max_message_size(size_t value)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_max_message_size_ = value;
  }

  bool undirected_spdp() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return undirected_spdp_;
  }
  void undirected_spdp(bool value)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    undirected_spdp_ = value;
  }

  bool periodic_directed_spdp() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return periodic_directed_spdp_;
  }
  void periodic_directed_spdp(bool value)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    periodic_directed_spdp_ = value;
  }

  bool secure_participant_user_data() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return secure_participant_user_data_;
  }
  void secure_participant_user_data(bool value)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    secure_participant_user_data_ = value;
  }

  DCPS::TimeDuration max_type_lookup_service_reply_period() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return max_type_lookup_service_reply_period_;
  }
  void max_type_lookup_service_reply_period(const DCPS::TimeDuration& x)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    max_type_lookup_service_reply_period_ = x;
  }

  bool use_xtypes() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return use_xtypes_;
  }
  void use_xtypes(bool use_xtypes)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    use_xtypes_ = use_xtypes;
  }

  DCPS::TimeDuration sedp_heartbeat_period() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return sedp_heartbeat_period_;
  }
  void sedp_heartbeat_period(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_heartbeat_period_ = period;
  }

  DCPS::TimeDuration sedp_nak_response_delay() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return sedp_nak_response_delay_;
  }
  void sedp_nak_response_delay(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_nak_response_delay_ = period;
  }

  DCPS::TimeDuration sedp_send_delay() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return sedp_send_delay_;
  }
  void sedp_send_delay(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_send_delay_ = period;
  }

  DCPS::TimeDuration sedp_passive_connect_duration() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return sedp_passive_connect_duration_;
  }
  void sedp_passive_connect_duration(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_passive_connect_duration_ = period;
  }

  CORBA::ULong participant_flags() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
    return participant_flags_;
  }
  void participant_flags(CORBA::ULong participant_flags)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    participant_flags_ = participant_flags;
  }

  bool sedp_responsive_mode() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
    return sedp_responsive_mode_;
  }
  void sedp_responsive_mode(bool sedp_responsive_mode)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_responsive_mode_ = sedp_responsive_mode;
  }

private:
  mutable ACE_Thread_Mutex lock_;
  DCPS::TimeDuration resend_period_;
  double quick_resend_ratio_;
  DCPS::TimeDuration min_resend_delay_;
  DCPS::TimeDuration lease_duration_;
  u_short pb_, dg_, pg_, d0_, d1_, dx_;
  unsigned char ttl_;
  bool sedp_multicast_;
  OPENDDS_STRING multicast_interface_;
  ACE_INET_Addr sedp_local_address_, sedp_advertised_address_, spdp_local_address_;
  ACE_INET_Addr default_multicast_group_;  /// FUTURE: handle > 1 group.
#ifdef ACE_HAS_IPV6
  ACE_INET_Addr ipv6_sedp_local_address_, ipv6_sedp_advertised_address_, ipv6_spdp_local_address_;
  ACE_INET_Addr ipv6_default_multicast_group_;
#endif
  OPENDDS_STRING guid_interface_;
  AddrVec spdp_send_addrs_;
  DCPS::TimeDuration max_auth_time_;
  DCPS::TimeDuration auth_resend_period_;
  u_short max_spdp_sequence_msg_reset_check_;
  ACE_INET_Addr spdp_rtps_relay_address_;
  DCPS::TimeDuration spdp_rtps_relay_send_period_;
  ACE_INET_Addr sedp_rtps_relay_address_;
  bool use_rtps_relay_;
  bool rtps_relay_only_;
  ACE_INET_Addr spdp_stun_server_address_;
  ACE_INET_Addr sedp_stun_server_address_;
  bool use_ice_;
  bool use_ncm_;
  size_t sedp_max_message_size_;
  bool undirected_spdp_;
  bool periodic_directed_spdp_;
  /// Should participant user data QoS only be sent when the message is secure?
  bool secure_participant_user_data_;
  DCPS::TimeDuration max_type_lookup_service_reply_period_;
  bool use_xtypes_;
  DCPS::TimeDuration sedp_heartbeat_period_;
  DCPS::TimeDuration sedp_nak_response_delay_;
  DCPS::TimeDuration sedp_send_delay_;
  DCPS::TimeDuration sedp_passive_connect_duration_;
  CORBA::ULong participant_flags_;
  bool sedp_responsive_mode_;
};

typedef OpenDDS::DCPS::RcHandle<RtpsDiscoveryConfig> RtpsDiscoveryConfig_rch;

/**
 * @class RtpsDiscovery
 *
 * @brief Discovery Strategy class that implements RTPS discovery
 *
 * This class implements the Discovery interface for Rtps-based
 * discovery.
 *
 */
class OpenDDS_Rtps_Export RtpsDiscovery : public OpenDDS::DCPS::PeerDiscovery<Spdp> {
public:
  typedef RtpsDiscoveryConfig::AddrVec AddrVec;

  explicit RtpsDiscovery(const RepoKey& key);
  ~RtpsDiscovery();

  virtual OpenDDS::DCPS::RepoId generate_participant_guid();

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos);

#if defined(OPENDDS_SECURITY)
#  if defined __GNUC__ && ((__GNUC__ == 5 && __GNUC_MINOR__ < 3) || __GNUC__ < 5) && ! defined __clang__
#    define OPENDDS_GCC_PRE53_DISABLE_OPTIMIZATION __attribute__((optimize("-O0")))
#  else
#    define OPENDDS_GCC_PRE53_DISABLE_OPTIMIZATION
#  endif

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    const OpenDDS::DCPS::RepoId& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto)
    OPENDDS_GCC_PRE53_DISABLE_OPTIMIZATION;
#endif

  virtual bool supports_liveliness() const { return true; }

  virtual void signal_liveliness(const DDS::DomainId_t domain_id,
                                 const OpenDDS::DCPS::RepoId& part_id,
                                 DDS::LivelinessQosPolicyKind kind);

  // configuration parameters:

  DCPS::TimeDuration resend_period() const { return config_->resend_period(); }
  void resend_period(const DCPS::TimeDuration& period) { config_->resend_period(period); }

  DCPS::TimeDuration lease_duration() const { return config_->lease_duration(); }
  void lease_duration(const DCPS::TimeDuration& period) { config_->lease_duration(period); }

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

  ACE_INET_Addr sedp_local_address() const { return config_->sedp_local_address(); }
  void sedp_local_address(const ACE_INET_Addr& mi) { config_->sedp_local_address(mi); }

  ACE_INET_Addr spdp_local_address() const { return config_->spdp_local_address(); }
  void spdp_local_address(const ACE_INET_Addr& mi) { config_->spdp_local_address(mi); }

  bool sedp_multicast() const { return config_->sedp_multicast(); }
  void sedp_multicast(bool sm) { config_->sedp_multicast(sm); }

  OPENDDS_STRING multicast_interface() const { return config_->multicast_interface(); }
  void multicast_interface(const OPENDDS_STRING& mi) { config_->multicast_interface(mi); }

  ACE_INET_Addr default_multicast_group() const { return config_->default_multicast_group(); }
  void default_multicast_group(const ACE_INET_Addr& group) { config_->default_multicast_group(group); }

  AddrVec spdp_send_addrs() const { return config_->spdp_send_addrs(); }
  void spdp_send_addrs(const AddrVec& addrs) { return config_->spdp_send_addrs(addrs); }

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

  bool use_ice() const { return config_->use_ice(); }
  void use_ice_now(bool f);

  bool secure_participant_user_data() const
  {
    return config_->secure_participant_user_data();
  }
  void secure_participant_user_data(bool value)
  {
    config_->secure_participant_user_data(value);
  }

  bool use_xtypes() const { return config_->use_xtypes(); }
  void use_xtypes(bool xt) { config_->use_xtypes(xt); }

  RtpsDiscoveryConfig_rch config() const { return config_; }

#ifdef OPENDDS_SECURITY
  DDS::Security::ParticipantCryptoHandle get_crypto_handle(DDS::DomainId_t domain,
                                                           const DCPS::RepoId& local_participant,
                                                           const DCPS::RepoId& remote_participant = GUID_UNKNOWN) const;
#endif

  u_short get_spdp_port(DDS::DomainId_t domain,
                        const DCPS::RepoId& local_participant) const;
  u_short get_sedp_port(DDS::DomainId_t domain,
                        const DCPS::RepoId& local_participant) const;
#ifdef ACE_HAS_IPV6
  u_short get_ipv6_spdp_port(DDS::DomainId_t domain,
                             const DCPS::RepoId& local_participant) const;
  u_short get_ipv6_sedp_port(DDS::DomainId_t domain,
                             const DCPS::RepoId& local_participant) const;
#endif
  void spdp_rtps_relay_address(const ACE_INET_Addr& address);
  void sedp_rtps_relay_address(const ACE_INET_Addr& address);
  void spdp_stun_server_address(const ACE_INET_Addr& address);
  void sedp_stun_server_address(const ACE_INET_Addr& address);

private:
  RtpsDiscoveryConfig_rch config_;

  /// Guids will be unique within this RTPS configuration
  GuidGenerator guid_gen_;

public:
  class Config : public Discovery::Config {
  public:
    int discovery_config(ACE_Configuration_Heap& cf);
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
