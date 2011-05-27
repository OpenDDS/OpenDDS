// -*- C++ -*-
//
// $Id$
#include "SimpleUnreliableDgram_pch.h"
#include "SimpleMcastGenerator.h"
#include "SimpleMcastConfiguration.h"
#include "SimpleMcastFactory.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"

TAO::DCPS::SimpleMcastGenerator::SimpleMcastGenerator()
{
}

TAO::DCPS::SimpleMcastGenerator::~SimpleMcastGenerator()
{
}

TAO::DCPS::TransportImplFactory* 
TAO::DCPS::SimpleMcastGenerator::new_factory() 
{
  SimpleMcastFactory* factory = 0;
  ACE_NEW_RETURN(factory, 
                 SimpleMcastFactory(), 
                 0);
  return factory;
}

TAO::DCPS::TransportConfiguration* 
TAO::DCPS::SimpleMcastGenerator::new_configuration(const TransportIdType id)
{
  SimpleMcastConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config, 
                 SimpleMcastConfiguration(id), 
                 0);

  return trans_config;
}


void 
TAO::DCPS::SimpleMcastGenerator::default_transport_ids (TransportIdList & ids)
{
  // SimpleMcast needs two default transport configuration, one for
  // publisher and one for subscriber.
  ids.clear ();
  ids.push_back (TAO::DCPS::DEFAULT_SIMPLE_MCAST_PUB_ID);
  ids.push_back (TAO::DCPS::DEFAULT_SIMPLE_MCAST_SUB_ID);
}


