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

  ++size_ ;

  if (!head_) {
    // First sample in the list.
    head_ = tail_ = data_sample ;

  } else {
    // Add to existing list.
    tail_->next_data_sample_ = data_sample ;
    data_sample->previous_data_sample_ = tail_;
    tail_ = data_sample;
  }

  if (instance_state_) {
    instance_state_->empty(false);
  }
}

ACE_INLINE
OpenDDS::DCPS::ReceivedDataElement *
OpenDDS::DCPS::ReceivedDataElementList::remove_head()
{
  if (!size_) {
    return 0 ;
  }

  OpenDDS::DCPS::ReceivedDataElement *ptr = head_ ;

  remove(head_) ;

  return ptr ;
}

ACE_INLINE
OpenDDS::DCPS::ReceivedDataElement *
OpenDDS::DCPS::ReceivedDataElementList::remove_tail()
{
  if (!size_) {
    return 0 ;
  }

  OpenDDS::DCPS::ReceivedDataElement *ptr = tail_ ;

  remove(tail_) ;

  return ptr ;
}
