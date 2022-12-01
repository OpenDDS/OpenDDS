/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsDiscoveryConfig.h"

namespace {
  u_short get_default_d0(u_short fallback)
  {
#if !defined ACE_LACKS_GETENV && !defined ACE_LACKS_ENV
    const char* from_env = std::getenv("OPENDDS_RTPS_DEFAULT_D0");
    if (from_env) {
      return static_cast<u_short>(std::atoi(from_env));
    }
#endif
    return fallback;
  }
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::TimeDuration;

RtpsDiscoveryConfig::RtpsDiscoveryConfig()
  : resend_period_(30 /*seconds*/) // see RTPS v2.1 9.6.1.4.2
  , quick_resend_ratio_(0.1)
  , min_resend_delay_(TimeDuration::from_msec(100))
  , lease_duration_(300)
  , max_lease_duration_(300)
#ifdef OPENDDS_SECURITY
  , security_unsecure_lease_duration_(30)
  , max_participants_in_authentication_(0)
#endif
  , lease_extension_(0)
  , pb_(7400) // see RTPS v2.1 9.6.1.3 for PB, DG, PG, D0, D1 defaults
  , dg_(250)
  , pg_(2)
  , d0_(get_default_d0(0))
  , d1_(10)
  , dx_(2)
  , ttl_(1)
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  , send_buffer_size_(ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  , recv_buffer_size_(ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
#else
  , send_buffer_size_(0)
  , recv_buffer_size_(0)
#endif
  , sedp_multicast_(true)
  , sedp_local_address_(u_short(0), "0.0.0.0")
  , spdp_local_address_(u_short(0), "0.0.0.0")
  , default_multicast_group_(u_short(0), "239.255.0.1") /*RTPS v2.1 9.6.1.4.1*/
#ifdef ACE_HAS_IPV6
  , ipv6_sedp_local_address_(u_short(0), "::")
  , ipv6_spdp_local_address_(u_short(0), "::")
  , ipv6_default_multicast_group_(u_short(0), "FF03::1")
#endif
  , spdp_request_random_port_(false)
  , max_auth_time_(300, 0)
  , auth_resend_period_(1, 0)
  , max_spdp_sequence_msg_reset_check_(3)
  , spdp_rtps_relay_send_period_(30, 0)
  , use_rtps_relay_(false)
  , rtps_relay_only_(false)
  , use_ice_(false)
  , sedp_max_message_size_(DCPS::TransportSendStrategy::UDP_MAX_MESSAGE_SIZE)
  , undirected_spdp_(true)
  , periodic_directed_spdp_(false)
  , secure_participant_user_data_(false)
  , max_type_lookup_service_reply_period_(5, 0)
  , use_xtypes_(XTYPES_MINIMAL)
  , sedp_heartbeat_period_(0, 200*1000 /*microseconds*/)
  , sedp_nak_response_delay_(0, 100*1000 /*microseconds*/)
  , sedp_send_delay_(0, 10 * 1000)
  , sedp_passive_connect_duration_(TimeDuration::from_msec(DCPS::TransportConfig::DEFAULT_PASSIVE_CONNECT_DURATION))
  , participant_flags_(PFLAGS_THIS_VERSION)
  , sedp_responsive_mode_(false)
  , sedp_receive_preallocated_message_blocks_(0)
  , sedp_receive_preallocated_data_blocks_(0)
  , check_source_ip_(true)
{}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
