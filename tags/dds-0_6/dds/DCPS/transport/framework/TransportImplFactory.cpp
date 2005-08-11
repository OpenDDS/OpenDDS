// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportImplFactory.h"


#if !defined (__ACE_INLINE__)
#include "TransportImplFactory.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::TransportImplFactory::~TransportImplFactory()
{
  DBG_ENTRY("TransportImplFactory","~TransportImplFactory");
}


int
TAO::DCPS::TransportImplFactory::requires_reactor() const
{
  DBG_ENTRY("TransportImplFactory","requires_reactor");
  // Return "false" (aka 0).
  return 0;
}

