// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpFactory.h"
#include  "SimpleTcpTransport.h"
#include  "SimpleTcpConnection.h"
#include  "SimpleTcpSendStrategy.h"

#if !defined (__ACE_INLINE__)
#include "SimpleTcpFactory.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleTcpFactory::~SimpleTcpFactory()
{
  DBG_ENTRY("SimpleTcpFactory","~SimpleTcpFactory");
}


int
TAO::DCPS::SimpleTcpFactory::requires_reactor() const
{
  DBG_ENTRY("SimpleTcpFactory","requires_reactor");
  // return "true"
  return 1;
}


TAO::DCPS::TransportImpl*
TAO::DCPS::SimpleTcpFactory::create()
{
  DBG_ENTRY("SimpleTcpFactory","create");
  return new SimpleTcpTransport();
}

