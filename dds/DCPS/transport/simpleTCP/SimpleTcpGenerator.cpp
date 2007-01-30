// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpGenerator.h"
#include  "SimpleTcpConfiguration.h"
#include  "SimpleTcpFactory.h"


TAO::DCPS::SimpleTcpGenerator::SimpleTcpGenerator()
{
}

TAO::DCPS::SimpleTcpGenerator::~SimpleTcpGenerator()
{
}

TAO::DCPS::TransportImplFactory* 
TAO::DCPS::SimpleTcpGenerator::new_factory() 
{
  SimpleTcpFactory* factory = 0;
  ACE_NEW_RETURN(factory, 
                 SimpleTcpFactory(), 
                 0);
  return factory;
}

TAO::DCPS::TransportConfiguration* 
TAO::DCPS::SimpleTcpGenerator::new_configuration(const TransportIdType id)
{
  ACE_UNUSED_ARG (id);

  SimpleTcpConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config, 
                 SimpleTcpConfiguration(), 
                 0);
  return trans_config;
}

void 
TAO::DCPS::SimpleTcpGenerator::default_transport_ids (TransportIdList & ids)
{
  ids.clear ();
  ids.push_back (TAO::DCPS::DEFAULT_SIMPLE_TCP_ID);
}
