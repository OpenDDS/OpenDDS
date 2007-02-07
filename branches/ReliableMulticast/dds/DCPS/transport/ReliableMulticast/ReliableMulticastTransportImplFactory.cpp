// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportImplFactory.h"
#include "ReliableMulticastTransportImpl.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImplFactory.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::ReliableMulticastTransportImplFactory::~ReliableMulticastTransportImplFactory()
{
  DBG_ENTRY_LVL("ReliableMulticastTransportImplFactory","~ReliableMulticastTransportImplFactory",5);
}

TAO::DCPS::TransportImpl*
TAO::DCPS::ReliableMulticastTransportImplFactory::create()
{
  DBG_ENTRY_LVL("ReliableMulticastTransportImplFactory","create",5);
  return new ReliableMulticastTransportImpl();
}
