/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "dcps_export.h"

#include "GuidConverter.h"
#include "WriterInfo.h"
#include "Qos_Helper.h"

#include "ace/OS_NS_sys_time.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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



WriterInfo::WriterInfo(WriterInfoListener*         reader,
                       const PublicationId&        writer_id,
                       const ::DDS::DataWriterQos& writer_qos)
  : last_liveliness_activity_time_(ACE_OS::gettimeofday()),
  seen_data_(false),
  historic_samples_timer_(NO_TIMER),
  remove_association_timer_(NO_TIMER),
  removal_deadline_(ACE_Time_Value::zero),
  last_historic_seq_(SequenceNumber::SEQUENCENUMBER_UNKNOWN()),
  waiting_for_end_historic_samples_(false),
  scheduled_for_removal_(false),
  notify_lost_(false),
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
               OPENDDS_STRING(writer_converter).c_str(),
               OPENDDS_STRING(reader_converter).c_str()));
  }
}

OPENDDS_STRING
WriterInfo::get_state_str() const
{
  OPENDDS_STRING ret;
  switch (this->state_) {
  case WriterInfo::NOT_SET:
    ret += "NOT_SET";
    break;
  case WriterInfo::ALIVE:
    ret += "ALIVE";
    break;
  case WriterInfo::DEAD:
    ret += "DEAD";
    break;
  default:
    ret += "UNSPECIFIED(";
    ret += to_dds_string(int(this->state_));
    ret += ")";
  }
  return ret.c_str();
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

bool
WriterInfo::active(ACE_Time_Value default_participant_timeout) const
{
  // Need some period of time by which to decide if a writer the
  // DataReaderImpl knows about has gone 'inactive'.  Used to determine
  // if a remove_associations should remove immediately or wait to let
  // reader process more information that may have queued up from the writer
  // Over-arching max wait time for removal is controlled in the
  // RemoveAssociationSweeper (10 seconds), but on a per writer basis set
  // activity_wait_period based on:
  //     1) Reader's liveliness_lease_duration
  //     2) DCPSPendingTimeout value (if not zero)
  //     3) Writer's max blocking time (could be infinite, in which case
  //        RemoveAssociationSweeper will remove after its max wait)
  //     4) Zero - don't wait, simply remove association
  ACE_Time_Value activity_wait_period(default_participant_timeout);
  if (reader_->liveliness_lease_duration_ != ACE_Time_Value::zero) {
    activity_wait_period = reader_->liveliness_lease_duration_;
  }
  if (activity_wait_period == ACE_Time_Value::zero) {
      activity_wait_period = duration_to_time_value(writer_qos_.reliability.max_blocking_time);
  }
  if (activity_wait_period == ACE_Time_Value::zero) {
    return false;
  }
  return (ACE_OS::gettimeofday() - last_liveliness_activity_time_) <= activity_wait_period;
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
               OPENDDS_STRING(sub_id).c_str(),
               OPENDDS_STRING(pub_id).c_str()));
  }

  this->writer_coherent_samples_ = info.coherent_samples_;
  this->group_coherent_samples_ = info.group_coherent_samples_;
}

#endif  // OPENDDS_NO_OBJECT_MODEL_PROFILE

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
