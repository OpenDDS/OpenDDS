/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "dcps_export.h"

#include "GuidConverter.h"
#include "WriterInfo.h"

#include "ace/OS_NS_sys_time.h"

#include <sstream>

namespace OpenDDS {
namespace DCPS {

WriterInfoListener::WriterInfoListener()
  : subscription_id_(GUID_UNKNOWN),
  liveliness_lease_duration_(ACE_Time_Value::zero)
{
}

WriterInfoListener::~WriterInfoListener()
{
}

/// tell instances when a DataWriter transitions to being alive
/// The writer state is inout parameter, it has to be set ALIVE before
/// handle_timeout is called since some subroutine use the state.
void
WriterInfoListener::writer_became_alive(WriterInfo&,
                                        const ACE_Time_Value& )
{
}

/// tell instances when a DataWriter transitions to DEAD
/// The writer state is inout parameter, the state is set to DEAD
/// when it returns.
void
WriterInfoListener::writer_became_dead(WriterInfo&,
                                       const ACE_Time_Value& )
{
}

/// tell instance when a DataWriter is removed.
/// The liveliness status need update.
void
WriterInfoListener::writer_removed(WriterInfo& )
{
}

WriterInfo::WriterInfo()
  : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
  seen_data_(false),
  historic_samples_timer_(NOT_WAITING),
  state_(NOT_SET),
  reader_(0),
  writer_id_(GUID_UNKNOWN),
  handle_(DDS::HANDLE_NIL)
{
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  this->reset_coherent_info();
#endif
}

WriterInfo::WriterInfo(WriterInfoListener*         reader,
                       const PublicationId&        writer_id,
                       const ::DDS::DataWriterQos& writer_qos,
                       const ::DDS::DataReaderQos& reader_qos)
  : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
  seen_data_(false),
  historic_samples_timer_(NOT_WAITING),
  state_(NOT_SET),
  reader_(reader),
  writer_id_(writer_id),
  writer_qos_(writer_qos),
  handle_(DDS::HANDLE_NIL)
{
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  this->reset_coherent_info();
#endif

  if (DCPS_debug_level >= 5) {
    GuidConverter writer_converter(writer_id);
    GuidConverter reader_converter(reader->subscription_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) WriterInfo::WriterInfo: ")
               ACE_TEXT("writer %C added to reader %C.\n"),
               std::string(writer_converter).c_str(),
               std::string(reader_converter).c_str()));
  }
}

std::string
WriterInfo::get_state_str() const
{
  std::ostringstream oss;

  switch (this->state_) {
  case WriterInfo::NOT_SET:
    oss << "NOT_SET";
    break;
  case WriterInfo::ALIVE:
    oss << "ALIVE";
    break;
  case WriterInfo::DEAD:
    oss << "DEAD";
    break;
  default:
    oss << "UNSPECIFIED(" << int(this->state_) << ")";
  }
  return oss.str();
}

void
WriterInfo::clear_owner_evaluated ()
{
  this->owner_evaluated_.clear ();
}

void
WriterInfo::set_owner_evaluated (::DDS::InstanceHandle_t instance, bool flag)
{
  if (flag ||
      (!flag && owner_evaluated_.find (instance) != owner_evaluated_.end ())) {
    this->owner_evaluated_ [instance] = flag;
  }
}

bool
WriterInfo::is_owner_evaluated (::DDS::InstanceHandle_t instance)
{
  OwnerEvaluateFlags::iterator iter = owner_evaluated_.find (instance);
  if (iter == owner_evaluated_.end ()) {
    this->owner_evaluated_.insert (OwnerEvaluateFlags::value_type (instance, false));
    return false;
  }
  else
    return iter->second;
}

ACE_Time_Value
WriterInfo::check_activity(const ACE_Time_Value& now)
{
  ACE_Time_Value expires_at = ACE_Time_Value::max_time;

  // We only need check the liveliness with the non-zero liveliness_lease_duration_.
  // if (state_ != DEAD && reader_->liveliness_lease_duration_ != ACE_Time_Value::zero)
  if (state_ == ALIVE && reader_->liveliness_lease_duration_ != ACE_Time_Value::zero) {
    expires_at = this->last_liveliness_activity_time_ +
                 reader_->liveliness_lease_duration_;

    if (expires_at <= now) {
      // let all instances know this write is not alive.
      reader_->writer_became_dead(*this, now);
      expires_at = ACE_Time_Value::max_time;
    }
  }

  return expires_at;
}

void
WriterInfo::removed()
{
  reader_->writer_removed(*this);
}

void
WriterInfo::clear_acks(
  SequenceNumber sequence)
{
  // sample_lock_ is held by the caller.

  DeadlineList::iterator current
    = this->ack_deadlines_.begin();

  while (current != this->ack_deadlines_.end()) {
    if (current->first <= sequence) {
      current = this->ack_deadlines_.erase(current);

    } else {
      break;
    }
  }
}

bool
WriterInfo::should_ack(
  ACE_Time_Value now)
{
  // sample_lock_ is held by the caller.

  if (this->ack_deadlines_.size() == 0) {
    return false;
  }

  DeadlineList::iterator current = this->ack_deadlines_.begin();

  while (current != this->ack_deadlines_.end()) {
    if (current->second < now) {
      // Remove any expired response deadlines.
      current = this->ack_deadlines_.erase(current);

    } else {
      if (!this->ack_sequence_.empty() &&
          current->first <= this->ack_sequence_.cumulative_ack()) {
        return true;
      }

      ++current;
    }
  }

  return false;
}

void
WriterInfo::ack_deadline(SequenceNumber sequence, ACE_Time_Value when)
{
  // sample_lock_ is held by the caller.

  if (this->ack_deadlines_.size() == 0) {
    this->ack_deadlines_.push_back(std::make_pair(sequence, when));
    return;
  }

  DeadlineList::iterator current = this->ack_deadlines_.begin();
  if (current != this->ack_deadlines_.end()) {
    // Insertion sort.
    if (sequence < current->first) {
      this->ack_deadlines_.insert(
        current,
        std::make_pair(sequence, when));
    } else if (sequence == current->first) {
      // Only update the deadline to be *later* than any existing one.
      if (current->second < when) {
        current->second = when;
      }
    }
  }
}

void
WriterInfo::ack_sequence(SequenceNumber value)
{
  // sample_lock_ is held by the caller.
  this->ack_sequence_.insert(value);
}

SequenceNumber
WriterInfo::ack_sequence() const
{
  // sample_lock_ is held by the caller.
  return this->ack_sequence_.cumulative_ack();
}


#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
Coherent_State
WriterInfo::coherent_change_received()
{
  if (this->writer_coherent_samples_.num_samples_ == 0) {
    return NOT_COMPLETED_YET;
  }

  if (!this->coherent_sample_sequence_.disjoint()
      && (this->coherent_sample_sequence_.high()
          == this->writer_coherent_samples_.last_sample_)) {
    return COMPLETED;
  }

  if (this->coherent_sample_sequence_.high() >
      this->writer_coherent_samples_.last_sample_) {
    return REJECTED;
  }

  return NOT_COMPLETED_YET;
}

void
WriterInfo::reset_coherent_info()
{
  this->coherent_samples_ = 0;
  this->group_coherent_ = false;
  this->publisher_id_ = GUID_UNKNOWN;
  this->coherent_sample_sequence_.reset();
  this->writer_coherent_samples_.reset();
  this->group_coherent_samples_.clear();
}


void
WriterInfo::set_group_info(const CoherentChangeControl& info)
{
  if (!(this->publisher_id_ == info.publisher_id_)
      || this->group_coherent_ != info.group_coherent_) {
    GuidConverter sub_id(this->reader_->subscription_id_);
    GuidConverter pub_id(this->writer_id_);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: WriterInfo::set_group_info()")
               ACE_TEXT(" reader %C writer %C incorrect coherent info !\n"),
               std::string(sub_id).c_str(),
               std::string(pub_id).c_str()));
  }

  this->writer_coherent_samples_ = info.coherent_samples_;
  this->group_coherent_samples_ = info.group_coherent_samples_;
}

#endif  // OPENDDS_NO_OBJECT_MODEL_PROFILE

}
}
