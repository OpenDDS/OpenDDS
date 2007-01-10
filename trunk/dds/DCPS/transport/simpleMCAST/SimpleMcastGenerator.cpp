// -*- C++ -*-
//
// $Id$
#include "SimpleMcast_pch.h"
#include  "SimpleMcastGenerator.h"
#include  "SimpleMcastConfiguration.h"
#include  "SimpleMcastFactory.h"


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
TAO::DCPS::SimpleMcastGenerator::new_configuration()
{
  SimpleMcastConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config, 
                 SimpleMcastConfiguration(), 
                 0);
  return trans_config;
}

