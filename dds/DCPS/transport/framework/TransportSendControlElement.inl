// -*- C++ -*-
//
// $Id$
#include "EntryExit.h"


ACE_INLINE
OpenDDS::DCPS::TransportSendControlElement::TransportSendControlElement
                                    (int                    initial_count,
                                     RepoId                 publisher_id,
                                     TransportSendListener* listener,
                                     ACE_Message_Block*     msg_block,
                                     TransportSendControlElementAllocator* allocator)
  : TransportQueueElement(initial_count),
    publisher_id_(publisher_id),
    listener_(listener),
    msg_(msg_block),
    allocator_(allocator)
{
  DBG_ENTRY_LVL("TransportSendControlElement","TransportSendControlElement",5);
}

