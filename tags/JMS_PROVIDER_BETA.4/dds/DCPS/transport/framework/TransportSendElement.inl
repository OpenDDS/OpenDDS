// -*- C++ -*-
//
// $Id$
#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportSendElement::TransportSendElement
                                     (int                    initial_count,
                                      DataSampleListElement* sample,
                                      TransportSendElementAllocator* allocator)
  : TransportQueueElement(initial_count),
    element_(sample),
    allocator_(allocator)
{
  DBG_ENTRY_LVL("TransportSendElement","TransportSendElement",6);
}

