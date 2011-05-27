/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpConfiguration.h"
#include "UdpLoader.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

namespace OpenDDS {
namespace DCPS {

UdpConfiguration::UdpConfiguration()
{
  this->transport_type_ = UDP_TRANSPORT_TYPE;
}

int
UdpConfiguration::load(const TransportIdType& id,
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
                      ACE_TEXT("UdpConfiguration::load: ")
                      ACE_TEXT("unable to open section: [%C]!\n"),
                      section_name.c_str()),
                     -1);
  }

  ACE_TString local_address_s;
  GET_CONFIG_STRING_VALUE(config, transport_key, ACE_TEXT("local_address"),
                          local_address_s)
  if (local_address_s.is_empty()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpConfiguration::load: ")
                      ACE_TEXT("local_address not specified!\n")),
                     -1);
  }
  this->local_address_.set(local_address_s.c_str());

  return 0;
}

} // namespace DCPS
} // namespace OpenDDS
