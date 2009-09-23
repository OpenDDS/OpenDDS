/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUdpFactory.h"
#include "SimpleUdpTransport.h"

#if !defined (__ACE_INLINE__)
#include "SimpleUdpFactory.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::SimpleUdpFactory::~SimpleUdpFactory()
{
  DBG_ENTRY_LVL("SimpleUdpFactory","~SimpleUdpFactory",6);
}

int
OpenDDS::DCPS::SimpleUdpFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("SimpleUdpFactory","requires_reactor",6);
  // return "true"
  return 1;
}

OpenDDS::DCPS::TransportImpl*
OpenDDS::DCPS::SimpleUdpFactory::create()
{
  DBG_ENTRY_LVL("SimpleUdpFactory","create",6);
  return new SimpleUdpTransport();
}
