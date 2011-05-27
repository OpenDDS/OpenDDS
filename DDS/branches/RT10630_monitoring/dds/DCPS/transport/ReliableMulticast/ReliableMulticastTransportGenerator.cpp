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

TAO::DCPS::TransportImplFactory*
TAO::DCPS::ReliableMulticastTransportGenerator::new_factory()
{
  ReliableMulticastTransportImplFactory* factory = 0;
  ACE_NEW_RETURN(factory,
                 ReliableMulticastTransportImplFactory(),
                 0);
  return factory;
}

TAO::DCPS::TransportConfiguration*
TAO::DCPS::ReliableMulticastTransportGenerator::new_configuration(const TransportIdType id)
{
  ReliableMulticastTransportConfiguration* trans_config = 0;
  ACE_NEW_RETURN(trans_config,
                 ReliableMulticastTransportConfiguration(id),
                 0);
  return trans_config;
}

void
TAO::DCPS::ReliableMulticastTransportGenerator::default_transport_ids(TransportIdList& ids)
{
  ids.clear();
  ids.push_back(TAO::DCPS::DEFAULT_RELIABLE_MULTICAST_PUB_ID);
  ids.push_back(TAO::DCPS::DEFAULT_RELIABLE_MULTICAST_SUB_ID);
}
