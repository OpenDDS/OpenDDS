/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAREADERIMPL_H
#define OPENDDS_DCPS_DATAREADERIMPL_H

#include "dcps_export.h"

#include "AssociationData.h"
#include "AtomicBool.h"
#include "Cached_Allocator_With_Overflow_T.h"
#include "CoherentChangeControl.h"
#include "ContentFilteredTopicImpl.h"
#include "DataReaderCallbacks.h"
#include "Definitions.h"
#include "DisjointSequence.h"
#include "DomainParticipantImpl.h"
#include "EntityImpl.h"
#include "GroupRakeData.h"
#include "InstanceState.h"
#include "MultiTopicImpl.h"
#include "OwnershipManager.h"
#include "PoolAllocator.h"
#include "RcEventHandler.h"
#include "RcHandle_T.h"
#include "RcObject.h"
#include "ReactorInterceptor.h"
#include "Service_Participant.h"
#include "Stats_T.h"
#include "SubscriptionInstance.h"
#include "TimeTypes.h"
#include "TopicImpl.h"
#include "WriterInfo.h"
#include "ZeroCopyInfoSeq_T.h"

#include "transport/framework/ReceivedDataSample.h"
#include "transport/framework/TransportClient.h"
#include "transport/framework/TransportReceiveListener.h"

#include <dds/DdsDcpsTopicC.h>
#include <dds/DdsDcpsSubscriptionExtC.h>
#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsTopicC.h>
#include <dds/DdsDcpsInfrastructureC.h>

#include <ace/String_Base.h>
#include <ace/Reverse_Lock_T.h>
#include <ace/Reactor.h>

#include <memory>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SubscriberImpl;
class DomainParticipantImpl;
class SubscriptionInstance;
class TopicImpl;
class TopicDescriptionImpl;
class Monitor;
class DataReaderImpl;
class FilterEvaluator;

typedef Cached_Allocator_With_Overflow<ReceivedDataElementMemoryBlock, ACE_Thread_Mutex>
ReceivedDataAllocator;

enum MarshalingType {
  FULL_MARSHALING,
  KEY_ONLY_MARSHALING
};

/// Elements stored for managing statistical data.
class OpenDDS_Dcps_Export WriterStats {
public:
  /// Default constructor.
  WriterStats(
    int amount = 0,
    DataCollector<double>::OnFull type = DataCollector<double>::KeepOldest);
#ifdef ACE_HAS_CPP11
  WriterStats(const WriterStats&) = default;
#endif

  /// Add a datum to the latency statistics.
  void add_stat(const TimeDuration& delay);

  /// Extract the current latency statistics for this writer.
  LatencyStatistics get_stats() const;

  /// Reset the latency statistics for this writer.
  void reset_stats();

#ifndef OPENDDS_SAFETY_PROFILE
  /// Dump any raw data.
  std::ostream& raw_data(std::ostream& str) const;
#endif

private:
  /// Latency statistics for the DataWriter to this DataReader.
  Stats<double> stats_;
};

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

class OpenDDS_Dcps_Export AbstractSamples
{
public:
  virtual ~AbstractSamples(){}
  virtual void reserve(CORBA::ULong size)=0;
  virtual void push_back(const DDS::SampleInfo& info, const void* sample)=0;
};

#endif

// Class to cleanup in case EndHistoricSamples is missed
class EndHistoricSamplesMissedSweeper : public ReactorInterceptor {
public:
  EndHistoricSamplesMissedSweeper(ACE_Reactor* reactor,
                                  ACE_thread_t owner,
                                  DataReaderImpl* reader);

  void schedule_timer(WriterInfo_rch& info);
  void cancel_timer(WriterInfo_rch& info);

  // Arg will be PublicationId
  int handle_timeout(const ACE_Time_Value& current_time, const void* arg);

  virtual bool reactor_is_shut_down() const
  {
    return TheServiceParticipant->is_shut_down();
  }

private:
  WeakRcHandle<DataReaderImpl> reader_;
  OPENDDS_SET(WriterInfo_rch) info_set_;

  class CommandBase : public Command {
  public:
    CommandBase(EndHistoricSamplesMissedSweeper* sweeper,
                WriterInfo_rch& info)
      : sweeper_(sweeper)
      , info_(info)
    { }

  protected:
    EndHistoricSamplesMissedSweeper* sweeper_;
    WriterInfo_rch info_;
  };

  class ScheduleCommand : public CommandBase {
  public:
    ScheduleCommand(EndHistoricSamplesMissedSweeper* sweeper,
                    WriterInfo_rch& info)
      : CommandBase(sweeper, info)
    { }
    virtual void execute();
  };

  class CancelCommand : public CommandBase {
  public:
    CancelCommand(EndHistoricSamplesMissedSweeper* sweeper,
                  WriterInfo_rch& info)
      : CommandBase(sweeper, info)
    { }
    virtual void execute();
  };
};

/**
* @class DataReaderImpl
*
* @brief Implements the DDS::DataReader interface.
*
* See the DDS specification, OMG formal/2015-04-10, for a description of
* the interface this class is implementing.
*
* This class must be inherited by the type-specific datareader which
* is specific to the data-type associated with the topic.
*
*/
class OpenDDS_Dcps_Export DataReaderImpl
  : public virtual LocalObject<DataReaderEx>,
    public virtual DataReaderCallbacks,
    public virtual EntityImpl,
    public virtual TransportClient,
    public virtual TransportReceiveListener,
    private WriterInfoListener {
public:
  friend class RequestedDeadlineWatchdog;
  friend class QueryConditionImpl;
  friend class SubscriberImpl;

  typedef OPENDDS_MAP(DDS::InstanceHandle_t, SubscriptionInstance_rch) SubscriptionInstanceMapType;
  typedef OPENDDS_SET(DDS::InstanceHandle_t) InstanceSet;
  typedef OPENDDS_SET(SubscriptionInstance_rch) SubscriptionInstanceSet;
  /// Type of collection of statistics for writers to this reader.
  typedef OPENDDS_MAP_CMP(GUID_t, WriterStats, GUID_tKeyLessThan) StatsMapType;

  DataReaderImpl();

  virtual ~DataReaderImpl();

  virtual DDS::InstanceHandle_t get_instance_handle();

  virtual void set_subscription_id(const GUID_t& guid);

  const GUID_t& subscription_id() const { return subscription_id_; }

  virtual void add_association(const WriterAssociation& writer,
                               bool active);

  virtual void transport_assoc_done(int flags, const GUID_t& remote_id);

  virtual void remove_associations(const WriterIdSeq& writers, bool callback);

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status);

  virtual void signal_liveliness(const GUID_t& remote_participant);

  /**
  * This is used to retrieve the listener for a certain status change.
  * If this datareader has a registered listener and the status kind
  * is in the listener mask then the listener is returned.
  * Otherwise, the query for the listener is propagated up to the
  * factory/subscriber.
  */
  DDS::DataReaderListener_ptr listener_for(DDS::StatusKind kind);

  /// tell instances when a DataWriter transitions to being alive
  /// The writer state is inout parameter, it has to be set ALIVE before
  /// handle_timeout is called since some subroutine use the state.
  void writer_became_alive(WriterInfo& info,
                           const MonotonicTimePoint& when);

  /// tell instances when a DataWriter transitions to DEAD
  /// The writer state is inout parameter, the state is set to DEAD
  /// when it returns.
  void writer_became_dead(WriterInfo& info);

  /// tell instance when a DataWriter is removed.
  /// The liveliness status need update.
  void writer_removed(WriterInfo& info);

  virtual void cleanup();

  void init(
    TopicDescriptionImpl* a_topic_desc,
    const DDS::DataReaderQos& qos,
    DDS::DataReaderListener_ptr a_listener,
    const DDS::StatusMask& mask,
    DomainParticipantImpl* participant,
    SubscriberImpl* subscriber);

  virtual DDS::ReadCondition_ptr create_readcondition(
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states);

#ifndef OPENDDS_NO_QUERY_CONDITION
  virtual DDS::QueryCondition_ptr create_querycondition(
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states,
    const char * query_expression,
    const DDS::StringSeq & query_parameters);
#endif

  virtual DDS::ReturnCode_t delete_readcondition(
    DDS::ReadCondition_ptr a_condition);

  virtual DDS::ReturnCode_t delete_contained_entities();

  virtual DDS::ReturnCode_t set_qos(
    const DDS::DataReaderQos & qos);

  virtual DDS::ReturnCode_t get_qos(
    DDS::DataReaderQos & qos);

  virtual DDS::ReturnCode_t set_listener(
    DDS::DataReaderListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::DataReaderListener_ptr get_listener();

  virtual DDS::TopicDescription_ptr get_topicdescription();

  virtual DDS::Subscriber_ptr get_subscriber();

  virtual DDS::ReturnCode_t get_sample_rejected_status(
    DDS::SampleRejectedStatus & status);

  virtual DDS::ReturnCode_t get_liveliness_changed_status(
    DDS::LivelinessChangedStatus & status);

  virtual DDS::ReturnCode_t get_requested_deadline_missed_status(
    DDS::RequestedDeadlineMissedStatus & status);

  virtual DDS::ReturnCode_t get_requested_incompatible_qos_status(
    DDS::RequestedIncompatibleQosStatus & status);

  virtual DDS::ReturnCode_t get_subscription_matched_status(
    DDS::SubscriptionMatchedStatus & status);

  virtual DDS::ReturnCode_t get_sample_lost_status(
    DDS::SampleLostStatus & status);

  virtual DDS::ReturnCode_t wait_for_historical_data(
    const DDS::Duration_t & max_wait);

  virtual DDS::ReturnCode_t get_matched_publications(
    DDS::InstanceHandleSeq & publication_handles);

#if !defined (DDS_HAS_MINIMUM_BIT)
  virtual DDS::ReturnCode_t get_matched_publication_data(
    DDS::PublicationBuiltinTopicData & publication_data,
    DDS::InstanceHandle_t publication_handle);
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  virtual DDS::ReturnCode_t enable();

#ifndef OPENDDS_SAFETY_PROFILE
  virtual void get_latency_stats(
    LatencyStatisticsSeq & stats);
#endif

  virtual void reset_latency_stats();

  virtual CORBA::Boolean statistics_enabled();

  virtual void statistics_enabled(
    CORBA::Boolean statistics_enabled);

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

  void transport_discovery_change();

  virtual bool check_transport_qos(const TransportInst& inst);

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
                             DDS::InstanceHandle_t publication_handle,
                             SubscriptionInstance_rch& instance,
                             bool& is_new_instance,
                             bool& filtered,
                             MarshalingType marshaling_type) = 0;

  virtual void dispose_unregister(const ReceivedDataSample& sample,
                                  DDS::InstanceHandle_t publication_handle,
                                  SubscriptionInstance_rch& instance);

  void process_latency(const ReceivedDataSample& sample);
  void notify_latency(GUID_t writer);

  size_t get_depth() const
  {
    return static_cast<size_t>(depth_);
  }
  size_t get_n_chunks() const
  {
    return n_chunks_;
  }

  void liveliness_lost();

  void remove_all_associations();

  void notify_subscription_disconnected(const WriterIdSeq& pubids);
  void notify_subscription_reconnected(const WriterIdSeq& pubids);
  void notify_subscription_lost(const WriterIdSeq& pubids);
  void notify_liveliness_change();

  bool is_bit() const;

  /**
   * This method is used for a precondition check of delete_datareader.
   *
   * @retval true We have zero-copy samples loaned out
   * @retval false We have no zero-copy samples loaned out
   */
  bool has_zero_copies();

  /// Release the instance with the handle.
  void release_instance(DDS::InstanceHandle_t handle);

  // Take appropriate actions upon learning instance or view state has been updated
  void state_updated(DDS::InstanceHandle_t handle);

  /// Release all instances held by the reader.
  virtual void release_all_instances() = 0;

  ACE_Reactor_Timer_Interface* get_reactor();

  GUID_t get_topic_id();
  GUID_t get_dp_id();

  typedef OPENDDS_VECTOR(DDS::InstanceHandle_t) InstanceHandleVec;
  void get_instance_handles(InstanceHandleVec& instance_handles);

  typedef std::pair<GUID_t, WriterInfo::WriterState> WriterStatePair;
  typedef OPENDDS_VECTOR(WriterStatePair) WriterStatePairVec;
  void get_writer_states(WriterStatePairVec& writer_states);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  void update_ownership_strength (const GUID_t& pub_id,
                                  const CORBA::Long& ownership_strength);

  // Access to OwnershipManager is only valid when the domain participant is valid;
  // therefore, we must lock the domain pariticipant when using  OwnershipManager.
  class OwnershipManagerPtr
  {
  public:
    OwnershipManagerPtr(DataReaderImpl* reader)
      : participant_( (reader && reader->is_exclusive_ownership_) ? reader->participant_servant_.lock() : RcHandle<DomainParticipantImpl>())
    {
    }
    operator bool() const { return participant_.in(); }
    OwnershipManager* operator->() const
    {
      return participant_ ? participant_->ownership_manager() : 0;
    }

  private:
    RcHandle<DomainParticipantImpl> participant_;
  };
  friend class OwnershipManagerPtr;

  struct OwnershipManagerScopedAccess {
    OwnershipManagerScopedAccess() : om_(0), lock_result_(0) {}
    explicit OwnershipManagerScopedAccess(DataReaderImpl::OwnershipManagerPtr om) : om_(om), lock_result_(om_ ? om_->instance_lock_acquire() : 0) {}
    ~OwnershipManagerScopedAccess() { release(); }

    void swap(OwnershipManagerScopedAccess& rhs)
    {
      if (&rhs != this) {
        std::swap(om_, rhs.om_);
        std::swap(lock_result_, rhs.lock_result_);
      }
    }

    int release()
    {
      int result = 0;
      if (om_ && !lock_result_) {
        result = om_->instance_lock_release();
      }
      om_ = 0;
      lock_result_ = 0;
      return result;
    }

    OwnershipManagerPtr om_;
    int lock_result_;
  };

  OwnershipManagerPtr ownership_manager() { return OwnershipManagerPtr(this); }
#endif

  virtual void lookup_instance(const ReceivedDataSample& sample,
                               SubscriptionInstance_rch& instance) = 0;

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC

  void enable_filtering(ContentFilteredTopicImpl* cft);

  DDS::ContentFilteredTopic_ptr get_cf_topic() const;

#endif

#ifndef OPENDDS_NO_MULTI_TOPIC

  void enable_multi_topic(MultiTopicImpl* mt);

#endif

  void update_subscription_params(const DDS::StringSeq& params) const;

  typedef OPENDDS_VECTOR(void*) GenericSeq;

  struct GenericBundle {
    GenericSeq samples_;
    DDS::SampleInfoSeq info_;
  };

  virtual DDS::ReturnCode_t read_generic(GenericBundle& gen,
    DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states, bool adjust_ref_count ) = 0;

  virtual DDS::ReturnCode_t take(
    AbstractSamples& samples,
    DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states)=0;

  virtual DDS::InstanceHandle_t lookup_instance_generic(const void* data) = 0;

  virtual DDS::ReturnCode_t read_instance_generic(void*& data,
    DDS::SampleInfo& info, DDS::InstanceHandle_t instance,
    DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states) = 0;

  virtual DDS::ReturnCode_t read_next_instance_generic(void*& data,
    DDS::SampleInfo& info, DDS::InstanceHandle_t previous_instance,
    DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states) = 0;

#endif

  void set_instance_state(DDS::InstanceHandle_t instance,
                          DDS::InstanceStateKind state,
                          const SystemTimePoint& timestamp = SystemTimePoint::now(),
                          const GUID_t& guid = GUID_UNKNOWN)
  {
    DDS::InstanceHandle_t publication_handle = DDS::HANDLE_NIL;
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, publication_handle_lock_);
      RepoIdToHandleMap::const_iterator pos = publication_id_to_handle_map_.find(guid);
      if (pos != publication_id_to_handle_map_.end()) {
        publication_handle = pos->second;
      }
    }

    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
    set_instance_state_i(instance, publication_handle, state, timestamp, guid);
  }

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  void begin_access();
  void end_access();
  void get_ordered_data(GroupRakeData& data,
                        DDS::SampleStateMask sample_states,
                        DDS::ViewStateMask view_states,
                        DDS::InstanceStateMask instance_states);

  void accept_coherent (const GUID_t& writer_id,
                        const GUID_t& publisher_id);
  void reject_coherent (const GUID_t& writer_id,
                        const GUID_t& publisher_id);
  void coherent_change_received (const GUID_t& publisher_id, Coherent_State& result);

  void coherent_changes_completed (DataReaderImpl* reader);

  void reset_coherent_info (const GUID_t& writer_id,
                            const GUID_t& publisher_id);
#endif

  // Called upon subscriber qos change to update the local cache.
  void set_subscriber_qos(const DDS::SubscriberQos & qos);

  // Set the instance related writers to reevaluate the owner.
  void reset_ownership (DDS::InstanceHandle_t instance);

  virtual RcHandle<EntityImpl> parent() const;

  void disable_transport();

  virtual void register_for_writer(const GUID_t& /*participant*/,
                                   const GUID_t& /*readerid*/,
                                   const GUID_t& /*writerid*/,
                                   const TransportLocatorSeq& /*locators*/,
                                   DiscoveryListener* /*listener*/);

  virtual void unregister_for_writer(const GUID_t& /*participant*/,
                                     const GUID_t& /*readerid*/,
                                     const GUID_t& /*writerid*/);

  virtual void update_locators(const GUID_t& remote,
                               const TransportLocatorSeq& locators);

  virtual DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint();

  void return_handle(DDS::InstanceHandle_t handle);

  const ValueDispatcher* get_value_dispatcher() const
  {
    TopicDescriptionPtr<TopicImpl> temp(topic_servant_);
    return temp ? dynamic_cast<const ValueDispatcher*>(temp->get_type_support()) : 0;
  }

protected:

  // Update max flag if the spec ever changes
  static const CORBA::ULong MAX_SAMPLE_STATE_FLAG = DDS::NOT_READ_SAMPLE_STATE;
  static const CORBA::ULong MAX_SAMPLE_STATE_MASK = (MAX_SAMPLE_STATE_FLAG << 1) - 1;
  static const CORBA::ULong MAX_SAMPLE_STATE_BITS = 2u;

  // Update max flag if the spec ever changes
  static const CORBA::ULong MAX_VIEW_STATE_FLAG = DDS::NOT_NEW_VIEW_STATE;
  static const CORBA::ULong MAX_VIEW_STATE_MASK = (MAX_VIEW_STATE_FLAG << 1) - 1;
  static const CORBA::ULong MAX_VIEW_STATE_BITS = 2u;

  // Update max flag if the spec ever changes
  static const CORBA::ULong MAX_INSTANCE_STATE_FLAG = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
  static const CORBA::ULong MAX_INSTANCE_STATE_MASK = (MAX_INSTANCE_STATE_FLAG << 1) - 1;
  static const CORBA::ULong MAX_INSTANCE_STATE_BITS = 3u;

  // These may need to be updated if the spec ever changes
  static const CORBA::ULong COMBINED_VIEW_STATE_SHIFT = MAX_INSTANCE_STATE_BITS;
  static const CORBA::ULong COMBINED_SAMPLE_STATE_SHIFT = COMBINED_VIEW_STATE_SHIFT + MAX_VIEW_STATE_BITS;

  typedef OPENDDS_SET(DDS::InstanceHandle_t) HandleSet;
  typedef OPENDDS_MAP(CORBA::ULong, HandleSet) LookupMap;

  static CORBA::ULong to_combined_states(CORBA::ULong sample_states, CORBA::ULong view_states, CORBA::ULong instance_states)
  {
    sample_states &= MAX_SAMPLE_STATE_MASK;
    view_states &= MAX_VIEW_STATE_MASK;
    instance_states &= MAX_INSTANCE_STATE_MASK;
    if (!(sample_states && view_states && instance_states)) {
      // catch-all for "bogus" lookups
      return 0;
    }
    return (sample_states << COMBINED_SAMPLE_STATE_SHIFT) | (view_states << COMBINED_VIEW_STATE_SHIFT) | instance_states;
  }

  static void split_combined_states(CORBA::ULong combined, CORBA::ULong& sample_states, CORBA::ULong& view_states, CORBA::ULong& instance_states)
  {
    sample_states = (combined >> COMBINED_SAMPLE_STATE_SHIFT) & MAX_SAMPLE_STATE_MASK;
    view_states = (combined >> COMBINED_VIEW_STATE_SHIFT) & MAX_VIEW_STATE_MASK;
    instance_states = combined & MAX_INSTANCE_STATE_MASK;
  }

  void initialize_lookup_maps();
  void update_lookup_maps(const SubscriptionInstanceMapType::iterator& input);
  void remove_from_lookup_maps(DDS::InstanceHandle_t handle);
  const HandleSet& lookup_matching_instances(CORBA::ULong sample_states, CORBA::ULong view_states, CORBA::ULong instance_states) const;

  LookupMap combined_state_lookup_;

  // Perform cast to get extended version of listener (otherwise nil)
  DataReaderListener_ptr get_ext_listener();

  virtual void remove_associations_i(const WriterIdSeq& writers, bool callback);

  void prepare_to_delete();

  /// Setup deserialization options
  DDS::ReturnCode_t setup_deserialization();

  RcHandle<SubscriberImpl> get_subscriber_servant();

  void post_read_or_take();

  // type specific DataReader's part of enable.
  virtual DDS::ReturnCode_t enable_specific() = 0;

  void sample_info(DDS::SampleInfo & sample_info,
                   const ReceivedDataElement *ptr);

  CORBA::Long total_samples() const;

  void set_sample_lost_status(const DDS::SampleLostStatus& status);
  void set_sample_rejected_status(
    const DDS::SampleRejectedStatus& status);

  SubscriptionInstance_rch get_handle_instance(
    DDS::InstanceHandle_t handle);

  /**
  * Get an instance handle for a new instance.
  */
  DDS::InstanceHandle_t get_next_handle(const DDS::BuiltinTopicKey_t& key);

  virtual void purge_data(SubscriptionInstance_rch instance) = 0;

  virtual void release_instance_i(DDS::InstanceHandle_t handle) = 0;
  virtual void state_updated_i(DDS::InstanceHandle_t handle) = 0;

  bool has_readcondition(DDS::ReadCondition_ptr a_condition);

  /// @TODO: document why the instances_ container is mutable.
  mutable SubscriptionInstanceMapType instances_;

  /// Assume since the container is mutable(?!!?) it may need to use the
  /// lock while const.
  /// @TODO: remove the recursive nature of the instances_lock if not needed.
  mutable ACE_Recursive_Thread_Mutex instances_lock_;

  /// Check if the received data sample or instance should
  /// be filtered.
  /**
   * @note Filtering will only occur if the application
   *       configured a finite duration in the Topic's LIFESPAN
   *       QoS policy or DataReader's TIME_BASED_FILTER QoS policy.
   */
  bool filter_sample(const DataSampleHeader& header);

  bool ownership_filter_instance(const SubscriptionInstance_rch& instance,
                                 const GUID_t& pubid);
  bool time_based_filter_instance(const SubscriptionInstance_rch& instance,
                                  MonotonicTimePoint& now,
                                  MonotonicTimePoint& deadline);

  void accept_sample_processing(const SubscriptionInstance_rch& instance, const DataSampleHeader& header, bool is_new_instance);

  virtual void qos_change(const DDS::DataReaderQos& qos);

  /// Data has arrived into the cache, unblock waiting ReadConditions
  void notify_read_conditions();

  unique_ptr<ReceivedDataAllocator> rd_allocator_;
  DDS::DataReaderQos qos_;
  DDS::DataReaderQos passed_qos_;

  // Status conditions accessible by subclasses.
  DDS::SampleRejectedStatus sample_rejected_status_;
  DDS::SampleLostStatus sample_lost_status_;

  /// lock protecting sample container as well as statuses.
  ACE_Recursive_Thread_Mutex sample_lock_;

  typedef ACE_Reverse_Lock<ACE_Recursive_Thread_Mutex> Reverse_Lock_t;
  Reverse_Lock_t reverse_sample_lock_;

  WeakRcHandle<DomainParticipantImpl> participant_servant_;
  TopicDescriptionPtr<TopicImpl> topic_servant_;
  TypeSupportImpl* type_support_;
  GUID_t topic_id_;

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  bool is_exclusive_ownership_;

#endif

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  mutable ACE_Thread_Mutex content_filtered_topic_mutex_;
  TopicDescriptionPtr<ContentFilteredTopicImpl> content_filtered_topic_;
#endif

#ifndef OPENDDS_NO_MULTI_TOPIC
  TopicDescriptionPtr<MultiTopicImpl> multi_topic_;
#endif

  /// Is accessing to Group coherent changes ?
  bool coherent_;

  /// Ordered group samples.
  GroupRakeData group_coherent_ordered_data_;

  DDS::SubscriberQos subqos_;

  virtual void add_link(const DataLink_rch& link, const GUID_t& peer);

private:
  virtual void install_type_support(TypeSupportImpl*) {}

  virtual void set_instance_state_i(DDS::InstanceHandle_t instance,
                                    DDS::InstanceHandle_t publication_handle,
                                    DDS::InstanceStateKind state,
                                    const SystemTimePoint& timestamp,
                                    const GUID_t& guid) = 0;

  void notify_subscription_lost(const DDS::InstanceHandleSeq& handles);

  /// Lookup the instance handles by the publication repo ids
  void lookup_instance_handles(const WriterIdSeq& ids,
                               DDS::InstanceHandleSeq& hdls);

  void instances_liveliness_update(const GUID_t& writer,
                                   DDS::InstanceHandle_t publication_handle);

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  bool verify_coherent_changes_completion(WriterInfo* writer);
  bool coherent_change_received(WriterInfo* writer);
#endif

  RcHandle<BitSubscriber> get_builtin_subscriber_proxy() const
  {
    RcHandle<DomainParticipantImpl> participant_servant = participant_servant_.lock();
    if (participant_servant) {
      return participant_servant->get_builtin_subscriber_proxy();
    }

    return RcHandle<BitSubscriber>();
  }

  DDS::DomainId_t domain_id() const { return this->domain_id_; }

  Priority get_priority_value(const AssociationData& data) const {
    return data.publication_transport_priority_;
  }

#if OPENDDS_CONFIG_SECURITY
  DDS::Security::ParticipantCryptoHandle get_crypto_handle() const;
#endif

  /// when done handling historic samples, resume
  void resume_sample_processing(const GUID_t& pub_id);

  /// collect samples received before END_HISTORIC_SAMPLES
  /// returns false if normal processing of this sample should be skipped
  bool check_historic(const ReceivedDataSample& sample);

  /// deliver samples that were held by check_historic()
  void deliver_historic(OPENDDS_MAP(SequenceNumber, ReceivedDataSample)& samples);

  friend class InstanceState;
  friend class EndHistoricSamplesMissedSweeper;

  friend class ::DDS_TEST; //allows tests to get at private data

  DDS::TopicDescription_var    topic_desc_;
  ACE_Thread_Mutex             listener_mutex_;
  DDS::StatusMask              listener_mask_;
  DDS::DataReaderListener_var  listener_;
  DDS::DomainId_t              domain_id_;
  GUID_t                       dp_id_;
  // subscriber_servant_ has to be a weak pinter because it may be used from the
  // transport reactor thread and that thread doesn't have the owenership of the
  // the subscriber_servant_ object.
  WeakRcHandle<SubscriberImpl>              subscriber_servant_;
  RcHandle<EndHistoricSamplesMissedSweeper> end_historic_sweeper_;

  CORBA::Long                  depth_;
  size_t                       n_chunks_;

  //Used to protect access to id_to_handle_map_
  ACE_Recursive_Thread_Mutex   publication_handle_lock_;

  typedef OPENDDS_MAP_CMP(GUID_t, DDS::InstanceHandle_t, GUID_tKeyLessThan) RepoIdToHandleMap;
  RepoIdToHandleMap            publication_id_to_handle_map_;

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
  ACE_Reactor_Timer_Interface* reactor_;

  class LivelinessTimer : public ReactorInterceptor {
  public:
    LivelinessTimer(ACE_Reactor* reactor,
                    ACE_thread_t owner,
                    DataReaderImpl* data_reader)
      : ReactorInterceptor(reactor, owner)
      , data_reader_(*data_reader)
      , liveliness_timer_id_(-1)
    { }

    void check_liveliness();

    void cancel_timer()
    {
      execute_or_enqueue(make_rch<CancelCommand>(this));
    }

    virtual bool reactor_is_shut_down() const
    {
      return TheServiceParticipant->is_shut_down();
    }

  private:
    WeakRcHandle<DataReaderImpl> data_reader_;

    /// liveliness timer id; -1 if no timer is set
    long liveliness_timer_id_;
    void check_liveliness_i(bool cancel, const MonotonicTimePoint& now);

    int handle_timeout(const ACE_Time_Value& current_time, const void* arg);

    class CommandBase : public Command {
    public:
      CommandBase(LivelinessTimer* timer)
        : timer_(timer)
      { }

    protected:
      LivelinessTimer* timer_;
    };

    class CheckLivelinessCommand : public CommandBase {
    public:
      CheckLivelinessCommand(LivelinessTimer* timer)
        : CommandBase(timer)
      { }
      virtual void execute()
      {
        timer_->check_liveliness_i(true, MonotonicTimePoint::now());
      }
    };

    class CancelCommand : public CommandBase {
    public:
      CancelCommand(LivelinessTimer* timer)
        : CommandBase(timer)
      { }
      virtual void execute()
      {
        if (timer_->liveliness_timer_id_ != -1) {
          timer_->reactor()->cancel_timer(timer_);
        }
      }
    };
  };
  RcHandle<LivelinessTimer> liveliness_timer_;

  CORBA::Long last_deadline_missed_total_count_;
  /// Watchdog responsible for reporting missed offered
  /// deadlines.
  TimeDuration deadline_period_;
  typedef OPENDDS_MULTIMAP(MonotonicTimePoint, SubscriptionInstance_rch) DeadlineQueue;
  DeadlineQueue deadline_queue_;
  bool deadline_queue_enabled_;
  typedef PmfSporadicTask<DataReaderImpl> DRISporadicTask;
  RcHandle<DRISporadicTask> deadline_task_;

  void schedule_deadline(SubscriptionInstance_rch instance,
                         bool timer_called);
  void reset_deadline_period(const TimeDuration& deadline_period);
  void reschedule_deadline(SubscriptionInstance_rch instance,
                           const MonotonicTimePoint& now);
  void cancel_deadline(SubscriptionInstance_rch instance);
  void cancel_all_deadlines();
  void deadline_task(const MonotonicTimePoint& now);
  void process_deadline(SubscriptionInstance_rch instance,
                        const MonotonicTimePoint& now,
                        bool timer_called);

  /// Flag indicates that this datareader is a builtin topic
  /// datareader.
  bool is_bit_;

  bool always_get_history_;

  /// Flag indicating status of statistics gathering.
  AtomicBool statistics_enabled_;

  /// publications writing to this reader.
  typedef OPENDDS_MAP_CMP(GUID_t, WriterInfo_rch,
                   GUID_tKeyLessThan) WriterMapType;

  WriterMapType writers_;

  /// RW lock for reading/writing publications.
  ACE_RW_Thread_Mutex writers_lock_;

  /// Statistics for this reader, collected for each writer.
  StatsMapType statistics_;
  ACE_Recursive_Thread_Mutex statistics_lock_;

  /// Bound (or initial reservation) of raw latency buffer.
  unsigned int raw_latency_buffer_size_;

  /// Type of raw latency data buffer.
  DataCollector<double>::OnFull raw_latency_buffer_type_;

  typedef VarLess<DDS::ReadCondition> RCCompLess;
  typedef OPENDDS_SET_CMP(DDS::ReadCondition_var,  RCCompLess) ReadConditionSet;
  ReadConditionSet read_conditions_;

  /// Monitor object for this entity
  unique_ptr<Monitor> monitor_;

  /// Periodic Monitor object for this entity
  unique_ptr<Monitor>  periodic_monitor_;

  bool transport_disabled_;

protected:
  typedef OPENDDS_SET(Encoding::Kind) EncodingKinds;
  EncodingKinds decoding_modes_;

public:
  class OpenDDS_Dcps_Export OnDataOnReaders : public Job {
  public:
    OnDataOnReaders(WeakRcHandle<SubscriberImpl> subscriber,
                    DDS::SubscriberListener_var sub_listener,
                    WeakRcHandle<DataReaderImpl> data_reader,
                    bool call,
                    bool set_reader_status)
      : subscriber_(subscriber)
      , sub_listener_(sub_listener)
      , data_reader_(data_reader)
      , call_(call)
      , set_reader_status_(set_reader_status)
    {}

  private:
    virtual void execute();

    WeakRcHandle<SubscriberImpl> subscriber_;
    DDS::SubscriberListener_var sub_listener_;
    WeakRcHandle<DataReaderImpl> data_reader_;
    const bool call_;
    const bool set_reader_status_;
  };

  class OpenDDS_Dcps_Export OnDataAvailable : public Job {
  public:
    OnDataAvailable(DDS::DataReaderListener_var listener,
                    WeakRcHandle<DataReaderImpl> data_reader,
                    bool call,
                    bool set_reader_status,
                    bool set_subscriber_status)
      : listener_(listener)
      , data_reader_(data_reader)
      , call_(call)
      , set_reader_status_(set_reader_status)
      , set_subscriber_status_(set_subscriber_status)
    {}

  private:
    virtual void execute();

    DDS::DataReaderListener_var listener_;
    WeakRcHandle<DataReaderImpl> data_reader_;
    const bool call_;
    const bool set_reader_status_;
    const bool set_subscriber_status_;
  };

protected:
#if OPENDDS_CONFIG_SECURITY
  Security::SecurityConfig_rch security_config_;
  DDS::DynamicType_var dynamic_type_;
#endif

  TransportMessageBlockAllocator mb_alloc_;
};

typedef RcHandle<DataReaderImpl> DataReaderImpl_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
# include "DataReaderImpl.inl"
#endif  /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_DATAREADER_H  */
