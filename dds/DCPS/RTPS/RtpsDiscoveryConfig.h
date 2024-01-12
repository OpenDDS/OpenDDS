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
  enum UseXTypes {
    XTYPES_NONE = 0, ///< Turn off support for XTypes
    XTYPES_MINIMAL, ///< Only use minimal TypeObjects
    XTYPES_COMPLETE ///< Use both minimal and complete TypeObjects
  };

  RtpsDiscoveryConfig(const String& name);

  const String& config_prefix() const { return config_prefix_; }
  String config_key(const String& key) const;

  DCPS::TimeDuration resend_period() const;
  void resend_period(const DCPS::TimeDuration& period);

  double quick_resend_ratio() const;
  void quick_resend_ratio(double ratio);

  DCPS::TimeDuration min_resend_delay() const;
  void min_resend_delay(const DCPS::TimeDuration& delay);

  DCPS::TimeDuration lease_duration() const;
  void lease_duration(const DCPS::TimeDuration& period);

  DCPS::TimeDuration max_lease_duration() const;
  void max_lease_duration(const DCPS::TimeDuration& period);

#ifdef OPENDDS_SECURITY
  DCPS::TimeDuration security_unsecure_lease_duration() const;
  void security_unsecure_lease_duration(const DCPS::TimeDuration& period);

  size_t max_participants_in_authentication() const;
  void max_participants_in_authentication(size_t m);
#endif

  DCPS::TimeDuration lease_extension() const;
  void lease_extension(const DCPS::TimeDuration& period);

  u_short pb() const;
  void pb(u_short port_base);

  u_short dg() const;
  void dg(u_short domain_gain);

  u_short pg() const;
  void pg(u_short participant_gain);

  u_short d0() const;
  void d0(u_short offset_zero);

  u_short d1() const;
  void d1(u_short offset_one);

  u_short dx() const;
  void dx(u_short offset_two);

  unsigned char ttl() const;
  void ttl(unsigned char time_to_live);

  ACE_INT32 send_buffer_size() const;
  void send_buffer_size(ACE_INT32 buffer_size);

  ACE_INT32 recv_buffer_size() const;
  void recv_buffer_size(ACE_INT32 buffer_size);

  DCPS::NetworkAddress sedp_local_address() const;
  void sedp_local_address(const DCPS::NetworkAddress& mi);

  DCPS::NetworkAddress sedp_advertised_local_address() const;
  void sedp_advertised_local_address(const DCPS::NetworkAddress& mi);

  DCPS::NetworkAddress spdp_local_address() const;
  void spdp_local_address(const DCPS::NetworkAddress& mi);

  bool sedp_multicast() const;
  void sedp_multicast(bool sm);

  OPENDDS_STRING multicast_interface() const;
  void multicast_interface(const OPENDDS_STRING& mi);

  DCPS::NetworkAddress default_multicast_group(DDS::DomainId_t domain) const;
  void default_multicast_group(const DCPS::NetworkAddress& group);

  u_short port_common(DDS::DomainId_t domain) const;

  DCPS::NetworkAddress multicast_address(u_short port_common,
                                         DDS::DomainId_t domain) const;

#ifdef ACE_HAS_IPV6
  DCPS::NetworkAddress ipv6_spdp_local_address() const;
  void ipv6_spdp_local_address(const DCPS::NetworkAddress& mi);

  DCPS::NetworkAddress ipv6_sedp_local_address() const;
  void ipv6_sedp_local_address(const DCPS::NetworkAddress& mi);

  DCPS::NetworkAddress ipv6_sedp_advertised_local_address() const;
  void ipv6_sedp_advertised_local_address(const DCPS::NetworkAddress& mi);

  DCPS::NetworkAddress ipv6_default_multicast_group() const;
  void ipv6_default_multicast_group(const DCPS::NetworkAddress& group);

  DCPS::NetworkAddress ipv6_multicast_address(u_short port_common) const;
#endif

  bool spdp_request_random_port() const;
  void spdp_request_random_port(bool f);

  OPENDDS_STRING guid_interface() const;
  void guid_interface(const OPENDDS_STRING& gi);

  DCPS::NetworkAddressSet spdp_send_addrs() const;
  void spdp_send_addrs(const DCPS::NetworkAddressSet& addrs);

  DCPS::TimeDuration max_auth_time() const;
  void max_auth_time(const DCPS::TimeDuration& x);

  DCPS::TimeDuration auth_resend_period() const;
  void auth_resend_period(const DCPS::TimeDuration& x);

  u_short max_spdp_sequence_msg_reset_check() const;
  void max_spdp_sequence_msg_reset_check(u_short reset_value);

  DCPS::NetworkAddress spdp_rtps_relay_address() const;
  void spdp_rtps_relay_address(const DCPS::NetworkAddress& address);

  DCPS::TimeDuration spdp_rtps_relay_send_period() const;
  void spdp_rtps_relay_send_period(const DCPS::TimeDuration& period);

  DCPS::NetworkAddress sedp_rtps_relay_address() const;
  void sedp_rtps_relay_address(const DCPS::NetworkAddress& address);

  bool use_rtps_relay() const;
  void use_rtps_relay(bool f);

  bool rtps_relay_only() const;
  void rtps_relay_only(bool f);

  DCPS::NetworkAddress spdp_stun_server_address() const;
  void spdp_stun_server_address(const DCPS::NetworkAddress& address);

  DCPS::NetworkAddress sedp_stun_server_address() const;
  void sedp_stun_server_address(const DCPS::NetworkAddress& address);

#ifdef OPENDDS_SECURITY
  bool use_ice() const;
  void use_ice(bool ui);
#endif

  size_t sedp_max_message_size() const;
  void sedp_max_message_size(size_t value);

  bool undirected_spdp() const;
  void undirected_spdp(bool value);

  bool periodic_directed_spdp() const;
  void periodic_directed_spdp(bool value);

  bool secure_participant_user_data() const;
  void secure_participant_user_data(bool value);

  DCPS::TimeDuration max_type_lookup_service_reply_period() const;
  void max_type_lookup_service_reply_period(const DCPS::TimeDuration& x);

  bool use_xtypes() const;
  void use_xtypes(UseXTypes use_xtypes);
  bool use_xtypes(const char* str);
  bool use_xtypes_complete() const;

  DCPS::TimeDuration sedp_heartbeat_period() const;
  void sedp_heartbeat_period(const DCPS::TimeDuration& period);

  DCPS::TimeDuration sedp_nak_response_delay() const;
  void sedp_nak_response_delay(const DCPS::TimeDuration& period);

  DCPS::TimeDuration sedp_send_delay() const;
  void sedp_send_delay(const DCPS::TimeDuration& period);

  DCPS::TimeDuration sedp_passive_connect_duration() const;
  void sedp_passive_connect_duration(const DCPS::TimeDuration& period);

  DCPS::TimeDuration sedp_fragment_reassembly_timeout() const;
  void sedp_fragment_reassembly_timeout(const DCPS::TimeDuration& timeout);

  CORBA::ULong participant_flags() const;
  void participant_flags(CORBA::ULong participant_flags);

  bool sedp_responsive_mode() const;
  void sedp_responsive_mode(bool sedp_responsive_mode);

  size_t sedp_receive_preallocated_message_blocks() const;
  void sedp_receive_preallocated_message_blocks(size_t n);

  size_t sedp_receive_preallocated_data_blocks() const;
  void sedp_receive_preallocated_data_blocks(size_t n);

  bool check_source_ip() const;
  void check_source_ip(bool flag);

private:
  const String config_prefix_;
};

typedef DCPS::RcHandle<RtpsDiscoveryConfig> RtpsDiscoveryConfig_rch;

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_RTPS_RTPSDISCOVERYCONFIG_H  */
