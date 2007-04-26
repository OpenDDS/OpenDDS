// -*- C++ -*-
//
// $Id$
#include "EntryExit.h"

ACE_INLINE
TAO::DCPS::TransportReplacedElement::TransportReplacedElement
                                           (TransportQueueElement* orig_elem,
                                            TransportReplacedElementAllocator* allocator)
  : TransportQueueElement(1),
    allocator_(allocator)
{
  DBG_ENTRY_LVL("TransportReplacedElement","TransportReplacedElement",5);

  // Obtain the publisher id.
  this->publisher_id_ = orig_elem->publication_id();

  // Make a deep-copy of the orig_elem->msg() chain of ACE_Message_Blocks.
  this->msg_ = orig_elem->msg()->clone();
}

