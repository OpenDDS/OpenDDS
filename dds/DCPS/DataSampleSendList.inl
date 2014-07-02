/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataSampleElement.h"
#include <algorithm>

namespace OpenDDS {
namespace DCPS {


ACE_INLINE
DataSampleSendList::DataSampleSendList()
  : head_(0),
    tail_(0),
    size_(0)
{
}

ACE_INLINE
void DataSampleSendList::reset()
{
  head_ = tail_ = 0;
  size_ = 0;
}

ACE_INLINE
void
DataSampleSendList::enqueue_tail(const DataSampleElement* sample)
{
  ++size_;

  // const_cast here so that higher layers don't need to pass around so many
  // non-const pointers to DataSampleElement.  Ideally the design would be
  // changed to accommodate const-correctness throughout.
  DataSampleElement* mSample = const_cast<DataSampleElement*>(sample);

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
DataSampleSendList::dequeue_head(DataSampleElement*& stale)
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
DataSampleSendList::iterator
DataSampleSendList::begin()
{
  return iterator(this->head_, this->tail_, this->head_);
}

ACE_INLINE
DataSampleSendList::iterator
DataSampleSendList::end()
{
  return iterator(this->head_, this->tail_, 0);
}

} // namespace DCPS
} // namespace OpenDDS
