// -*- C++ -*-
//
// $Id$
#include  "ace/Message_Block.h"
#include  "EntryExit.h"


ACE_INLINE
TAO::DCPS::TransportQueueElement::TransportQueueElement(int initial_count)
  : count_(initial_count),
    dropped_(false)
{
  DBG_ENTRY("TransportQueueElement","TransportQueueElement");
}


ACE_INLINE
bool
TAO::DCPS::TransportQueueElement::operator==
                                     (const ACE_Message_Block* sample) const
{
  DBG_ENTRY("TransportQueueElement","operator==");
  return (sample->rd_ptr() == this->msg()->rd_ptr());
}


ACE_INLINE
void
TAO::DCPS::TransportQueueElement::data_dropped()
{
  DBG_ENTRY("TransportQueueElement","data_dropped");
  this->dropped_ = true;
  this->decision_made();
}


ACE_INLINE
void
TAO::DCPS::TransportQueueElement::data_delivered()
{
  DBG_ENTRY("TransportQueueElement","data_delivered");
  this->decision_made();
}


ACE_INLINE
void
TAO::DCPS::TransportQueueElement::decision_made()
{
  DBG_ENTRY("TransportQueueElement","decision_made");

  int new_count;

  {
    GuardType guard(this->lock_);
    new_count = --this->count_;
  }

  if (new_count > 0)
    {
      return;
    }

  // The queue elements are released to its cached allocator
  // in release_element() call.
  this->release_element();
}


ACE_INLINE
bool
TAO::DCPS::TransportQueueElement::was_dropped() const
{
  return this->dropped_;
}
