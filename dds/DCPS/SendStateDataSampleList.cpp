/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "SendStateDataSampleList.h"
#include "DataSampleElement.h"
#include "Definitions.h"
#include "PublicationInstance.h"

#include "dds/DCPS/transport/framework/TransportSendListener.h"

#if !defined (__ACE_INLINE__)
#include "SendStateDataSampleList.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


const SendStateDataSampleList*
SendStateDataSampleList::send_list_containing_element(const DataSampleElement* element,
                                                      SendStateDataSampleList** begin,
                                                      SendStateDataSampleList** end)
{
  const DataSampleElement* head = element;

  while (head->previous_send_sample_ != 0) {
    head = head->previous_send_sample_;
  }

  for (SendStateDataSampleList** it = begin; it != end; ++it) {
    if ((*it)->head_ == head) {
      return *it;
    }
  }
  return 0;
}


bool
SendStateDataSampleList::dequeue(const DataSampleElement* stale)
{
  if (head_ == 0) {
    return false;
  }

  // Same as dequeue from head.
  if (stale == head_) {
    DataSampleElement* tmp = head_;
    return dequeue_head(tmp);
  }

  // Search from head_->next_send_sample_.
  DataSampleElement* toRemove = 0;
  for (DataSampleElement* item = head_->next_send_sample_;
       item != 0 && toRemove == 0;
       item = item->next_send_sample_) {
    if (item == stale) {
      toRemove = item;
    }
  }

  if (toRemove) {
    size_ --;
    // Remove from the previous element.
    toRemove->previous_send_sample_->next_send_sample_ = toRemove->next_send_sample_ ;

    // Remove from the next element.
    if (toRemove->next_send_sample_ != 0) {
      // Remove from the inside of the list.
      toRemove->next_send_sample_->previous_send_sample_ = toRemove->previous_send_sample_ ;

    } else {
      toRemove->previous_send_sample_->next_send_sample_ = 0;
      // Remove from the tail of the list.
      tail_ = toRemove->previous_send_sample_ ;
    }

    toRemove->next_send_sample_ = 0;
    toRemove->previous_send_sample_ = 0;
  }

  return toRemove;
}

void
SendStateDataSampleList::enqueue_tail(SendStateDataSampleList list)
{
  //// Make the appended list linked with next_send_sample_ first.
  //DataSampleElement* cur = list.head_;

  //if (list.size_ > 1 && cur->next_send_sample_ == 0)
  // {
  //   for (ssize_t i = 0; i < list.size_; i ++)
  //     {
  //       cur->next_send_sample_ = cur->next_writer_sample_;
  //       cur = cur->next_writer_sample_;
  //     }
  // }

  if (head_ == 0) {
    head_ = list.head_;
    tail_ = list.tail_;
    size_ = list.size_;

  } else {
    tail_->next_send_sample_
    //= tail_->next_writer_sample_
    = list.head_;
    list.head_->previous_send_sample_ = tail_;
    //list.head_->previous_writer_sample_ = tail_;
    tail_ = list.tail_;
    size_ = size_ + list.size_;
  }
}

// -----------------------------------------------

SendStateDataSampleListIterator::SendStateDataSampleListIterator(
  DataSampleElement* head,
  DataSampleElement* tail,
  DataSampleElement* current)
  : head_(head)
  , tail_(tail)
  , current_(current)
{
}

SendStateDataSampleListIterator&
SendStateDataSampleListIterator::operator++()
{
  if (this->current_)
    this->current_ = this->current_->next_send_sample_;

  return *this;
}

SendStateDataSampleListIterator
SendStateDataSampleListIterator::operator++(int)
{
  SendStateDataSampleListIterator tmp(*this);
  ++(*this);
  return tmp;
}

SendStateDataSampleListIterator&
SendStateDataSampleListIterator::operator--()
{
  if (this->current_)
    this->current_ = this->current_->previous_send_sample_;

  else
    this->current_ = this->tail_;

  return *this;
}

SendStateDataSampleListIterator
SendStateDataSampleListIterator::operator--(int)
{
  SendStateDataSampleListIterator tmp(*this);
  --(*this);
  return tmp;
}

SendStateDataSampleListIterator::reference
SendStateDataSampleListIterator::operator*()
{
  // Hopefully folks will be smart enough to not dereference a
  // null iterator.  Such a case should only exist for an "end"
  // iterator.  Otherwise we may want to throw an exception here.
  // assert (this->current_ != 0);

  return *(this->current_);
}

SendStateDataSampleListIterator::pointer
SendStateDataSampleListIterator::operator->()
{
  return this->current_;
}

SendStateDataSampleListConstIterator::SendStateDataSampleListConstIterator(
  const DataSampleElement* head,
  const DataSampleElement* tail,
  const DataSampleElement* current)
  : head_(head)
  , tail_(tail)
  , current_(current)
{
}

SendStateDataSampleListConstIterator::SendStateDataSampleListConstIterator(
  const SendStateDataSampleListIterator& iterator)
  : head_(iterator.head_)
  , tail_(iterator.tail_)
  , current_(iterator.current_)
{
}

SendStateDataSampleListConstIterator&
SendStateDataSampleListConstIterator::operator++()
{
  if (this->current_)
    this->current_ = this->current_->next_send_sample_;

  return *this;
}

SendStateDataSampleListConstIterator
SendStateDataSampleListConstIterator::operator++(int)
{
  SendStateDataSampleListConstIterator tmp(*this);
  ++(*this);
  return tmp;
}

SendStateDataSampleListConstIterator&
SendStateDataSampleListConstIterator::operator--()
{
  if (this->current_)
    this->current_ = this->current_->previous_send_sample_;

  else
    this->current_ = this->tail_;

  return *this;
}

SendStateDataSampleListConstIterator
SendStateDataSampleListConstIterator::operator--(int)
{
  SendStateDataSampleListConstIterator tmp(*this);
  --(*this);
  return tmp;
}

SendStateDataSampleListConstIterator::reference
SendStateDataSampleListConstIterator::operator*() const
{
  // Hopefully folks will be smart enough to not dereference a
  // null iterator.  Such a case should only exist for an "end"
  // iterator.  Otherwise we may want to throw an exception here.
  // assert (this->current_ != 0);

  return *(this->current_);
}

SendStateDataSampleListConstIterator::pointer
SendStateDataSampleListConstIterator::operator->() const
{
  return this->current_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
