/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpInst.h"
#include "UdpLoader.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

#include "ace/Configuration.h"

#include <iostream>

namespace OpenDDS {
namespace DCPS {

UdpInst::UdpInst(const std::string& name)
  : TransportInst("udp", name),
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
    send_buffer_size_(ACE_DEFAULT_MAX_SOCKET_BUFSIZ),
    rcv_buffer_size_(ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
#else
    send_buffer_size_(0),
    rcv_buffer_size_(0)
#endif
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
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("local_address"),
                           local_address_s)
  this->local_address_.set(local_address_s.c_str());

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("send_buffer_size"), this->send_buffer_size_, ACE_UINT32);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("rcv_buffer_size"), this->rcv_buffer_size_, ACE_UINT32);

  return 0;
}

void
UdpInst::dump(std::ostream& os)
{
  TransportInst::dump(os);

  os << formatNameForDump("local_address") << this->local_address_.get_host_addr()
                                           << ":" << this->local_address_.get_port_number() << std::endl;
}

} // namespace DCPS
} // namespace OpenDDS
