/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "dcps_export.h"

#include "DataReaderImpl.h"
#include "GuidConverter.h"
#include "Service_Participant.h"
#include "Time_Helper.h"
#include "WriterInfo.h"

#include "ace/OS_NS_sys_time.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

WriterInfoListener::WriterInfoListener()
  : subscription_id_(GUID_UNKNOWN)
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
                                        const MonotonicTimePoint&,
                                        WriterState)
{
}

/// tell instances when a DataWriter transitions to DEAD
/// The writer state is inout parameter, the state is set to DEAD
/// when it returns.
void
WriterInfoListener::writer_became_dead(WriterInfo&,
                                       WriterState)
{
}

/// tell instance when a DataWriter is removed.
/// The liveliness status need update.
void
WriterInfoListener::writer_removed(WriterInfo&)
{
}

void
WriterInfoListener::resume_sample_processing(WriterInfo&)
{
}

WriterInfo::WriterInfo(const WriterInfoListener_rch& reader,
                       const GUID_t& writer_id,
                       const ::DDS::DataWriterQos& writer_qos,
                       const DDS::Duration_t& reader_liveliness_lease_duration)
  : historic_samples_sweeper_task_(make_rch<WriterInfoSporadicTask>(TheServiceParticipant->time_source(), TheServiceParticipant->reactor_task(), rchandle_from(this), &WriterInfo::sweep_historic_samples))
  , liveliness_check_task_(make_rch<WriterInfoSporadicTask>(TheServiceParticipant->time_source(), TheServiceParticipant->reactor_task(), rchandle_from(this), &WriterInfo::check_liveliness))
  , last_historic_seq_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
  , waiting_for_end_historic_samples_(false)
  , delivering_historic_samples_(false)
  , delivering_historic_samples_cv_(mutex_)
  , state_(NOT_SET)
  , reader_(reader)
  , writer_id_(writer_id)
  , writer_qos_(writer_qos)
  , reader_liveliness_lease_duration_(reader_liveliness_lease_duration)
  , reader_liveliness_lease_duration_is_finite_(!is_infinite(reader_liveliness_lease_duration))
  , handle_(DDS::HANDLE_NIL)
{
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  reset_coherent_info();
#endif

  if (DCPS_debug_level >= 5) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) WriterInfo::WriterInfo: ")
               ACE_TEXT("writer %C added to reader %C.\n"),
               LogGuid(writer_id).c_str(),
               LogGuid(reader->subscription_id_).c_str()));
  }
}

WriterInfo::~WriterInfo()
{
  historic_samples_sweeper_task_->cancel();
  liveliness_check_task_->cancel();
}

const char* get_state_str(WriterState state)
{
  switch (state) {
  case NOT_SET:
    return "NOT_SET";
  case ALIVE:
    return "ALIVE";
  case DEAD:
    return "DEAD";
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR:get_state_str(WriterState): ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      state));
    return "Invalid state";
  }
}

void
WriterInfo::schedule_historic_samples_timer()
{
  waiting_for_end_historic_samples(true);
  const TimeDuration ten_seconds(10, 0);
  historic_samples_sweeper_task_->schedule(ten_seconds);
}

void
WriterInfo::cancel_historic_samples_timer()
{
  waiting_for_end_historic_samples(false);
  historic_samples_sweeper_task_->cancel();
}

bool
WriterInfo::check_end_historic_samples(OPENDDS_MAP(SequenceNumber, ReceivedDataSample)& to_deliver)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  while (delivering_historic_samples_) {
    delivering_historic_samples_cv_.wait(TheServiceParticipant->get_thread_status_manager());
  }
  if (waiting_for_end_historic_samples_) {
    bool result = false;
    RcHandle<WriterInfo> info = rchandle_from(this);
    if (!historic_samples_.empty()) {
      last_historic_seq_ = historic_samples_.rbegin()->first;
      delivering_historic_samples_ = true;
      to_deliver.swap(historic_samples_);
      result = true;
    }
    waiting_for_end_historic_samples_ = false;
    guard.release();
    historic_samples_sweeper_task_->cancel();
    return result;
  }
  return false;
}

void
WriterInfo::sweep_historic_samples(const MonotonicTimePoint&)
{
  WriterInfoListener_rch reader = reader_.lock();
  if (reader) {
    reader->resume_sample_processing(*this);
  }
}

bool
WriterInfo::check_historic(const SequenceNumber& seq, const ReceivedDataSample& sample, SequenceNumber& last_historic_seq)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  last_historic_seq = last_historic_seq_;
  if (waiting_for_end_historic_samples_) {
    historic_samples_.insert(std::make_pair(seq, sample));
    return true;
  }
  return false;
}

void
WriterInfo::finished_delivering_historic()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  delivering_historic_samples_ = false;
  delivering_historic_samples_cv_.notify_all();
}

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
void
WriterInfo::add_coherent_samples(const SequenceNumber& seq)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (coherent_samples_ == 0) {
    static const SequenceNumber defaultSN;
    const SequenceRange resetRange(defaultSN, seq);
    coherent_sample_sequence_.reset();
    coherent_sample_sequence_.insert(resetRange);
  }
  else {
    coherent_sample_sequence_.insert(seq);
  }
}

void
WriterInfo::coherent_change(bool group_coherent, const GUID_t& publisher_id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  group_coherent_ = group_coherent;
  publisher_id_ = publisher_id;
  ++coherent_samples_;
}
#endif

void
WriterInfo::clear_owner_evaluated()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  owner_evaluated_.clear();
}

void
WriterInfo::set_owner_evaluated(::DDS::InstanceHandle_t instance, bool flag)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (flag ||
      (!flag && owner_evaluated_.find(instance) != owner_evaluated_.end())) {
    owner_evaluated_[instance] = flag;
  }
}

bool
WriterInfo::is_owner_evaluated(::DDS::InstanceHandle_t instance)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  OwnerEvaluateFlags::iterator iter = owner_evaluated_.find(instance);
  if (iter == owner_evaluated_.end()) {
    owner_evaluated_.insert(OwnerEvaluateFlags::value_type(instance, false));
    return false;
  }
  else
    return iter->second;
}

void
WriterInfo::remove_instance(DDS::InstanceHandle_t instance)
{
  owner_evaluated_.erase(instance);
}

void
WriterInfo::start_liveliness_timer()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  last_liveliness_activity_time_ = MonotonicTimePoint::now();
  if (reader_liveliness_lease_duration_is_finite_) {
    liveliness_check_task_->schedule(reader_liveliness_lease_duration_);
  }
}

void
WriterInfo::check_liveliness(const MonotonicTimePoint& now)
{
  bool became_dead = false;
  WriterState previous_state;

  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    previous_state = state_;
    if (state_ == ALIVE) {
      const TimeDuration elapsed = now - last_liveliness_activity_time_;
      became_dead = elapsed > reader_liveliness_lease_duration_;
      if (became_dead) {
        state_ = DEAD;
      } else {
        liveliness_check_task_->schedule(reader_liveliness_lease_duration_ - elapsed);
      }
    }
  }

  WriterInfoListener_rch reader = reader_.lock();
  if (reader && became_dead) {
    reader->writer_became_dead(*this, previous_state);
  }
}

void
WriterInfo::removed()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  WriterInfoListener_rch reader = reader_.lock();
  guard.release();
  if (reader) {
    reader->writer_removed(*this);
  }
  historic_samples_sweeper_task_->cancel();
  liveliness_check_task_->cancel();
}

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
Coherent_State
WriterInfo::coherent_change_received()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (writer_coherent_samples_.num_samples_ == 0) {
    return NOT_COMPLETED_YET;
  }

  if (!coherent_sample_sequence_.disjoint()
      && (coherent_sample_sequence_.high()
          == writer_coherent_samples_.last_sample_)) {
    return COMPLETED;
  }

  if (coherent_sample_sequence_.high() >
      writer_coherent_samples_.last_sample_) {
    return REJECTED;
  }

  return NOT_COMPLETED_YET;
}

void
WriterInfo::reset_coherent_info()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  coherent_samples_ = 0;
  group_coherent_ = false;
  publisher_id_ = GUID_UNKNOWN;
  coherent_sample_sequence_.reset();
  writer_coherent_samples_.reset();
  group_coherent_samples_.clear();
}


void
WriterInfo::set_group_info(const CoherentChangeControl& info)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!(publisher_id_ == info.publisher_id_)
      || group_coherent_ != info.group_coherent_) {
    WriterInfoListener_rch reader = reader_.lock();
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: WriterInfo::set_group_info()")
               ACE_TEXT(" reader %C writer %C incorrect coherent info !\n"),
               reader ? LogGuid(reader->subscription_id_).c_str() : "",
               LogGuid(writer_id_).c_str()));
  }

  writer_coherent_samples_ = info.coherent_samples_;
  group_coherent_samples_ = info.group_coherent_samples_;
}

#endif  // OPENDDS_NO_OBJECT_MODEL_PROFILE

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
