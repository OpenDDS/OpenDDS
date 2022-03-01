/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsDiscovery.h"

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/ConfigUtils.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/Registered_Data_Types.h>

#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>

#include <dds/DdsDcpsInfoUtilsC.h>

#include <cstdlib>
#include <limits>

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

RtpsDiscovery::RtpsDiscovery(const RepoKey& key)
  : Discovery(key)
  , config_(DCPS::make_rch<RtpsDiscoveryConfig>())
{
}

RtpsDiscovery::~RtpsDiscovery()
{
}

namespace {
  const ACE_TCHAR RTPS_SECTION_NAME[] = ACE_TEXT("rtps_discovery");
}

int
RtpsDiscovery::Config::discovery_config(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key rtps_sect;

  if (cf.open_section(root, RTPS_SECTION_NAME, 0, rtps_sect) == 0) {

    // Ensure there are no properties in this section
    DCPS::ValueMap vm;
    if (DCPS::pullValues(cf, rtps_sect, vm) > 0) {
      // There are values inside [rtps_discovery]
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                        ACE_TEXT("rtps_discovery sections must have a subsection name\n")),
                       -1);
    }
    // Process the subsections of this section (the individual rtps_discovery/*)
    DCPS::KeyList keys;
    if (DCPS::processSections(cf, rtps_sect, keys) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                        ACE_TEXT("too many nesting layers in the [rtps] section.\n")),
                       -1);
    }

    // Loop through the [rtps_discovery/*] sections
    for (DCPS::KeyList::const_iterator it = keys.begin();
         it != keys.end(); ++it) {
      const OPENDDS_STRING& rtps_name = it->first;

      RtpsDiscovery_rch discovery = OpenDDS::DCPS::make_rch<RtpsDiscovery>(rtps_name);
      RtpsDiscoveryConfig_rch config = discovery->config();

      // spdpaddr defaults to DCPSDefaultAddress if set
      if (TheServiceParticipant->default_address().to_addr() != ACE_INET_Addr()) {
        config->spdp_local_address(TheServiceParticipant->default_address().to_addr());
        config->multicast_interface(DCPS::LogAddr::ip(TheServiceParticipant->default_address().to_addr()));
      }

      DCPS::ValueMap values;
      DCPS::pullValues(cf, it->second, values);
      for (DCPS::ValueMap::const_iterator it = values.begin();
           it != values.end(); ++it) {
        const OPENDDS_STRING& name = it->first;
        if (name == "ResendPeriod") {
          const OPENDDS_STRING& value = it->second;
          int resend;
          if (!DCPS::convertToInteger(value, resend)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for ResendPeriod in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->resend_period(TimeDuration(resend));
        } else if (name == "QuickResendRatio") {
          const OPENDDS_STRING& value = it->second;
          double ratio;
          if (!DCPS::convertToDouble(value, ratio)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for QuickResendRatio in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->quick_resend_ratio(ratio);
        } else if (name == "MinResendDelay") {
          const OPENDDS_STRING& value = it->second;
          int delay;
          if (!DCPS::convertToInteger(value, delay)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for MinResendDelay in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->min_resend_delay(TimeDuration::from_msec(delay));
        } else if (name == "LeaseDuration") {
          const OPENDDS_STRING& value = it->second;
          int duration;
          if (!DCPS::convertToInteger(value, duration)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for LeaseDuration in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->lease_duration(TimeDuration(duration));
        } else if (name == "MaxLeaseDuration") {
          const OPENDDS_STRING& value = it->second;
          int duration;
          if (!DCPS::convertToInteger(value, duration)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for MaxLeaseDuration in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->max_lease_duration(TimeDuration(duration));
#ifdef OPENDDS_SECURITY
        } else if (name == "SecurityUnsecureLeaseDuration") {
          const OPENDDS_STRING& value = it->second;
          int duration;
          if (!DCPS::convertToInteger(value, duration)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for SecurityUnsecureLeaseDuration in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->security_unsecure_lease_duration(TimeDuration(duration));
        } else if (name == "MaxParticipantsInAuthentication") {
          const OPENDDS_STRING& value = it->second;
          unsigned int max_participants;
          if (!DCPS::convertToInteger(value, max_participants)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for MaxParticipantsInAuthentication in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->max_participants_in_authentication(max_participants);
#endif
        } else if (name == "LeaseExtension") {
          const OPENDDS_STRING& value = it->second;
          int extension;
          if (!DCPS::convertToInteger(value, extension)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for LeaseExtension in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->lease_extension(TimeDuration(extension));
        } else if (name == "PB") {
          const OPENDDS_STRING& value = it->second;
          u_short pb;
          if (!DCPS::convertToInteger(value, pb)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for PB in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->pb(pb);
        } else if (name == "DG") {
          const OPENDDS_STRING& value = it->second;
          u_short dg;
          if (!DCPS::convertToInteger(value, dg)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for DG in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->dg(dg);
        } else if (name == "PG") {
          const OPENDDS_STRING& value = it->second;
          u_short pg;
          if (!DCPS::convertToInteger(value, pg)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for PG in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->pg(pg);
        } else if (name == "D0") {
          const OPENDDS_STRING& value = it->second;
          u_short d0;
          if (!DCPS::convertToInteger(value, d0)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for D0 in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->d0(d0);
        } else if (name == "D1") {
          const OPENDDS_STRING& value = it->second;
          u_short d1;
          if (!DCPS::convertToInteger(value, d1)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for D1 in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->d1(d1);
        } else if (name == "DX") {
          const OPENDDS_STRING& value = it->second;
          u_short dx;
          if (!DCPS::convertToInteger(value, dx)) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for DX in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
          config->dx(dx);
        } else if (name == "TTL") {
          const OPENDDS_STRING& value = it->second;
          unsigned short ttl_us;
          if (!DCPS::convertToInteger(value, ttl_us) || ttl_us > UCHAR_MAX) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for TTL in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
          config->ttl(static_cast<unsigned char>(ttl_us));
        } else if (name == "SendBufferSize") {
          const OPENDDS_STRING& value = it->second;
          ACE_INT32 send_buffer_size;
          if (!DCPS::convertToInteger(value, send_buffer_size)) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for SendBufferSize in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
          config->send_buffer_size(send_buffer_size);
        } else if (name == "RecvBufferSize") {
          const OPENDDS_STRING& value = it->second;
          ACE_INT32 recv_buffer_size;
          if (!DCPS::convertToInteger(value, recv_buffer_size)) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for RecvBufferSize in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
          config->recv_buffer_size(recv_buffer_size);
        } else if (name == "SedpMulticast") {
          const OPENDDS_STRING& value = it->second;
          int smInt;
          if (!DCPS::convertToInteger(value, smInt)) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
               ACE_TEXT("Invalid entry (%C) for SedpMulticast in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
          config->sedp_multicast(bool(smInt));
        } else if (name == "MulticastInterface") {
          config->multicast_interface(it->second);
        } else if (name == "SedpLocalAddress") {
          ACE_INET_Addr addr;
          if (addr.set(it->second.c_str())) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("failed to parse SedpLocalAddress %C\n"),
                              it->second.c_str()),
                             -1);
          }
          config->sedp_local_address(addr);
        } else if (name == "SedpAdvertisedLocalAddress") {
          ACE_INET_Addr addr;
          if (addr.set(it->second.c_str())) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("failed to parse SedpAdvertisedLocalAddress %C\n"),
                              it->second.c_str()),
                             -1);
          }
          config->sedp_advertised_address(addr);
        } else if (name == "SpdpLocalAddress") {
          ACE_INET_Addr addr;
          if (addr.set(u_short(0), it->second.c_str())) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("failed to parse SpdpLocalAddress %C\n"),
                              it->second.c_str()),
                             -1);
          }
          config->spdp_local_address(addr);
        } else if (name == "GuidInterface") {
          config->guid_interface(it->second);
        } else if (name == "InteropMulticastOverride") {
          /// FUTURE: handle > 1 group.
          ACE_INET_Addr addr;
          if (addr.set(u_short(0), it->second.c_str())) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("failed to parse InteropMulticastOverride %C\n"),
                              it->second.c_str()),
                             -1);
          }
          config->default_multicast_group(addr);
        } else if (name == "SpdpSendAddrs") {
          AddrVec spdp_send_addrs;
          const OPENDDS_STRING& value = it->second;
          size_t i = 0;
          do {
            i = value.find_first_not_of(' ', i); // skip spaces
            const size_t n = value.find_first_of(", ", i);
            spdp_send_addrs.push_back(value.substr(i, (n == OPENDDS_STRING::npos) ? n : n - i));
            i = value.find(',', i);
          } while (i++ != OPENDDS_STRING::npos); // skip past comma if there is one
          config->spdp_send_addrs(spdp_send_addrs);
        } else if (name == "SpdpRtpsRelayAddress") {
          ACE_INET_Addr addr;
          if (addr.set(it->second.c_str())) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("failed to parse SpdpRtpsRelayAddress %C\n"),
                              it->second.c_str()),
                             -1);
          }
          config->spdp_rtps_relay_address(addr);
        } else if (name == "SpdpRtpsRelayBeaconPeriod") {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                     ACE_TEXT("Entry SpdpRtpsRelayBeaconPeriod is deprecated and will be ignored.\n")));
        } else if (name == "SpdpRtpsRelaySendPeriod") {
          const OPENDDS_STRING& value = it->second;
          int period;
          if (!DCPS::convertToInteger(value, period)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for SpdpRtpsRelaySendPeriod in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->spdp_rtps_relay_send_period(TimeDuration(period));
        } else if (name == "SedpRtpsRelayAddress") {
          ACE_INET_Addr addr;
          if (addr.set(it->second.c_str())) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("failed to parse SedpRtpsRelayAddress %C\n"),
                              it->second.c_str()),
                             -1);
          }
          config->sedp_rtps_relay_address(addr);
        } else if (name == "SedpRtpsRelayBeaconPeriod") {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                     ACE_TEXT("Entry SedpRtpsRelayBeaconPeriod is deprecated and will be ignored.\n")));
        } else if (name == "RtpsRelayOnly") {
          const OPENDDS_STRING& value = it->second;
          int smInt;
          if (!DCPS::convertToInteger(value, smInt)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
                              ACE_TEXT("Invalid entry (%C) for RtpsRelayOnly in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              value.c_str(), rtps_name.c_str()), -1);
          }
          config->rtps_relay_only(bool(smInt));
        } else if (name == "UseRtpsRelay") {
          const OPENDDS_STRING& value = it->second;
          int smInt;
          if (!DCPS::convertToInteger(value, smInt)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
                              ACE_TEXT("Invalid entry (%C) for UseRtpsRelay in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              value.c_str(), rtps_name.c_str()), -1);
          }
          config->use_rtps_relay(bool(smInt));
#ifdef OPENDDS_SECURITY
        } else if (name == "SpdpStunServerAddress") {
          ACE_INET_Addr addr;
          if (addr.set(it->second.c_str())) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("failed to parse SpdpStunServerAddress %C\n"),
                              it->second.c_str()),
                             -1);
          }
          config->spdp_stun_server_address(addr);
        } else if (name == "SedpStunServerAddress") {
          ACE_INET_Addr addr;
          if (addr.set(it->second.c_str())) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("failed to parse SedpStunServerAddress %C\n"),
                              it->second.c_str()),
                             -1);
          }
          config->sedp_stun_server_address(addr);
        } else if (name == "UseIce") {
          const OPENDDS_STRING& value = it->second;
          int smInt;
          if (!DCPS::convertToInteger(value, smInt)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
                              ACE_TEXT("Invalid entry (%C) for UseIce in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              value.c_str(), rtps_name.c_str()), -1);
          }
          config->use_ice(bool(smInt));
          if (smInt && !TheServiceParticipant->get_security()) {
            ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Security must be enabled (-DCPSSecurity 1) when using ICE (UseIce)\n")), -1);
          }
        } else if (name == "IceTa") {
          // In milliseconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->T_a(TimeDuration::from_msec(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceTa in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "IceConnectivityCheckTTL") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->connectivity_check_ttl(TimeDuration(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceConnectivityCheckTTL in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "IceChecklistPeriod") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->checklist_period(TimeDuration(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceChecklistPeriod in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "IceIndicationPeriod") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->indication_period(TimeDuration(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceIndicationPeriod in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "IceNominatedTTL") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->nominated_ttl(TimeDuration(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceNominatedTTL in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "IceServerReflexiveAddressPeriod") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->server_reflexive_address_period(TimeDuration(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceServerReflexiveAddressPeriod in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "IceServerReflexiveIndicationCount") {
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->server_reflexive_indication_count(int_value);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceServerReflexiveIndicationCount in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "IceDeferredTriggeredCheckTTL") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->deferred_triggered_check_ttl(TimeDuration(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceDeferredTriggeredCheckTTL in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "IceChangePasswordPeriod") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            ICE::Configuration::instance()->change_password_period(TimeDuration(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for IceChangePasswordPeriod in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "MaxAuthTime") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (DCPS::convertToInteger(string_value, int_value)) {
            config->max_auth_time(TimeDuration(int_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for MaxAuthTime in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "AuthResendPeriod") {
          // In seconds.
          const OPENDDS_STRING& string_value = it->second;
          double double_value;
          if (DCPS::convertToDouble(string_value, double_value)) {
            config->auth_resend_period(TimeDuration::from_double(double_value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for AuthResendPeriod in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
#endif /* OPENDDS_SECURITY */
        } else if (name == "MaxSpdpSequenceMsgResetChecks") {
          const OPENDDS_STRING& string_value = it->second;
          u_short value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->max_spdp_sequence_msg_reset_check(value);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for MaxSpdpSequenceMsgResetChecks in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpMaxMessageSize") {
          const OPENDDS_STRING& string_value = it->second;
          size_t value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->sedp_max_message_size(value);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for SedpMaxMessageSize in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpHeartbeatPeriod") {
          const OPENDDS_STRING& string_value = it->second;
          int value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->sedp_heartbeat_period(TimeDuration::from_msec(value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for SedpHeartbeatPeriod in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpNakResponseDelay") {
          const OPENDDS_STRING& string_value = it->second;
          int value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->sedp_nak_response_delay(TimeDuration::from_msec(value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for SedpNakResponseDelay in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpSendDelay") {
          const OPENDDS_STRING& string_value = it->second;
          int value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->sedp_send_delay(TimeDuration::from_msec(value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for SedpSendDelay in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpFragmentReassemblyTimeout") {
          const OPENDDS_STRING& string_value = it->second;
          int value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->sedp_fragment_reassembly_timeout(TimeDuration::from_msec(value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for SedpFragmentReassemblyTimeout in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpPassiveConnectDuration") {
          const OPENDDS_STRING& string_value = it->second;
          int value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->sedp_passive_connect_duration(TimeDuration::from_msec(value));
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                              ACE_TEXT("Invalid entry (%C) for SedpPassiveConnectDuration in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SecureParticipantUserData") {
          const OPENDDS_STRING& string_value = it->second;
          int int_value;
          if (!DCPS::convertToInteger(string_value, int_value)) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsDiscovery::Config::discovery_config: ")
                       ACE_TEXT("Invalid entry (%C) for SecureParticipantUserData in ")
                       ACE_TEXT("[rtps_discovery/%C] section.\n"),
                       string_value.c_str(), rtps_name.c_str()));
            return -1;
          }
          config->secure_participant_user_data(int_value);
        } else if (name == "Customization") {
          if (DCPS::DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                       ACE_TEXT("%C section has a Customization setting.\n"),
                       rtps_name.c_str()));
          }
        } else if (name == "UndirectedSpdp") {
          const OPENDDS_STRING& value = it->second;
          int smInt;
          if (!DCPS::convertToInteger(value, smInt)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
                              ACE_TEXT("Invalid entry (%C) for UndirectedSpdp in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              value.c_str(), rtps_name.c_str()), -1);
          }
          config->undirected_spdp(bool(smInt));
        } else if (name == "PeriodicDirectedSpdp") {
          const OPENDDS_STRING& value = it->second;
          int smInt;
          if (!DCPS::convertToInteger(value, smInt)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
                              ACE_TEXT("Invalid entry (%C) for PeriodicDirectedSpdp in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              value.c_str(), rtps_name.c_str()), -1);
          }
          config->periodic_directed_spdp(bool(smInt));
        } else if (name == "TypeLookupServiceReplyTimeout") {
          const OPENDDS_STRING& value = it->second;
          int timeout;
          if (!DCPS::convertToInteger(value, timeout)) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for TypeLookupServiceReplyTimeout in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
          config->max_type_lookup_service_reply_period(TimeDuration::from_msec(timeout));
        } else if (name == "UseXTypes") {
          const OPENDDS_STRING& value = it->second;
          if (value.size() == 1) {
            int smInt;
            if (!DCPS::convertToInteger(value, smInt) || smInt < 0 || smInt > 2) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
                                ACE_TEXT("Invalid entry (%C) for UseXTypes in ")
                                ACE_TEXT("[rtps_discovery/%C] section.\n"),
                                value.c_str(), rtps_name.c_str()), -1);
            }
            config->use_xtypes(static_cast<RtpsDiscoveryConfig::UseXTypes>(smInt));
          } else {
            config->use_xtypes(value.c_str());
          }
        } else if (name == "SedpResponsiveMode") {
          const OPENDDS_STRING& value = it->second;
          int smInt;
          if (!DCPS::convertToInteger(value, smInt)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
                              ACE_TEXT("Invalid entry (%C) for SedpResponsiveMode in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              value.c_str(), rtps_name.c_str()), -1);
          }
          config->sedp_responsive_mode(bool(smInt));
        } else if (name == "SedpReceivePreallocatedMessageBlocks") {
          const String& string_value = it->second;
          size_t value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->sedp_receive_preallocated_message_blocks(value);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              "(%P|%t) RtpsDiscovery::Config::discovery_config(): "
                              "Invalid entry (%C) for SedpReceivePreallocatedMessageBlocks in "
                              "[rtps_discovery/%C] section.\n",
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpReceivePreallocatedDataBlocks") {
          const String& string_value = it->second;
          size_t value;
          if (DCPS::convertToInteger(string_value, value)) {
            config->sedp_receive_preallocated_data_blocks(value);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              "(%P|%t) RtpsDiscovery::Config::discovery_config(): "
                              "Invalid entry (%C) for SedpReceivePreallocatedDataBlocks in "
                              "[rtps_discovery/%C] section.\n",
                              string_value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "CheckSourceIp") {
          const OPENDDS_STRING& value = it->second;
          int smInt;
          if (!DCPS::convertToInteger(value, smInt)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
                              ACE_TEXT("Invalid entry (%C) for CheckSourceIp in ")
                              ACE_TEXT("[rtps_discovery/%C] section.\n"),
                              value.c_str(), rtps_name.c_str()), -1);
          }
          config->check_source_ip(bool(smInt));
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
            ACE_TEXT("Unexpected entry (%C) in [rtps_discovery/%C] section.\n"),
            name.c_str(), rtps_name.c_str()), -1);
        }
      }

      TheServiceParticipant->add_discovery(discovery);
    }
  }

  // If the default RTPS discovery object has not been configured,
  // instantiate it now.
  const DCPS::Service_Participant::RepoKeyDiscoveryMap& discoveryMap = TheServiceParticipant->discoveryMap();
  if (discoveryMap.find(Discovery::DEFAULT_RTPS) == discoveryMap.end()) {
    TheServiceParticipant->add_discovery(OpenDDS::DCPS::make_rch<RtpsDiscovery>(Discovery::DEFAULT_RTPS));
  }

  return 0;
}

// Participant operations:
OpenDDS::DCPS::RepoId
RtpsDiscovery::generate_participant_guid()
{
  OpenDDS::DCPS::RepoId id = GUID_UNKNOWN;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, id);
  const OPENDDS_STRING guid_interface = config_->guid_interface();
  if (!guid_interface.empty()) {
    if (guid_gen_.interfaceName(guid_interface.c_str()) != 0) {
      if (DCPS::DCPS_debug_level) {
        ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsDiscovery::generate_participant_guid()"
                   " - attempt to use network interface %C MAC addr for"
                   " GUID generation failed.\n", guid_interface.c_str()));
      }
    }
  }
  guid_gen_.populate(id);
  id.entityId = ENTITYID_PARTICIPANT;
  return id;
}

DCPS::AddDomainStatus
RtpsDiscovery::add_domain_participant(DDS::DomainId_t domain,
                                      const DDS::DomainParticipantQos& qos,
                                      XTypes::TypeLookupService_rch tls)
{
  DCPS::AddDomainStatus ads = {OpenDDS::DCPS::RepoId(), false /*federated*/};
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ads);
    const OPENDDS_STRING guid_interface = config_->guid_interface();
    if (!guid_interface.empty()) {
      if (guid_gen_.interfaceName(guid_interface.c_str()) != 0) {
        if (DCPS::DCPS_debug_level) {
          ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsDiscovery::add_domain_participant()"
                     " - attempt to use specific network interface %C MAC addr for"
                     " GUID generation failed.\n", guid_interface.c_str()));
        }
      }
    }
    guid_gen_.populate(ads.id);
  }
  ads.id.entityId = ENTITYID_PARTICIPANT;
  try {
    const DCPS::RcHandle<Spdp> spdp(DCPS::make_rch<Spdp>(domain, ref(ads.id), qos, this, tls));
    // ads.id may change during Spdp constructor
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, participants_lock_, ads);
    participants_[domain][ads.id] = spdp;
  } catch (const std::exception& e) {
    ads.id = GUID_UNKNOWN;
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsDiscovery::add_domain_participant() - "
      "failed to initialize RTPS Simple Participant Discovery Protocol: %C\n",
      e.what()));
  }
  return ads;
}

#if defined(OPENDDS_SECURITY)
DCPS::AddDomainStatus
RtpsDiscovery::add_domain_participant_secure(
  DDS::DomainId_t domain,
  const DDS::DomainParticipantQos& qos,
  XTypes::TypeLookupService_rch tls,
  const OpenDDS::DCPS::RepoId& guid,
  DDS::Security::IdentityHandle id,
  DDS::Security::PermissionsHandle perm,
  DDS::Security::ParticipantCryptoHandle part_crypto)
{
  DCPS::AddDomainStatus ads = {guid, false /*federated*/};
  ads.id.entityId = ENTITYID_PARTICIPANT;
  try {
    const DCPS::RcHandle<Spdp> spdp(DCPS::make_rch<Spdp>(
      domain, ads.id, qos, this, tls, id, perm, part_crypto));
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, participants_lock_, ads);
    participants_[domain][ads.id] = spdp;
  } catch (const std::exception& e) {
    ads.id = GUID_UNKNOWN;
    ACE_ERROR((LM_WARNING, "(%P|%t) RtpsDiscovery::add_domain_participant_secure() - "
      "failed to initialize RTPS Simple Participant Discovery Protocol: %C\n",
      e.what()));
  }
  return ads;
}
#endif

void
RtpsDiscovery::signal_liveliness(const DDS::DomainId_t domain_id,
                                 const OpenDDS::DCPS::RepoId& part_id,
                                 DDS::LivelinessQosPolicyKind kind)
{
  get_part(domain_id, part_id)->signal_liveliness(kind);
}

void
RtpsDiscovery::rtps_relay_only_now(bool after)
{
  RtpsDiscoveryConfig_rch config = get_config();
  const bool before = config->rtps_relay_only();
  config->rtps_relay_only(after);

  if (before != after) {
    ACE_GUARD(ACE_Thread_Mutex, g, participants_lock_);
    for (DomainParticipantMap::const_iterator dom_pos = participants_.begin(), dom_limit = participants_.end();
         dom_pos != dom_limit; ++dom_pos) {
      for (ParticipantMap::const_iterator part_pos = dom_pos->second.begin(), part_limit = dom_pos->second.end(); part_pos != part_limit; ++part_pos) {
        part_pos->second->rtps_relay_only_now(after);
      }
    }
  }
}

void
RtpsDiscovery::use_rtps_relay_now(bool after)
{
  RtpsDiscoveryConfig_rch config = get_config();
  const bool before = config->use_rtps_relay();
  config->use_rtps_relay(after);

  if (before != after) {
    ACE_GUARD(ACE_Thread_Mutex, g, participants_lock_);
    for (DomainParticipantMap::const_iterator dom_pos = participants_.begin(), dom_limit = participants_.end();
         dom_pos != dom_limit; ++dom_pos) {
      for (ParticipantMap::const_iterator part_pos = dom_pos->second.begin(), part_limit = dom_pos->second.end(); part_pos != part_limit; ++part_pos) {
        part_pos->second->use_rtps_relay_now(after);
      }
    }
  }
}

void
RtpsDiscovery::use_ice_now(bool after)
{
  RtpsDiscoveryConfig_rch config = get_config();
  const bool before = config->use_ice();
  config->use_ice(after);

  if (before != after) {
    ACE_GUARD(ACE_Thread_Mutex, g, participants_lock_);
    for (DomainParticipantMap::const_iterator dom_pos = participants_.begin(), dom_limit = participants_.end();
         dom_pos != dom_limit; ++dom_pos) {
      for (ParticipantMap::const_iterator part_pos = dom_pos->second.begin(), part_limit = dom_pos->second.end(); part_pos != part_limit; ++part_pos) {
        part_pos->second->use_ice_now(after);
      }
    }
  }
}

#ifdef OPENDDS_SECURITY
DDS::Security::ParticipantCryptoHandle
RtpsDiscovery::get_crypto_handle(DDS::DomainId_t domain,
                                 const DCPS::RepoId& local_participant,
                                 const DCPS::RepoId& remote_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    if (remote_participant == GUID_UNKNOWN || remote_participant == local_participant) {
      return p->crypto_handle();
    } else {
      return p->remote_crypto_handle(remote_participant);
    }
  }

  return DDS::HANDLE_NIL;
}

#endif

RtpsDiscovery::StaticInitializer::StaticInitializer()
{
  TheServiceParticipant->register_discovery_type("rtps_discovery", new Config);
}

u_short
RtpsDiscovery::get_spdp_port(DDS::DomainId_t domain,
                             const DCPS::RepoId& local_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    return p->get_spdp_port();
  }

  return 0;
}

u_short
RtpsDiscovery::get_sedp_port(DDS::DomainId_t domain,
                             const DCPS::RepoId& local_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    return p->get_sedp_port();
  }

  return 0;
}

#ifdef ACE_HAS_IPV6

u_short
RtpsDiscovery::get_ipv6_spdp_port(DDS::DomainId_t domain,
                                  const DCPS::RepoId& local_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    return p->get_ipv6_spdp_port();
  }

  return 0;
}

u_short
RtpsDiscovery::get_ipv6_sedp_port(DDS::DomainId_t domain,
                                  const DCPS::RepoId& local_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    return p->get_ipv6_sedp_port();
  }

  return 0;
}
#endif

void
RtpsDiscovery::spdp_rtps_relay_address(const ACE_INET_Addr& address)
{
  RtpsDiscoveryConfig_rch config = get_config();
  const ACE_INET_Addr prev = config->spdp_rtps_relay_address();
  if (prev == address) {
    return;
  }

  config->spdp_rtps_relay_address(address);

  if (address == ACE_INET_Addr()) {
    return;
  }

  ACE_GUARD(ACE_Thread_Mutex, g, participants_lock_);
  for (DomainParticipantMap::const_iterator dom_pos = participants_.begin(), dom_limit = participants_.end();
       dom_pos != dom_limit; ++dom_pos) {
    for (ParticipantMap::const_iterator part_pos = dom_pos->second.begin(), part_limit = dom_pos->second.end(); part_pos != part_limit; ++part_pos) {
      part_pos->second->spdp_rtps_relay_address_change();
    }
  }
}

void
RtpsDiscovery::sedp_rtps_relay_address(const ACE_INET_Addr& address)
{
  RtpsDiscoveryConfig_rch config = get_config();
  config->sedp_rtps_relay_address(address);

  ACE_GUARD(ACE_Thread_Mutex, g, participants_lock_);
  for (DomainParticipantMap::const_iterator dom_pos = participants_.begin(), dom_limit = participants_.end();
       dom_pos != dom_limit; ++dom_pos) {
    for (ParticipantMap::const_iterator part_pos = dom_pos->second.begin(), part_limit = dom_pos->second.end(); part_pos != part_limit; ++part_pos) {
      part_pos->second->sedp_rtps_relay_address(address);
    }
  }
}

void
RtpsDiscovery::spdp_stun_server_address(const ACE_INET_Addr& address)
{
  get_config()->spdp_stun_server_address(address);
}

void
RtpsDiscovery::sedp_stun_server_address(const ACE_INET_Addr& address)
{
  get_config()->sedp_stun_server_address(address);

  ACE_GUARD(ACE_Thread_Mutex, g, participants_lock_);
  for (DomainParticipantMap::const_iterator dom_pos = participants_.begin(), dom_limit = participants_.end();
       dom_pos != dom_limit; ++dom_pos) {
    for (ParticipantMap::const_iterator part_pos = dom_pos->second.begin(), part_limit = dom_pos->second.end(); part_pos != part_limit; ++part_pos) {
      part_pos->second->sedp_stun_server_address(address);
    }
  }
}

void
RtpsDiscovery::append_transport_statistics(DDS::DomainId_t domain,
                                           const DCPS::RepoId& local_participant,
                                           DCPS::TransportStatisticsSequence& seq)
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    p->append_transport_statistics(seq);
  }
}

DDS::Subscriber_ptr RtpsDiscovery::init_bit(DCPS::DomainParticipantImpl* participant)
{
  DDS::Subscriber_var bit_subscriber;
#ifndef DDS_HAS_MINIMUM_BIT
  if (!TheServiceParticipant->get_BIT()) {
    get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber);
    return 0;
  }

  if (create_bit_topics(participant) != DDS::RETCODE_OK) {
    return 0;
  }

  bit_subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil(),
                                   DCPS::DEFAULT_STATUS_MASK);
  DCPS::SubscriberImpl* sub = dynamic_cast<DCPS::SubscriberImpl*>(bit_subscriber.in());
  if (sub == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::init_bit")
               ACE_TEXT(" - Could not cast Subscriber to SubscriberImpl\n")));
    return 0;
  }

  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

  dr_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay =
    TheServiceParticipant->bit_autopurge_nowriter_samples_delay();
  dr_qos.reader_data_lifecycle.autopurge_disposed_samples_delay =
    TheServiceParticipant->bit_autopurge_disposed_samples_delay();

  DDS::TopicDescription_var bit_part_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_PARTICIPANT_TOPIC);
  create_bit_dr(bit_part_topic, DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_topic_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_TOPIC_TOPIC);
  create_bit_dr(bit_topic_topic, DCPS::BUILT_IN_TOPIC_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_pub_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_PUBLICATION_TOPIC);
  create_bit_dr(bit_pub_topic, DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_sub_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  create_bit_dr(bit_sub_topic, DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_part_loc_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
  create_bit_dr(bit_part_loc_topic, DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_connection_record_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_CONNECTION_RECORD_TOPIC);
  create_bit_dr(bit_connection_record_topic, DCPS::BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_internal_thread_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
  create_bit_dr(bit_internal_thread_topic, DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE,
                sub, dr_qos);

  const DDS::ReturnCode_t ret = bit_subscriber->enable();
  if (ret != DDS::RETCODE_OK) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) PeerDiscovery::init_bit")
                 ACE_TEXT(" - Error %d enabling subscriber\n"), ret));
    }
    return 0;
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber);

  return bit_subscriber._retn();
}

void RtpsDiscovery::fini_bit(DCPS::DomainParticipantImpl* participant)
{
  get_part(participant->get_domain_id(), participant->get_id())->fini_bit();
}

bool RtpsDiscovery::attach_participant(
  DDS::DomainId_t /*domainId*/, const GUID_t& /*participantId*/)
{
  return false; // This is just for DCPSInfoRepo?
}

bool RtpsDiscovery::remove_domain_participant(
  DDS::DomainId_t domain_id, const GUID_t& participantId)
{
  // Use reference counting to ensure participant
  // does not get deleted until lock as been released.
  ParticipantHandle participant;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, participants_lock_, false);
  DomainParticipantMap::iterator domain = participants_.find(domain_id);
  if (domain == participants_.end()) {
    return false;
  }
  ParticipantMap::iterator part = domain->second.find(participantId);
  if (part == domain->second.end()) {
    return false;
  }
  participant = part->second;
  domain->second.erase(part);
  if (domain->second.empty()) {
    participants_.erase(domain);
  }
  g.release();

  participant->shutdown();
  return true;
}

bool RtpsDiscovery::ignore_domain_participant(
  DDS::DomainId_t domain, const GUID_t& myParticipantId, const GUID_t& ignoreId)
{
  get_part(domain, myParticipantId)->ignore_domain_participant(ignoreId);
  return true;
}

bool RtpsDiscovery::remove_domain_participant(
  DDS::DomainId_t domain, const GUID_t& myParticipantId, const GUID_t& removeId)
{
  get_part(domain, myParticipantId)->remove_domain_participant(removeId);
  return true;
}

bool RtpsDiscovery::update_domain_participant_qos(
  DDS::DomainId_t domain, const GUID_t& participant, const DDS::DomainParticipantQos& qos)
{
  return get_part(domain, participant)->update_domain_participant_qos(qos);
}

bool RtpsDiscovery::has_domain_participant(DDS::DomainId_t domain, const GUID_t& local, const GUID_t& remote) const
{
  return get_part(domain, local)->has_domain_participant(remote);
}

DCPS::TopicStatus RtpsDiscovery::assert_topic(
  GUID_t& topicId,
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const char* topicName,
  const char* dataTypeName,
  const DDS::TopicQos& qos,
  bool hasDcpsKey,
  DCPS::TopicCallbacks* topic_callbacks)
{
  ParticipantHandle part = get_part(domainId, participantId);
  if (part) {
    return part->assert_topic(topicId, topicName,
                              dataTypeName, qos,
                              hasDcpsKey, topic_callbacks);
  }
  return DCPS::INTERNAL_ERROR;
}

DCPS::TopicStatus RtpsDiscovery::find_topic(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const char* topicName,
  CORBA::String_out dataTypeName,
  DDS::TopicQos_out qos,
  GUID_t& topicId)
{
  ParticipantHandle part = get_part(domainId, participantId);
  if (part) {
    return part->find_topic(topicName, dataTypeName, qos, topicId);
  }
  return DCPS::INTERNAL_ERROR;
}

DCPS::TopicStatus RtpsDiscovery::remove_topic(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const GUID_t& topicId)
{
  ParticipantHandle part = get_part(domainId, participantId);
  if (part) {
    return part->remove_topic(topicId);
  }
  return DCPS::INTERNAL_ERROR;
}

bool RtpsDiscovery::ignore_topic(DDS::DomainId_t domainId, const GUID_t& myParticipantId,
                                 const GUID_t& ignoreId)
{
  get_part(domainId, myParticipantId)->ignore_topic(ignoreId);
  return true;
}

bool RtpsDiscovery::update_topic_qos(const GUID_t& topicId, DDS::DomainId_t domainId,
                                    const GUID_t& participantId, const DDS::TopicQos& qos)
{
  ParticipantHandle part = get_part(domainId, participantId);
  if (part) {
    return part->update_topic_qos(topicId, qos);
  }
  return false;
}

GUID_t RtpsDiscovery::add_publication(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const GUID_t& topicId,
  DCPS::DataWriterCallbacks_rch publication,
  const DDS::DataWriterQos& qos,
  const DCPS::TransportLocatorSeq& transInfo,
  const DDS::PublisherQos& publisherQos,
  const XTypes::TypeInformation& type_info)
{
  return get_part(domainId, participantId)->add_publication(
    topicId, publication, qos, transInfo, publisherQos, type_info);
}

bool RtpsDiscovery::remove_publication(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& publicationId)
{
  get_part(domainId, participantId)->remove_publication(publicationId);
  return true;
}

bool RtpsDiscovery::ignore_publication(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& ignoreId)
{
  get_part(domainId, participantId)->ignore_publication(ignoreId);
  return true;
}

bool RtpsDiscovery::update_publication_qos(
  DDS::DomainId_t domainId,
  const GUID_t& partId,
  const GUID_t& dwId,
  const DDS::DataWriterQos& qos,
  const DDS::PublisherQos& publisherQos)
{
  return get_part(domainId, partId)->update_publication_qos(dwId, qos,
                                                            publisherQos);
}

void RtpsDiscovery::update_publication_locators(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& dwId,
  const DCPS::TransportLocatorSeq& transInfo)
{
  get_part(domainId, partId)->update_publication_locators(dwId, transInfo);
}

GUID_t RtpsDiscovery::add_subscription(
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
  const XTypes::TypeInformation& type_info)
{
  return get_part(domainId, participantId)->add_subscription(
    topicId, subscription, qos, transInfo, subscriberQos, filterClassName,
    filterExpr, params, type_info);
}

bool RtpsDiscovery::remove_subscription(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& subscriptionId)
{
  get_part(domainId, participantId)->remove_subscription(subscriptionId);
  return true;
}

bool RtpsDiscovery::ignore_subscription(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& ignoreId)
{
  get_part(domainId, participantId)->ignore_subscription(ignoreId);
  return true;
}

bool RtpsDiscovery::update_subscription_qos(
  DDS::DomainId_t domainId,
  const GUID_t& partId,
  const GUID_t& drId,
  const DDS::DataReaderQos& qos,
  const DDS::SubscriberQos& subQos)
{
  return get_part(domainId, partId)->update_subscription_qos(drId, qos, subQos);
}

bool RtpsDiscovery::update_subscription_params(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& subId, const DDS::StringSeq& params)
{
  return get_part(domainId, partId)->update_subscription_params(subId, params);
}

void RtpsDiscovery::update_subscription_locators(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& subId,
  const DCPS::TransportLocatorSeq& transInfo)
{
  get_part(domainId, partId)->update_subscription_locators(subId, transInfo);
}

RcHandle<DCPS::TransportInst> RtpsDiscovery::sedp_transport_inst(DDS::DomainId_t domainId,
                                                                 const GUID_t& partId) const
{
  return get_part(domainId, partId)->sedp_transport_inst();
}

ParticipantHandle RtpsDiscovery::get_part(const DDS::DomainId_t domain_id, const GUID_t& part_id) const
{
  ACE_Guard<ACE_Thread_Mutex> guard(participants_lock_);
  const DomainParticipantMap::const_iterator domain = participants_.find(domain_id);
  if (domain == participants_.end()) {
    return ParticipantHandle();
  }
  const ParticipantMap::const_iterator part = domain->second.find(part_id);
  if (part == domain->second.end()) {
    return ParticipantHandle();
  }
  return part->second;
}

RtpsDiscoveryConfig_rch RtpsDiscovery::get_config() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(lock_);
  return config_;
}

void RtpsDiscovery::create_bit_dr(DDS::TopicDescription_ptr topic,
  const char* type, DCPS::SubscriberImpl* sub, const DDS::DataReaderQos& qos)
{
  DCPS::TopicDescriptionImpl* bit_topic_i =
    dynamic_cast<DCPS::TopicDescriptionImpl*>(topic);
  if (bit_topic_i == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PeerDiscovery::create_bit_dr: ")
               ACE_TEXT("Could not cast TopicDescription to TopicDescriptionImpl\n")));
    return;
  }

  DDS::DomainParticipant_var participant = sub->get_participant();
  DCPS::DomainParticipantImpl* participant_i =
    dynamic_cast<DCPS::DomainParticipantImpl*>(participant.in());
  if (participant_i == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PeerDiscovery::create_bit_dr: ")
               ACE_TEXT("Could not cast DomainParticipant to DomainParticipantImpl\n")));
    return;
  }

  DCPS::TypeSupport_var type_support =
    Registered_Data_Types->lookup(participant, type);

  DDS::DataReader_var dr = type_support->create_datareader();
  DCPS::DataReaderImpl* dri = dynamic_cast<DCPS::DataReaderImpl*>(dr.in());
  if (dri == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PeerDiscovery::create_bit_dr: ")
               ACE_TEXT("Could not cast DataReader to DataReaderImpl\n")));
    return;
  }

  dri->init(bit_topic_i, qos, 0 /*listener*/, 0 /*mask*/, participant_i, sub);
  dri->disable_transport();
  dri->enable();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
