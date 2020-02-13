/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */


#ifndef OPENDDS_DCPS_WRITERINFO_H
#define OPENDDS_DCPS_WRITERINFO_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsCoreC.h"
#include "dds/DCPS/PoolAllocator.h"
#include "RcObject.h"
#include "Definitions.h"
#include "CoherentChangeControl.h"
#include "DisjointSequence.h"
#include "transport/framework/ReceivedDataSample.h"
#include "TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class WriterInfo;

class OpenDDS_Dcps_Export WriterInfoListener
{
public:
  WriterInfoListener();
  virtual ~WriterInfoListener();

  RepoId subscription_id_;

  /// The time interval for checking liveliness.
  /// TBD: Should this be initialized with
  ///      DDS::DURATION_INFINITE_SEC and DDS::DURATION_INFINITE_NSEC
  ///      instead of ACE_Time_Value::zero to be consistent with default
  ///      duration qos ? Or should we simply use the ACE_Time_Value::zero
  ///      to indicate the INFINITY duration ?
  TimeDuration liveliness_lease_duration_;

  /// tell instances when a DataWriter transitions to being alive
  /// The writer state is inout parameter, it has to be set ALIVE before
  /// handle_timeout is called since some subroutine use the state.
  virtual void writer_became_alive(WriterInfo& info,
                                   const MonotonicTimePoint& when);

  /// tell instances when a DataWriter transitions to DEAD
  /// The writer state is inout parameter, the state is set to DEAD
  /// when it returns.
  virtual void writer_became_dead(WriterInfo& info,
                                  const MonotonicTimePoint& when);

  /// tell instance when a DataWriter is removed.
  /// The liveliness status need update.
  virtual void writer_removed(WriterInfo& info);
};


#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
enum Coherent_State {
  NOT_COMPLETED_YET,
  COMPLETED,
  REJECTED
};
#endif



/// Keeps track of a DataWriter's liveliness for a DataReader.
class OpenDDS_Dcps_Export WriterInfo : public RcObject {
  friend class WriteInfoListner;

public:
  enum WriterState { NOT_SET, ALIVE, DEAD };
  enum HistoricSamplesState { NO_TIMER = -1 };

  WriterInfo(WriterInfoListener* reader,
             const PublicationId& writer_id,
             const DDS::DataWriterQos& writer_qos);

  /// check to see if this writer is alive (called by handle_timeout).
  /// @param now next monotonic time this DataWriter will become not active (not alive)
  ///      if no sample or liveliness message is received.
  /// @returns montonic time when the Writer will become not active (if no activity)
  ///          of MonotonicTimePoint::max_value if the writer is already or became not alive
  MonotonicTimePoint check_activity(const MonotonicTimePoint& now);

  /// called when a sample or other activity is received from this writer.
  void received_activity(const MonotonicTimePoint& when);

  /// returns 1 if the DataWriter is lively; 2 if dead; otherwise returns 0.
  WriterState get_state() {
    return state_;
  };

  const char* get_state_str() const;

  /// update liveliness when remove_association is called.
  void removed();

  TimeDuration activity_wait_period() const;

  /// Checks to see if writer has registered activity in either
  /// liveliness_lease_duration or DCPSPendingTimeout duration
  /// to allow it to finish before reader removes it
  bool active() const;

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  Coherent_State coherent_change_received ();
  void reset_coherent_info ();
  void set_group_info (const CoherentChangeControl& info);
#endif

  void clear_owner_evaluated ();
  void set_owner_evaluated (DDS::InstanceHandle_t instance, bool flag);
  bool is_owner_evaluated (DDS::InstanceHandle_t instance);

  //private:

  /// Timestamp of last write/dispose/assert_liveliness from this DataWriter
  MonotonicTimePoint last_liveliness_activity_time_;

  // Non-negative if this a durable writer which has a timer scheduled
  long historic_samples_timer_;
  long remove_association_timer_;
  MonotonicTimePoint removal_deadline_;

  /// Temporary holding place for samples received before
  /// the END_HISTORIC_SAMPLES control message.
  OPENDDS_MAP(SequenceNumber, ReceivedDataSample) historic_samples_;

  /// After receiving END_HISTORIC_SAMPLES, check for duplicates
  SequenceNumber last_historic_seq_;

  bool waiting_for_end_historic_samples_;

  bool scheduled_for_removal_;
  bool notify_lost_;

  /// State of the writer.
  WriterState state_;

  /// The DataReader owning this WriterInfo
  WriterInfoListener* reader_;

  /// DCPSInfoRepo ID of the DataWriter
  PublicationId writer_id_;

  /// Writer qos
  DDS::DataWriterQos writer_qos_;

  /// The publication entity instance handle.
  DDS::InstanceHandle_t handle_;

  /// Number of received coherent changes in active change set.
  ACE_Atomic_Op<ACE_Thread_Mutex, ACE_UINT32> coherent_samples_;

  /// Is this writer evaluated for owner ?
  typedef OPENDDS_MAP(DDS::InstanceHandle_t, bool) OwnerEvaluateFlags;
  OwnerEvaluateFlags owner_evaluated_;

  /// Data to support GROUP access scope.
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  bool group_coherent_;
  RepoId publisher_id_;
  DisjointSequence coherent_sample_sequence_;
  WriterCoherentSample writer_coherent_samples_;
  GroupCoherentSamples group_coherent_samples_;
#endif

};

inline
void
WriterInfo::received_activity(const MonotonicTimePoint& when)
{
  last_liveliness_activity_time_ = when;

  if (state_ != ALIVE) { // NOT_SET || DEAD
    reader_->writer_became_alive(*this, when);
  }
}

} // namespace DCPS
} // namespace

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* end of include guard: OPENDDS_DCPS_WRITERINFO_H */
