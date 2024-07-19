/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsDiscoveryConfig.h"

#include <dds/DCPS/LogAddr.h>

#include <dds/OpenDDSConfigWrapper.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::TimeDuration;

RtpsDiscoveryConfig::RtpsDiscoveryConfig(const String& name)
  : config_prefix_(DCPS::ConfigPair::canonicalize("RTPS_DISCOVERY_" + name))
{}

String
RtpsDiscoveryConfig::config_key(const String& key) const
{
  return DCPS::ConfigPair::canonicalize(config_prefix_ + "_" + key);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::resend_period() const
{
  // see RTPS v2.1 9.6.1.4.2
  return TheServiceParticipant->config_store()->get(config_key("RESEND_PERIOD").c_str(),
                                                    TimeDuration(30 /*seconds*/),
                                                    DCPS::ConfigStoreImpl::Format_FractionalSeconds);
}

void
RtpsDiscoveryConfig::resend_period(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("RESEND_PERIOD").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_FractionalSeconds);
}

double
RtpsDiscoveryConfig::quick_resend_ratio() const
{
  return TheServiceParticipant->config_store()->get_float64(config_key("QUICK_RESEND_RATIO").c_str(), 0.1);
}

void
RtpsDiscoveryConfig::quick_resend_ratio(double ratio)
{
  TheServiceParticipant->config_store()->set_float64(config_key("QUICK_RESEND_RATIO").c_str(),
                                                     ratio);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::min_resend_delay() const
{
  return TheServiceParticipant->config_store()->get(config_key("MIN_RESEND_DELAY").c_str(),
                                                    TimeDuration::from_msec(100),
                                                    DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);

}

void
RtpsDiscoveryConfig::min_resend_delay(const DCPS::TimeDuration& delay)
{
  TheServiceParticipant->config_store()->set(config_key("MIN_RESEND_DELAY").c_str(),
                                             delay,
                                             DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::lease_duration() const
{
  return TheServiceParticipant->config_store()->get(config_key("LEASE_DURATION").c_str(),
                                                    TimeDuration(300),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void
RtpsDiscoveryConfig::lease_duration(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("LEASE_DURATION").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::max_lease_duration() const
{
  return TheServiceParticipant->config_store()->get(config_key("MAX_LEASE_DURATION").c_str(),
                                                    TimeDuration(300),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void
RtpsDiscoveryConfig::max_lease_duration(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("MAX_LEASE_DURATION").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

#if OPENDDS_CONFIG_SECURITY
DCPS::TimeDuration
RtpsDiscoveryConfig::security_unsecure_lease_duration() const
{
  return TheServiceParticipant->config_store()->get(config_key("SECURITY_UNSECURE_LEASE_DURATION").c_str(),
                                                    TimeDuration(30),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void
RtpsDiscoveryConfig::security_unsecure_lease_duration(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("SECURITY_UNSECURE_LEASE_DURATION").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

size_t
RtpsDiscoveryConfig::max_participants_in_authentication() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("MAX_PARTICIPANTS_IN_AUTHENTICATION").c_str(),
                                                           0);
}

void
RtpsDiscoveryConfig::max_participants_in_authentication(size_t m)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("MAX_PARTICIPANTS_IN_AUTHENTICATION").c_str(),
                                                    static_cast<DDS::UInt32>(m));
}
#endif

DCPS::TimeDuration
RtpsDiscoveryConfig::lease_extension() const
{
  return TheServiceParticipant->config_store()->get(config_key("LEASE_EXTENSION").c_str(),
                                                    TimeDuration(0),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void
RtpsDiscoveryConfig::lease_extension(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("LEASE_EXTENSION").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

PortMode RtpsDiscoveryConfig::spdp_port_mode() const
{
  // TODO: Deprecated, remove in OpenDDS 4
  const bool request_random_port = TheServiceParticipant->config_store()->get_boolean(
    config_key("SPDP_REQUEST_RANDOM_PORT").c_str(), false);
  return get_port_mode(config_key("SPDP_PORT_MODE"),
    request_random_port ? PortMode_System : PortMode_Probe);
}

void RtpsDiscoveryConfig::spdp_port_mode(PortMode value)
{
  set_port_mode(config_key("SPDP_PORT_MODE"), value);
}

bool
RtpsDiscoveryConfig::spdp_request_random_port() const
{
  return spdp_port_mode() == PortMode_System;
}

void
RtpsDiscoveryConfig::spdp_request_random_port(bool f)
{
  spdp_port_mode(f ? PortMode_System : PortMode_Probe);
}

PortMode RtpsDiscoveryConfig::sedp_port_mode() const
{
  return get_port_mode(config_key("SEDP_PORT_MODE"), PortMode_System);
}

void RtpsDiscoveryConfig::sedp_port_mode(PortMode value)
{
  set_port_mode(config_key("SEDP_PORT_MODE"), value);
}

DDS::UInt16
RtpsDiscoveryConfig::pb() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("PB").c_str(),
                                                           default_port_base);
}

void
RtpsDiscoveryConfig::pb(DDS::UInt16 port_base)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("PB").c_str(),
                                                    port_base);
}

DDS::UInt16
RtpsDiscoveryConfig::dg() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("DG").c_str(),
                                                           default_domain_gain);
}

void
RtpsDiscoveryConfig::dg(DDS::UInt16 domain_gain)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("DG").c_str(),
                                                    domain_gain);
}

DDS::UInt16
RtpsDiscoveryConfig::pg() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("PG").c_str(),
                                                           default_part_gain);
}

void
RtpsDiscoveryConfig::pg(DDS::UInt16 participant_gain)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("PG").c_str(),
                                                    participant_gain);
}

DDS::UInt16
RtpsDiscoveryConfig::d0() const
{
  DDS::UInt16 default_value = 0;
#if !defined ACE_LACKS_GETENV && !defined ACE_LACKS_ENV
  const char* const from_env = ACE_OS::getenv("OPENDDS_RTPS_DEFAULT_D0");
  if (from_env) {
    default_value = static_cast<DDS::UInt16>(std::atoi(from_env));
  }
#endif
  return TheServiceParticipant->config_store()->get_uint32(config_key("D0").c_str(),
                                                           default_value);
}

void
RtpsDiscoveryConfig::d0(DDS::UInt16 spdp_multicast_offset)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("D0").c_str(),
                                                    spdp_multicast_offset);
}

DDS::UInt16
RtpsDiscoveryConfig::d1() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("D1").c_str(),
                                                           default_spdp_unicast_offset);
}

void
RtpsDiscoveryConfig::d1(DDS::UInt16 spdp_unicast_offset)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("D1").c_str(),
                                                    spdp_unicast_offset);
}
DDS::UInt16
RtpsDiscoveryConfig::dx() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("DX").c_str(),
                                                           default_sedp_multicast_offset);
}

void
RtpsDiscoveryConfig::dx(DDS::UInt16 sedp_multicast_offset)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("DX").c_str(),
                                                    sedp_multicast_offset);
}

DDS::UInt16
RtpsDiscoveryConfig::dy() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("DY").c_str(),
                                                           default_sedp_unicast_offset);
}

void
RtpsDiscoveryConfig::dy(DDS::UInt16 sedp_unicast_offset)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("DY").c_str(),
                                                    sedp_unicast_offset);
}

bool RtpsDiscoveryConfig::set_spdp_multicast_port(DCPS::NetworkAddress& addr,
  DDS::DomainId_t domain) const
{
  return set_rtps_multicast_port(addr, "SPDP multicast", pb(), d0(), domain, dg());
}

bool RtpsDiscoveryConfig::set_spdp_unicast_port(DCPS::NetworkAddress& addr, bool& fixed_port,
  DDS::DomainId_t domain, DDS::UInt16 part_id) const
{
  return set_rtps_unicast_port(addr, fixed_port, "SPDP unicast", spdp_port_mode(),
    pb(), d1(), domain, dg(), part_id, pg());
}

unsigned char
RtpsDiscoveryConfig::ttl() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("TTL").c_str(), 1);
}

void
RtpsDiscoveryConfig::ttl(unsigned char time_to_live)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("TTL").c_str(),
                                                    time_to_live);
}

ACE_INT32
RtpsDiscoveryConfig::send_buffer_size() const
{
  return TheServiceParticipant->config_store()->get_int32(config_key("SEND_BUFFER_SIZE").c_str(),
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
                                                          ACE_DEFAULT_MAX_SOCKET_BUFSIZ
#else
                                                          0
#endif
                                                          );
}

void
RtpsDiscoveryConfig::send_buffer_size(ACE_INT32 buffer_size)
{
  TheServiceParticipant->config_store()->set_int32(config_key("SEND_BUFFER_SIZE").c_str(),
                                                   buffer_size);
}

ACE_INT32
RtpsDiscoveryConfig::recv_buffer_size() const
{
  return TheServiceParticipant->config_store()->get_int32(config_key("RECV_BUFFER_SIZE").c_str(),
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
                                                          ACE_DEFAULT_MAX_SOCKET_BUFSIZ
#else
                                                          0
#endif
                                                          );
}

void
RtpsDiscoveryConfig::recv_buffer_size(ACE_INT32 buffer_size)
{
  TheServiceParticipant->config_store()->set_int32(config_key("RECV_BUFFER_SIZE").c_str(),
                                                   buffer_size);
}

bool
RtpsDiscoveryConfig::sedp_multicast() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("SEDP_MULTICAST").c_str(),
                                                            true);
}

void
RtpsDiscoveryConfig::sedp_multicast(bool sm)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("SEDP_MULTICAST").c_str(),
                                                     sm);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::sedp_local_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_LOCAL_ADDRESS").c_str(),
                                                    TheServiceParticipant->default_address(),
                                                    DCPS::ConfigStoreImpl::Format_Optional_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void
RtpsDiscoveryConfig::sedp_local_address(const DCPS::NetworkAddress& mi)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_LOCAL_ADDRESS").c_str(),
                                             mi,
                                             DCPS::ConfigStoreImpl::Format_Optional_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::sedp_advertised_local_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_ADVERTISED_LOCAL_ADDRESS").c_str(),
                                                    DCPS::NetworkAddress::default_IPV4,
                                                    DCPS::ConfigStoreImpl::Format_Required_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void
RtpsDiscoveryConfig::sedp_advertised_local_address(const DCPS::NetworkAddress& mi)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_ADVERTISED_LOCAL_ADDRESS").c_str(),
                                             mi,
                                             DCPS::ConfigStoreImpl::Format_Required_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::spdp_local_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("SPDP_LOCAL_ADDRESS").c_str(),
                                                    TheServiceParticipant->default_address(),
                                                    DCPS::ConfigStoreImpl::Format_Optional_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void
RtpsDiscoveryConfig::spdp_local_address(const DCPS::NetworkAddress& mi)
{
  TheServiceParticipant->config_store()->set(config_key("SPDP_LOCAL_ADDRESS").c_str(),
                                             mi,
                                             DCPS::ConfigStoreImpl::Format_Optional_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

bool RtpsDiscoveryConfig::spdp_unicast_address(DCPS::NetworkAddress& addr, bool& fixed_port,
  DDS::DomainId_t domain, DDS::UInt16 part_id) const
{
  addr = spdp_local_address();
  return set_spdp_unicast_port(addr, fixed_port, domain, part_id);
}

OPENDDS_STRING
RtpsDiscoveryConfig::multicast_interface() const
{
  const String d = (TheServiceParticipant->default_address() != DCPS::NetworkAddress::default_IPV4) ?
    DCPS::LogAddr::ip(TheServiceParticipant->default_address().to_addr()) : "";
  return TheServiceParticipant->config_store()->get(config_key("MULTICAST_INTERFACE").c_str(), d);
}

void
RtpsDiscoveryConfig::multicast_interface(const OPENDDS_STRING& mi)
{
  TheServiceParticipant->config_store()->set(config_key("MULTICAST_INTERFACE").c_str(),
                                             mi);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::default_multicast_group(DDS::DomainId_t domain) const
{
  // RTPS v2.1 9.6.1.4.1
  DCPS::NetworkAddress na =
    TheServiceParticipant->config_store()->get(config_key("INTEROP_MULTICAST_OVERRIDE").c_str(),
                                               DCPS::NetworkAddress("239.255.0.1:0"),
                                               DCPS::ConfigStoreImpl::Format_No_Port,
                                               DCPS::ConfigStoreImpl::Kind_IPV4);
  // Customize.
  const String customization_name = TheServiceParticipant->config_store()->get(config_key("CUSTOMIZATION").c_str(), "");
  if (!customization_name.empty()) {
    const String directive = TheServiceParticipant->config_store()->get(String("CUSTOMIZATION_" + customization_name + "_INTEROP_MULTICAST_OVERRIDE").c_str(), "");
    if (directive == "AddDomainId") {
      String addr = DCPS::LogAddr(na, DCPS::LogAddr::Ip).str();
      const size_t pos = addr.find_last_of(".");
      if (pos != String::npos) {
        String custom = addr.substr(pos + 1);
        int val = 0;
        if (!DCPS::convertToInteger(custom, val)) {
          if (log_level >= DCPS::LogLevel::Error) {
            ACE_ERROR((LM_ERROR,
                       "(%P|%t) ERROR: RtpsDiscoveryConfig::default_multicast_group: "
                       "could not convert %C to integer\n",
                       custom.c_str()));
          }
          return na;
        }
        val += domain;
        addr = addr.substr(0, pos);
        addr += "." + DCPS::to_dds_string(val);
      } else {
        if (log_level >= DCPS::LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: RtpsDiscoveryConfig::default_multicast_group: "
                     "could not AddDomainId for %s\n",
                     addr.c_str()));
        }
        return na;
      }

      na = DCPS::NetworkAddress(0, addr.c_str());
    }
  }

  return na;
}

void
RtpsDiscoveryConfig::default_multicast_group(const DCPS::NetworkAddress& group)
{
  TheServiceParticipant->config_store()->set(config_key("INTEROP_MULTICAST_OVERRIDE").c_str(),
                                             group,
                                             DCPS::ConfigStoreImpl::Format_No_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

bool RtpsDiscoveryConfig::spdp_multicast_address(
  DCPS::NetworkAddress& addr, DDS::DomainId_t domain) const
{
  addr = TheServiceParticipant->config_store()->get(
    config_key("SPDP_MULTICAST_ADDRESS").c_str(),
    default_multicast_group(domain),
    DCPS::ConfigStoreImpl::Format_Optional_Port,
    DCPS::ConfigStoreImpl::Kind_IPV4);
  return set_spdp_multicast_port(addr, domain);
}

void RtpsDiscoveryConfig::spdp_multicast_address(const DCPS::NetworkAddress& addr)
{
  TheServiceParticipant->config_store()->set(config_key("SPDP_MULTICAST_ADDRESS").c_str(),
                                             addr,
                                             DCPS::ConfigStoreImpl::Format_Optional_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

DCPS::NetworkAddress RtpsDiscoveryConfig::sedp_multicast_address(DDS::DomainId_t domain) const
{
  return TheServiceParticipant->config_store()->get(
    config_key("SEDP_MULTICAST_ADDRESS").c_str(),
    default_multicast_group(domain),
    DCPS::ConfigStoreImpl::Format_Optional_Port,
    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void RtpsDiscoveryConfig::sedp_multicast_address(const DCPS::NetworkAddress& addr)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_MULTICAST_ADDRESS").c_str(),
                                             addr,
                                             DCPS::ConfigStoreImpl::Format_Optional_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

#ifdef ACE_HAS_IPV6
DCPS::NetworkAddress
RtpsDiscoveryConfig::ipv6_spdp_local_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("IPV6_SPDP_LOCAL_ADDRESS").c_str(),
                                                    DCPS::NetworkAddress::default_IPV6,
                                                    DCPS::ConfigStoreImpl::Format_Optional_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV6);
}

void
RtpsDiscoveryConfig::ipv6_spdp_local_address(const DCPS::NetworkAddress& mi)
{
  TheServiceParticipant->config_store()->set(config_key("IPV6_SPDP_LOCAL_ADDRESS").c_str(),
                                             mi,
                                             DCPS::ConfigStoreImpl::Format_Optional_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV6);
}

bool RtpsDiscoveryConfig::ipv6_spdp_unicast_address(DCPS::NetworkAddress& addr, bool& fixed_port,
  DDS::DomainId_t domain, DDS::UInt16 part_id) const
{
  addr = ipv6_spdp_local_address();
  return set_spdp_unicast_port(addr, fixed_port, domain, part_id);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::ipv6_sedp_local_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("IPV6_SEDP_LOCAL_ADDRESS").c_str(),
                                                    DCPS::NetworkAddress::default_IPV6,
                                                    DCPS::ConfigStoreImpl::Format_Required_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV6);
}

void
RtpsDiscoveryConfig::ipv6_sedp_local_address(const DCPS::NetworkAddress& mi)
{
  TheServiceParticipant->config_store()->set(config_key("IPV6_SEDP_LOCAL_ADDRESS").c_str(),
                                             mi,
                                             DCPS::ConfigStoreImpl::Format_Required_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV6);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::ipv6_sedp_advertised_local_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("IPV6_SEDP_ADVERTISED_LOCAL_ADDRESS").c_str(),
                                                    DCPS::NetworkAddress::default_IPV6,
                                                    DCPS::ConfigStoreImpl::Format_Required_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV6);
}

void
RtpsDiscoveryConfig::ipv6_sedp_advertised_local_address(const DCPS::NetworkAddress& mi)
{
  TheServiceParticipant->config_store()->set(config_key("IPV6_SEDP_ADVERTISED_LOCAL_ADDRESS").c_str(),
                                             mi,
                                             DCPS::ConfigStoreImpl::Format_Required_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV6);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::ipv6_default_multicast_group() const
{
  return TheServiceParticipant->config_store()->get(config_key("IPV6_DEFAULT_MULTICAST_GROUP").c_str(),
                                                    DCPS::NetworkAddress("[FF03::1]:0"),
                                                    DCPS::ConfigStoreImpl::Format_No_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV6);
}

void
RtpsDiscoveryConfig::ipv6_default_multicast_group(const DCPS::NetworkAddress& group)
{
  TheServiceParticipant->config_store()->set(config_key("IPV6_DEFAULT_MULTICAST_GROUP").c_str(),
                                             group,
                                             DCPS::ConfigStoreImpl::Format_No_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV6);
}

bool RtpsDiscoveryConfig::ipv6_spdp_multicast_address(
  DCPS::NetworkAddress& addr, DDS::DomainId_t domain) const
{
  addr = TheServiceParticipant->config_store()->get(
    config_key("IPV6_SPDP_MULTICAST_ADDRESS").c_str(),
    ipv6_default_multicast_group(),
    DCPS::ConfigStoreImpl::Format_Optional_Port,
    DCPS::ConfigStoreImpl::Kind_IPV6);
  return set_spdp_multicast_port(addr, domain);
}

void RtpsDiscoveryConfig::ipv6_spdp_multicast_address(const DCPS::NetworkAddress& addr)
{
  TheServiceParticipant->config_store()->set(config_key("IPV6_SPDP_MULTICAST_ADDRESS").c_str(),
                                             addr,
                                             DCPS::ConfigStoreImpl::Format_Optional_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV6);
}

void RtpsDiscoveryConfig::ipv6_sedp_multicast_address(const DCPS::NetworkAddress& addr)
{
  TheServiceParticipant->config_store()->set(config_key("IPV6_SEDP_MULTICAST_ADDRESS").c_str(),
                                             addr,
                                             DCPS::ConfigStoreImpl::Format_Optional_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV6);
}
#endif

OPENDDS_STRING
RtpsDiscoveryConfig::guid_interface() const
{
  return TheServiceParticipant->config_store()->get(config_key("GUID_INTERFACE").c_str(),
                                                    "");
}

void
RtpsDiscoveryConfig::guid_interface(const OPENDDS_STRING& gi)
{
  TheServiceParticipant->config_store()->set(config_key("GUID_INTERFACE").c_str(),
                                             gi);
}

DCPS::NetworkAddressSet
RtpsDiscoveryConfig::spdp_send_addrs() const
{
  return TheServiceParticipant->config_store()->get(config_key("SPDP_SEND_ADDRS").c_str(),
                                                    DCPS::NetworkAddressSet(),
                                                    DCPS::ConfigStoreImpl::Format_Required_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void
RtpsDiscoveryConfig::spdp_send_addrs(const DCPS::NetworkAddressSet& addrs)
{
  TheServiceParticipant->config_store()->set(config_key("SPDP_SEND_ADDRS").c_str(),
                                             addrs,
                                             DCPS::ConfigStoreImpl::Format_Required_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::max_auth_time() const
{
  return TheServiceParticipant->config_store()->get(config_key("MAX_AUTH_TIME").c_str(),
                                                    TimeDuration(300 /*seconds*/),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void
RtpsDiscoveryConfig::max_auth_time(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(config_key("MAX_AUTH_TIME").c_str(),
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::auth_resend_period() const
{
  return TheServiceParticipant->config_store()->get(config_key("AUTH_RESEND_PERIOD").c_str(),
                                                    TimeDuration(1 /*seconds*/),
                                                    DCPS::ConfigStoreImpl::Format_FractionalSeconds);
}

void
RtpsDiscoveryConfig::auth_resend_period(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(config_key("AUTH_RESEND_PERIOD").c_str(),
                                             x,
                                             DCPS::ConfigStoreImpl::Format_FractionalSeconds);
}

u_short
RtpsDiscoveryConfig::max_spdp_sequence_msg_reset_checks() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("MAX_SPDP_SEQUENCE_MSG_RESET_CHECKS").c_str(),
                                                           3);
}

void
RtpsDiscoveryConfig::max_spdp_sequence_msg_reset_checks(u_short reset_value)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("MAX_SPDP_SEQUENCE_MSG_RESET_CHECKS").c_str(),
                                                    reset_value);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::spdp_rtps_relay_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("SPDP_RTPS_RELAY_ADDRESS").c_str(),
                                                    DCPS::NetworkAddress::default_IPV4,
                                                    DCPS::ConfigStoreImpl::Format_Required_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void
RtpsDiscoveryConfig::spdp_rtps_relay_address(const DCPS::NetworkAddress& address)
{
  TheServiceParticipant->config_store()->set(config_key("SPDP_RTPS_RELAY_ADDRESS").c_str(),
                                             address,
                                             DCPS::ConfigStoreImpl::Format_Required_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::spdp_rtps_relay_send_period() const
{
  return TheServiceParticipant->config_store()->get(config_key("SPDP_RTPS_RELAY_SEND_PERIOD").c_str(),
                                                    TimeDuration(30 /*seconds*/),
                                                    DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

void
RtpsDiscoveryConfig::spdp_rtps_relay_send_period(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("SPDP_RTPS_RELAY_SEND_PERIOD").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_IntegerSeconds);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::sedp_rtps_relay_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_RTPS_RELAY_ADDRESS").c_str(),
                                                    DCPS::NetworkAddress::default_IPV4,
                                                    DCPS::ConfigStoreImpl::Format_Required_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void
RtpsDiscoveryConfig::sedp_rtps_relay_address(const DCPS::NetworkAddress& address)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_RTPS_RELAY_ADDRESS").c_str(),
                                             address,
                                             DCPS::ConfigStoreImpl::Format_Required_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

bool
RtpsDiscoveryConfig::use_rtps_relay() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("USE_RTPS_RELAY").c_str(),
                                                            false);
}

void
RtpsDiscoveryConfig::use_rtps_relay(bool f)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("USE_RTPS_RELAY").c_str(),
                                                     f);
}

bool
RtpsDiscoveryConfig::rtps_relay_only() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("RTPS_RELAY_ONLY").c_str(),
                                                            false);
}

void
RtpsDiscoveryConfig::rtps_relay_only(bool f)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("RTPS_RELAY_ONLY").c_str(),
                                                     f);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::spdp_stun_server_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("SPDP_STUN_SERVER_ADDRESS").c_str(),
                                                    DCPS::NetworkAddress::default_IPV4,
                                                    DCPS::ConfigStoreImpl::Format_Required_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void
RtpsDiscoveryConfig::spdp_stun_server_address(const DCPS::NetworkAddress& address)
{
  TheServiceParticipant->config_store()->set(config_key("SPDP_STUN_SERVER_ADDRESS").c_str(),
                                             address,
                                             DCPS::ConfigStoreImpl::Format_Required_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

DCPS::NetworkAddress
RtpsDiscoveryConfig::sedp_stun_server_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_STUN_SERVER_ADDRESS").c_str(),
                                                    DCPS::NetworkAddress::default_IPV4,
                                                    DCPS::ConfigStoreImpl::Format_Required_Port,
                                                    DCPS::ConfigStoreImpl::Kind_IPV4);
}

void
RtpsDiscoveryConfig::sedp_stun_server_address(const DCPS::NetworkAddress& address)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_STUN_SERVER_ADDRESS").c_str(),
                                             address,
                                             DCPS::ConfigStoreImpl::Format_Required_Port,
                                             DCPS::ConfigStoreImpl::Kind_IPV4);
}

#if OPENDDS_CONFIG_SECURITY
bool
RtpsDiscoveryConfig::use_ice() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("USE_ICE").c_str(),
                                                            false);
}

void
RtpsDiscoveryConfig::use_ice(bool ui)
{
  if (ui && !TheServiceParticipant->get_security() && log_level >= LogLevel::Warning) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING RtpsDiscoveryConfig::use_ice:  Security must be enabled (-DCPSSecurity 1) when using ICE\n"));
  }
  TheServiceParticipant->config_store()->set_boolean(config_key("USE_ICE").c_str(),
                                                     ui);
}
#endif

size_t
RtpsDiscoveryConfig::sedp_max_message_size() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("SEDP_MAX_MESSAGE_SIZE").c_str(),
                                                           DCPS::TransportSendStrategy::UDP_MAX_MESSAGE_SIZE);
}

void
RtpsDiscoveryConfig::sedp_max_message_size(size_t value)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("SEDP_MAX_MESSAGE_SIZE").c_str(),
                                                    static_cast<DDS::UInt32>(value));
}

bool
RtpsDiscoveryConfig::undirected_spdp() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("UNDIRECTED_SPDP").c_str(),
                                                            true);
}

void
RtpsDiscoveryConfig::undirected_spdp(bool value)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("UNDIRECTED_SPDP").c_str(),
                                                     value);
}

bool
RtpsDiscoveryConfig::periodic_directed_spdp() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("PERIODIC_DIRECTED_SPDP").c_str(),
                                                            false);
}

void
RtpsDiscoveryConfig::periodic_directed_spdp(bool value)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("PERIODIC_DIRECTED_SPDP").c_str(),
                                                     value);
}

bool
RtpsDiscoveryConfig::secure_participant_user_data() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("SECURE_PARTICIPANT_USER_DATA").c_str(),
                                                            false);
}

void
RtpsDiscoveryConfig::secure_participant_user_data(bool value)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("SECURE_PARTICIPANT_USER_DATA").c_str(),
                                                     value);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::max_type_lookup_service_reply_period() const
{
  return TheServiceParticipant->config_store()->get(config_key("TYPE_LOOKUP_SERVICE_REPLY_TIMEOUT").c_str(),
                                                    TimeDuration(5 /*seconds*/),
                                                    DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
RtpsDiscoveryConfig::max_type_lookup_service_reply_period(const DCPS::TimeDuration& x)
{
  TheServiceParticipant->config_store()->set(config_key("TYPE_LOOKUP_SERVICE_REPLY_TIMEOUT").c_str(),
                                             x,
                                             DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

bool
RtpsDiscoveryConfig::use_xtypes() const
{
  const String x = TheServiceParticipant->config_store()->get(config_key("USE_X_TYPES").c_str(),
                                                              "minimal");
  return x == "1" /* minimal */ || x == "2" /* complete */ || x == "minimal" || x == "complete";
}

void
RtpsDiscoveryConfig::use_xtypes(UseXTypes use_xtypes)
{
  switch (use_xtypes) {
  case XTYPES_NONE:
    TheServiceParticipant->config_store()->set(config_key("USE_X_TYPES").c_str(), "no");
    break;
  case XTYPES_MINIMAL:
    TheServiceParticipant->config_store()->set(config_key("USE_X_TYPES").c_str(), "minimal");
    break;
  case XTYPES_COMPLETE:
    TheServiceParticipant->config_store()->set(config_key("USE_X_TYPES").c_str(), "complete");
    break;
  }
}

bool
RtpsDiscoveryConfig::use_xtypes(const char* str)
{
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
      TheServiceParticipant->config_store()->set(config_key("USE_X_TYPES").c_str(), str);
      return true;
    }
  }
  if (log_level >= LogLevel::Warning) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: RtpsDiscoveryConfig::use_xtypes: "
               "invalid XTypes configuration: %C\n", str));
  }
  return false;
}

bool
RtpsDiscoveryConfig::use_xtypes_complete() const
{
  const String x = TheServiceParticipant->config_store()->get(config_key("USE_X_TYPES").c_str(),
                                                              "minimal");
  return x == "2" || x == "complete";
}

DCPS::TimeDuration
RtpsDiscoveryConfig::sedp_heartbeat_period() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_HEARTBEAT_PERIOD").c_str(),
                                                    TimeDuration(0, 200*1000 /*microseconds*/),
                                                    DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
RtpsDiscoveryConfig::sedp_heartbeat_period(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_HEARTBEAT_PERIOD").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::sedp_nak_response_delay() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_NAK_RESPONSE_DELAY").c_str(),
                                                    TimeDuration(0, 100*1000 /*microseconds*/),
                                                    DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
RtpsDiscoveryConfig::sedp_nak_response_delay(const DCPS::TimeDuration& period)
{
   TheServiceParticipant->config_store()->set(config_key("SEDP_NAK_RESPONSE_DELAY").c_str(),
                                              period,
                                              DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::sedp_send_delay() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_SEND_DELAY").c_str(),
                                                    TimeDuration(0, 10 * 1000 /*microseconds*/),
                                                    DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
RtpsDiscoveryConfig::sedp_send_delay(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_SEND_DELAY").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::sedp_passive_connect_duration() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_PASSIVE_CONNECT_DURATION").c_str(),
                                                    TimeDuration::from_msec(DCPS::TransportConfig::DEFAULT_PASSIVE_CONNECT_DURATION),
                                                    DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
RtpsDiscoveryConfig::sedp_passive_connect_duration(const DCPS::TimeDuration& period)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_PASSIVE_CONNECT_DURATION").c_str(),
                                             period,
                                             DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

DCPS::TimeDuration
RtpsDiscoveryConfig::sedp_fragment_reassembly_timeout() const
{
  return TheServiceParticipant->config_store()->get(config_key("SEDP_FRAGMENT_REASSEMBLY_TIMEOUT").c_str(),
                                                    TimeDuration(),
                                                    DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
RtpsDiscoveryConfig::sedp_fragment_reassembly_timeout(const DCPS::TimeDuration& timeout)
{
  TheServiceParticipant->config_store()->set(config_key("SEDP_FRAGMENT_REASSEMBLY_TIMEOUT").c_str(),
                                             timeout,
                                             DCPS::ConfigStoreImpl::Format_IntegerMilliseconds);
}

CORBA::ULong
RtpsDiscoveryConfig::participant_flags() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("PARTICIPANT_FLAGS").c_str(),
                                                           PFLAGS_THIS_VERSION);
}

void
RtpsDiscoveryConfig::participant_flags(CORBA::ULong participant_flags)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("PARTICIPANT_FLAGS").c_str(),
                                                    participant_flags);
}

bool
RtpsDiscoveryConfig::sedp_responsive_mode() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("SEDP_RESPONSIVE_MODE").c_str(),
                                                            false);
}

void
RtpsDiscoveryConfig::sedp_responsive_mode(bool srm)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("SEDP_RESPONSIVE_MODE").c_str(),
                                                     srm);
}

size_t
RtpsDiscoveryConfig::sedp_receive_preallocated_message_blocks() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("SEDP_RECEIVE_PREALLOCATED_MESSAGE_BLOCKS").c_str(),
                                                           0);
}

void
RtpsDiscoveryConfig::sedp_receive_preallocated_message_blocks(size_t n)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("SEDP_RECEIVE_PREALLOCATED_MESSAGE_BLOCKS").c_str(),
                                                    static_cast<DDS::UInt32>(n));
}

size_t
RtpsDiscoveryConfig::sedp_receive_preallocated_data_blocks() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("SEDP_RECEIVE_PREALLOCATED_DATA_BLOCKS").c_str(),
                                                           0);
}

void
RtpsDiscoveryConfig::sedp_receive_preallocated_data_blocks(size_t n)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("SEDP_RECEIVE_PREALLOCATED_DATA_BLOCKS").c_str(),
                                                    static_cast<DDS::UInt32>(n));
}

bool
RtpsDiscoveryConfig::check_source_ip() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("CHECK_SOURCE_IP").c_str(),
                                                            true);
}

void
RtpsDiscoveryConfig::check_source_ip(bool flag)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("CHECK_SOURCE_IP").c_str(),
                                                     flag);
}

ACE_CDR::ULong
RtpsDiscoveryConfig::spdp_user_tag() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("SPDP_USER_TAG").c_str(),
                                                           0);
}

void
RtpsDiscoveryConfig::spdp_user_tag(ACE_CDR::ULong tag)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("SPDP_USER_TAG").c_str(),
                                                    tag);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
