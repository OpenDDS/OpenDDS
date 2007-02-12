// -*- C++ -*-
//
// $Id$

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUdpFactory.h"
#include "SimpleUdpTransport.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpFactory.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUdpFactory::~SimpleUdpFactory()
{
  DBG_ENTRY_LVL("SimpleUdpFactory","~SimpleUdpFactory",5);
}


int
TAO::DCPS::SimpleUdpFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("SimpleUdpFactory","requires_reactor",5);
  // return "true"
  return 1;
}


TAO::DCPS::TransportImpl*
TAO::DCPS::SimpleUdpFactory::create()
{
  DBG_ENTRY_LVL("SimpleUdpFactory","create",5);
  return new SimpleUdpTransport();
}
