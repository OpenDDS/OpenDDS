/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpInst.h"
#include "UdpLoader.h"

#include "dds/DCPS/transport/framework/NullSynchStrategy.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"

#include "ace/Configuration.h"

#include <iostream>

namespace OpenDDS {
namespace DCPS {

UdpInst::UdpInst(const std::string& name)
  : TransportInst("udp", name, new NullSynchStrategy)
{
}

UdpTransport*
UdpInst::new_impl(const TransportInst_rch& inst)
{
  return new UdpTransport(inst);
}

int
UdpInst::load(ACE_Configuration_Heap& cf,
              ACE_Configuration_Section_Key& sect)
{
  TransportInst::load(cf, sect); // delegate to parent

  ACE_TString local_address_s;
  GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("local_address"),
                          local_address_s)
  this->local_address_.set(local_address_s.c_str());

  return 0;
}

void
UdpInst::dump(std::ostream& os)
{
  TransportInst::dump(os);

  os << formatNameForDump(ACE_TEXT("local_address")) << this->local_address_.get_host_addr()
                                                     << ":" << this->local_address_.get_port_number() << std::endl;
}

} // namespace DCPS
} // namespace OpenDDS
