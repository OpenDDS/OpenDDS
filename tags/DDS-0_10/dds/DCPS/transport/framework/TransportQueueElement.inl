// -*- C++ -*-
//
// $Id$
#include  "ace/Message_Block.h"
#include  "EntryExit.h"

#include <assert.h>

ACE_INLINE
TAO::DCPS::TransportQueueElement::TransportQueueElement(int initial_count)
  : sub_loan_count_(initial_count),
    dropped_(false)
{
  DBG_ENTRY_LVL("TransportQueueElement","TransportQueueElement",5);
}


ACE_INLINE
bool
TAO::DCPS::TransportQueueElement::operator==
                                     (const ACE_Message_Block* sample) const
{
  DBG_ENTRY_LVL("TransportQueueElement","operator==",5);
  return (sample->rd_ptr() == this->msg()->rd_ptr());
}


ACE_INLINE
void
TAO::DCPS::TransportQueueElement::data_dropped(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportQueueElement","data_dropped",5);
  this->dropped_ = true;
  this->decision_made(dropped_by_transport);
}


ACE_INLINE
void
TAO::DCPS::TransportQueueElement::data_delivered()
{
  DBG_ENTRY_LVL("TransportQueueElement","data_delivered",5);
  bool dropped = false;
  this->decision_made(dropped);
}


ACE_INLINE
void
TAO::DCPS::TransportQueueElement::decision_made(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportQueueElement","decision_made",5);

  int new_count;

  {
    GuardType guard(this->lock_);
    new_count = --this->sub_loan_count_;
  }

  if (new_count == 0)
    {
      // All interested subscriptions have been satisfied.

      // The queue elements are released to its cached allocator
      // in release_element() call.
      this->release_element(dropped_by_transport);
      return;
    }

  // ciju: The sub_loan_count_ has been observed to drop below zero.
  // Since it isn't exactly a ref count and the object is created in
  // allocater memory (user space) we *probably* can disregard the
  // count for now. Ideally we would like to prevent the count from
  // falling below 0 and opening up this assert.
  // assert (new_count > 0);
  return;
}


ACE_INLINE
bool
TAO::DCPS::TransportQueueElement::was_dropped() const
{
  return this->dropped_;
}
