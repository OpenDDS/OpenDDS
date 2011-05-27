/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportSendListener.h"
#include "EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "TransportSendListener.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportSendListener::TransportSendListener()
{
  DBG_ENTRY_LVL("TransportSendListener","TransportSendListener",6);
}

OpenDDS::DCPS::TransportSendListener::~TransportSendListener()
{
  DBG_ENTRY_LVL("TransportSendListener","~TransportSendListener",6);
}

void
OpenDDS::DCPS::TransportSendListener::control_delivered(ACE_Message_Block* sample)
{
  ACE_UNUSED_ARG(sample);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: Subclass should override if sending control samples."));
}

void
OpenDDS::DCPS::TransportSendListener::control_dropped(ACE_Message_Block* sample,
                                                      bool dropped_by_transport)
{
  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(dropped_by_transport);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: Subclass should override if sending control samples."));
}
