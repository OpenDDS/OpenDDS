// -*- C++ -*-
//
// $Id$
#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::TransportSendElement::TransportSendElement
                                     (int                    initial_count,
                                      DataSampleListElement* sample)
  : TransportQueueElement(initial_count),
    element_(sample)
{
  DBG_ENTRY("TransportSendElement","TransportSendElement");
}

