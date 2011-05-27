// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportImplFactory.h"
#include "ReliableMulticastTransportImpl.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImplFactory.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::ReliableMulticastTransportImplFactory::~ReliableMulticastTransportImplFactory()
{
  DBG_ENTRY_LVL("ReliableMulticastTransportImplFactory","~ReliableMulticastTransportImplFactory",5);
}

OpenDDS::DCPS::TransportImpl*
OpenDDS::DCPS::ReliableMulticastTransportImplFactory::create()
{
  DBG_ENTRY_LVL("ReliableMulticastTransportImplFactory","create",5);
  return new ReliableMulticastTransportImpl();
}
