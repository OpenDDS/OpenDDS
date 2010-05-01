/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TAO_DDS_DCPS_DATAREADER_H
#define TAO_DDS_DCPS_DATAREADER_H

#include "dcps_export.h"
#include "EntityImpl.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsSubscriptionExtS.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsDataReaderRemoteC.h"
#include "Definitions.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include "DisjointSequence.h"
#include "SubscriptionInstance.h"
#include "InstanceState.h"
#include "Cached_Allocator_With_Overflow_T.h"
#include "ZeroCopyInfoSeq_T.h"
#include "Stats_T.h"
#include "OwnershipManager.h"
#include "ContentFilteredTopicImpl.h"
#include "dds/DdsDcpsInfrastructureC.h"

#include "ace/String_Base.h"
#include "ace/Reverse_Lock_T.h"

#include <vector>
#include <list>
#include <map>
#include <memory>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

namespace OpenDDS {
namespace DCPS {

class SubscriberImpl;
class DomainParticipantImpl;
class SubscriptionInstance;
class TopicImpl;
class RequestedDeadlineWatchdog;
class Monitor;
class DataReaderImpl;
class FilterEvaluator;

typedef Cached_Allocator_With_Overflow<OpenDDS::DCPS::ReceivedDataElement, ACE_Null_Mutex>
ReceivedDataAllocator;

/// Keeps track of a DataWriter's liveliness for a DataReader.
class OpenDDS_Dcps_Export WriterInfo {
  friend class DataReaderImpl;

public:
  enum WriterState { NOT_SET, ALIVE, DEAD };

  WriterInfo();  // needed for maps

  WriterInfo(DataReaderImpl* reader,
             const PublicationId&  writer_id,
             const ::DDS::DataWriterQos&  writer_qos);

  /// check to see if this writer is alive (called by handle_timeout).
  /// @param now next time this DataWriter will become not active (not alive)
  ///      if no sample or liveliness message is received.
  /// @returns absolute time when the Writer will become not active (if no activity)
  ///          of ACE_Time_Value::zero if the writer is already or became not alive
  ACE_Time_Value check_activity(const ACE_Time_Value& now);

  /// called when a sample or other activity is received from this writer.
  int received_activity(const ACE_Time_Value& when);

  /// returns 1 if the DataWriter is lively; otherwise returns 0.
  WriterState get_state() {
    return state_;
  };

  /// update liveliness when remove_association is called.
  void removed();

  /// Remove ack requests prior to the given sequence.
  /// NOTE: This removes *all* ack requests for this publication
  ///       satisfied by this sequence.
  void clear_acks(SequenceNumber sequence);

  /// Determine if a SAMPLE_ACK message should be sent to this
  /// publication.
  bool should_ack(ACE_Time_Value now);

  /// Set the time after which we no longer need to generate a
  /// SAMPLE_ACK for this sequence value.
  void ack_deadline(SequenceNumber sequence, ACE_Time_Value when);

  /// Update the last observed sequence number.
  void ack_sequence(SequenceNumber value);

  /// Return the most recently observed contiguous sequence number.
  SequenceNumber ack_sequence() const;

private:
  /// Timestamp of last write/dispose/assert_liveliness from this DataWriter
  ACE_Time_Value last_liveliness_activity_time_;

  /// Times after which we no longer need to respond to a REQUEST_ACK message.
  typedef std::list<std::pair<SequenceNumber, ACE_Time_Value> > DeadlineList;
  DeadlineList ack_deadlines_;

  DisjointSequence ack_sequence_;

  bool seen_data_;

  /// State of the writer.
  WriterState state_;

  /// The DataReader owning this WriterInfo
  DataReaderImpl* reader_;

  /// DCPSInfoRepo ID of the DataWriter
  PublicationId writer_id_;

  /// Writer qos
  ::DDS::DataWriterQos writer_qos_;
  
  /// The publication entity instance handle.
  ::DDS::InstanceHandle_t handle_;

  /// Number of received coherent changes in active change set.
  ACE_Atomic_Op<ACE_Thread_Mutex, ACE_UINT32> coherent_samples_;

  /// Is this writer evaluated for owner ?
  bool owner_evaluated_;
};

/// Elements stored for managing statistical data.
class OpenDDS_Dcps_Export WriterStats {
public:
  /// Default constructor.
  WriterStats(
    int amount = 0,
    DataCollector<double>::OnFull type = DataCollector<double>::KeepOldest);

  /// Add a datum to the latency statistics.
  void add_stat(const ACE_Time_Value& delay);

  /// Extract the current latency statistics for this writer.
  LatencyStatistics get_stats() const;

  /// Reset the latency statistics for this writer.
  void reset_stats();

  /// Dump any raw data.
  std::ostream& raw_data(std::ostream& str) const;

private:
  /// Latency statistics for the DataWriter to this DataReader.
  Stats<double> stats_;
};

/**
* @class DataReaderImpl
*
* @brief Implements the DDS::DataReader interface.
*
* See the DDS specification, OMG formal/04-12-02, for a description of
* the interface this class is implementing.
*
* This class must be inherited by the type-specific datareader which
* is specific to the data-type associated with the topic.
*
*/
class OpenDDS_Dcps_Export DataReaderImpl
  : public virtual LocalObject<DataReaderEx>,
    public virtual EntityImpl,
    public virtual TransportReceiveListener,
    public virtual ACE_Event_Handler {
public:
  friend class RequestedDeadlineWatchdog;
  
  typedef std::map<DDS::InstanceHandle_t, SubscriptionInstance*> SubscriptionInstanceMapType;

  /// Type of collection of statistics for writers to this reader.
  typedef std::map<PublicationId, WriterStats, GUID_tKeyLessThan> StatsMapType;

  //Constructor
  DataReaderImpl();

  //Destructor
  virtual ~DataReaderImpl();

  virtual DDS::InstanceHandle_t get_instance_handle()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void add_associations(
    OpenDDS::DCPS::RepoId yourId,
    const OpenDDS::DCPS::WriterAssociationSeq & writers)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void remove_associations(
    const OpenDDS::DCPS::WriterIdSeq & writers,
    CORBA::Boolean callback)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void update_incompatible_qos(
    const OpenDDS::DCPS::IncompatibleQosStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  /**
  * This is used to retrieve the listener for a certain status change.
  * If this datareader has a registered listener and the status kind
  * is in the listener mask then the listener is returned.
  * Otherwise, the query for the listener is propagated up to the
  * factory/subscriber.
  */
  DDS::DataReaderListener* listener_for(DDS::StatusKind kind);

  /// Handle the assert liveliness timeout.
  virtual int handle_timeout(const ACE_Time_Value &tv,
                             const void *arg);

  /// tell instances when a DataWriter transitions to being alive
  /// The writer state is inout parameter, it has to be set ALIVE before
  /// handle_timeout is called since some subroutine use the state.
  void writer_became_alive(WriterInfo& info,
                           const ACE_Time_Value& when);

  /// tell instances when a DataWriter transitions to DEAD
  /// The writer state is inout parameter, the state is set to DEAD
  /// when it returns.
  void writer_became_dead(WriterInfo& info,
                          const ACE_Time_Value& when);

  /// tell instance when a DataWriter is removed.
  /// The liveliness status need update.
  void writer_removed(WriterInfo& info);

  virtual int handle_close(ACE_HANDLE,
                           ACE_Reactor_Mask);

  /**
   * cleanup the DataWriter.
   */
  void cleanup();

  virtual void init(
    TopicImpl*                    a_topic,
    const DDS::DataReaderQos &  qos,
    const DataReaderQosExt &      ext_qos,
    DDS::DataReaderListener_ptr a_listener,
    const DDS::StatusMask &     mask,
    DomainParticipantImpl*        participant,
    SubscriberImpl*               subscriber,
    DDS::DataReader_ptr         dr_objref,
    OpenDDS::DCPS::DataReaderRemote_ptr dr_remote_objref)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReadCondition_ptr create_readcondition(
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states)
  ACE_THROW_SPEC((CORBA::SystemException));

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual DDS::QueryCondition_ptr create_querycondition(
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states,
    const char * query_expression,
    const DDS::StringSeq & query_parameters)
  ACE_THROW_SPEC((CORBA::SystemException));
#endif

  virtual DDS::ReturnCode_t delete_readcondition(
    DDS::ReadCondition_ptr a_condition)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t delete_contained_entities()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_qos(
    const DDS::DataReaderQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_qos(
    DDS::DataReaderQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_listener(
    DDS::DataReaderListener_ptr a_listener,
    DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::DataReaderListener_ptr get_listener()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::TopicDescription_ptr get_topicdescription()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::Subscriber_ptr get_subscriber()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_sample_rejected_status(
    DDS::SampleRejectedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_liveliness_changed_status(
    DDS::LivelinessChangedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_requested_deadline_missed_status(
    DDS::RequestedDeadlineMissedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_requested_incompatible_qos_status(
    DDS::RequestedIncompatibleQosStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_subscription_matched_status(
    DDS::SubscriptionMatchedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_sample_lost_status(
    DDS::SampleLostStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t wait_for_historical_data(
    const DDS::Duration_t & max_wait)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_matched_publications(
    DDS::InstanceHandleSeq & publication_handles)
  ACE_THROW_SPEC((CORBA::SystemException));

#if !defined (DDS_HAS_MINIMUM_BIT)
  virtual DDS::ReturnCode_t get_matched_publication_data(
    DDS::PublicationBuiltinTopicData & publication_data,
    DDS::InstanceHandle_t publication_handle)
  ACE_THROW_SPEC((CORBA::SystemException));
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  virtual DDS::ReturnCode_t enable()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void get_latency_stats(
    OpenDDS::DCPS::LatencyStatisticsSeq & stats)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void reset_latency_stats(
    void)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual CORBA::Boolean statistics_enabled(
    void)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void statistics_enabled(
    CORBA::Boolean statistics_enabled)
  ACE_THROW_SPEC((CORBA::SystemException));

  /// @name Raw Latency Statistics Interfaces
  /// @{

  /// Expose the statistics container.
  const StatsMapType& raw_latency_statistics() const;

  /// Configure the size of the raw data collection buffer.
  unsigned int& raw_latency_buffer_size();

  /// Configure the type of the raw data collection buffer.
  DataCollector<double>::OnFull& raw_latency_buffer_type();

  /// @}

  /// update liveliness info for this writer.
  void writer_activity(const DataSampleHeader& header);

  /// process a message that has been received - could be control or a data sample.
  virtual void data_received(const ReceivedDataSample& sample);

  RepoId get_subscription_id() const;
  void set_subscription_id(RepoId subscription_id);

  DDS::DataReader_ptr get_dr_obj_ref();

  char *get_topic_name() const;

  bool have_sample_states(DDS::SampleStateMask sample_states) const;
  bool have_view_states(DDS::ViewStateMask view_states) const;
  bool have_instance_states(DDS::InstanceStateMask instance_states) const;
  bool contains_sample(DDS::SampleStateMask sample_states,
                       DDS::ViewStateMask view_states,
                       DDS::InstanceStateMask instance_states);

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual bool contains_sample_filtered(DDS::SampleStateMask sample_states,
                                        DDS::ViewStateMask view_states,
                                        DDS::InstanceStateMask instance_states,
                                        const FilterEvaluator& evaluator,
                                        const DDS::StringSeq& params) = 0;
#endif

  virtual void dds_demarshal(const ReceivedDataSample& sample,
                             SubscriptionInstance*& instance,
                             bool & is_new_instance,
                             bool & filtered)= 0;
  virtual void dispose(const ReceivedDataSample& sample,
                       SubscriptionInstance*& instance);
  virtual void unregister(const ReceivedDataSample& sample,
                          SubscriptionInstance*& instance);

  void process_latency(const ReceivedDataSample& sample);
  void notify_latency(PublicationId writer);

  CORBA::Long get_depth() const {
    return depth_;
  }
  size_t get_n_chunks() const {
    return n_chunks_;
  }

  void liveliness_lost();

  void remove_all_associations();

  void notify_subscription_disconnected(const WriterIdSeq& pubids);
  void notify_subscription_reconnected(const WriterIdSeq& pubids);
  void notify_subscription_lost(const WriterIdSeq& pubids);
  void notify_connection_deleted();
  void notify_liveliness_change();

  bool is_bit() const;

  /** This method provides virtual access to type specific code
   * that is used when loans are automatically returned.
   * The destructor of the sequence supporing zero-copy read calls this
   * method on the datareader that provided the loan.
   *
   * @param seq - The sequence of loaned values.
   *
   * @returns Always RETCODE_OK.
   *
   * thows NONE.
   */
  virtual DDS::ReturnCode_t auto_return_loan(void* seq) = 0;

  /** This method is used for a precondition check of delete_datareader.
   *
   * @returns the number of outstanding zero-copy samples loaned out.
   */
  virtual int num_zero_copies();

  virtual void dec_ref_data_element(ReceivedDataElement* r) = 0;

  /// Release the instance with the handle.
  void release_instance(DDS::InstanceHandle_t handle);

  // Reset time interval for each instance.
  void reschedule_deadline();

  ACE_Reactor* get_reactor();

  RepoId get_topic_id();
  RepoId get_dp_id();

  typedef std::vector<DDS::InstanceHandle_t> InstanceHandleVec;
  void get_instance_handles(InstanceHandleVec& instance_handles);

  typedef std::pair<PublicationId, WriterInfo::WriterState> WriterStatePair;
  typedef std::vector<WriterStatePair> WriterStatePairVec;
  void get_writer_states(WriterStatePairVec& writer_states);
  
  void update_ownership_strength (const PublicationId& pub_id,
                                  const CORBA::Long& ownership_strength);

  virtual void delete_instance_map (void* map) = 0;
  virtual void lookup_instance(const OpenDDS::DCPS::ReceivedDataSample& sample,
                               OpenDDS::DCPS::SubscriptionInstance*& instance) = 0;

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  void enable_filtering(ContentFilteredTopicImpl* cft);
#endif

protected:

  SubscriberImpl* get_subscriber_servant();

  void post_read_or_take();

  // type specific DataReader's part of enable.
  virtual DDS::ReturnCode_t enable_specific()
  ACE_THROW_SPEC((CORBA::SystemException)) = 0;

  void sample_info(DDS::SampleInfo & sample_info,
                   const ReceivedDataElement *ptr);

  CORBA::Long total_samples() const;

  void set_sample_lost_status(const DDS::SampleLostStatus& status);
  void set_sample_rejected_status(
    const DDS::SampleRejectedStatus& status);

//remove document this!
  SubscriptionInstance* get_handle_instance(
    DDS::InstanceHandle_t handle);

  /**
  * Get an instance handle for a new instance.
  */
  DDS::InstanceHandle_t get_next_handle(const DDS::BuiltinTopicKey_t& key);

  virtual void purge_data(SubscriptionInstance* instance) = 0;

  virtual void release_instance_i(DDS::InstanceHandle_t handle) = 0;

  bool has_readcondition(DDS::ReadCondition_ptr a_condition);

  mutable SubscriptionInstanceMapType           instances_;

  /// Check if the received data sample or instance should
  /// be filtered.
  /**
   * @note Filtering will only occur if the application
   *       configured a finite duration in the Topic's LIFESPAN
   *       QoS policy or DataReader's TIME_BASED_FILTER QoS policy.
   */
  bool filter_sample(const DataSampleHeader& header);
  bool filter_instance(SubscriptionInstance* instance, 
                       const PublicationId& pubid);

  ReceivedDataAllocator        *rd_allocator_;
  DDS::DataReaderQos           qos_;

  // Status conditions accessible by subclasses.
  DDS::SampleRejectedStatus sample_rejected_status_;
  DDS::SampleLostStatus sample_lost_status_;

  /// lock protecting sample container as well as statuses.
  ACE_Recursive_Thread_Mutex   sample_lock_;

  typedef ACE_Reverse_Lock<ACE_Recursive_Thread_Mutex> Reverse_Lock_t;
  Reverse_Lock_t reverse_sample_lock_;
  
  DomainParticipantImpl*       participant_servant_;
  TopicImpl*                   topic_servant_;

  bool is_exclusive_ownership_;
  OwnershipManager* owner_manager_;  

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  DDS::ContentFilteredTopic_var content_filtered_topic_;
#endif

private:
  /// Send a SAMPLE_ACK message in response to a REQUEST_ACK message.
  bool send_sample_ack(
    const RepoId& publication,
    ACE_INT16 sequence,
    DDS::Time_t when);

  /// Data has arrived into the cache, unblock waiting ReadConditions
  void notify_read_conditions();

  void notify_subscription_lost(const DDS::InstanceHandleSeq& handles);

  /// Lookup the instance handles by the publication repo ids
  bool lookup_instance_handles(const WriterIdSeq& ids,
                               DDS::InstanceHandleSeq& hdls);

  friend class WriterInfo;
  friend class InstanceState;

  friend class ::DDS_TEST; //allows tests to get at dr_remote_objref_

  DDS::TopicDescription_var    topic_desc_;
  DDS::StatusMask              listener_mask_;
  DDS::DataReaderListener_var  listener_;
  DDS::DataReaderListener*     fast_listener_;
  DDS::DomainId_t              domain_id_;
  SubscriberImpl*              subscriber_servant_;
  DataReaderRemote_var         dr_remote_objref_;
  DDS::DataReader_var          dr_local_objref_;
  RepoId                       subscription_id_;

  CORBA::Long                  depth_;
  size_t                       n_chunks_;

  ACE_Recursive_Thread_Mutex   publication_handle_lock_;

  typedef std::map<RepoId, DDS::InstanceHandle_t, GUID_tKeyLessThan> RepoIdToHandleMap;
  RepoIdToHandleMap            id_to_handle_map_;

  // Status conditions.
  DDS::LivelinessChangedStatus         liveliness_changed_status_;
  DDS::RequestedDeadlineMissedStatus   requested_deadline_missed_status_;
  DDS::RequestedIncompatibleQosStatus  requested_incompatible_qos_status_;
  DDS::SubscriptionMatchedStatus       subscription_match_status_;

  // OpenDDS extended status.  This is only available via listener.
  BudgetExceededStatus                 budget_exceeded_status_;

  /**
   * @todo The subscription_lost_status_ and
   *       subscription_reconnecting_status_ are left here for
   *       future use when we add get_subscription_lost_status()
   *       and get_subscription_reconnecting_status() methods.
   */
  // Statistics of the lost subscriptions due to lost connection.
  SubscriptionLostStatus               subscription_lost_status_;
  // Statistics of the subscriptions that are associated with a
  // reconnecting datalink.
  // SubscriptionReconnectingStatus      subscription_reconnecting_status_;

  /// The orb's reactor to be used to register the liveliness
  /// timer.
  ACE_Reactor*                 reactor_;

  /// The time interval for checking liveliness.
  /// TBD: Should this be initialized with
  ///      DDS::DURATION_INFINITE_SEC and DDS::DURATION_INFINITE_NSEC
  ///      instead of ACE_Time_Value::zero to be consistent with default
  ///      duration qos ? Or should we simply use the ACE_Time_Value::zero
  ///      to indicate the INFINITY duration ?
  ACE_Time_Value               liveliness_lease_duration_;

  /// liveliness timer id; -1 if no timer is set
  long liveliness_timer_id_;

  CORBA::Long last_deadline_missed_total_count_;
  /// Watchdog responsible for reporting missed offered
  /// deadlines.
  std::auto_ptr<RequestedDeadlineWatchdog> watchdog_;

  /// Flag indicates that this datareader is a builtin topic
  /// datareader.
  bool is_bit_;

  /// Flag indicates that the init() is called.
  bool initialized_;
  bool always_get_history_;

  /// Flag indicating status of statistics gathering.
  bool statistics_enabled_;

  /// publications writing to this reader.
  typedef std::map<PublicationId, WriterInfo*, GUID_tKeyLessThan> WriterMapType;
  WriterMapType writers_;

  /// RW lock for reading/writing publications.
  ACE_RW_Thread_Mutex writers_lock_;

  /// Statistics for this reader, collected for each writer.
  StatsMapType statistics_;

  /// Bound (or initial reservation) of raw latency buffer.
  unsigned int raw_latency_buffer_size_;

  /// Type of raw latency data buffer.
  DataCollector<double>::OnFull raw_latency_buffer_type_;

  typedef std::set<DDS::ReadCondition_var, VarLess<DDS::ReadCondition> > ReadConditionSet;
  ReadConditionSet read_conditions_;

  /// Monitor object for this entity
  Monitor* monitor_;

  /// Periodic Monitor object for this entity
  Monitor* periodic_monitor_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
# include "DataReaderImpl.inl"
#endif  /* __ACE_INLINE__ */

// Insertion of WriterState enumeration values;
ostream& operator<<(ostream& str, OpenDDS::DCPS::WriterInfo::WriterState value);

#endif /* TAO_DDS_DCPS_DATAREADER_H  */
