// -*- C++ -*-
//
// $Id$
#include  "EntryExit.h"


ACE_INLINE
TAO::DCPS::TransportSendControlElement::TransportSendControlElement
                                    (int                    initial_count,
                                     RepoId                 publisher_id,
                                     TransportSendListener* listener,
                                     ACE_Message_Block*     msg_block)
  : TransportQueueElement(initial_count),
    publisher_id_(publisher_id),
    listener_(listener),
    msg_(msg_block)
{
  DBG_ENTRY("TransportSendControlElement","TransportSendControlElement");
}

