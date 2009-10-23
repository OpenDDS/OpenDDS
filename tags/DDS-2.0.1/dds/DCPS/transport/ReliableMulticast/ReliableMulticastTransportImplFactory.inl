/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ReliableMulticastTransportImplFactory::ReliableMulticastTransportImplFactory()
{
  DBG_ENTRY_LVL("ReliableMulticastTransportImplFactory","ReliableMulticastTransportImplFactory",6);
}

ACE_INLINE int
OpenDDS::DCPS::ReliableMulticastTransportImplFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("ReliableMulticastTransportImplFactory","requires_reactor",6);
  // return "true"
  return 1;
}
