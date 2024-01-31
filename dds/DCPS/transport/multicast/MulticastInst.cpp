/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastInst.h"
#include "MulticastLoader.h"
#include "MulticastTransport.h"

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/transport/framework/TransportDefs.h>

#include <ace/Configuration.h>

#include <iostream>
#include <sstream>

namespace {

const bool DEFAULT_TO_IPV6(false);

const u_short DEFAULT_PORT_OFFSET(49152);

const char* DEFAULT_IPV4_GROUP_ADDRESS("224.0.0.128");
const char* DEFAULT_IPV6_GROUP_ADDRESS("FF01::80");

const bool DEFAULT_RELIABLE(true);

const double DEFAULT_SYN_BACKOFF(2.0);
const long DEFAULT_SYN_INTERVAL(250);
const long DEFAULT_SYN_TIMEOUT(30000);

const unsigned char DEFAULT_TTL(1);
const bool DEFAULT_ASYNC_SEND(false);

} // namespace

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

MulticastInst::MulticastInst(const std::string& name)
  : TransportInst("multicast", name)
  , default_to_ipv6_(*this, &MulticastInst::default_to_ipv6, &MulticastInst::default_to_ipv6)
  , port_offset_(*this, &MulticastInst::port_offset, &MulticastInst::port_offset)
  , group_address_(*this, &MulticastInst::group_address, &MulticastInst::group_address)
  , local_address_(*this, &MulticastInst::local_address, &MulticastInst::local_address)
  , reliable_(*this, &MulticastInst::reliable, &MulticastInst::reliable)
  , syn_backoff_(*this, &MulticastInst::syn_backoff, &MulticastInst::syn_backoff)
  , syn_interval_(*this, &MulticastInst::syn_interval, &MulticastInst::syn_interval)
  , syn_timeout_(*this, &MulticastInst::syn_timeout, &MulticastInst::syn_timeout)
  , nak_depth_(*this, &MulticastInst::nak_depth, &MulticastInst::nak_depth)
  , nak_interval_(*this, &MulticastInst::nak_interval, &MulticastInst::nak_interval)
  , nak_delay_intervals_(*this, &MulticastInst::nak_delay_intervals, &MulticastInst::nak_delay_intervals)
  , nak_max_(*this, &MulticastInst::nak_max, &MulticastInst::nak_max)
  , nak_timeout_(*this, &MulticastInst::nak_timeout, &MulticastInst::nak_timeout)
  , ttl_(*this, &MulticastInst::ttl, &MulticastInst::ttl)
  , rcv_buffer_size_(*this, &MulticastInst::rcv_buffer_size, &MulticastInst::rcv_buffer_size)
  , async_send_(*this, &MulticastInst::async_send, &MulticastInst::async_send)
{}

TransportImpl_rch
MulticastInst::new_impl(DDS::DomainId_t domain)
{
  return make_rch<MulticastTransport>(rchandle_from(this), domain);
}

OPENDDS_STRING
MulticastInst::dump_to_str(DDS::DomainId_t domain) const
{
  std::ostringstream os;
  os << TransportInst::dump_to_str(domain);

  os << formatNameForDump("group_address")       << LogAddr(group_address()).str() << std::endl;
  os << formatNameForDump("local_address")       << this->local_address() << std::endl;
  os << formatNameForDump("default_to_ipv6")     << (this->default_to_ipv6() ? "true" : "false") << std::endl;
  os << formatNameForDump("port_offset")         << this->port_offset() << std::endl;
  os << formatNameForDump("reliable")            << (this->reliable() ? "true" : "false") << std::endl;
  os << formatNameForDump("syn_backoff")         << this->syn_backoff() << std::endl;
  os << formatNameForDump("syn_interval")        << this->syn_interval().str() << std::endl;
  os << formatNameForDump("syn_timeout")         << this->syn_timeout().str() << std::endl;
  os << formatNameForDump("nak_depth")           << this->nak_depth() << std::endl;
  os << formatNameForDump("nak_interval")        << this->nak_interval().str() << std::endl;
  os << formatNameForDump("nak_delay_intervals") << this->nak_delay_intervals() << std::endl;
  os << formatNameForDump("nak_max")             << this->nak_max() << std::endl;
  os << formatNameForDump("nak_timeout")         << this->nak_timeout().str() << std::endl;
  os << formatNameForDump("ttl")                 << int(this->ttl()) << std::endl;
  os << formatNameForDump("rcv_buffer_size");

  if (this->rcv_buffer_size() == 0) {
    os << "System Default Value" << std::endl;
  } else {
    os << this->rcv_buffer_size() << std::endl;
  }

  os << formatNameForDump("async_send");

#if defined (ACE_WIN32) && defined (ACE_HAS_WIN32_OVERLAPPED_IO)
  os << (this->async_send() ? "true" : "false") << std::endl;
#else
  os << "Not Supported on this Platform" << std::endl;
#endif
  return OPENDDS_STRING(os.str());
}

size_t
MulticastInst::populate_locator(OpenDDS::DCPS::TransportLocator& info,
                                ConnectionInfoFlags,
                                DDS::DomainId_t) const
{
  const NetworkAddress ga = group_address();
  if (ga != NetworkAddress::default_IPV4
#ifdef ACE_HAS_IPV6
      && ga != NetworkAddress::default_IPV6
#endif
      ) {
    NetworkResource network_resource(ga.to_addr());

    ACE_OutputCDR cdr;
    cdr << network_resource;
    cdr << ACE_OutputCDR::from_boolean (ACE_CDR::Boolean (is_reliable()));

    const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
    char* buffer = const_cast<char*>(cdr.buffer()); // safe

    info.transport_type = "multicast";
    info.data = TransportBLOB(len, len, reinterpret_cast<CORBA::Octet*>(buffer));
    return 1;
  } else {
    return 0;
  }
}

void
MulticastInst::default_to_ipv6(bool flag)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("DEFAULT_TO_IPV6").c_str(), flag);
}

bool
MulticastInst::default_to_ipv6() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("DEFAULT_TO_IPV6").c_str(), DEFAULT_TO_IPV6);
}

void
MulticastInst::port_offset(u_short po)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("PORT_OFFSET").c_str(), po);
}

u_short
MulticastInst::port_offset() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("PORT_OFFSET").c_str(), DEFAULT_PORT_OFFSET);
}

void
MulticastInst::group_address(const NetworkAddress& na)
{
  TheServiceParticipant->config_store()->set(config_key("GROUP_ADDRESS").c_str(),
                                             na,
                                             ConfigStoreImpl::Format_Required_Port,
                                             ConfigStoreImpl::Kind_ANY);
}

NetworkAddress
MulticastInst::group_address() const
{
  ACE_INET_Addr default_group_address;
  if (default_to_ipv6()) {
    default_group_address.set(port_offset(), DEFAULT_IPV6_GROUP_ADDRESS);
  } else {
    default_group_address.set(port_offset(), DEFAULT_IPV4_GROUP_ADDRESS);
  }

  return TheServiceParticipant->config_store()->get(config_key("GROUP_ADDRESS").c_str(),
                                                    NetworkAddress(default_group_address),
                                                    ConfigStoreImpl::Format_Required_Port,
                                                    ConfigStoreImpl::Kind_ANY);
}

void
MulticastInst::local_address(const String& la)
{
  TheServiceParticipant->config_store()->set(config_key("LOCAL_ADDRESS").c_str(), la);
}

String
MulticastInst::local_address() const
{
  String la = TheServiceParticipant->config_store()->get(config_key("LOCAL_ADDRESS").c_str(), "");
  // Override with DCPSDefaultAddress.
  if (la.empty() &&
      TheServiceParticipant->default_address() != NetworkAddress::default_IPV4) {
    char buffer[INET6_ADDRSTRLEN];
    la = TheServiceParticipant->default_address().to_addr().get_host_addr(buffer, sizeof buffer);
  }
  return la;
}

void
MulticastInst::reliable(bool flag)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("RELIABLE").c_str(), flag);
}

bool
MulticastInst::reliable() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("RELIABLE").c_str(), DEFAULT_RELIABLE);
}

void
MulticastInst::syn_backoff(double sb)
{
  TheServiceParticipant->config_store()->set_float64(config_key("SYN_BACKOFF").c_str(), sb);
}

double
MulticastInst::syn_backoff() const
{
  return TheServiceParticipant->config_store()->get_float64(config_key("SYN_BACKOFF").c_str(), DEFAULT_SYN_BACKOFF);
}

void
MulticastInst::syn_interval(const TimeDuration& si)
{
  TheServiceParticipant->config_store()->set(config_key("SYN_INTERVAL").c_str(),
                                             si,
                                             ConfigStoreImpl::Format_IntegerMilliseconds);
}

TimeDuration
MulticastInst::syn_interval() const
{
  return TheServiceParticipant->config_store()->get(config_key("SYN_INTERVAL").c_str(),
                                                    TimeDuration::from_msec(DEFAULT_SYN_INTERVAL),
                                                    ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
MulticastInst::syn_timeout(const TimeDuration& st)
{
  TheServiceParticipant->config_store()->set(config_key("SYN_TIMEOUT").c_str(),
                                             st,
                                             ConfigStoreImpl::Format_IntegerMilliseconds);
}

TimeDuration
MulticastInst::syn_timeout() const
{
  return TheServiceParticipant->config_store()->get(config_key("SYN_TIMEOUT").c_str(),
                                                    TimeDuration::from_msec(DEFAULT_SYN_TIMEOUT),
                                                    ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
MulticastInst::nak_depth(size_t nd)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("NAK_DEPTH").c_str(), static_cast<DDS::UInt32>(nd));
}

size_t
MulticastInst::nak_depth() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("NAK_DEPTH").c_str(), DEFAULT_NAK_DEPTH);
}

void
MulticastInst::nak_interval(const TimeDuration& ni)
{
  TheServiceParticipant->config_store()->set(config_key("NAK_INTERVAL").c_str(),
                                             ni,
                                             ConfigStoreImpl::Format_IntegerMilliseconds);
}

TimeDuration
MulticastInst::nak_interval() const
{
  return TheServiceParticipant->config_store()->get(config_key("NAK_INTERVAL").c_str(),
                                                    TimeDuration::from_msec(DEFAULT_NAK_INTERVAL),
                                                    ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
MulticastInst::nak_delay_intervals(size_t ndi)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("NAK_DELAY_INTERVALS").c_str(),
                                                    static_cast<DDS::UInt32>(ndi));
}

size_t
MulticastInst::nak_delay_intervals() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("NAK_DELAY_INTERVALS").c_str(),
                                                           DEFAULT_NAK_DELAY_INTERVALS);
}

void
MulticastInst::nak_max(size_t nm)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("NAK_MAX").c_str(), static_cast<DDS::UInt32>(nm));
}

size_t
MulticastInst::nak_max() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("NAK_MAX").c_str(), DEFAULT_NAK_MAX);
}

void
MulticastInst::nak_timeout(const TimeDuration& nt)
{
  TheServiceParticipant->config_store()->set(config_key("NAK_TIMEOUT").c_str(),
                                             nt,
                                             ConfigStoreImpl::Format_IntegerMilliseconds);
}

TimeDuration
MulticastInst::nak_timeout() const
{
  return TheServiceParticipant->config_store()->get(config_key("NAK_TIMEOUT").c_str(),
                                                    TimeDuration::from_msec(DEFAULT_NAK_TIMEOUT),
                                                    ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
MulticastInst::ttl(unsigned char t)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("TTL").c_str(), t);
}

unsigned char
MulticastInst::ttl() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("TTL").c_str(), DEFAULT_TTL);
}

void
MulticastInst::rcv_buffer_size(size_t rbs)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("RCV_BUFFER_SIZE").c_str(), static_cast<DDS::UInt32>(rbs));
}

size_t
MulticastInst::rcv_buffer_size() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("RCV_BUFFER_SIZE").c_str(),
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
                                                           ACE_DEFAULT_MAX_SOCKET_BUFSIZ
#else
                                                           // Use system default values.
                                                           0
#endif
                                                           );
}

void
MulticastInst::async_send(bool flag)
{
  ACE_UNUSED_ARG(flag);
#if defined (ACE_WIN32) && defined (ACE_HAS_WIN32_OVERLAPPED_IO)
  TheServiceParticipant->config_store()->set_boolean(config_key("ASYNC_SEND").c_str(), flag);
#endif
}

bool
MulticastInst::async_send() const
{
#if defined (ACE_WIN32) && defined (ACE_HAS_WIN32_OVERLAPPED_IO)
  return TheServiceParticipant->config_store()->get_boolean(config_key("ASYNC_SEND").c_str(), DEFAULT_ASYNC_SEND);
#else
  ACE_UNUSED_ARG(DEFAULT_ASYNC_SEND);
  return false;
#endif
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
