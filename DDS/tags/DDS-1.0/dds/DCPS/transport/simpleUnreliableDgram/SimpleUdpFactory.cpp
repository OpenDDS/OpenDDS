// -*- C++ -*-
//
// $Id$

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUdpFactory.h"
#include "SimpleUdpTransport.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpFactory.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::SimpleUdpFactory::~SimpleUdpFactory()
{
  DBG_ENTRY_LVL("SimpleUdpFactory","~SimpleUdpFactory",5);
}


int
OpenDDS::DCPS::SimpleUdpFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("SimpleUdpFactory","requires_reactor",5);
  // return "true"
  return 1;
}


OpenDDS::DCPS::TransportImpl*
OpenDDS::DCPS::SimpleUdpFactory::create()
{
  DBG_ENTRY_LVL("SimpleUdpFactory","create",5);
  return new SimpleUdpTransport();
}
