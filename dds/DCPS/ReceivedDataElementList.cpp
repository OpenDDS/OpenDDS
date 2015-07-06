/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReceivedDataElementList.h"

#if !defined (__ACE_INLINE__)
# include "ReceivedDataElementList.inl"
#endif /* !__ACE_INLINE__ */

namespace {

class IdentityFilter
  : public OpenDDS::DCPS::ReceivedDataFilter {
public:
  explicit IdentityFilter(OpenDDS::DCPS::ReceivedDataElement* data_sample)
    : data_sample_(data_sample)
  {}

  bool
  operator()(OpenDDS::DCPS::ReceivedDataElement* data_sample) {
    return this->data_sample_ == data_sample;
  }

private:
  OpenDDS::DCPS::ReceivedDataElement* data_sample_;
};

} // namespace

OpenDDS::DCPS::ReceivedDataElementList::ReceivedDataElementList(InstanceState *instance_state)
  : head_(0), tail_(0), size_(0), instance_state_(instance_state)
{
}

OpenDDS::DCPS::ReceivedDataElementList::~ReceivedDataElementList()
{
  // The memory pointed to by instance_state_ is owned by
  // another object.
}

void
OpenDDS::DCPS::ReceivedDataElementList::apply_all(
  ReceivedDataFilter& match,
  ReceivedDataOperation& op)
{
  for (ReceivedDataElement* it = head_;
       it != 0 ; it = it->next_data_sample_) {
    if (match(it)) {
      op(it);
    }
  }
}

bool
OpenDDS::DCPS::ReceivedDataElementList::remove(
  ReceivedDataFilter& match,
  bool eval_all)
{
  if (!head_) {
    return false;
  }

  bool released = false;

  for (ReceivedDataElement* item = head_ ; item != 0 ;
       item = item->next_data_sample_) {
    if (match(item)) {
      size_-- ;

      if (item == head_) {
        if (head_ == tail_) {
          head_ = tail_ = 0;

        } else {
          head_ = item->next_data_sample_ ;

          if (head_) {
            head_->previous_data_sample_ = 0 ;
          }
        }

      } else if (item == tail_) {
        tail_ = item->previous_data_sample_ ;

        if (tail_) {
          tail_->next_data_sample_ = 0 ;
        }

      } else {
        item->previous_data_sample_->next_data_sample_ =
          item->next_data_sample_ ;
        item->next_data_sample_->previous_data_sample_ =
          item->previous_data_sample_ ;
      }

      if (instance_state_ && size_ == 0) {
        // let the instance know it is empty
        released = released || instance_state_->empty(true);
      }

      if (!eval_all) break;
    }
  }

  return released;
}

bool
OpenDDS::DCPS::ReceivedDataElementList::remove(ReceivedDataElement *data_sample)
{
  IdentityFilter match(data_sample);
  return remove(match, false); // short-circuit evaluation
}
