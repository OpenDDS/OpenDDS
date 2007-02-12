// -*- C++ -*-
//
// $Id$

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleMcastFactory.h"
#include "SimpleMcastTransport.h"


#if !defined (__ACE_INLINE__)
#include "SimpleMcastFactory.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleMcastFactory::~SimpleMcastFactory()
{
  DBG_ENTRY_LVL("SimpleMcastFactory","~SimpleMcastFactory",5);
}


int
TAO::DCPS::SimpleMcastFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("SimpleMcastFactory","requires_reactor",5);
  // return "true"
  return 1;
}


TAO::DCPS::TransportImpl*
TAO::DCPS::SimpleMcastFactory::create()
{
  DBG_ENTRY_LVL("SimpleMcastFactory","create",5);
  return new SimpleMcastTransport();
}
