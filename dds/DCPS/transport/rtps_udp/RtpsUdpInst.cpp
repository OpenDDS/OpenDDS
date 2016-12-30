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
  , use_multicast_(true)
  , ttl_(1)
  , multicast_group_address_(7401, "239.255.0.2")
  , nak_depth_(32) // default nak_depth in OpenDDS_Multicast
  , nak_response_delay_(0, 200*1000 /*microseconds*/) // default from RTPS
  , heartbeat_period_(1) // no default in RTPS spec
  , heartbeat_response_delay_(0, 500*1000 /*microseconds*/) // default from RTPS
  , handshake_timeout_(30) // default syn_timeout in OpenDDS_Multicast
  , durable_data_timeout_(60)
  , opendds_discovery_guid_(GUID_UNKNOWN)
{
}

TransportImpl_rch
RtpsUdpInst::new_impl(const TransportInst_rch& inst)
{
  return make_rch<RtpsUdpTransport>(inst);
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
  }

  GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("multicast_interface"),
                          multicast_interface_);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("nak_depth"), nak_depth_, size_t);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ttl"), ttl_, unsigned char);

  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("nak_response_delay"),
                        nak_response_delay_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("heartbeat_period"),
                        heartbeat_period_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("heartbeat_response_delay"),
                        heartbeat_response_delay_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("handshake_timeout"),
                        handshake_timeout_);
  return 0;
}

OPENDDS_STRING
RtpsUdpInst::dump_to_str()
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
  ret += formatNameForDump("nak_response_delay") + to_dds_string(nak_response_delay_.msec()) + '\n';
  ret += formatNameForDump("heartbeat_period") + to_dds_string(heartbeat_period_.msec()) + '\n';
  ret += formatNameForDump("heartbeat_response_delay") + to_dds_string(heartbeat_response_delay_.msec()) + '\n';
  ret += formatNameForDump("handshake_timeout") + to_dds_string(handshake_timeout_.msec()) + '\n';
  return ret;
}

size_t
RtpsUdpInst::populate_locator(OpenDDS::DCPS::TransportLocator& info) const
{
  using namespace OpenDDS::RTPS;

  LocatorSeq locators;
  CORBA::ULong idx = 0;

  // multicast first so it's preferred by remote peers
  if (this->use_multicast_ && this->multicast_group_address_ != ACE_INET_Addr()) {
    idx = locators.length();
    locators.length(idx + 1);
    locators[idx].kind = address_to_kind(this->multicast_group_address_);
    locators[idx].port = this->multicast_group_address_.get_port_number();
    RTPS::address_to_bytes(locators[idx].address,
                           this->multicast_group_address_);
  }

  //if local_address_string is empty, or only the port has been set
  //need to get interface addresses to populate into the locator
  if (this->local_address_string().empty() ||
      this->local_address_string().rfind(':') == 0) {
    typedef OPENDDS_VECTOR(ACE_INET_Addr) AddrVector;
    AddrVector addrs;
    if (TheServiceParticipant->default_address ().empty ()) {
      get_interface_addrs(addrs);
    } else {
      addrs.push_back (ACE_INET_Addr (static_cast<u_short> (0), TheServiceParticipant->default_address ().c_str ()));
    }
    for (AddrVector::iterator adr_it = addrs.begin(); adr_it != addrs.end(); ++adr_it) {
      idx = locators.length();
      locators.length(idx + 1);
      locators[idx].kind = address_to_kind(*adr_it);
      locators[idx].port = this->local_address().get_port_number();
      RTPS::address_to_bytes(locators[idx].address, *adr_it);
    }
  } else {
    idx = locators.length();
    locators.length(idx + 1);
    locators[idx].kind = address_to_kind(this->local_address());
    locators[idx].port = this->local_address().get_port_number();
    RTPS::address_to_bytes(locators[idx].address,
                           this->local_address());
  }

  info.transport_type = "rtps_udp";
  RTPS::locators_to_blob(locators, info.data);

  return locators.length();
}

const TransportBLOB*
RtpsUdpInst::get_blob(const OpenDDS::DCPS::TransportLocatorSeq& trans_info) const
{
  for (CORBA::ULong idx = 0, limit = trans_info.length(); idx != limit; ++idx) {
    if (std::strcmp(trans_info[idx].transport_type, "rtps_udp") == 0) {
      return &trans_info[idx].data;
    }
  }

  return 0;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
