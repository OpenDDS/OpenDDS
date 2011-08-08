/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastInst.h"
#include "MulticastLoader.h"
#include "MulticastTransport.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

#include "ace/Configuration.h"

#include <iostream>

namespace {

const bool DEFAULT_TO_IPV6(false);

const u_short DEFAULT_PORT_OFFSET(49400);

const char* DEFAULT_IPV4_GROUP_ADDRESS("224.0.0.128");
const char* DEFAULT_IPV6_GROUP_ADDRESS("FF01::80");

const bool DEFAULT_RELIABLE(true);

const double DEFAULT_SYN_BACKOFF(2.0);
const long DEFAULT_SYN_INTERVAL(250);
const long DEFAULT_SYN_TIMEOUT(30000);

const size_t DEFAULT_NAK_DEPTH(32);
const long DEFAULT_NAK_INTERVAL(500);
const long DEFAULT_NAK_DELAY_INTERVALS(4);
const long DEFAULT_NAK_MAX(3);
const long DEFAULT_NAK_TIMEOUT(30000);

const char DEFAULT_TTL(1);

} // namespace

namespace OpenDDS {
namespace DCPS {

MulticastInst::MulticastInst(const std::string& name)
  : TransportInst("multicast", name),
    default_to_ipv6_(DEFAULT_TO_IPV6),
    port_offset_(DEFAULT_PORT_OFFSET),
    reliable_(DEFAULT_RELIABLE),
    syn_backoff_(DEFAULT_SYN_BACKOFF),
    nak_depth_(DEFAULT_NAK_DEPTH),
    nak_delay_intervals_(DEFAULT_NAK_DELAY_INTERVALS),
    nak_max_(DEFAULT_NAK_MAX),
    ttl_(DEFAULT_TTL),
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
    rcv_buffer_size_(ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
#else
    // Use system default values.
    rcv_buffer_size_(0)
#endif
{
  default_group_address(this->group_address_, 0xFFFFFF08);

  this->syn_interval_.msec(DEFAULT_SYN_INTERVAL);
  this->syn_timeout_.msec(DEFAULT_SYN_TIMEOUT);

  this->nak_interval_.msec(DEFAULT_NAK_INTERVAL);
  this->nak_timeout_.msec(DEFAULT_NAK_TIMEOUT);
}

int
MulticastInst::load(ACE_Configuration_Heap& cf,
                    ACE_Configuration_Section_Key& sect)
{
  TransportInst::load(cf, sect); // delegate to parent

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("default_to_ipv6"),
                   this->default_to_ipv6_, bool)

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("port_offset"),
                   this->port_offset_, u_short)

  ACE_TString group_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("group_address"),
                           group_address_s)
  if (group_address_s.is_empty()) {
    // TODO: Passing 0 instead of transport id.  Does this cause complications?
    default_group_address(this->group_address_, 0);
  } else {
    this->group_address_.set(group_address_s.c_str());
  }

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("reliable"), this->reliable_, bool)

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("syn_backoff"),
                   this->syn_backoff_, double)

  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("syn_interval"), this->syn_interval_)

  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("syn_timeout"), this->syn_timeout_)

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("nak_depth"),
                   this->nak_depth_, size_t)

  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("nak_interval"), this->nak_interval_)

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("nak_delay_intervals"),
                        this->nak_delay_intervals_, size_t)

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("nak_max"), this->nak_max_, size_t)

  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("nak_timeout"), this->nak_timeout_)

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ttl"), this->ttl_, char)

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("rcv_buffer_size"),
                   this->rcv_buffer_size_, size_t)

   return 0;
}

void
MulticastInst::default_group_address(ACE_INET_Addr& group_address,
                                     ACE_UINT32 id)
{
  u_short port_number(this->port_offset_ + id);

  if (this->default_to_ipv6_) {
    group_address.set(port_number, DEFAULT_IPV6_GROUP_ADDRESS);
  } else {
    group_address.set(port_number, DEFAULT_IPV4_GROUP_ADDRESS);
  }
}

MulticastTransport*
MulticastInst::new_impl(const TransportInst_rch& inst)
{
  return new MulticastTransport(inst);
}

void
MulticastInst::dump(std::ostream& os)
{
  TransportInst::dump(os);

  os << formatNameForDump("group_address")       << this->group_address_.get_host_addr()
                                                           << ":" << this->group_address_.get_port_number() << std::endl;
  os << formatNameForDump("default_to_ipv6")     << (this->default_to_ipv6_ ? "true" : "false") << std::endl;
  os << formatNameForDump("port_offset")         << this->port_offset_ << std::endl;
  os << formatNameForDump("reliable")            << (this->reliable_ ? "true" : "false") << std::endl;
  os << formatNameForDump("syn_backoff")         << this->syn_backoff_ << std::endl;
  os << formatNameForDump("syn_interval")        << this->syn_interval_.msec() << std::endl;
  os << formatNameForDump("syn_timeout")         << this->syn_timeout_.msec() << std::endl;
  os << formatNameForDump("nak_depth")           << this->nak_depth_ << std::endl;
  os << formatNameForDump("nak_interval")        << this->nak_interval_.msec() << std::endl;
  os << formatNameForDump("nak_delay_intervals") << this->nak_delay_intervals_ << std::endl;
  os << formatNameForDump("nak_max")             << this->nak_max_ << std::endl;
  os << formatNameForDump("nak_timeout")         << this->nak_timeout_.msec() << std::endl;
  os << formatNameForDump("ttl")                 << this->ttl_ << std::endl;
  os << formatNameForDump("rcv_buffer_size");

  if (this->rcv_buffer_size_ == 0) {
    os << "System Default Value" << std::endl;
  } else {
    os << this->rcv_buffer_size_ << std::endl;
  }
}

} // namespace DCPS
} // namespace OpenDDS
