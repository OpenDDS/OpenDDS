// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportQueueElement.h"
#include  "EntryExit.h"

#if !defined (__ACE_INLINE__)
# include "TransportQueueElement.inl"
#endif /* ! __ACE_INLINE__ */

TAO::DCPS::TransportQueueElement::~TransportQueueElement()
{
  DBG_ENTRY_LVL("TransportQueueElement","~TransportQueueElement",5);
}


bool
TAO::DCPS::TransportQueueElement::requires_exclusive_packet() const
{
  DBG_ENTRY_LVL("TransportQueueElement","requires_exclusive_packet",5);
  return false;
}


bool
TAO::DCPS::TransportQueueElement::is_control(RepoId pub_id) const
{
  DBG_ENTRY_LVL("TransportQueueElement","is_control",5);
  ACE_UNUSED_ARG(pub_id);
  return false;
}
