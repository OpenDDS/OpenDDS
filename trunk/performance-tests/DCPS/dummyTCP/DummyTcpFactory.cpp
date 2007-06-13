// -*- C++ -*-
//
// $Id$

#include "DummyTcp_pch.h"
#include "DummyTcpFactory.h"
#include "DummyTcpTransport.h"
#include "DummyTcpConnection.h"
#include "DummyTcpSendStrategy.h"

#if !defined (__ACE_INLINE__)
#include "DummyTcpFactory.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::DummyTcpFactory::~DummyTcpFactory()
{
  DBG_ENTRY_LVL("DummyTcpFactory","~DummyTcpFactory",5);
}


int
TAO::DCPS::DummyTcpFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("DummyTcpFactory","requires_reactor",5);
  // return "true"
  return 1;
}


TAO::DCPS::TransportImpl*
TAO::DCPS::DummyTcpFactory::create()
{
  DBG_ENTRY_LVL("DummyTcpFactory","create",5);
  return new DummyTcpTransport();
}

