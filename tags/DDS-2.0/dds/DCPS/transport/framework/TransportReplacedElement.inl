/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportReplacedElement::TransportReplacedElement
(TransportQueueElement* orig_elem,
 TransportReplacedElementAllocator* allocator)
  : TransportQueueElement(1),
    allocator_(allocator)
{
  DBG_ENTRY_LVL("TransportReplacedElement","TransportReplacedElement",6);

  // Obtain the publisher id.
  this->publisher_id_ = orig_elem->publication_id();

  // Make a deep-copy of the orig_elem->msg() chain of ACE_Message_Blocks.
  this->msg_ = orig_elem->msg()->clone();
}
