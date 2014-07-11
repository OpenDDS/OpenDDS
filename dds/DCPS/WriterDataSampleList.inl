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
WriterDataSampleList::WriterDataSampleList()
  : head_(0),
    tail_(0),
    size_(0)
{
}

ACE_INLINE
void WriterDataSampleList::reset()
{
  head_ = tail_ = 0;
  size_ = 0;
}

ACE_INLINE
ssize_t
WriterDataSampleList::size() const
{
  return size_;
}

ACE_INLINE
DataSampleElement*
WriterDataSampleList::head() const
{
  return head_;
}

ACE_INLINE
DataSampleElement*
WriterDataSampleList::tail() const
{
  return tail_;
}

ACE_INLINE
void
WriterDataSampleList::enqueue_tail(const DataSampleElement* sample)
{
  // const_cast here so that higher layers don't need to pass around so many
  // non-const pointers to DataSampleElement.  Ideally the design would be
  // changed to accommodate const-correctness throughout.
  DataSampleElement* mSample = const_cast<DataSampleElement*>(sample);

  //sample->previous_writer_sample_ = 0;
  //sample->next_writer_sample_ = 0;

  ++size_ ;

  if (head_ == 0) {
    // First sample in the list.
    head_ = tail_ = mSample ;

  } else {
    // Add to existing list.
    tail_->next_writer_sample_ = mSample ;
    mSample->previous_writer_sample_ = tail_;
    tail_ = mSample;
  }
}

ACE_INLINE
bool
WriterDataSampleList::dequeue_head(DataSampleElement*& stale)
{
  //
  // Remove the oldest sample from the list.
  //
  stale = head_;

  if (head_ == 0) {
    return false;

  } else {
    --size_ ;
    head_ = head_->next_writer_sample_;

    if (head_ == 0) {
      tail_ = 0;

    } else {
      head_->previous_writer_sample_ = 0;
    }

    stale->next_writer_sample_ = 0;
    stale->previous_writer_sample_ = 0;
    return true;
  }
}


} // namespace DCPS
} // namespace OpenDDS
