// -*- C++ -*-
//
// $Id$

#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::ReliableMulticastTransportImplFactory::ReliableMulticastTransportImplFactory()
{
  DBG_ENTRY_LVL("ReliableMulticastTransportImplFactory","ReliableMulticastTransportImplFactory",5);
}

ACE_INLINE int
TAO::DCPS::ReliableMulticastTransportImplFactory::requires_reactor() const
{
  DBG_ENTRY_LVL("ReliableMulticastTransportImplFactory","requires_reactor",5);
  // return "true"
  return 1;
}
