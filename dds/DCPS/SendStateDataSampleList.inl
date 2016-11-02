/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataSampleElement.h"
#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
SendStateDataSampleList::SendStateDataSampleList()
  : head_(0),
    tail_(0),
    size_(0)
{
}

ACE_INLINE
void SendStateDataSampleList::reset()
{
  head_ = tail_ = 0;
  size_ = 0;
}

ACE_INLINE
ssize_t
SendStateDataSampleList::size() const
{
  return size_;
}

ACE_INLINE
DataSampleElement*
SendStateDataSampleList::head() const
{
  return head_;
}

ACE_INLINE
DataSampleElement*
SendStateDataSampleList::tail() const
{
  return tail_;
}

ACE_INLINE
void
SendStateDataSampleList::enqueue_head(const DataSampleElement* sample)
{
  ++size_;

  // const_cast here so that higher layers don't need to pass around so many
  // non-const pointers to DataSampleElement.  Ideally the design would be
  // changed to accommodate const-correctness throughout.
  DataSampleElement* mSample = const_cast<DataSampleElement*>(sample);

  if (head_ == 0) {
    head_ = tail_ = mSample;
    sample->next_send_sample_ = sample->previous_send_sample_ = 0;

  } else {
    sample->next_send_sample_ = head_;
    sample->previous_send_sample_ = 0;
    head_->previous_send_sample_ = mSample;
    head_ = mSample;
  }
}

ACE_INLINE
void
SendStateDataSampleList::enqueue_tail(const DataSampleElement* sample)
{
  ++size_;

  // const_cast here so that higher layers don't need to pass around so many
  // non-const pointers to DataSampleElement.  Ideally the design would be
  // changed to accommodate const-correctness throughout.
  DataSampleElement* mSample = const_cast<DataSampleElement*>(sample);

  if (head_ == 0) {
    head_ = tail_ = mSample;
    sample->next_send_sample_ = sample->previous_send_sample_ = 0;

  } else {
    sample->previous_send_sample_ = tail_;
    sample->next_send_sample_ = 0;
    tail_->next_send_sample_ = mSample;
    tail_ = mSample;
  }
}

ACE_INLINE
bool
SendStateDataSampleList::dequeue_head(DataSampleElement*& stale)
{
  //
  // Remove the oldest sample from the instance list.
  //
  stale = head_;

  if (head_ == 0) {
    return false;

  } else {
    --size_;

    head_ = head_->next_send_sample_;

    if (head_ == 0) {
      tail_ = 0;

    } else {
      head_->previous_send_sample_ = 0;
    }

    stale->next_send_sample_ = 0;
    stale->previous_send_sample_ = 0;

    return true;
  }
}

ACE_INLINE
void
SendStateDataSampleList::remove(DataSampleElement* stale)
{
  if (stale->previous_send_sample_)
    stale->previous_send_sample_->next_send_sample_ = stale->next_send_sample_;
  if (stale->next_send_sample_)
    stale->next_send_sample_->previous_send_sample_ = stale->previous_send_sample_;
}

ACE_INLINE
SendStateDataSampleList::iterator
SendStateDataSampleList::begin()
{
  return iterator(this->head_, this->tail_, this->head_);
}

ACE_INLINE
SendStateDataSampleList::iterator
SendStateDataSampleList::end()
{
  return iterator(this->head_, this->tail_, 0);
}

ACE_INLINE
SendStateDataSampleList::const_iterator
SendStateDataSampleList::begin() const
{
  return const_iterator(this->head_, this->tail_, this->head_);
}

ACE_INLINE
SendStateDataSampleList::const_iterator
SendStateDataSampleList::end() const
{
  return const_iterator(this->head_, this->tail_, 0);
}

ACE_INLINE
SendStateDataSampleList::reverse_iterator
SendStateDataSampleList::rbegin()
{
  return reverse_iterator(end());
}

ACE_INLINE
SendStateDataSampleList::reverse_iterator
SendStateDataSampleList::rend()
{
  return reverse_iterator(begin());
}

ACE_INLINE
SendStateDataSampleList::const_reverse_iterator
SendStateDataSampleList::rbegin() const
{
  return const_reverse_iterator(end());
}

ACE_INLINE
SendStateDataSampleList::const_reverse_iterator
SendStateDataSampleList::rend() const
{
  return const_reverse_iterator(begin());
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
