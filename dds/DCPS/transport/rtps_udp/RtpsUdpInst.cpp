/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpInst.h"
#include "RtpsUdpLoader.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

#include "ace/Configuration.h"

#include <iostream>

namespace OpenDDS {
namespace DCPS {

RtpsUdpInst::RtpsUdpInst(const std::string& name)
  : TransportInst("rtps_udp", name)
  , use_multicast_(true)
  , multicast_group_address_(7401, "239.255.0.1")
  , nak_depth_(32) // default nak_depth in OpenDDS_Multicast
  , nak_response_delay_(0, 200*1000 /*microseconds*/) // default from RTPS
  , heartbeat_period_(1) // no default in RTPS spec
  , heartbeat_response_delay_(0, 500*1000 /*microseconds*/) // default from RTPS
{
}

RtpsUdpTransport*
RtpsUdpInst::new_impl(const TransportInst_rch& inst)
{
  return new RtpsUdpTransport(inst);
}

int
RtpsUdpInst::load(ACE_Configuration_Heap& cf,
                  ACE_Configuration_Section_Key& sect)
{
  TransportInst::load(cf, sect); // delegate to parent

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
  return 0;
}

void
RtpsUdpInst::dump(std::ostream& os)
{
  TransportInst::dump(os);
  const std::ios::fmtflags flags = os.setf(ios::boolalpha);
  os << formatNameForDump("local_address") << local_address_.get_host_addr()
     << ':' << local_address_.get_port_number() << '\n'

     << formatNameForDump("use_multicast") << use_multicast_ << '\n'
     << formatNameForDump("multicast_group_address")
     << multicast_group_address_.get_host_addr() << ':'
     << multicast_group_address_.get_port_number() << '\n'

     << formatNameForDump("nak_depth") << nak_depth_ << '\n'
     << formatNameForDump("nak_response_delay") << nak_response_delay_.msec()
     << '\n'
     << formatNameForDump("heartbeat_period") << heartbeat_period_.msec()
     << '\n'
     << formatNameForDump("heartbeat_response_delay")
     << heartbeat_response_delay_.msec() << std::endl;
  os.flags(flags);
}

} // namespace DCPS
} // namespace OpenDDS
