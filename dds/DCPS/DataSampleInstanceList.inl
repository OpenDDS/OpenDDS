/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DataSampleListElement.h"
#include <algorithm>

namespace OpenDDS {
namespace DCPS {


ACE_INLINE
DataSampleInstanceList::DataSampleInstanceList()
  : head_(0),
    tail_(0),
    size_(0)
{
}

ACE_INLINE
void DataSampleInstanceList::reset()
{
  head_ = tail_ = 0;
  size_ = 0;
}

ACE_INLINE
void
DataSampleInstanceList::enqueue_tail(const DataSampleListElement* sample)
{
  // const_cast here so that higher layers don't need to pass around so many
  // non-const pointers to DataSampleListElement.  Ideally the design would be
  // changed to accommodate const-correctness throughout.
  DataSampleListElement* mSample = const_cast<DataSampleListElement*>(sample);
  
  mSample->next_instance_sample_ = 0;

  ++ size_ ;
  

  
  if (head_ == 0) {
    // First sample on queue.
    head_ = tail_ = mSample ;

  } else {
    // Another sample on an existing queue.
    tail_->next_instance_sample_ = mSample ;
    tail_ = mSample ;
  }
}

ACE_INLINE
bool
DataSampleInstanceList::dequeue_head(DataSampleListElement*& stale)
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

} // namespace DCPS
} // namespace OpenDDS
