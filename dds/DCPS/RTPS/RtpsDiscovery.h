/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_RTPSDISCOVERY_H
#define OPENDDS_DCPS_RTPS_RTPSDISCOVERY_H

#include "GuidGenerator.h"
#include "Spdp.h"
#include "rtps_export.h"

#include <dds/DCPS/PoolAllocator.h>

#include <ace/Configuration.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

typedef RcHandle<Spdp> ParticipantHandle;
typedef OPENDDS_MAP_CMP(GUID_t, ParticipantHandle, GUID_tKeyLessThan) ParticipantMap;
typedef OPENDDS_MAP(DDS::DomainId_t, ParticipantMap) DomainParticipantMap;

const char RTPS_DISCOVERY_ENDPOINT_ANNOUNCEMENTS[] = "OpenDDS.RtpsDiscovery.EndpointAnnouncements";
const char RTPS_DISCOVERY_TYPE_LOOKUP_SERVICE[] = "OpenDDS.RtpsDiscovery.TypeLookupService";
const char RTPS_RELAY_APPLICATION_PARTICIPANT[] = "OpenDDS.Rtps.RelayApplicationParticipant";
const char RTPS_REFLECT_HEARTBEAT_COUNT[] = "OpenDDS.Rtps.ReflectHeartbeatCount";

class OpenDDS_Rtps_Export RtpsDiscoveryConfig : public OpenDDS::DCPS::RcObject {
public:
  typedef OPENDDS_VECTOR(OPENDDS_STRING) AddrVec;

  enum UseXTypes {
    XTYPES_NONE = 0, ///< Turn off support for XTypes
    XTYPES_MINIMAL, ///< Only use minimal TypeObjects
    XTYPES_COMPLETE ///< Use both minimal and complete TypeObjects
  };

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

  DCPS::TimeDuration max_lease_duration() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return max_lease_duration_;
  }
  void max_lease_duration(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    max_lease_duration_ = period;
  }

#ifdef OPENDDS_SECURITY
  DCPS::TimeDuration security_unsecure_lease_duration() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return security_unsecure_lease_duration_;
  }
  void security_unsecure_lease_duration(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    security_unsecure_lease_duration_ = period;
  }

  size_t max_participants_in_authentication() const
  {
    ACE_Guard<ACE_Thread_Mutex> g(lock_);
    return max_participants_in_authentication_;
  }
  void max_participants_in_authentication(size_t m)
  {
    ACE_Guard<ACE_Thread_Mutex> g(lock_);
    max_participants_in_authentication_ = m;
  }
#endif

  DCPS::TimeDuration lease_extension() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return lease_extension_;
  }
  void lease_extension(const DCPS::TimeDuration& period)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    lease_extension_ = period;
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

  ACE_INT32 send_buffer_size() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
    return send_buffer_size_;
  }
  void send_buffer_size(ACE_INT32 buffer_size)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    send_buffer_size_ = buffer_size;
  }

  ACE_INT32 recv_buffer_size() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
    return recv_buffer_size_;
  }
  void recv_buffer_size(ACE_INT32 buffer_size)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    recv_buffer_size_ = buffer_size;
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
    return sedp_multicast_.value();
  }
  void sedp_multicast(bool sm)
  {
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

  u_short port_common(DDS::DomainId_t domain) const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
    // Ports are set by the formulas in RTPS v2.1 Table 9.8
    return  pb_ + (dg_ * domain);
  }

  ACE_INET_Addr multicast_address(u_short port_common) const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());

    ACE_INET_Addr addr = default_multicast_group_;
    // Ports are set by the formulas in RTPS v2.1 Table 9.8
    addr.set_port_number(port_common + d0_);
    return addr;
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

  ACE_INET_Addr ipv6_multicast_address(u_short port_common) const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ACE_INET_Addr());

    ACE_INET_Addr addr = ipv6_default_multicast_group_;
    // Ports are set by the formulas in RTPS v2.1 Table 9.8
    addr.set_port_number(port_common + d0_);
    return addr;
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
    return use_rtps_relay_.value();
  }
  void use_rtps_relay(bool f)
  {
    use_rtps_relay_ = f;
  }

  bool rtps_relay_only() const
  {
    return rtps_relay_only_.value();
  }
  void rtps_relay_only(bool f)
  {
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
    return use_ice_.value();
  }
  void use_ice(bool ui)
  {
    use_ice_ = ui;
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
    return undirected_spdp_.value();
  }
  void undirected_spdp(bool value)
  {
    undirected_spdp_ = value;
  }

  bool periodic_directed_spdp() const
  {
    return periodic_directed_spdp_.value();
  }
  void periodic_directed_spdp(bool value)
  {
    periodic_directed_spdp_ = value;
  }

  bool secure_participant_user_data() const
  {
    return secure_participant_user_data_.value();
  }
  void secure_participant_user_data(bool value)
  {
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
    return use_xtypes_ != XTYPES_NONE;
  }
  void use_xtypes(UseXTypes use_xtypes)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    use_xtypes_ = use_xtypes;
  }
  void use_xtypes(const char* str)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    struct NameValue {
      const char* name;
      UseXTypes value;
    };
    static const NameValue entries[] = {
      {"no", XTYPES_NONE},
      {"minimal", XTYPES_MINIMAL},
      {"complete", XTYPES_COMPLETE}
    };

    for (size_t i = 0; i < sizeof entries / sizeof entries[0]; ++i) {
      if (0 == std::strcmp(entries[i].name, str)) {
        use_xtypes(entries[i].value);
        return;
      }
    }
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscoveryConfig::use_xtypes -")
               ACE_TEXT(" invalid XTypes configuration: %C\n"), str));
  }
  bool use_xtypes_complete() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, bool());
    return use_xtypes_ == XTYPES_COMPLETE;
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

  DCPS::TimeDuration sedp_fragment_reassembly_timeout() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::TimeDuration());
    return sedp_fragment_reassembly_timeout_;
  }
  void sedp_fragment_reassembly_timeout(const DCPS::TimeDuration& timeout)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_fragment_reassembly_timeout_ = timeout;
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
    return sedp_responsive_mode_.value();
  }
  void sedp_responsive_mode(bool sedp_responsive_mode)
  {
    sedp_responsive_mode_ = sedp_responsive_mode;
  }

  size_t sedp_receive_preallocated_message_blocks() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
    return sedp_receive_preallocated_message_blocks_;
  }
  void sedp_receive_preallocated_message_blocks(size_t n)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_receive_preallocated_message_blocks_ = n;
  }

  size_t sedp_receive_preallocated_data_blocks() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
    return sedp_receive_preallocated_data_blocks_;
  }
  void sedp_receive_preallocated_data_blocks(size_t n)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_receive_preallocated_data_blocks_ = n;
  }

  bool check_source_ip() const
  {
    return check_source_ip_;
  }
  void check_source_ip(bool flag)
  {
    check_source_ip_ = flag;
  }

private:
  mutable ACE_Thread_Mutex lock_;
  DCPS::TimeDuration resend_period_;
  double quick_resend_ratio_;
  DCPS::TimeDuration min_resend_delay_;
  DCPS::TimeDuration lease_duration_;
  DCPS::TimeDuration max_lease_duration_;
#ifdef OPENDDS_SECURITY
  DCPS::TimeDuration security_unsecure_lease_duration_;
  size_t max_participants_in_authentication_;
#endif
  DCPS::TimeDuration lease_extension_;
  u_short pb_, dg_, pg_, d0_, d1_, dx_;
  unsigned char ttl_;
  ACE_INT32 send_buffer_size_;
  ACE_INT32 recv_buffer_size_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> sedp_multicast_;
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
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> use_rtps_relay_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> rtps_relay_only_;
  ACE_INET_Addr spdp_stun_server_address_;
  ACE_INET_Addr sedp_stun_server_address_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> use_ice_;
  size_t sedp_max_message_size_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> undirected_spdp_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> periodic_directed_spdp_;
  /// Should participant user data QoS only be sent when the message is secure?
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> secure_participant_user_data_;
  DCPS::TimeDuration max_type_lookup_service_reply_period_;
  UseXTypes use_xtypes_;
  DCPS::TimeDuration sedp_heartbeat_period_;
  DCPS::TimeDuration sedp_nak_response_delay_;
  DCPS::TimeDuration sedp_send_delay_;
  DCPS::TimeDuration sedp_passive_connect_duration_;
  DCPS::TimeDuration sedp_fragment_reassembly_timeout_;
  CORBA::ULong participant_flags_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> sedp_responsive_mode_;
  size_t sedp_receive_preallocated_message_blocks_, sedp_receive_preallocated_data_blocks_;
  bool check_source_ip_;
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
class OpenDDS_Rtps_Export RtpsDiscovery : public DCPS::Discovery {
public:
  typedef RtpsDiscoveryConfig::AddrVec AddrVec;

  explicit RtpsDiscovery(const RepoKey& key);
  ~RtpsDiscovery();

  virtual OpenDDS::DCPS::RepoId generate_participant_guid();

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls);

#if defined(OPENDDS_SECURITY)
#  if defined __GNUC__ && ((__GNUC__ == 5 && __GNUC_MINOR__ < 3) || __GNUC__ < 5) && ! defined __clang__
#    define OPENDDS_GCC_PRE53_DISABLE_OPTIMIZATION __attribute__((optimize("-O0")))
#  else
#    define OPENDDS_GCC_PRE53_DISABLE_OPTIMIZATION
#  endif

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls,
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
  void use_xtypes(RtpsDiscoveryConfig::UseXTypes val) { return config_->use_xtypes(val); }
  bool use_xtypes_complete() const { return config_->use_xtypes_complete(); }

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

  void append_transport_statistics(DDS::DomainId_t domain,
                                   const DCPS::RepoId& local_participant,
                                   DCPS::TransportStatisticsSequence& seq);

  DDS::Subscriber_ptr init_bit(DCPS::DomainParticipantImpl* participant);

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

  GUID_t add_publication(
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

  GUID_t add_subscription(
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

private:

  // This mutex only protects the participants map
  mutable ACE_Thread_Mutex participants_lock_;

  DomainParticipantMap participants_;

  ParticipantHandle get_part(const DDS::DomainId_t domain_id, const GUID_t& part_id) const;

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
