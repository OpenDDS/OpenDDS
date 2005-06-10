// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleUdpFactory.h"
#include  "SimpleUdpTransport.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpFactory.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUdpFactory::~SimpleUdpFactory()
{
  DBG_ENTRY("SimpleUdpFactory","~SimpleUdpFactory");
}


int
TAO::DCPS::SimpleUdpFactory::requires_reactor() const
{
  DBG_ENTRY("SimpleUdpFactory","requires_reactor");
  // return "true"
  return 1;
}


TAO::DCPS::TransportImpl*
TAO::DCPS::SimpleUdpFactory::create()
{
  DBG_ENTRY("SimpleUdpFactory","create");
  return new SimpleUdpTransport();
}
