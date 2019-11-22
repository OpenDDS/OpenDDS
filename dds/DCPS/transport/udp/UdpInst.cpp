/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpInst.h"
#include "UdpLoader.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"

#include "ace/Configuration.h"

#include <iostream>
#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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

TransportImpl_rch
UdpInst::new_impl()
{
  return make_rch<UdpTransport>(ref(*this));
}

int
UdpInst::load(ACE_Configuration_Heap& cf,
              ACE_Configuration_Section_Key& sect)
{
  TransportInst::load(cf, sect); // delegate to parent

  ACE_TString local_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("local_address"),
                           local_address_s);

  if (!local_address_s.empty()) {
    local_address(ACE_TEXT_ALWAYS_CHAR(local_address_s.c_str()));
  }

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("send_buffer_size"), this->send_buffer_size_, ACE_UINT32);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("rcv_buffer_size"), this->rcv_buffer_size_, ACE_UINT32);

  return 0;
}

OPENDDS_STRING
UdpInst::dump_to_str() const
{
  std::ostringstream os;
  os << TransportInst::dump_to_str() << std::endl;

  os << formatNameForDump("local_address") << this->local_address().get_host_addr()
                                           << ":" << this->local_address().get_port_number() << std::endl;
  os << formatNameForDump("send_buffer_size") << this->send_buffer_size_ << std::endl;
  os << formatNameForDump("rcv_buffer_size") << this->rcv_buffer_size_ << std::endl;
  return OPENDDS_STRING(os.str());
}

size_t
UdpInst::populate_locator(OpenDDS::DCPS::TransportLocator& info, ConnectionInfoFlags) const
{
  if (this->local_address() != ACE_INET_Addr()) {
    NetworkAddress network_address;
    if (!this->local_address_string().empty()) {
      network_address = NetworkAddress(this->local_address_string());
    } else {
      network_address = NetworkAddress(this->local_address(), true);
    }
    ACE_OutputCDR cdr;
    cdr << network_address;

    const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
    char* buffer = const_cast<char*>(cdr.buffer()); // safe

    info.transport_type = "udp";
    info.data = TransportBLOB(len, len, reinterpret_cast<CORBA::Octet*>(buffer));
    return 1;
  } else {
    return 0;
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
