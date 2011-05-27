// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "TransportInterface.h"


#if !defined (__ACE_INLINE__)
#include "TransportInterface.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::TransportInterface::~TransportInterface()
{
  DBG_ENTRY("TransportInterface","~TransportInterface");
}

void
TAO::DCPS::TransportInterface::transport_detached_i()
{
  DBG_ENTRY("TransportInterface","transport_detached_i");
  // Subclass should override if interested in the "transport detached event".
}

