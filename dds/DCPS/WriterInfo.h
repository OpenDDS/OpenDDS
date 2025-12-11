/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */


#ifndef OPENDDS_DCPS_WRITERINFO_H
#define OPENDDS_DCPS_WRITERINFO_H

#include "Atomic.h"
#include "CoherentChangeControl.h"
#include "ConditionVariable.h"
#include "Definitions.h"
#include "DisjointSequence.h"
#include "PoolAllocator.h"
#include "RcObject.h"
#include "SporadicTask.h"
#include "TimeTypes.h"

#include "transport/framework/ReceivedDataSample.h"

#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/DdsDcpsCoreC.h>

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Reactor;
class ACE_Event_Handler;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class WriterInfo;

enum WriterState { NOT_SET, ALIVE, DEAD };

OpenDDS_Dcps_Export const char* get_state_str(WriterState state);

class OpenDDS_Dcps_Export WriterInfoListener: public virtual RcObject
{
public:
  WriterInfoListener();
  virtual ~WriterInfoListener();

  GUID_t subscription_id_;

  /// tell instances when a DataWriter transitions to being alive
  /// The writer state is inout parameter, it has to be set ALIVE before
  /// handle_timeout is called since some subroutine use the state.
  virtual void writer_became_alive(WriterInfo& info,
                                   const MonotonicTimePoint& when,
                                   WriterState previous_state);

  /// tell instances when a DataWriter transitions to DEAD
  /// The writer state is inout parameter, the state is set to DEAD
  /// when it returns.
  virtual void writer_became_dead(WriterInfo& info,
                                  WriterState previous_state);

  /// tell instance when a DataWriter is removed.
  /// The liveliness status need update.
  virtual void writer_removed(WriterInfo& info);

  virtual void resume_sample_processing(WriterInfo& info);
};

typedef RcHandle<WriterInfoListener> WriterInfoListener_rch;

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
enum Coherent_State {
  NOT_COMPLETED_YET,
  COMPLETED,
  REJECTED
};
#endif

/// Keeps track of a DataWriter's liveliness for a DataReader.
class OpenDDS_Dcps_Export WriterInfo : public RcObject {
public:
  WriterInfo(const WriterInfoListener_rch& reader,
             const GUID_t& writer_id,
             const DDS::DataWriterQos& writer_qos,
             const DDS::Duration_t& reader_liveliness_lease_duration);
  ~WriterInfo();

  /// called when a sample or other activity is received from this writer.
  void received_activity(const MonotonicTimePoint& when);

  /// returns 1 if the DataWriter is lively; 2 if dead; otherwise returns 0.
  WriterState state() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return state_;
  };

  DDS::InstanceHandle_t handle() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return handle_;
  }

  void handle(DDS::InstanceHandle_t handle)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    handle_ = handle;
  };

  GUID_t writer_id() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return writer_id_;
  }

  CORBA::Long writer_qos_ownership_strength() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return writer_qos_.ownership_strength.value;
  }

  void writer_qos_ownership_strength(const CORBA::Long writer_qos_ownership_strength)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    writer_qos_.ownership_strength.value = writer_qos_ownership_strength;
  }

  bool waiting_for_end_historic_samples() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return waiting_for_end_historic_samples_;
  }

  void waiting_for_end_historic_samples(bool waiting_for_end_historic_samples)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    waiting_for_end_historic_samples_ = waiting_for_end_historic_samples;
  }

  SequenceNumber last_historic_seq() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return last_historic_seq_;
  }

  void last_historic_seq(const SequenceNumber& last_historic_seq)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    last_historic_seq_ = last_historic_seq;
  }

  /// update liveliness when remove_association is called.
  void removed();

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  Coherent_State coherent_change_received();
  void reset_coherent_info();
  void set_group_info(const CoherentChangeControl& info);
  void add_coherent_samples(const SequenceNumber& seq);
  void coherent_change(bool group_coherent, const GUID_t& publisher_id);

  bool group_coherent() const {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return group_coherent_;
  }

  GUID_t publisher_id() const {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return publisher_id_;
  }

#endif

  void clear_owner_evaluated();
  void set_owner_evaluated(DDS::InstanceHandle_t instance, bool flag);
  bool is_owner_evaluated(DDS::InstanceHandle_t instance);

  void remove_instance(DDS::InstanceHandle_t instance);
  void schedule_historic_samples_timer();
  void cancel_historic_samples_timer();
  bool check_end_historic_samples(OPENDDS_MAP(SequenceNumber, ReceivedDataSample)& to_deliver);
  bool check_historic(const SequenceNumber& seq, const ReceivedDataSample& sample, SequenceNumber& last_historic_seq);
  void finished_delivering_historic();

  void start_liveliness_timer();
private:

  mutable ACE_Thread_Mutex mutex_;

  /// Timestamp of last write/dispose/assert_liveliness from this DataWriter
  MonotonicTimePoint last_liveliness_activity_time_;

  typedef PmfSporadicTask<WriterInfo> WriterInfoSporadicTask;

  const RcHandle<WriterInfoSporadicTask> historic_samples_sweeper_task_;
  void sweep_historic_samples(const MonotonicTimePoint& now);

  const RcHandle<WriterInfoSporadicTask> liveliness_check_task_;
  void check_liveliness(const MonotonicTimePoint& now);

  /// Temporary holding place for samples received before
  /// the END_HISTORIC_SAMPLES control message.
  OPENDDS_MAP(SequenceNumber, ReceivedDataSample) historic_samples_;

  /// After receiving END_HISTORIC_SAMPLES, check for duplicates
  SequenceNumber last_historic_seq_;

  bool waiting_for_end_historic_samples_;

  bool delivering_historic_samples_;
  ConditionVariable<ACE_Thread_Mutex> delivering_historic_samples_cv_;

  /// State of the writer.
  WriterState state_;

  /// The DataReader owning this WriterInfo
  const WeakRcHandle<WriterInfoListener> reader_;

  /// DCPSInfoRepo ID of the DataWriter
  const GUID_t writer_id_;

  /// Writer qos
  DDS::DataWriterQos writer_qos_;
  const TimeDuration reader_liveliness_lease_duration_;
  const bool reader_liveliness_lease_duration_is_finite_;

  /// The publication entity instance handle.
  DDS::InstanceHandle_t handle_;

  /// Number of received coherent changes in active change set.
  Atomic<ACE_UINT32> coherent_samples_;

  /// Is this writer evaluated for owner ?
  typedef OPENDDS_MAP(DDS::InstanceHandle_t, bool) OwnerEvaluateFlags;
  OwnerEvaluateFlags owner_evaluated_;

  /// Data to support GROUP access scope.
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  bool group_coherent_;
  GUID_t publisher_id_;
  DisjointSequence coherent_sample_sequence_;
  WriterCoherentSample writer_coherent_samples_;
  GroupCoherentSamples group_coherent_samples_;
#endif

};

inline
void
WriterInfo::received_activity(const MonotonicTimePoint& now)
{
  WriterState prev;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    last_liveliness_activity_time_ = now;
    prev = state_;
    state_ = ALIVE;
    if (prev != ALIVE && reader_liveliness_lease_duration_is_finite_) {
      liveliness_check_task_->schedule(reader_liveliness_lease_duration_);
    }
  }

  if (prev != ALIVE) { // NOT_SET || DEAD
    RcHandle<WriterInfoListener> reader = reader_.lock();
    if (reader) {
      reader->writer_became_alive(*this, now, prev);
    }
  }
}

typedef RcHandle<WriterInfo> WriterInfo_rch;

} // namespace DCPS
} // namespace

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* end of include guard: OPENDDS_DCPS_WRITERINFO_H */
