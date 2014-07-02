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
DataSampleWriterList::DataSampleWriterList()
  : head_(0),
    tail_(0),
    size_(0)
{
}

ACE_INLINE
void DataSampleWriterList::reset()
{
  head_ = tail_ = 0;
  size_ = 0;
}

ACE_INLINE
void
DataSampleWriterList::enqueue_tail(const DataSampleElement* sample)
{
  // const_cast here so that higher layers don't need to pass around so many
  // non-const pointers to DataSampleElement.  Ideally the design would be
  // changed to accommodate const-correctness throughout.
  DataSampleElement* mSample = const_cast<DataSampleElement*>(sample);
  
  //sample->previous_sample_ = 0;
  //sample->next_sample_ = 0;

  ++size_ ;

  if (head_ == 0) {
    // First sample in the list.
    head_ = tail_ = mSample ;

  } else {
    // Add to existing list.
    tail_->next_sample_ = mSample ;
    mSample->previous_sample_ = tail_;
    tail_ = mSample;
  }
}

ACE_INLINE
bool
DataSampleWriterList::dequeue_head(DataSampleElement*& stale)
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


} // namespace DCPS
} // namespace OpenDDS
