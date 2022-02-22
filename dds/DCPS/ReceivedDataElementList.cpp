/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReceivedDataElementList.h"

#include "DataReaderImpl.h"

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


void* OpenDDS::DCPS::ReceivedDataElement::operator new(size_t , ACE_New_Allocator& pool)
{
  OpenDDS::DCPS::ReceivedDataElementMemoryBlock* block =  static_cast<OpenDDS::DCPS::ReceivedDataElementMemoryBlock*>(pool.malloc(sizeof(OpenDDS::DCPS::ReceivedDataElementMemoryBlock)));
  block->allocator_ = &pool;
  return block;
}

void OpenDDS::DCPS::ReceivedDataElement::operator delete(void* memory)
{
  if (memory) {
    OpenDDS::DCPS::ReceivedDataElementMemoryBlock* block = static_cast<OpenDDS::DCPS::ReceivedDataElementMemoryBlock*>(memory);
    block->allocator_->free(block);
  }
}

void OpenDDS::DCPS::ReceivedDataElement::operator delete(void* memory, ACE_New_Allocator&)
{
  operator delete(memory);
}

OpenDDS::DCPS::ReceivedDataElementList::ReceivedDataElementList(DataReaderImpl* reader, InstanceState_rch instance_state)
  : reader_(reader), head_(0), tail_(0), size_(0)
  , read_sample_count_(0), not_read_sample_count_(0), sample_states_(0)
  , instance_state_(instance_state)
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
      if (item->sample_state_ == DDS::NOT_READ_SAMPLE_STATE) {
        decrement_not_read_count();
      } else {
        decrement_read_count();
      }
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
OpenDDS::DCPS::ReceivedDataElementList::has_zero_copies() const
{
  for (ReceivedDataElement* item = head_; item != 0; item = item->next_data_sample_) {
    if (item->zero_copy_cnt_) {
      return true;
    }
  }
  return false;
}

bool
OpenDDS::DCPS::ReceivedDataElementList::matches(CORBA::ULong sample_states) const
{
  return sample_states_ & sample_states;
}

OpenDDS::DCPS::ReceivedDataElement*
OpenDDS::DCPS::ReceivedDataElementList::get_next_match(CORBA::ULong sample_states, ReceivedDataElement* prev)
{
  OPENDDS_ASSERT(prev == NULL || prev == tail_ || prev->next_data_sample_->previous_data_sample_ == prev);
  OPENDDS_ASSERT(prev == NULL || prev == head_ || prev->previous_data_sample_->next_data_sample_ == prev);
  if (prev == tail_) {
    return NULL;
  }
  ReceivedDataElement* item;
  if (prev == NULL) {
    item = head_;
  } else {
    item = prev->next_data_sample_;
  }
  for (; item != 0; item = item->next_data_sample_) {
    if (item->sample_state_ & sample_states) {
      return item;
    }
  }
  return NULL;
}

bool
OpenDDS::DCPS::ReceivedDataElementList::remove(ReceivedDataElement* data_sample)
{
  IdentityFilter match(data_sample);
  return remove(match, false); // short-circuit evaluation
}

void
OpenDDS::DCPS::ReceivedDataElementList::mark_read(ReceivedDataElement* item)
{
  if (item->sample_state_ & DDS::NOT_READ_SAMPLE_STATE) {
    item->sample_state_ = DDS::READ_SAMPLE_STATE;
    decrement_not_read_count();
    increment_read_count();
  }
}

void OpenDDS::DCPS::ReceivedDataElementList::increment_read_count()
{
  if (!read_sample_count_) {
    sample_states_ |= DDS::READ_SAMPLE_STATE;
    reader_->state_updated(instance_state_->instance_handle());
  }
  ++read_sample_count_;
}

void OpenDDS::DCPS::ReceivedDataElementList::decrement_read_count()
{
  OPENDDS_ASSERT(read_sample_count_);
  --read_sample_count_;
  if (!read_sample_count_) {
    sample_states_ &= (~DDS::READ_SAMPLE_STATE);
    reader_->state_updated(instance_state_->instance_handle());
  }
}

void OpenDDS::DCPS::ReceivedDataElementList::increment_not_read_count()
{
  if (!not_read_sample_count_) {
    sample_states_ |= DDS::NOT_READ_SAMPLE_STATE;
    reader_->state_updated(instance_state_->instance_handle());
  }
  ++not_read_sample_count_;
}

void OpenDDS::DCPS::ReceivedDataElementList::decrement_not_read_count()
{
  OPENDDS_ASSERT(not_read_sample_count_);
  --not_read_sample_count_;
  if (!not_read_sample_count_) {
    sample_states_ &= (~DDS::NOT_READ_SAMPLE_STATE);
    reader_->state_updated(instance_state_->instance_handle());
  }
}
