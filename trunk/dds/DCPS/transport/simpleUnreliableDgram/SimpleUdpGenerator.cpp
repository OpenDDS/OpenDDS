// -*- C++ -*-
//
// $Id$
#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUdpGenerator.h"
#include "SimpleUdpConfiguration.h"
#include "SimpleUdpFactory.h"


TAO::DCPS::SimpleUdpGenerator::SimpleUdpGenerator()
{
}

TAO::DCPS::SimpleUdpGenerator::~SimpleUdpGenerator()
{
}

TAO::DCPS::TransportImplFactory* 
TAO::DCPS::SimpleUdpGenerator::new_factory() 
{
  SimpleUdpFactory* factory = 0;
  ACE_NEW_RETURN(factory, 
                 SimpleUdpFactory(), 
                 0);
  return factory;
}

TAO::DCPS::TransportConfiguration* 
TAO::DCPS::SimpleUdpGenerator::new_configuration(const TransportIdType id)
{
  ACE_UNUSED_ARG (id);

  SimpleUdpConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config, 
                 SimpleUdpConfiguration(), 
                 0);
  return trans_config;
}

void 
TAO::DCPS::SimpleUdpGenerator::default_transport_ids (TransportIdList & ids)
{
  ids.clear ();
  ids.push_back (TAO::DCPS::DEFAULT_SIMPLE_UDP_ID);
}
