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
    new_count = --this->count_;
  }

  if (new_count > 0)
    {
      return;
    }

  // The queue elements are released to its cached allocator
  // in release_element() call.
  this->release_element(dropped_by_transport);
}


ACE_INLINE
bool
TAO::DCPS::TransportQueueElement::was_dropped() const
{
  return this->dropped_;
}
