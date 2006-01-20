// -*- C++ -*-
//
// $Id$
#include "SimpleUdp_pch.h"
#include  "SimpleUdpGenerator.h"
#include  "SimpleUdpConfiguration.h"
#include  "SimpleUdpFactory.h"


TAO::DCPS::SimpleUdpGenerator::SimpleUdpGenerator()
{
}

TAO::DCPS::SimpleUdpGenerator::~SimpleUdpGenerator()
{
}

TransportImplFactory* 
TAO::DCPS::SimpleUdpGenerator::new_factory() 
{
  SimpleUdpFactory* factory = 0;
  ACE_NEW_RETURN(factory, 
                 SimpleUdpFactory(/*this->transport_name_*/), 
                 0);
  return factory;
}

TransportConfiguration* 
TAO::DCPS::SimpleUdpGenerator::new_configuration()
{
  SimpleUdpConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config, 
                 SimpleUdpConfiguration(), 
                 0);
  return trans_config;
}

