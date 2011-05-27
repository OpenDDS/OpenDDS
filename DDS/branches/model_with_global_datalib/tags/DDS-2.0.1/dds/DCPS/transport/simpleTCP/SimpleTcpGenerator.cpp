/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleTcp_pch.h"
#include "SimpleTcpGenerator.h"
#include "SimpleTcpConfiguration.h"
#include "SimpleTcpFactory.h"

OpenDDS::DCPS::SimpleTcpGenerator::SimpleTcpGenerator()
{
}

OpenDDS::DCPS::SimpleTcpGenerator::~SimpleTcpGenerator()
{
}

OpenDDS::DCPS::TransportImplFactory*
OpenDDS::DCPS::SimpleTcpGenerator::new_factory()
{
  SimpleTcpFactory* factory = 0;
  ACE_NEW_RETURN(factory,
                 SimpleTcpFactory(),
                 0);
  return factory;
}

OpenDDS::DCPS::TransportConfiguration*
OpenDDS::DCPS::SimpleTcpGenerator::new_configuration(const TransportIdType id)
{
  ACE_UNUSED_ARG(id);

  SimpleTcpConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config,
                 SimpleTcpConfiguration(),
                 0);
  return trans_config;
}

void
OpenDDS::DCPS::SimpleTcpGenerator::default_transport_ids(TransportIdList & ids)
{
  ids.clear();
  ids.push_back(OpenDDS::DCPS::DEFAULT_SIMPLE_TCP_ID);
}
