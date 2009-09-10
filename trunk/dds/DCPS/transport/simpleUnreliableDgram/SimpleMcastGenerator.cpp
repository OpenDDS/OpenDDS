/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleMcastGenerator.h"
#include "SimpleMcastConfiguration.h"
#include "SimpleMcastFactory.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"

OpenDDS::DCPS::SimpleMcastGenerator::SimpleMcastGenerator()
{
}

OpenDDS::DCPS::SimpleMcastGenerator::~SimpleMcastGenerator()
{
}

OpenDDS::DCPS::TransportImplFactory*
OpenDDS::DCPS::SimpleMcastGenerator::new_factory()
{
  SimpleMcastFactory* factory = 0;
  ACE_NEW_RETURN(factory,
                 SimpleMcastFactory(),
                 0);
  return factory;
}

OpenDDS::DCPS::TransportConfiguration*
OpenDDS::DCPS::SimpleMcastGenerator::new_configuration(const TransportIdType id)
{
  SimpleMcastConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config,
                 SimpleMcastConfiguration(),
                 0);

  if (id == OpenDDS::DCPS::DEFAULT_SIMPLE_MCAST_SUB_ID)
    trans_config->receiver_ = true;

  return trans_config;
}

void
OpenDDS::DCPS::SimpleMcastGenerator::default_transport_ids(TransportIdList & ids)
{
  // SimpleMcast needs two default transport configuration, one for
  // publisher and one for subscriber.
  ids.clear();
  ids.push_back(OpenDDS::DCPS::DEFAULT_SIMPLE_MCAST_PUB_ID);
  ids.push_back(OpenDDS::DCPS::DEFAULT_SIMPLE_MCAST_SUB_ID);
}
