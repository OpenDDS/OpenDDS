/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpInst.h"
#include "RtpsUdpLoader.h"
#include "RtpsUdpTransport.h"

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/transport/framework/TransportDefs.h>
#include <dds/DCPS/RTPS/BaseMessageUtils.h>

#include <ace/Configuration.h>

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsUdpInst::RtpsUdpInst(const OPENDDS_STRING& name)
  : TransportInst("rtps_udp", name)
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  , send_buffer_size_(ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  , rcv_buffer_size_(ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
#else
  , send_buffer_size_(0)
  , rcv_buffer_size_(0)
#endif
  , use_multicast_(true)
  , ttl_(1)
  , anticipated_fragments_(RtpsUdpSendStrategy::UDP_MAX_MESSAGE_SIZE / RtpsSampleHeader::FRAG_SIZE)
  , max_message_size_(RtpsUdpSendStrategy::UDP_MAX_MESSAGE_SIZE)
  , nak_depth_(0)
  , nak_response_delay_(0, 200*1000 /*microseconds*/) // default from RTPS
  , heartbeat_period_(1) // no default in RTPS spec
  , receive_address_duration_(5)
  , responsive_mode_(false)
  , send_delay_(0, 10 * 1000)
  , opendds_discovery_guid_(GUID_UNKNOWN)
  , multicast_group_address_(7401, "239.255.0.2")
  , local_address_(u_short(0), "0.0.0.0")
#ifdef ACE_HAS_IPV6
  , ipv6_multicast_group_address_(7401, "FF03::2")
  , ipv6_local_address_(u_short(0), "::")
#endif
  , rtps_relay_only_(false)
  , use_rtps_relay_(false)
  , use_ice_(false)
{}

TransportImpl_rch
RtpsUdpInst::new_impl()
{
  return make_rch<RtpsUdpTransport>(ref(*this));
}

int
RtpsUdpInst::load(ACE_Configuration_Heap& cf,
                  ACE_Configuration_Section_Key& sect)
{
  TransportInst::load(cf, sect); // delegate to parent

  ACE_TString local_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("local_address"),
                           local_address_s);
  if (!local_address_s.is_empty()) {
    NetworkAddress addr(local_address_s.c_str());
    local_address(addr);
  }

  ACE_TString advertised_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("advertised_address"),
                           advertised_address_s);
  if (!advertised_address_s.is_empty()) {
    NetworkAddress addr(advertised_address_s.c_str());
    advertised_address(addr);
  }

#ifdef ACE_HAS_IPV6
  ACE_TString ipv6_local_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("ipv6_local_address"),
                           ipv6_local_address_s);
  if (!ipv6_local_address_s.is_empty()) {
    NetworkAddress addr(ipv6_local_address_s.c_str());
    ipv6_local_address(addr);
  }

  ACE_TString ipv6_advertised_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("ipv6_advertised_address"),
                           ipv6_advertised_address_s);
  if (!ipv6_advertised_address_s.is_empty()) {
    NetworkAddress addr(ipv6_advertised_address_s.c_str());
    ipv6_advertised_address(addr);
  }
#endif

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("send_buffer_size"), send_buffer_size_, ACE_UINT32);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("rcv_buffer_size"), rcv_buffer_size_, ACE_UINT32);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("use_multicast"), use_multicast_, bool);

  ACE_TString group_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("multicast_group_address"),
                           group_address_s);
  if (!group_address_s.is_empty()) {
    if (group_address_s.rfind(':') == group_address_s.npos) {
      // Concatenate a port number if the user does not supply one.
      group_address_s += ACE_TEXT(":7401");
    }
    NetworkAddress addr(group_address_s.c_str());
    multicast_group_address(addr);
  }

  GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("multicast_interface"),
                          multicast_interface_);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("anticipated_fragments"), anticipated_fragments_, size_t);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("max_message_size"), max_message_size_, size_t);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("nak_depth"), nak_depth_, size_t);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ttl"), ttl_, unsigned char);

  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("nak_response_delay"),
                        nak_response_delay_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("heartbeat_period"),
                        heartbeat_period_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("send_delay"),
                        send_delay_);

  ACE_TString rtps_relay_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("DataRtpsRelayAddress"),
                           rtps_relay_address_s);
  if (!rtps_relay_address_s.is_empty()) {
    NetworkAddress addr(rtps_relay_address_s.c_str());
    rtps_relay_address(addr);
  }

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("RtpsRelayOnly"), rtps_relay_only_, bool);
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("UseRtpsRelay"), use_rtps_relay_, bool);

  ACE_TString stun_server_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("DataStunServerAddress"),
                           stun_server_address_s);
  if (!stun_server_address_s.is_empty()) {
    NetworkAddress addr(stun_server_address_s.c_str());
    stun_server_address(addr);
  }

#ifdef OPENDDS_SECURITY
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("UseIce"), use_ice_, bool);
  if (use_ice_ && !TheServiceParticipant->get_security()) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Security must be enabled (-DCPSSecurity 1) when using ICE (UseIce)\n")), -1);
  }
#endif

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ResponsiveMode"), responsive_mode_, bool);

  return 0;
}

OPENDDS_STRING
RtpsUdpInst::dump_to_str() const
{
  OPENDDS_STRING ret = TransportInst::dump_to_str();
  ret += formatNameForDump("local_address") + LogAddr(local_address()).str() + '\n';
  ret += formatNameForDump("use_multicast") + (use_multicast_ ? "true" : "false") + '\n';
  ret += formatNameForDump("multicast_group_address") + LogAddr(multicast_group_address_).str() + '\n';
  ret += formatNameForDump("multicast_interface") + multicast_interface_ + '\n';
  ret += formatNameForDump("nak_depth") + to_dds_string(unsigned(nak_depth_)) + '\n';
  ret += formatNameForDump("anticipated_fragments") + to_dds_string(unsigned(anticipated_fragments_)) + '\n';
  ret += formatNameForDump("max_message_size") + to_dds_string(unsigned(max_message_size_)) + '\n';
  ret += formatNameForDump("nak_response_delay") + to_dds_string(nak_response_delay_.value().msec()) + '\n';
  ret += formatNameForDump("heartbeat_period") + to_dds_string(heartbeat_period_.value().msec()) + '\n';
  ret += formatNameForDump("send_buffer_size") + to_dds_string(send_buffer_size_) + '\n';
  ret += formatNameForDump("rcv_buffer_size") + to_dds_string(rcv_buffer_size_) + '\n';
  ret += formatNameForDump("ttl") + to_dds_string(ttl_) + '\n';
  ret += formatNameForDump("responsive_mode") + (responsive_mode_ ? "true" : "false") + '\n';
  return ret;
}

size_t
RtpsUdpInst::populate_locator(TransportLocator& info, ConnectionInfoFlags flags) const
{
  using namespace OpenDDS::RTPS;

  LocatorSeq locators;
  CORBA::ULong idx = 0;

  // multicast first so it's preferred by remote peers
  if ((flags & CONNINFO_MULTICAST) && use_multicast_ && multicast_group_address_ != NetworkAddress()) {
    grow(locators);
    address_to_locator(locators[idx++], multicast_group_address_.to_addr());
  }
#ifdef ACE_HAS_IPV6
  if ((flags & CONNINFO_MULTICAST) && use_multicast_ && ipv6_multicast_group_address_ != NetworkAddress()) {
    grow(locators);
    address_to_locator(locators[idx++], ipv6_multicast_group_address_.to_addr());
  }
#endif

  if (flags & CONNINFO_UNICAST) {
    if (local_address() != NetworkAddress()) {
      if (advertised_address() != NetworkAddress()) {
        grow(locators);
        if (locators[idx].port == 0) {
          locators[idx].port = local_address().get_port_number();
        }
        ++idx;
      } else if (local_address().is_any()) {
        typedef OPENDDS_VECTOR(ACE_INET_Addr) AddrVector;
        AddrVector addrs;
        get_interface_addrs(addrs);
        for (AddrVector::iterator adr_it = addrs.begin(); adr_it != addrs.end(); ++adr_it) {
          if (*adr_it != ACE_INET_Addr() && adr_it->get_type() == AF_INET) {
            grow(locators);
            address_to_locator(locators[idx], *adr_it);
            locators[idx].port = local_address().get_port_number();
            ++idx;
          }
        }
      } else {
        grow(locators);
        address_to_locator(locators[idx++], local_address().to_addr());
      }
    }
#ifdef ACE_HAS_IPV6
    if (ipv6_local_address() != NetworkAddress()) {
      if (ipv6_advertised_address() != NetworkAddress()) {
        grow(locators);
        if (locators[idx].port == 0) {
          locators[idx].port = ipv6_local_address().get_port_number();
        }
        ++idx;
      } else if (ipv6_local_address().is_any()) {
        typedef OPENDDS_VECTOR(ACE_INET_Addr) AddrVector;
        AddrVector addrs;
        get_interface_addrs(addrs);
        for (AddrVector::iterator adr_it = addrs.begin(); adr_it != addrs.end(); ++adr_it) {
          if (*adr_it != ACE_INET_Addr() && adr_it->get_type() == AF_INET6) {
            grow(locators);
            address_to_locator(locators[idx], *adr_it);
            locators[idx].port = ipv6_local_address().get_port_number();
            ++idx;
          }
        }
      } else {
        grow(locators);
        address_to_locator(locators[idx++], ipv6_local_address().to_addr());
      }
    }
#endif
  }

  info.transport_type = "rtps_udp";
  RTPS::locators_to_blob(locators, info.data);

  return locators.length();
}

const TransportBLOB*
RtpsUdpInst::get_blob(const TransportLocatorSeq& trans_info) const
{
  for (CORBA::ULong idx = 0, limit = trans_info.length(); idx != limit; ++idx) {
    if (std::strcmp(trans_info[idx].transport_type, "rtps_udp") == 0) {
      return &trans_info[idx].data;
    }
  }

  return 0;
}

void
RtpsUdpInst::update_locators(const RepoId& remote_id,
                             const TransportLocatorSeq& locators)
{
  TransportImpl_rch imp = impl();
  if (imp) {
    RtpsUdpTransport_rch rtps_impl = static_rchandle_cast<RtpsUdpTransport>(imp);
    rtps_impl->update_locators(remote_id, locators);
  }
}

void
RtpsUdpInst::get_last_recv_locator(const RepoId& remote_id,
                                   TransportLocator& locator)
{
  TransportImpl_rch imp = impl();
  if (imp) {
    RtpsUdpTransport_rch rtps_impl = static_rchandle_cast<RtpsUdpTransport>(imp);
    rtps_impl->get_last_recv_locator(remote_id, locator);
  }
}

void
RtpsUdpInst::rtps_relay_address_change()
{
  TransportImpl_rch imp = impl();
  if (imp) {
    RtpsUdpTransport_rch rtps_impl = static_rchandle_cast<RtpsUdpTransport>(imp);
    rtps_impl->rtps_relay_address_change();
  }
}

void
RtpsUdpInst::append_transport_statistics(TransportStatisticsSequence& seq)
{
  TransportImpl_rch imp = impl();
  if (imp) {
    RtpsUdpTransport_rch rtps_impl = static_rchandle_cast<RtpsUdpTransport>(imp);
    rtps_impl->append_transport_statistics(seq);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
