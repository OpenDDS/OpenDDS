/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpInst.h"
#include "UdpLoader.h"

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/transport/framework/TransportDefs.h>
#include <dds/DCPS/NetworkResource.h>

#include <ace/Configuration.h>

#include <iostream>
#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

UdpInst::UdpInst(const std::string& name)
  : TransportInst("udp", name)
  , send_buffer_size_(*this, &UdpInst::send_buffer_size, &UdpInst::send_buffer_size)
  , rcv_buffer_size_(*this, &UdpInst::rcv_buffer_size, &UdpInst::rcv_buffer_size)
{
}

TransportImpl_rch
UdpInst::new_impl(DDS::DomainId_t domain)
{
  return make_rch<UdpTransport>(rchandle_from(this), domain);
}

OPENDDS_STRING
UdpInst::dump_to_str(DDS::DomainId_t domain) const
{
  std::ostringstream os;
  os << TransportInst::dump_to_str(domain);

  os << formatNameForDump("local_address") << local_address() << std::endl;
  os << formatNameForDump("send_buffer_size") << this->send_buffer_size() << std::endl;
  os << formatNameForDump("rcv_buffer_size") << this->rcv_buffer_size() << std::endl;
  return OPENDDS_STRING(os.str());
}

size_t
UdpInst::populate_locator(OpenDDS::DCPS::TransportLocator& info,
                          ConnectionInfoFlags,
                          DDS::DomainId_t) const
{
  const std::string locator_addr = get_locator_address();
  if (!locator_addr.empty()) {
    NetworkResource network_resource(locator_addr);
    ACE_OutputCDR cdr;
    cdr << network_resource;

    const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
    char* buffer = const_cast<char*>(cdr.buffer()); // safe

    info.transport_type = "udp";
    info.data = TransportBLOB(len, len, reinterpret_cast<CORBA::Octet*>(buffer));
    return 1;
  } else {
    return 0;
  }
}

void
UdpInst::send_buffer_size(ACE_INT32 sbs)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("SEND_BUFFER_SIZE").c_str(), sbs);
}

ACE_INT32
UdpInst::send_buffer_size() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("SEND_BUFFER_SIZE").c_str(),
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
                                                           ACE_DEFAULT_MAX_SOCKET_BUFSIZ
#else
                                                           0
#endif
                                                           );
}

void
UdpInst::rcv_buffer_size(ACE_INT32 sbs)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("RCV_BUFFER_SIZE").c_str(), sbs);
}

ACE_INT32
UdpInst::rcv_buffer_size() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("RCV_BUFFER_SIZE").c_str(),
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
                                                           ACE_DEFAULT_MAX_SOCKET_BUFSIZ
#else
                                                           0
#endif
                                                           );
}

void
UdpInst::local_address(const String& la)
{
  TheServiceParticipant->config_store()->set(config_key("LOCAL_ADDRESS").c_str(), la);
}

String
UdpInst::local_address() const
{
  return TheServiceParticipant->config_store()->get(config_key("LOCAL_ADDRESS").c_str(), "");
}

ACE_INET_Addr
UdpInst::send_receive_address() const
{
  ACE_INET_Addr addr = choose_single_coherent_address(local_address(), false);
  // Override with DCPSDefaultAddress.
  if (addr == ACE_INET_Addr() &&
      TheServiceParticipant->default_address() != NetworkAddress::default_IPV4) {
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) UdpInst::accept_address overriding with DCPSDefaultAddress\n")), 2);
    addr = TheServiceParticipant->default_address().to_addr();
  }

  return addr;
}

bool
UdpInst::set_locator_address(const ACE_INET_Addr& address)
{
  const ACE_INET_Addr local_addr = send_receive_address();
  const unsigned short port = address.get_port_number();

  // If listening on "any" host/port, need to record the actual port number
  // selected by the OS, as well as our actual hostname, into the config_
  // object's local_address_ for use in UdpTransport::connection_info_i().
  if (local_addr.is_any()) {
    const std::string hostname = get_fully_qualified_hostname();
    locator_address_ = hostname + ":" + to_dds_string(port);

    const ACE_INET_Addr addr = choose_single_coherent_address(locator_address_);
    if (addr == ACE_INET_Addr()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Failed to resolve a local address using fully qualified hostname '%C'\n"),
                 hostname.c_str()));
      return false;
    }
  } else if (local_addr.get_port_number() == 0) {
    // Similar case to the "if" case above, but with a bound host/IP but no port
    locator_address_ = local_address();
    set_port_in_addr_string(locator_address_, port);
  } else {
    locator_address_ = local_address();
  }

  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
