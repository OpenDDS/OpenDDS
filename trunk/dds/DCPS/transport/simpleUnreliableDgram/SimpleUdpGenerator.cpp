/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUdpGenerator.h"
#include "SimpleUdpConfiguration.h"
#include "SimpleUdpFactory.h"

OpenDDS::DCPS::SimpleUdpGenerator::SimpleUdpGenerator()
{
}

OpenDDS::DCPS::SimpleUdpGenerator::~SimpleUdpGenerator()
{
}

OpenDDS::DCPS::TransportImplFactory*
OpenDDS::DCPS::SimpleUdpGenerator::new_factory()
{
  SimpleUdpFactory* factory = 0;
  ACE_NEW_RETURN(factory,
                 SimpleUdpFactory(),
                 0);
  return factory;
}

OpenDDS::DCPS::TransportConfiguration*
OpenDDS::DCPS::SimpleUdpGenerator::new_configuration(const TransportIdType id)
{
  ACE_UNUSED_ARG(id);

  SimpleUdpConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config,
                 SimpleUdpConfiguration(),
                 0);
  return trans_config;
}

void
OpenDDS::DCPS::SimpleUdpGenerator::default_transport_ids(TransportIdList & ids)
{
  ids.clear();
  ids.push_back(OpenDDS::DCPS::DEFAULT_SIMPLE_UDP_ID);
}
