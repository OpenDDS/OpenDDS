/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportImplFactory.h"

#if !defined (__ACE_INLINE__)
#include "TransportImplFactory.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportImplFactory::~TransportImplFactory()
{
  DBG_ENTRY_LVL("TransportImplFactory","~TransportImplFactory",6);
}

int
OpenDDS::DCPS::TransportImplFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("TransportImplFactory","requires_reactor",6);
  // Return "false" (aka 0).
  return 0;
}
