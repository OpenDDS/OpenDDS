/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <algorithm>

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
DataSampleListElement::DataSampleListElement(
  PublicationId           publication_id,
  TransportSendListener*  send_listner,
  PublicationInstance*    handle,
  TransportSendElementAllocator* tse_allocator,
  TransportCustomizedElementAllocator* tce_allocator)
  : sample_(0),
    publication_id_(publication_id),
    num_subs_(0),
    previous_sample_(0),
    next_sample_(0),
    next_instance_sample_(0),
    next_send_sample_(0),
    previous_send_sample_(0),
    send_listener_(send_listner),
    space_available_(false),
    handle_(handle),
    transport_send_element_allocator_(tse_allocator),
    transport_customized_element_allocator_(tce_allocator)
{
}

ACE_INLINE
DataSampleListElement::DataSampleListElement(const DataSampleListElement& elem)
  : header_(elem.header_)
  , sample_(elem.sample_->duplicate())
  , publication_id_(elem.publication_id_)
  , num_subs_(elem.num_subs_)
  , previous_sample_(elem.previous_sample_)
  , next_sample_(elem.next_sample_)
  , next_instance_sample_(elem.next_instance_sample_)
  , next_send_sample_(elem.next_send_sample_)
  , previous_send_sample_(elem.previous_send_sample_)
  , send_listener_(elem.send_listener_)
  , space_available_(elem.space_available_)
  , handle_(elem.handle_)
  , transport_send_element_allocator_(
      elem.transport_send_element_allocator_)
  , transport_customized_element_allocator_(
      elem.transport_customized_element_allocator_)
  , filter_out_(elem.filter_out_)
  , filter_per_link_(elem.filter_per_link_)
{
  std::copy(elem.subscription_ids_,
            elem.subscription_ids_ + num_subs_,
            subscription_ids_);

}

ACE_INLINE
DataSampleListElement::~DataSampleListElement()
{
  if (sample_) {
    sample_->release();
  }
}

ACE_INLINE
DataSampleListElement&
DataSampleListElement::operator=(const DataSampleListElement& rhs)
{
  header_ = rhs.header_;
  sample_ = rhs.sample_->duplicate();
  publication_id_ = rhs.publication_id_;
  num_subs_ = rhs.num_subs_;
  std::copy(rhs.subscription_ids_,
            rhs.subscription_ids_ + num_subs_,
            subscription_ids_);
  previous_sample_ = rhs.previous_sample_;
  next_sample_ = rhs.next_sample_;
  next_instance_sample_ = rhs.next_instance_sample_;
  next_send_sample_ = rhs.next_send_sample_;
  previous_send_sample_ = rhs.previous_send_sample_;
  send_listener_ = rhs.send_listener_;
  space_available_ = rhs.space_available_;
  handle_ = rhs.handle_;
  transport_send_element_allocator_ = rhs.transport_send_element_allocator_;
  transport_customized_element_allocator_ =
    rhs.transport_customized_element_allocator_;
  filter_out_ = rhs.filter_out_;
  filter_per_link_ = rhs.filter_per_link_;

  return *this;
}

// --------------------------------------------

ACE_INLINE
DataSampleList::DataSampleList()
  : head_(0),
    tail_(0),
    size_(0)
{
}

ACE_INLINE
void DataSampleList::reset()
{
  head_ = tail_ = 0;
  size_ = 0;
}

ACE_INLINE
void
DataSampleList::enqueue_tail_next_sample(DataSampleListElement* sample)
{
  //sample->previous_sample_ = 0;
  //sample->next_sample_ = 0;

  ++size_ ;

  if (head_ == 0) {
    // First sample in the list.
    head_ = tail_ = sample ;

  } else {
    // Add to existing list.
    tail_->next_sample_ = sample ;
    sample->previous_sample_ = tail_;
    tail_ = sample;
  }
}

ACE_INLINE
bool
DataSampleList::dequeue_head_next_sample(DataSampleListElement*& stale)
{
  //
  // Remove the oldest sample from the list.
  //
  stale = head_;

  if (head_ == 0) {
    return false;

  } else {
    --size_ ;
    head_ = head_->next_sample_;

    if (head_ == 0) {
      tail_ = 0;

    } else {
      head_->previous_sample_ = 0;
    }

    stale->next_sample_ = 0;
    stale->previous_sample_ = 0;
    return true;
  }
}

ACE_INLINE
void
DataSampleList::enqueue_tail_next_send_sample(const DataSampleListElement* sample)
{
  ++size_;

  // const_cast here so that higher layers don't need to pass around so many
  // non-const pointers to DataSampleListElement.  Ideally the design would be
  // changed to accomdate const-correctness throughout.
  DataSampleListElement* mSample = const_cast<DataSampleListElement*>(sample);

  if (head_ == 0) {
    // First sample in list.
    head_ = tail_ = mSample;

  } else {
    // Add to existing list.
    //sample->previous_sample_ = tail_;
    //tail_->next_sample_ = sample;
    mSample->previous_send_sample_ = tail_;
    tail_->next_send_sample_ = mSample;
    tail_ = mSample;
  }
}

ACE_INLINE
bool
DataSampleList::dequeue_head_next_send_sample(DataSampleListElement*& stale)
{
  //
  // Remove the oldest sample from the instance list.
  //
  stale = head_;

  if (head_ == 0) {
    return false;

  } else {
    --size_ ;

    head_ = head_->next_send_sample_ ;

    if (head_ == 0) {
      tail_ = 0;

    } else {
      head_->previous_send_sample_ = 0;
    }

    //else
    //  {
    //    head_->previous_sample_ = 0;
    //  }

    stale->next_send_sample_ = 0 ;
    stale->previous_send_sample_ = 0 ;

    return true;
  }
}

ACE_INLINE
void
DataSampleList::enqueue_tail_next_instance_sample(DataSampleListElement* sample)
{
  sample->next_instance_sample_ = 0;

  ++ size_ ;

  if (head_ == 0) {
    // First sample on queue.
    head_ = tail_ = sample ;

  } else {
    // Another sample on an existing queue.
    tail_->next_instance_sample_ = sample ;
    tail_ = sample ;
  }
}

ACE_INLINE
bool
DataSampleList::dequeue_head_next_instance_sample(DataSampleListElement*& stale)
{
  //
  // Remove the oldest sample from the instance list.
  //
  stale = head_;

  if (head_ == 0) {
    // try to dequeue empty instance list.
    return false;

  } else {
    --size_ ;
    head_ = head_->next_instance_sample_ ;

    if (head_ == 0) {
      tail_ = 0;
    }

    stale->next_instance_sample_ = 0;
    return true;
  }
}

ACE_INLINE
DataSampleList::iterator
DataSampleList::begin()
{
  return iterator(this->head_, this->tail_, this->head_);
}

ACE_INLINE
DataSampleList::iterator
DataSampleList::end()
{
  return iterator(this->head_, this->tail_, 0);
}

} // namespace DCPS
} // namespace OpenDDS
