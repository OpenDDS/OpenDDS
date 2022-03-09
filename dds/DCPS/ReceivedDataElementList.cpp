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
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
      if (!data_sample->coherent_change_)
#endif
      {
        if (data_sample->sample_state_ == DDS::NOT_READ_SAMPLE_STATE) {
          increment_not_read_count();
        } else {
          increment_read_count();
        }
      }

      return;
    }
  }

  add(data_sample);
}

void
OpenDDS::DCPS::ReceivedDataElementList::apply_all(
  ReceivedDataFilter& match,
  ReceivedDataOperation& op)
{
  for (ReceivedDataElement* it = head_; it != 0; it = it->next_data_sample_) {
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

  for (ReceivedDataElement* item = head_; item != 0; item = item->next_data_sample_) {
    if (match(item)) {
      released = released || remove(item);
      if (!eval_all) break;
    }
  }

  return released;
}

bool
OpenDDS::DCPS::ReceivedDataElementList::remove(ReceivedDataElement* item)
{
  OPENDDS_ASSERT(sanity_check(item));

  if (!head_) {
    return false;
  }

  bool released = false;

  size_--;
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (!item->coherent_change_)
#endif
  {
    if (item->sample_state_ == DDS::NOT_READ_SAMPLE_STATE) {
      decrement_not_read_count();
    } else {
      decrement_read_count();
    }
  }
  if (item == head_) {
    if (head_ == tail_) {
      head_ = tail_ = 0;

    } else {
      head_ = item->next_data_sample_;

      if (head_) {
        head_->previous_data_sample_ = 0;
      }
    }

  } else if (item == tail_) {
    tail_ = item->previous_data_sample_;

    if (tail_) {
      tail_->next_data_sample_ = 0;
    }

  } else {
    item->previous_data_sample_->next_data_sample_ =
      item->next_data_sample_;
    item->next_data_sample_->previous_data_sample_ =
      item->previous_data_sample_;
  }

  item->previous_data_sample_ = 0;
  item->next_data_sample_ = 0;

  if (instance_state_ && size_ == 0) {
    // let the instance know it is empty
    released = instance_state_->empty(true);
  }

  return released;
}

bool
OpenDDS::DCPS::ReceivedDataElementList::has_zero_copies() const
{
  for (ReceivedDataElement* item = head_; item != 0; item = item->next_data_sample_) {
#ifdef ACE_HAS_CPP11
    if (item->zero_copy_cnt_) {
#else
    if (item->zero_copy_cnt_.value()) {
#endif
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
  OPENDDS_ASSERT(sanity_check(prev));
  if (prev == tail_) {
    return 0;
  }
  ReceivedDataElement* item = prev ? prev->next_data_sample_ : head_;
  for (; item != 0; item = item->next_data_sample_) {
    if ((item->sample_state_ & sample_states)
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
      && !item->coherent_change_
#endif
      ) {
      return item;
    }
  }
  return 0;
}

void
OpenDDS::DCPS::ReceivedDataElementList::mark_read(ReceivedDataElement* item)
{
  OPENDDS_ASSERT(sanity_check(item));
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (!item->coherent_change_)
#endif
  {
    if (item->sample_state_ & DDS::NOT_READ_SAMPLE_STATE) {
      item->sample_state_ = DDS::READ_SAMPLE_STATE;
      decrement_not_read_count();
      increment_read_count();
    }
  }
}

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
void
OpenDDS::DCPS::ReceivedDataElementList::accept_coherent_change(OpenDDS::DCPS::ReceivedDataElement* item)
{
  OPENDDS_ASSERT(sanity_check(item));
  if (item->coherent_change_) {
    item->coherent_change_ = false;
    increment_not_read_count();
  }
}
#endif

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

bool OpenDDS::DCPS::ReceivedDataElementList::sanity_check()
{
  OPENDDS_ASSERT(head_ == 0 || head_->previous_data_sample_ == 0);
  for (ReceivedDataElement* item = head_; item != 0; item = item->next_data_sample_) {
    sanity_check(item);
  }
  OPENDDS_ASSERT(tail_ == 0 || tail_->next_data_sample_ == 0);
  return true;
}

bool OpenDDS::DCPS::ReceivedDataElementList::sanity_check(ReceivedDataElement* item)
{
  ACE_UNUSED_ARG(item);
  OPENDDS_ASSERT(item == 0 || (item->next_data_sample_ == 0 && item == tail_) || (item->next_data_sample_ && item->next_data_sample_->previous_data_sample_ == item));
  OPENDDS_ASSERT(item == 0 || (item->previous_data_sample_ == 0 && item == head_) || (item->previous_data_sample_ && item->previous_data_sample_->next_data_sample_ == item));
  return true;
}
