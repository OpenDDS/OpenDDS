// -*- C++ -*-
//
// $Id$

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleMcastFactory.h"
#include "SimpleMcastTransport.h"


#if !defined (__ACE_INLINE__)
#include "SimpleMcastFactory.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::SimpleMcastFactory::~SimpleMcastFactory()
{
  DBG_ENTRY_LVL("SimpleMcastFactory","~SimpleMcastFactory",6);
}


int
OpenDDS::DCPS::SimpleMcastFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("SimpleMcastFactory","requires_reactor",6);
  // return "true"
  return 1;
}


OpenDDS::DCPS::TransportImpl*
OpenDDS::DCPS::SimpleMcastFactory::create()
{
  DBG_ENTRY_LVL("SimpleMcastFactory","create",6);
  return new SimpleMcastTransport();
}
