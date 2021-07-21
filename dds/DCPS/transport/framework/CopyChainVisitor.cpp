/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "CopyChainVisitor.h"
#include "TransportQueueElement.h"

#if !defined (__ACE_INLINE__)
#include "CopyChainVisitor.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::CopyChainVisitor::~CopyChainVisitor()
{
  DBG_ENTRY_LVL("CopyChainVisitor", "~CopyChainVisitor", 6);
}

int
OpenDDS::DCPS::CopyChainVisitor::visit_element(TransportQueueElement* element)
{
  DBG_ENTRY_LVL("CopyChainVisitor", "visit_element", 6);

  TransportRetainedElement* copiedElement = 0;
  if (duplicate_) {
    // Create a new copy of the current element.
    copiedElement = new
      TransportRetainedElement(
        element->duplicate_msg(),
        element->publication_id()
      );
  } else {
    // Create a new copy of the current element.
    copiedElement = new
      TransportRetainedElement(
        element->msg(),
        element->publication_id(),
        mb_allocator_,
        db_allocator_
      );
  }

  // Add the copy to the target.
  target_.put(copiedElement);

  // Visit entire queue.
  return 1;
}
