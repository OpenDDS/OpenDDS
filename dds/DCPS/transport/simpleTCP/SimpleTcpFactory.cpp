// -*- C++ -*-
//
// $Id$

#include "SimpleTcp_pch.h"
#include "SimpleTcpFactory.h"
#include "SimpleTcpTransport.h"
#include "SimpleTcpConnection.h"
#include "SimpleTcpSendStrategy.h"

#if !defined (__ACE_INLINE__)
#include "SimpleTcpFactory.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleTcpFactory::~SimpleTcpFactory()
{
  DBG_ENTRY_LVL("SimpleTcpFactory","~SimpleTcpFactory",5);
}


int
TAO::DCPS::SimpleTcpFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("SimpleTcpFactory","requires_reactor",5);
  // return "true"
  return 1;
}


TAO::DCPS::TransportImpl*
TAO::DCPS::SimpleTcpFactory::create()
{
  DBG_ENTRY_LVL("SimpleTcpFactory","create",5);
  return new SimpleTcpTransport();
}

