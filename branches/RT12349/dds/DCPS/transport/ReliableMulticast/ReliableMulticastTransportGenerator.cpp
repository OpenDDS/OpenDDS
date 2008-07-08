// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportGenerator.h"
#include "ReliableMulticastTransportImplFactory.h"
#include "ReliableMulticastTransportConfiguration.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportGenerator.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportImplFactory*
OpenDDS::DCPS::ReliableMulticastTransportGenerator::new_factory()
{
  ReliableMulticastTransportImplFactory* factory = 0;
  ACE_NEW_RETURN(factory,
                 ReliableMulticastTransportImplFactory(),
                 0);
  return factory;
}

OpenDDS::DCPS::TransportConfiguration*
OpenDDS::DCPS::ReliableMulticastTransportGenerator::new_configuration(const TransportIdType id)
{
  ACE_UNUSED_ARG (id);

  ReliableMulticastTransportConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config,
                 ReliableMulticastTransportConfiguration(),
                 0);
  return trans_config;
}

void
OpenDDS::DCPS::ReliableMulticastTransportGenerator::default_transport_ids(TransportIdList& ids)
{
  ids.clear();
  ids.push_back(OpenDDS::DCPS::DEFAULT_RELIABLE_MULTICAST_PUB_ID);
  ids.push_back(OpenDDS::DCPS::DEFAULT_RELIABLE_MULTICAST_SUB_ID);
}
