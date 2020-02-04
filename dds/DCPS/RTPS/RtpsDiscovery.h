/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_RTPSDISCOVERY_H
#define OPENDDS_RTPS_RTPSDISCOVERY_H


#include "dds/DCPS/DiscoveryBase.h"
#include "dds/DCPS/RTPS/GuidGenerator.h"
#include "dds/DCPS/RTPS/Spdp.h"
#include "rtps_export.h"

#include "ace/Configuration.h"

#include "dds/DCPS/PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

const char RTPS_DISCOVERY_ENDPOINT_ANNOUNCEMENTS[] = "OpenDDS.RtpsDiscovery.EndpointAnnouncements";

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

  OPENDDS_STRING sedp_local_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, "");
    return sedp_local_address_;
  }
  void sedp_local_address(const OPENDDS_STRING& mi)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_local_address_ = mi;
  }

  OPENDDS_STRING spdp_local_address() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, "");
    return spdp_local_address_;
  }
  void spdp_local_address(const OPENDDS_STRING& mi)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    spdp_local_address_ = mi;
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

  OPENDDS_STRING default_multicast_group() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, "");
    return default_multicast_group_;
  }
  void default_multicast_group(const OPENDDS_STRING& group)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    default_multicast_group_ = group;
  }

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

  DCPS::TimeDuration spdp_rtps_relay_beacon_period() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return spdp_rtps_relay_beacon_period_;
  }
  void spdp_rtps_relay_beacon_period(const DCPS::TimeDuration& period) {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    spdp_rtps_relay_beacon_period_ = period;
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

  DCPS::TimeDuration sedp_rtps_relay_beacon_period() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return sedp_rtps_relay_beacon_period_;
  }
  void sedp_rtps_relay_beacon_period(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_rtps_relay_beacon_period_ = period;
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

private:
  mutable ACE_Thread_Mutex lock_;
  DCPS::TimeDuration resend_period_;
  DCPS::TimeDuration lease_duration_;
  u_short pb_, dg_, pg_, d0_, d1_, dx_;
  unsigned char ttl_;
  bool sedp_multicast_;
  OPENDDS_STRING multicast_interface_, sedp_local_address_, spdp_local_address_;
  OPENDDS_STRING default_multicast_group_;  /// FUTURE: handle > 1 group.
  OPENDDS_STRING guid_interface_;
  AddrVec spdp_send_addrs_;
  DCPS::TimeDuration max_auth_time_;
  DCPS::TimeDuration auth_resend_period_;
  u_short max_spdp_sequence_msg_reset_check_;
  ACE_INET_Addr spdp_rtps_relay_address_;
  DCPS::TimeDuration spdp_rtps_relay_beacon_period_;
  DCPS::TimeDuration spdp_rtps_relay_send_period_;
  ACE_INET_Addr sedp_rtps_relay_address_;
  DCPS::TimeDuration sedp_rtps_relay_beacon_period_;
  bool use_rtps_relay_;
  bool rtps_relay_only_;
  ACE_INET_Addr sedp_stun_server_address_;
  bool use_ice_;
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
  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    const OpenDDS::DCPS::RepoId& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto);
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

  OPENDDS_STRING sedp_local_address() const { return config_->sedp_local_address(); }
  void sedp_local_address(const OPENDDS_STRING& mi) { config_->sedp_local_address(mi); }

  OPENDDS_STRING spdp_local_address() const { return config_->spdp_local_address(); }
  void spdp_local_address(const OPENDDS_STRING& mi) { config_->spdp_local_address(mi); }

  bool sedp_multicast() const { return config_->sedp_multicast(); }
  void sedp_multicast(bool sm) { config_->sedp_multicast(sm); }

  OPENDDS_STRING multicast_interface() const { return config_->multicast_interface(); }
  void multicast_interface(const OPENDDS_STRING& mi) { config_->multicast_interface(mi); }

  OPENDDS_STRING default_multicast_group() const { return config_->default_multicast_group(); }
  void default_multicast_group(const OPENDDS_STRING& group) { config_->default_multicast_group(group); }

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

  void spdp_rtps_relay_address(const ACE_INET_Addr& address);
  void sedp_rtps_relay_address(const ACE_INET_Addr& address);
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
