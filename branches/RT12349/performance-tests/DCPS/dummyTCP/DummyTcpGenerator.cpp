// -*- C++ -*-
//
// $Id$
#include "DummyTcp_pch.h"
#include "DummyTcpGenerator.h"
#include "DummyTcpConfiguration.h"
#include "DummyTcpFactory.h"


OpenDDS::DCPS::DummyTcpGenerator::DummyTcpGenerator()
{
}

OpenDDS::DCPS::DummyTcpGenerator::~DummyTcpGenerator()
{
}

OpenDDS::DCPS::TransportImplFactory* 
OpenDDS::DCPS::DummyTcpGenerator::new_factory() 
{
  DummyTcpFactory* factory = 0;
  ACE_NEW_RETURN(factory, 
                 DummyTcpFactory(), 
                 0);
  return factory;
}

OpenDDS::DCPS::TransportConfiguration* 
OpenDDS::DCPS::DummyTcpGenerator::new_configuration(const TransportIdType id)
{
  ACE_UNUSED_ARG (id);

  DummyTcpConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config, 
                 DummyTcpConfiguration(), 
                 0);
  return trans_config;
}

void 
OpenDDS::DCPS::DummyTcpGenerator::default_transport_ids (TransportIdList & ids)
{
  ids.clear ();
  ids.push_back (OpenDDS::DCPS::DEFAULT_DUMMY_TCP_ID);
}
