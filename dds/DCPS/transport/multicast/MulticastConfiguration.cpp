/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastConfiguration.h"
#include "dds/DCPS/transport/framework/TransportDebug.h"

namespace {

const bool DEFAULT_DEFAULT_TO_IPV6(false);

const char* DEFAULT_IPV4_GROUP_ADDRESS("224.0.0.128");
const char* DEFAULT_IPV6_GROUP_ADDRESS("FF01::80");

const u_short DEFAULT_PORT_OFFSET(49400);

const bool DEFAULT_RELIABLE(true);

const long DEFAULT_SYN_INTERVAL(500);
const long DEFAULT_SYN_TIMEOUT(30000);

const long DEFAULT_NAK_INTERVAL(2000);
const long DEFAULT_NAK_TIMEOUT(30000);

const size_t DEFAULT_NAK_REPAIR_SIZE(32);

} // namespace

namespace OpenDDS {
namespace DCPS {

MulticastConfiguration::MulticastConfiguration()
  : default_to_ipv6_(DEFAULT_DEFAULT_TO_IPV6),
    port_offset_(DEFAULT_PORT_OFFSET),
    reliable_(DEFAULT_RELIABLE),
    nak_repair_size_(DEFAULT_NAK_REPAIR_SIZE)
{
  default_group_address(this->group_address_, DEFAULT_MULTICAST_ID);

  this->syn_interval_.msec(DEFAULT_SYN_INTERVAL);
  this->syn_timeout_.msec(DEFAULT_SYN_TIMEOUT);

  this->nak_interval_.msec(DEFAULT_NAK_INTERVAL);
  this->nak_timeout_.msec(DEFAULT_NAK_TIMEOUT);
}

int
MulticastConfiguration::load(const TransportIdType& id,
                             ACE_Configuration_Heap& config)
{
  TransportConfiguration::load(id, config); // delegate to parent

  ACE_Configuration_Section_Key transport_key;

  ACE_TString section_name = id_to_section_name(id);
  if (config.open_section(config.root_section(),
                          section_name.c_str(),
                          0,  // create
                          transport_key) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastConfiguration::load: ")
                      ACE_TEXT("unable to open section: [%C]\n"),
                      section_name.c_str()), -1);
  }

  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("default_to_ipv6"),
                   this->default_to_ipv6_, bool)

  ACE_TString group_address_s;
  GET_CONFIG_STRING_VALUE(config, transport_key, ACE_TEXT("group_address"),
                          group_address_s)
  if (group_address_s.is_empty()) {
    default_group_address(this->group_address_, id);
  } else {
    this->group_address_.set(group_address_s.c_str());
  }

  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("port_offset"),
                   this->port_offset_, u_short)

  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("reliable"),
                   this->reliable_, bool)

  long syn_interval_msec = -1;
  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("syn_interval"),
                   syn_interval_msec, long)
  if (syn_interval_msec != -1) {
    this->syn_interval_.msec(syn_interval_msec);
  }

  long syn_timeout_msec = -1;
  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("syn_timeout"),
                   syn_timeout_msec, long)
  if (syn_timeout_msec != -1) {
    this->syn_timeout_.msec(syn_timeout_msec);
  }

  long nak_interval_msec = -1;
  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("nak_interval"),
                   nak_interval_msec, long)
  if (nak_interval_msec != -1) {
    this->nak_interval_.msec(nak_interval_msec);
  }

  long nak_timeout_msec = -1;
  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("nak_timeout"),
                   nak_timeout_msec, long)
  if (nak_timeout_msec != -1) {
    this->nak_timeout_.msec(nak_timeout_msec);
  }

  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("nak_repair_size"),
                   this->nak_repair_size_, size_t)

  return 0;
}

void
MulticastConfiguration::default_group_address(ACE_INET_Addr& group_address,
                                              const TransportIdType& id)
{
  u_short port_number(this->port_offset_ + id);

  if (this->default_to_ipv6_) {
    group_address.set(port_number, DEFAULT_IPV6_GROUP_ADDRESS);
  } else {
    group_address.set(port_number, DEFAULT_IPV4_GROUP_ADDRESS);
  }
}

} // namespace DCPS
} // namespace OpenDDS
