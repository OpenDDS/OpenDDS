/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReceivedDataElementList.h"
#include "InstanceState.h"

ACE_INLINE
void
OpenDDS::DCPS::ReceivedDataElementList::add(ReceivedDataElement *data_sample)
{
  // The default action is to simply add to the
  // tail - in the future we may want to add
  // to the middle of the list based on sequence
  // number and/or source timestamp

  data_sample->previous_data_sample_ = 0;
  data_sample->next_data_sample_ = 0;

  ++size_;

  if (data_sample->sample_state_ == DDS::NOT_READ_SAMPLE_STATE) {
    increment_not_read_count();
  } else {
    increment_read_count();
  }

  if (!head_) {
    // First sample in the list.
    head_ = tail_ = data_sample;

  } else {
    // Add to existing list.
    tail_->next_data_sample_ = data_sample;
    data_sample->previous_data_sample_ = tail_;
    tail_ = data_sample;
  }

  if (instance_state_) {
    instance_state_->empty(false);
  }
  OPENDDS_ASSERT(sanity_check());
}

ACE_INLINE
void
OpenDDS::DCPS::ReceivedDataElementList::add_by_timestamp(ReceivedDataElement *data_sample)
{
  data_sample->previous_data_sample_ = 0;
  data_sample->next_data_sample_ = 0;

  for (ReceivedDataElement* it = head_; it != 0; it = it->next_data_sample_) {
    if (data_sample->source_timestamp_ < it->source_timestamp_) {
      data_sample->previous_data_sample_ = it->previous_data_sample_;
      data_sample->next_data_sample_ = it;

      // Are we replacing the head?
      if (it->previous_data_sample_ == 0) {
        head_ = data_sample;
      } else {
        it->previous_data_sample_->next_data_sample_ = data_sample;
      }
      it->previous_data_sample_ = data_sample;

      ++size_;
      if (data_sample->sample_state_ == DDS::NOT_READ_SAMPLE_STATE) {
        increment_not_read_count();
      } else {
        increment_read_count();
      }

      OPENDDS_ASSERT(sanity_check());
      return;
    }
  }

  add(data_sample);
}

ACE_INLINE
OpenDDS::DCPS::ReceivedDataElement*
OpenDDS::DCPS::ReceivedDataElementList::remove_head()
{
  if (!size_) {
    return 0;
  }

  OpenDDS::DCPS::ReceivedDataElement *ptr = head_;
  remove(head_);
  return ptr;
}

ACE_INLINE
OpenDDS::DCPS::ReceivedDataElement*
OpenDDS::DCPS::ReceivedDataElementList::remove_tail()
{
  if (!size_) {
    return 0;
  }

  OpenDDS::DCPS::ReceivedDataElement* ptr = tail_;
  remove(tail_);
  return ptr;
}
