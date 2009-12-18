/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpGenerator.h"
#include "UdpFactory.h"
#include "UdpConfiguration.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

namespace OpenDDS {
namespace DCPS {

TransportImplFactory*
UdpGenerator::new_factory()
{
  TransportImplFactory* factory;
  ACE_NEW_RETURN(factory, UdpFactory, 0);
  return factory;
}

TransportConfiguration*
UdpGenerator::new_configuration(const TransportIdType /*id*/)
{
  TransportConfiguration* configuration;
  ACE_NEW_RETURN(configuration, UdpConfiguration, 0);
  return configuration;
}

void
UdpGenerator::default_transport_ids(TransportIdList& ids)
{
  ids.clear();
  ids.push_back(DEFAULT_UDP_ID);
}

} // namespace DCPS
} // namespace OpenDDS
