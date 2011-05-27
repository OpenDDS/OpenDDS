/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportReceiveListener.h"
#include "EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "TransportReceiveListener.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportReceiveListener::TransportReceiveListener()
{
  DBG_ENTRY_LVL("TransportReceiveListener","TransportReceiveListener",6);
}

OpenDDS::DCPS::TransportReceiveListener::~TransportReceiveListener()
{
  DBG_ENTRY_LVL("TransportReceiveListener","~TransportReceiveListener",6);
}
