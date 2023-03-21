/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_RTPSDISCOVERYCONFIG_H
#define OPENDDS_DCPS_RTPS_RTPSDISCOVERYCONFIG_H

#include "Spdp.h"

#include <dds/DCPS/debug.h>
#include <dds/DCPS/AtomicBool.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::log_level;
using DCPS::LogLevel;
using DCPS::AtomicBool;

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
    return sedp_multicast_;
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

  bool spdp_request_random_port() const
  {
    return spdp_request_random_port_;
  }
  void spdp_request_random_port(bool f)
  {
    spdp_request_random_port_ = f;
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
    return use_rtps_relay_;
  }
  void use_rtps_relay(bool f)
  {
    use_rtps_relay_ = f;
  }

  bool rtps_relay_only() const
  {
    return rtps_relay_only_;
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
    return use_ice_;
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
    return undirected_spdp_;
  }
  void undirected_spdp(bool value)
  {
    undirected_spdp_ = value;
  }

  bool periodic_directed_spdp() const
  {
    return periodic_directed_spdp_;
  }
  void periodic_directed_spdp(bool value)
  {
    periodic_directed_spdp_ = value;
  }

  bool secure_participant_user_data() const
  {
    return secure_participant_user_data_;
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
  bool use_xtypes(const char* str)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
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
        use_xtypes_ = entries[i].value;
        return true;
      }
    }
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: RtpsDiscoveryConfig::use_xtypes: "
                 "invalid XTypes configuration: %C\n", str));
    }
    return false;
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
    return sedp_responsive_mode_;
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
  AtomicBool sedp_multicast_;
  OPENDDS_STRING multicast_interface_;
  ACE_INET_Addr sedp_local_address_, sedp_advertised_address_, spdp_local_address_;
  ACE_INET_Addr default_multicast_group_;  /// FUTURE: handle > 1 group.
#ifdef ACE_HAS_IPV6
  ACE_INET_Addr ipv6_sedp_local_address_, ipv6_sedp_advertised_address_, ipv6_spdp_local_address_;
  ACE_INET_Addr ipv6_default_multicast_group_;
#endif
  AtomicBool spdp_request_random_port_;
  OPENDDS_STRING guid_interface_;
  AddrVec spdp_send_addrs_;
  DCPS::TimeDuration max_auth_time_;
  DCPS::TimeDuration auth_resend_period_;
  u_short max_spdp_sequence_msg_reset_check_;
  ACE_INET_Addr spdp_rtps_relay_address_;
  DCPS::TimeDuration spdp_rtps_relay_send_period_;
  ACE_INET_Addr sedp_rtps_relay_address_;
  AtomicBool use_rtps_relay_;
  AtomicBool rtps_relay_only_;
  ACE_INET_Addr spdp_stun_server_address_;
  ACE_INET_Addr sedp_stun_server_address_;
  AtomicBool use_ice_;
  size_t sedp_max_message_size_;
  AtomicBool undirected_spdp_;
  AtomicBool periodic_directed_spdp_;
  /// Should participant user data QoS only be sent when the message is secure?
  AtomicBool secure_participant_user_data_;
  DCPS::TimeDuration max_type_lookup_service_reply_period_;
  UseXTypes use_xtypes_;
  DCPS::TimeDuration sedp_heartbeat_period_;
  DCPS::TimeDuration sedp_nak_response_delay_;
  DCPS::TimeDuration sedp_send_delay_;
  DCPS::TimeDuration sedp_passive_connect_duration_;
  DCPS::TimeDuration sedp_fragment_reassembly_timeout_;
  CORBA::ULong participant_flags_;
  AtomicBool sedp_responsive_mode_;
  size_t sedp_receive_preallocated_message_blocks_, sedp_receive_preallocated_data_blocks_;
  bool check_source_ip_;
};

typedef DCPS::RcHandle<RtpsDiscoveryConfig> RtpsDiscoveryConfig_rch;

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_RTPS_RTPSDISCOVERYCONFIG_H  */
