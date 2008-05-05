// -*- C++ -*-
//
// $Id$

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
