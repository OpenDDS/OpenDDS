/*
 * $Id$
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

#include <iostream>

namespace OpenDDS {
namespace DCPS {

RtpsUdpInst::RtpsUdpInst(const std::string& name)
  : TransportInst("rtps_udp", name)
  , use_multicast_(true)
  , multicast_group_address_(7401, "239.255.0.2")
  , nak_depth_(32) // default nak_depth in OpenDDS_Multicast
  , nak_response_delay_(0, 200*1000 /*microseconds*/) // default from RTPS
  , heartbeat_period_(1) // no default in RTPS spec
  , heartbeat_response_delay_(0, 500*1000 /*microseconds*/) // default from RTPS
  , handshake_timeout_(30) // default syn_timeout in OpenDDS_Multicast
  , durable_data_timeout_(60)
  , opendds_discovery_default_listener_(0)
  , opendds_discovery_guid_(GUID_UNKNOWN)
{
}

TransportImpl*
RtpsUdpInst::new_impl(const TransportInst_rch& inst)
{
  return new RtpsUdpTransport(inst);
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
    local_address_.set(local_address_s.c_str());
  }

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("use_multicast"), use_multicast_, bool);

  ACE_TString group_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("multicast_group_address"),
                           group_address_s);
  if (!group_address_s.is_empty()) {
    multicast_group_address_.set(group_address_s.c_str());
  }

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("nak_depth"), nak_depth_, size_t);

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

void
RtpsUdpInst::dump(std::ostream& os)
{
  // ACE_INET_Addr uses a static buffer for get_host_addr() so we can't
  // directly call it on both local_address_ and multicast_group_address_,
  // since the second call could overwrite the result of the first before the
  // ostream gets a chance to see it.
  const std::string local = local_address_.get_host_addr(),
    multi = multicast_group_address_.get_host_addr();
  TransportInst::dump(os);
  const std::ios::fmtflags flags = os.setf(ios::boolalpha);
  os << formatNameForDump("local_address") << local
     << ':' << local_address_.get_port_number() << '\n'
     << formatNameForDump("use_multicast") << use_multicast_ << '\n'
     << formatNameForDump("multicast_group_address") << multi
     << ':' << multicast_group_address_.get_port_number() << '\n'
     << formatNameForDump("nak_depth") << nak_depth_ << '\n'
     << formatNameForDump("nak_response_delay") << nak_response_delay_.msec()
     << '\n'
     << formatNameForDump("heartbeat_period") << heartbeat_period_.msec()
     << '\n'
     << formatNameForDump("heartbeat_response_delay")
     << heartbeat_response_delay_.msec() << '\n'
     << formatNameForDump("handshake_timeout") << handshake_timeout_.msec()
     << std::endl;
  os.flags(flags);
}

} // namespace DCPS
} // namespace OpenDDS
