/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/Message_Block.h"
#include "EntryExit.h"

#include <assert.h>

ACE_INLINE
OpenDDS::DCPS::TransportQueueElement::TransportQueueElement(int initial_count)
  : sub_loan_count_(initial_count),
    dropped_(false),
    released_(false)
{
  DBG_ENTRY_LVL("TransportQueueElement","TransportQueueElement",6);
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportQueueElement::operator==(
  const TransportQueueElement& rhs
) const
{
  DBG_ENTRY_LVL("TransportQueueElement","operator==",6);
  if( this->msg()) {
    return this->msg()->rd_ptr() == rhs.msg()->rd_ptr();

  } else {
    return  (this->publication_id() == rhs.publication_id())
        && !(this->publication_id() == GUID_UNKNOWN);
  }
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportQueueElement::data_dropped(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportQueueElement","data_dropped",6);
  this->dropped_ = true;
  return this->decision_made(dropped_by_transport);
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportQueueElement::data_delivered()
{
  DBG_ENTRY_LVL("TransportQueueElement","data_delivered",6);
  // Decision made depend on dropped_ flag. If any link drops
  // the sample even other links deliver successfully, the
  // data dropped by transport will called back to writer.
  return this->decision_made(this->dropped_);
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportQueueElement::decision_made(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportQueueElement","decision_made",6);

  int new_count;

  {
    GuardType guard(this->lock_);
    new_count = --this->sub_loan_count_;
  }

  if (new_count == 0) {
    // All interested subscriptions have been satisfied.

    // The queue elements are released to its cached allocator
    // in release_element() call.
    // It's not necessary to set the released_ flag to true
    // as this element will be released anyway and not be 
    // accessible. Note it can not be set after release_element 
    // call.
    // this->released_ = true;
    this->release_element(dropped_by_transport);
    return true;
  }

  // ciju: The sub_loan_count_ has been observed to drop below zero.
  // Since it isn't exactly a ref count and the object is created in
  // allocater memory (user space) we *probably* can disregard the
  // count for now. Ideally we would like to prevent the count from
  // falling below 0 and opening up this assert.
  // assert (new_count > 0);
  return false;
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportQueueElement::was_dropped() const
{
  return this->dropped_;
}

ACE_INLINE
bool 
OpenDDS::DCPS::TransportQueueElement::released() const
{
  return this->released_;
}


ACE_INLINE
void 
OpenDDS::DCPS::TransportQueueElement::released(bool flag)
{
  this->released_ = flag;
}

   

