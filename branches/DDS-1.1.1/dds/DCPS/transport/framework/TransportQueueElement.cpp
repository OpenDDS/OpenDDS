// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportQueueElement.h"
#include "EntryExit.h"

#if !defined (__ACE_INLINE__)
# include "TransportQueueElement.inl"
#endif /* ! __ACE_INLINE__ */

OpenDDS::DCPS::TransportQueueElement::~TransportQueueElement()
{
  DBG_ENTRY_LVL("TransportQueueElement","~TransportQueueElement",6);
}


bool
OpenDDS::DCPS::TransportQueueElement::requires_exclusive_packet() const
{
  DBG_ENTRY_LVL("TransportQueueElement","requires_exclusive_packet",6);
  return false;
}


bool
OpenDDS::DCPS::TransportQueueElement::is_control(RepoId pub_id) const
{
  DBG_ENTRY_LVL("TransportQueueElement","is_control",6);
  ACE_UNUSED_ARG(pub_id);
  return false;
}
