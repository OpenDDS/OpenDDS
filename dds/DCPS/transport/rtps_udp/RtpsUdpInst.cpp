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
  , heartbeat_response_delay_(0, 500*1000 /*microseconds*/) // default from RTPS
  , heartbeat_period_(1) // no default in RTPS spec
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

  return 0;
}

void
RtpsUdpInst::dump(std::ostream& os)
{
  TransportInst::dump(os);
}

} // namespace DCPS
} // namespace OpenDDS
