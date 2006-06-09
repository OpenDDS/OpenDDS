// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportSendListener.h"
#include  "EntryExit.h"


#if !defined (__ACE_INLINE__)
#include "TransportSendListener.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::TransportSendListener::TransportSendListener()
{
  DBG_ENTRY("TransportSendListener","TransportSendListener");
}


TAO::DCPS::TransportSendListener::~TransportSendListener()
{
  DBG_ENTRY("TransportSendListener","~TransportSendListener");
}


void
TAO::DCPS::TransportSendListener::control_delivered(ACE_Message_Block* sample)
{
  ACE_UNUSED_ARG(sample);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: Subclass should override if sending control samples."));
}


void
TAO::DCPS::TransportSendListener::control_dropped(ACE_Message_Block* sample, 
                                                  bool dropped_by_transport)
{
  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(dropped_by_transport);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: Subclass should override if sending control samples."));
}

