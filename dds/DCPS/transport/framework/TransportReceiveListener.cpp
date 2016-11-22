/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportReceiveListener.h"
#include "EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TransportReceiveListener::TransportReceiveListener()
{
  DBG_ENTRY_LVL("TransportReceiveListener","TransportReceiveListener",6);
}

OpenDDS::DCPS::TransportReceiveListener::~TransportReceiveListener()
{
  DBG_ENTRY_LVL("TransportReceiveListener","~TransportReceiveListener",6);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
