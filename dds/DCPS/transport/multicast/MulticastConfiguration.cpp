/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastConfiguration.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

namespace {

const bool DEFAULT_DEFAULT_TO_IPV6(false);

const char* DEFAULT_IPV4_GROUP_ADDRESS("224.0.0.128");
const char* DEFAULT_IPV6_GROUP_ADDRESS("FF01::80");

const bool DEFAULT_RELIABLE(true);

const long DEFAULT_HANDSHAKE_TIMEOUT(30000);

const size_t DEFAULT_NAK_DEPTH(32);
const long DEFAULT_NAK_TIMEOUT(30000);

} // namespace

namespace OpenDDS {
namespace DCPS {

MulticastConfiguration::MulticastConfiguration()
  : default_to_ipv6_(DEFAULT_DEFAULT_TO_IPV6),
    reliable_(DEFAULT_RELIABLE),
    handshake_timeout_(DEFAULT_HANDSHAKE_TIMEOUT),
    nak_depth_(DEFAULT_NAK_DEPTH),
    nak_timeout_(DEFAULT_NAK_TIMEOUT)
{
  default_group_address(this->group_address_, DEFAULT_MULTICAST_ID);
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
                          0,
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
    default_group_address(this->group_address_, id);    // default
  } else {
    this->group_address_.set(group_address_s.c_str());  // user-defined
  }

  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("reliable"),
                   this->reliable_, bool)

  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("handshake_timeout"),
                   this->handshake_timeout_, long)

  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("nak_depth"),
                   this->nak_depth_, size_t)

  GET_CONFIG_VALUE(config, transport_key, ACE_TEXT("nak_timeout"),
                   this->nak_timeout_, long)

  return 0;
}

void
MulticastConfiguration::default_group_address(ACE_INET_Addr& group_address,
                                              const TransportIdType& id)
{
  if (this->default_to_ipv6_) {
    group_address.set(u_short(id), DEFAULT_IPV6_GROUP_ADDRESS);
  } else {
    group_address.set(u_short(id), DEFAULT_IPV4_GROUP_ADDRESS);
  }
}

} // namespace DCPS
} // namespace OpenDDS
