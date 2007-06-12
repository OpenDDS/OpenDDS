// -*- C++ -*-
//
// $Id$
#include "DummyTcp_pch.h"
#include "DummyTcpGenerator.h"
#include "DummyTcpConfiguration.h"
#include "DummyTcpFactory.h"


TAO::DCPS::DummyTcpGenerator::DummyTcpGenerator()
{
}

TAO::DCPS::DummyTcpGenerator::~DummyTcpGenerator()
{
}

TAO::DCPS::TransportImplFactory* 
TAO::DCPS::DummyTcpGenerator::new_factory() 
{
  DummyTcpFactory* factory = 0;
  ACE_NEW_RETURN(factory, 
                 DummyTcpFactory(), 
                 0);
  return factory;
}

TAO::DCPS::TransportConfiguration* 
TAO::DCPS::DummyTcpGenerator::new_configuration(const TransportIdType id)
{
  ACE_UNUSED_ARG (id);

  DummyTcpConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config, 
                 DummyTcpConfiguration(), 
                 0);
  return trans_config;
}

void 
TAO::DCPS::DummyTcpGenerator::default_transport_ids (TransportIdList & ids)
{
  ids.clear ();
  ids.push_back (TAO::DCPS::DEFAULT_DUMMY_TCP_ID);
}
