/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpInst.h"
#include "RtpsUdpLoader.h"
#include "RtpsUdpTransport.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "ace/Configuration.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/Service_Participant.h"

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
  , multicast_group_address_(7401, "239.255.0.2")
  , multicast_group_address_str_("239.255.0.2:7401")
  , nak_depth_(0)
  , max_bundle_size_(TransportSendStrategy::UDP_MAX_MESSAGE_SIZE - RTPS::RTPSHDR_SZ) // default maximum bundled message size is max udp message size (see TransportStrategy) minus RTPS header
  , nak_response_delay_(0, 200*1000 /*microseconds*/) // default from RTPS
  , heartbeat_period_(1) // no default in RTPS spec
  , heartbeat_response_delay_(0, 500*1000 /*microseconds*/) // default from RTPS
  , handshake_timeout_(30) // default syn_timeout in OpenDDS_Multicast
  , durable_data_timeout_(60)
  , rtps_relay_beacon_period_(30)
  , use_rtps_relay_(false)
  , rtps_relay_only_(false)
  , use_ice_(false)
  , opendds_discovery_guid_(GUID_UNKNOWN)
{
}

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
    local_address(ACE_TEXT_ALWAYS_CHAR(local_address_s.c_str()));
  }

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("send_buffer_size"), this->send_buffer_size_, ACE_UINT32);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("rcv_buffer_size"), this->rcv_buffer_size_, ACE_UINT32);


  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("use_multicast"), use_multicast_, bool);

  ACE_TString group_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("multicast_group_address"),
                           group_address_s);
  if (!group_address_s.is_empty()) {
    if (group_address_s.rfind(':') == group_address_s.npos) {
      // Concatenate a port number if the user does not supply one.
      group_address_s += ACE_TEXT(":7401");
    }
    multicast_group_address_.set(group_address_s.c_str());
    multicast_group_address_str_ = ACE_TEXT_ALWAYS_CHAR(group_address_s.c_str());
  }

  GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("multicast_interface"),
                          multicast_interface_);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("nak_depth"), nak_depth_, size_t);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("max_bundle_size"), max_bundle_size_, size_t);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ttl"), ttl_, unsigned char);

  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("nak_response_delay"),
                        nak_response_delay_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("heartbeat_period"),
                        heartbeat_period_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("heartbeat_response_delay"),
                        heartbeat_response_delay_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("handshake_timeout"),
                        handshake_timeout_);

  ACE_TString rtps_relay_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("DataRtpsRelayAddress"),
                           rtps_relay_address_s);
  if (!rtps_relay_address_s.is_empty()) {
    ACE_INET_Addr addr(rtps_relay_address_s.c_str());
    rtps_relay_address(addr);
  }
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("RtpsRelayBeaconPeriod"),
                        rtps_relay_beacon_period_);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("RtpsRelayOnly"), rtps_relay_only_, bool);
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("UseRtpsRelay"), use_rtps_relay_, bool);

  ACE_TString stun_server_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("DataStunServerAddress"),
                           stun_server_address_s);
  if (!stun_server_address_s.is_empty()) {
    ACE_INET_Addr addr(stun_server_address_s.c_str());
    stun_server_address(addr);
  }

#ifdef OPENDDS_SECURITY
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("UseIce"), use_ice_, bool);
  if (use_ice_ && !TheServiceParticipant->get_security()) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Security must be enabled (-DCPSSecurity 1) when using ICE (UseIce)\n")), -1);
  }
#endif

  return 0;
}

OPENDDS_STRING
RtpsUdpInst::dump_to_str() const
{
  OPENDDS_STRING ret;
  // ACE_INET_Addr uses a static buffer for get_host_addr() so we can't
  // directly call it on both local_address_ and multicast_group_address_,
  // since the second call could overwrite the result of the first before the
  // OPENDDS_STRING gets a chance to see it.
  OPENDDS_STRING local;
  OPENDDS_STRING multi;
  const char* loc = local_address().get_host_addr();
  if (loc) {
    local = loc;
  } else {
    local = "NOT_SUPPORTED";
  }
  const char* mul = multicast_group_address_.get_host_addr();
  if (mul) {
    multi = mul;
  } else {
    multi = "NOT_SUPPORTED";
  }
  ret += TransportInst::dump_to_str();
  ret += formatNameForDump("local_address") + local;
  ret += ':' + to_dds_string(local_address().get_port_number()) + '\n';
  ret += formatNameForDump("use_multicast") + (use_multicast_ ? "true" : "false") + '\n';
  ret += formatNameForDump("multicast_group_address") + multi
      + ':' + to_dds_string(multicast_group_address_.get_port_number()) + '\n';
  ret += formatNameForDump("multicast_interface") + multicast_interface_ + '\n';
  ret += formatNameForDump("nak_depth") + to_dds_string(unsigned(nak_depth_)) + '\n';
  ret += formatNameForDump("max_bundle_size") + to_dds_string(unsigned(max_bundle_size_)) + '\n';
  ret += formatNameForDump("nak_response_delay") + to_dds_string(nak_response_delay_.value().msec()) + '\n';
  ret += formatNameForDump("heartbeat_period") + to_dds_string(heartbeat_period_.value().msec()) + '\n';
  ret += formatNameForDump("heartbeat_response_delay") + to_dds_string(heartbeat_response_delay_.value().msec()) + '\n';
  ret += formatNameForDump("handshake_timeout") + to_dds_string(handshake_timeout_.value().msec()) + '\n';
  return ret;
}

size_t
RtpsUdpInst::populate_locator(TransportLocator& info, ConnectionInfoFlags flags) const
{
  using namespace OpenDDS::RTPS;

  LocatorSeq locators;
  CORBA::ULong idx = 0;

  // multicast first so it's preferred by remote peers
  if ((flags & CONNINFO_MULTICAST) && use_multicast_ && multicast_group_address_ != ACE_INET_Addr()) {
    idx = locators.length();
    locators.length(idx + 1);
    locators[idx].kind = address_to_kind(this->multicast_group_address_);
    locators[idx].port = this->multicast_group_address_.get_port_number();
    RTPS::address_to_bytes(locators[idx].address,
                           this->multicast_group_address_);
  }

  //if local_address_string is empty, or only the port has been set
  //need to get interface addresses to populate into the locator
  if (flags & CONNINFO_UNICAST) {
    if (local_address_string().empty() ||
        local_address_string().rfind(':') == 0) {
      typedef OPENDDS_VECTOR(ACE_INET_Addr) AddrVector;
      AddrVector addrs;
      if (TheServiceParticipant->default_address ().empty ()) {
        get_interface_addrs(addrs);
      } else {
        addrs.push_back (ACE_INET_Addr (static_cast<u_short> (0), TheServiceParticipant->default_address().c_str()));
      }
      for (AddrVector::iterator adr_it = addrs.begin(); adr_it != addrs.end(); ++adr_it) {
        idx = locators.length();
        locators.length(idx + 1);
        locators[idx].kind = address_to_kind(*adr_it);
        locators[idx].port = local_address().get_port_number();
        RTPS::address_to_bytes(locators[idx].address, *adr_it);
      }
    } else {
      idx = locators.length();
      locators.length(idx + 1);
      locators[idx].kind = address_to_kind(local_address());
      locators[idx].port = local_address().get_port_number();
      RTPS::address_to_bytes(locators[idx].address,
                             local_address());
    }
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

#ifdef OPENDDS_SECURITY

ICE::AddressListType
RtpsUdpInst::host_addresses() const {
  ICE::AddressListType addresses;

  //if local_address_string is empty, or only the port has been set
  //need to get interface addresses to populate into the locator
  if (this->local_address_string().empty() ||
      this->local_address_string().rfind(':') == 0) {
    if (TheServiceParticipant->default_address ().empty ()) {
      get_interface_addrs(addresses);
    } else {
      addresses.push_back (ACE_INET_Addr (static_cast<u_short> (0), TheServiceParticipant->default_address().c_str()));
    }
  } else {
    addresses.push_back(this->local_address());
  }

  for (ICE::AddressListType::iterator pos = addresses.begin(), limit = addresses.end(); pos != limit; ++pos) {
    pos->set_port_number(this->local_address().get_port_number());
  }

  return addresses;
}

#endif

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

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
