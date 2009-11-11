/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastGenerator.h"
#include "MulticastFactory.h"
#include "MulticastConfiguration.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

namespace OpenDDS {
namespace DCPS {

MulticastGenerator::~MulticastGenerator()
{
}

TransportImplFactory*
MulticastGenerator::new_factory()
{
  TransportImplFactory* factory;
  ACE_NEW_RETURN(factory, MulticastFactory, 0);
  return factory;
}

TransportConfiguration*
MulticastGenerator::new_configuration(const TransportIdType /*id*/)
{
  TransportConfiguration* configuration;
  ACE_NEW_RETURN(configuration, MulticastConfiguration, 0);
  return configuration;
}

void
MulticastGenerator::default_transport_ids(TransportIdList& ids)
{
  ids.clear();
  ids.push_back(DEFAULT_MULTICAST_ID);
}

} // namespace DCPS
} // namespace OpenDDS
