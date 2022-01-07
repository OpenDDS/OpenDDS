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
                                        const MonotonicTimePoint&)
{
}

/// tell instances when a DataWriter transitions to DEAD
/// The writer state is inout parameter, the state is set to DEAD
/// when it returns.
void
WriterInfoListener::writer_became_dead(WriterInfo&)
{
}

/// tell instance when a DataWriter is removed.
/// The liveliness status need update.
void
WriterInfoListener::writer_removed(WriterInfo&)
{
}

WriterInfo::WriterInfo(WriterInfoListener* reader,
                       const PublicationId& writer_id,
                       const ::DDS::DataWriterQos& writer_qos)
  : last_liveliness_activity_time_(MonotonicTimePoint::now())
  , historic_samples_timer_(NO_TIMER)
  , last_historic_seq_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
  , waiting_for_end_historic_samples_(false)
  , state_(NOT_SET)
  , reader_(reader)
  , writer_id_(writer_id)
  , writer_qos_(writer_qos)
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

const char* WriterInfo::get_state_str() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  switch (state_) {
  case NOT_SET:
    return "NOT_SET";
  case ALIVE:
    return "ALIVE";
  case DEAD:
    return "DEAD";
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: WriterInfo::get_state_str: ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      state_));
    return "Invalid state";
  }
}

void
WriterInfo::schedule_historic_samples_timer(EndHistoricSamplesMissedSweeper* sweeper, const ACE_Time_Value& ten_seconds)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  const void* arg = reinterpret_cast<const void*>(this);
  historic_samples_timer_ = sweeper->reactor()->schedule_timer(sweeper, arg, ten_seconds);
}

void
WriterInfo::cancel_historic_samples_timer(EndHistoricSamplesMissedSweeper* sweeper)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (historic_samples_timer_ != WriterInfo::NO_TIMER) {
    sweeper->reactor()->cancel_timer(historic_samples_timer_);
    historic_samples_timer_ = WriterInfo::NO_TIMER;
  }
}

bool
WriterInfo::check_end_historic_samples(EndHistoricSamplesMissedSweeper* sweeper, OPENDDS_MAP(SequenceNumber, ReceivedDataSample)& to_deliver)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (waiting_for_end_historic_samples_) {
    RcHandle<WriterInfo> info = rchandle_from(this);
    if (!historic_samples_.empty()) {
      last_historic_seq_ = historic_samples_.rbegin()->first;
    }
    to_deliver.swap(historic_samples_);
    guard.release();
    sweeper->cancel_timer(info);
    return true;
  }
  return false;
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
WriterInfo::coherent_change(bool group_coherent, const RepoId& publisher_id)
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

MonotonicTimePoint
WriterInfo::check_activity(const MonotonicTimePoint& now)
{
  MonotonicTimePoint expires_at(MonotonicTimePoint::max_value);

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

  // We only need check the liveliness with the non-zero liveliness_lease_duration_.
  if (state_ == ALIVE && !reader_->liveliness_lease_duration_.is_zero()) {
    expires_at = last_liveliness_activity_time_ + reader_->liveliness_lease_duration_;

    if (expires_at <= now) {
      // let all instances know this write is not alive.
      WriterInfoListener* reader = reader_;
      guard.release();
      reader->writer_became_dead(*this);
      expires_at = MonotonicTimePoint::max_value;
    }
  }

  return expires_at;
}

void
WriterInfo::removed()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  WriterInfoListener* reader = reader_;
  guard.release();
  reader->writer_removed(*this);
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
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: WriterInfo::set_group_info()")
               ACE_TEXT(" reader %C writer %C incorrect coherent info !\n"),
               LogGuid(reader_->subscription_id_).c_str(),
               LogGuid(writer_id_).c_str()));
  }

  writer_coherent_samples_ = info.coherent_samples_;
  group_coherent_samples_ = info.group_coherent_samples_;
}

#endif  // OPENDDS_NO_OBJECT_MODEL_PROFILE

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
