/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAWRITER_H
#define OPENDDS_DCPS_DATAWRITER_H

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DCPS/DataWriterCallbacks.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/MessageTracker.h"
#include "dds/DCPS/DataBlockLockPool.h"
#include "dds/DCPS/PoolAllocator.h"
#include "WriteDataContainer.h"
#include "Definitions.h"
#include "DataSampleHeader.h"
#include "TopicImpl.h"
#include "Time_Helper.h"
#include "CoherentChangeControl.h"
#include "GuidUtils.h"
#include "RcEventHandler.h"
#include "unique_ptr.h"
#include "Message_Block_Ptr.h"
#include "TimeTypes.h"

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
#include "FilterEvaluator.h"
#endif

#include "ace/Event_Handler.h"
#include "ace/OS_NS_sys_time.h"

#include <memory>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class PublisherImpl;
class DomainParticipantImpl;
class OfferedDeadlineWatchdog;
class Monitor;
class DataSampleElement;
class SendStateDataSampleList;
struct AssociationData;
class LivenessTimer;

/**
* @class DataWriterImpl
*
* @brief Implements the OpenDDS::DCPS::DataWriterRemote interfaces and
*        DDS::DataWriter interfaces.
*
* See the DDS specification, OMG formal/04-12-02, for a description of
* the interface this class is implementing.
*
* This class must be inherited by the type-specific datawriter which
* is specific to the data-type associated with the topic.
*
* @note: This class is responsible for allocating memory for the
*        header message block
*        (MessageBlock + DataBlock + DataSampleHeader) and the
*        DataSampleElement.
*        The data-type datawriter is responsible for allocating
*        memory for the sample data message block.
*        (e.g. MessageBlock + DataBlock + Foo data). But it gives
*        up ownership to this WriteDataContainer.
*/
class OpenDDS_Dcps_Export DataWriterImpl
  : public virtual LocalObject<DDS::DataWriter>,
    public virtual DataWriterCallbacks,
    public virtual EntityImpl,
    public virtual TransportClient,
    public virtual TransportSendListener {
public:
  friend class WriteDataContainer;
  friend class PublisherImpl;

  typedef OPENDDS_MAP_CMP(RepoId, SequenceNumber, GUID_tKeyLessThan) RepoIdToSequenceMap;

  struct AckToken {
    MonotonicTimePoint tstamp_;
    DDS::Duration_t max_wait_;
    SequenceNumber sequence_;

    AckToken(const DDS::Duration_t& max_wait,
             const SequenceNumber& sequence)
      : tstamp_(MonotonicTimePoint::now())
      , max_wait_(max_wait)
      , sequence_(sequence)
    {
    }

    ~AckToken() {}

    MonotonicTimePoint deadline() const {
      return tstamp_ + TimeDuration(max_wait_);
    }
  };

  DataWriterImpl();

  virtual ~DataWriterImpl();

  virtual DDS::InstanceHandle_t get_instance_handle();

  virtual DDS::ReturnCode_t set_qos(const DDS::DataWriterQos & qos);

  virtual DDS::ReturnCode_t get_qos(DDS::DataWriterQos & qos);

  virtual DDS::ReturnCode_t set_listener(
    DDS::DataWriterListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::DataWriterListener_ptr get_listener();

  virtual DDS::Topic_ptr get_topic();

  virtual DDS::ReturnCode_t wait_for_acknowledgments(
    const DDS::Duration_t & max_wait);

  virtual DDS::Publisher_ptr get_publisher();

  virtual DDS::ReturnCode_t get_liveliness_lost_status(
    DDS::LivelinessLostStatus & status);

  virtual DDS::ReturnCode_t get_offered_deadline_missed_status(
    DDS::OfferedDeadlineMissedStatus & status);

  virtual DDS::ReturnCode_t get_offered_incompatible_qos_status(
    DDS::OfferedIncompatibleQosStatus & status);

  virtual DDS::ReturnCode_t get_publication_matched_status(
    DDS::PublicationMatchedStatus & status);

  TimeDuration liveliness_check_interval(DDS::LivelinessQosPolicyKind kind);

  bool participant_liveliness_activity_after(const MonotonicTimePoint& tv);

  virtual DDS::ReturnCode_t assert_liveliness();

  virtual DDS::ReturnCode_t assert_liveliness_by_participant();

  typedef OPENDDS_VECTOR(DDS::InstanceHandle_t) InstanceHandleVec;
  void get_instance_handles(InstanceHandleVec& instance_handles);

  void get_readers(RepoIdSet& readers);

  virtual DDS::ReturnCode_t get_matched_subscriptions(
    DDS::InstanceHandleSeq & subscription_handles);

#if !defined (DDS_HAS_MINIMUM_BIT)
  virtual DDS::ReturnCode_t get_matched_subscription_data(
    DDS::SubscriptionBuiltinTopicData & subscription_data,
    DDS::InstanceHandle_t subscription_handle);
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  virtual DDS::ReturnCode_t enable();

  virtual void add_association(const RepoId& yourId,
                               const ReaderAssociation& reader,
                               bool active);

  virtual void transport_assoc_done(int flags, const RepoId& remote_id);

  virtual void association_complete(const RepoId& remote_id);

  virtual void remove_associations(const ReaderIdSeq & readers,
                                   bool callback);

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status);

  virtual void update_subscription_params(const RepoId& readerId,
                                          const DDS::StringSeq& params);


  /**
   * cleanup the DataWriter.
   */
  void cleanup();

  /**
   * Initialize the data members.
   */
  void init(
    TopicImpl*                            topic_servant,
    const DDS::DataWriterQos &            qos,
    DDS::DataWriterListener_ptr           a_listener,
    const DDS::StatusMask &               mask,
    WeakRcHandle<OpenDDS::DCPS::DomainParticipantImpl> participant_servant,
    OpenDDS::DCPS::PublisherImpl*         publisher_servant);

  void send_all_to_flush_control(ACE_Guard<ACE_Recursive_Thread_Mutex>& guard);

  /**
   * Delegate to the WriteDataContainer to register
   * Must tell the transport to broadcast the registered
   * instance upon returning.
   */
  DDS::ReturnCode_t
  register_instance_i(
    DDS::InstanceHandle_t& handle,
    Message_Block_Ptr data,
    const DDS::Time_t& source_timestamp);

  /**
   * Delegate to the WriteDataContainer to register and tell
   * the transport to broadcast the registered instance.
   */
  DDS::ReturnCode_t
  register_instance_from_durable_data(
    DDS::InstanceHandle_t& handle,
    Message_Block_Ptr data,
    const DDS::Time_t & source_timestamp);

  /**
   * Delegate to the WriteDataContainer to unregister and tell
   * the transport to broadcast the unregistered instance.
   */
  DDS::ReturnCode_t
  unregister_instance_i(
    DDS::InstanceHandle_t handle,
    const DDS::Time_t & source_timestamp);

  /**
   * Unregister all registered instances and tell the transport
   * to broadcast the unregistered instances.
   */
  void unregister_instances(const DDS::Time_t& source_timestamp);

  /**
   * Delegate to the WriteDataContainer to queue the instance
   * sample and finally tell the transport to send the sample.
   * \param filter_out can either be null (if the writer can't
   *        or won't evaluate the filters), or a list of
   *        associated reader RepoIds that should NOT get the
   *        data sample due to content filtering.
   */
  DDS::ReturnCode_t write(Message_Block_Ptr sample,
                          DDS::InstanceHandle_t handle,
                          const DDS::Time_t& source_timestamp,
                          GUIDSeq* filter_out);

  /**
   * Delegate to the WriteDataContainer to dispose all data
   * samples for a given instance and tell the transport to
   * broadcast the disposed instance.
   */
  DDS::ReturnCode_t dispose(DDS::InstanceHandle_t handle,
                            const DDS::Time_t & source_timestamp);

  /**
   * Return the number of samples for a given instance.
   */
  DDS::ReturnCode_t num_samples(DDS::InstanceHandle_t handle,
                                size_t&               size);

  /**
   * Retrieve the unsent data from the WriteDataContainer.
   */
   ACE_UINT64 get_unsent_data(SendStateDataSampleList& list) {
    return data_container_->get_unsent_data(list);
  }

  SendStateDataSampleList get_resend_data() {
    return data_container_->get_resend_data();
  }

  /**
   * Accessor of the repository id of this datawriter/publication.
   */
  RepoId get_publication_id();

  /**
   * Accessor of the repository id of the domain participant.
   */
  RepoId get_dp_id();

  /**
   * Delegate to WriteDataContainer to unregister all instances.
   */
  void unregister_all();

  /**
   * This is called by transport to notify that the sample is
   * delivered and it is delegated to WriteDataContainer
   * to adjust the internal data sample threads.
   */
  void data_delivered(const DataSampleElement* sample);

  void transport_discovery_change();

  /**
   * This is called by transport to notify that the control
   * message is delivered.
   */
  void control_delivered(const Message_Block_Ptr& sample);

  /// Does this writer have samples to be acknowledged?
  bool should_ack() const;

  /// Create an AckToken for ack operations.
  AckToken create_ack_token(DDS::Duration_t max_wait) const;

  virtual void retrieve_inline_qos_data(TransportSendListener::InlineQosData& qos_data) const;

  virtual bool check_transport_qos(const TransportInst& inst);

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE

  /// Are coherent changes pending?
  bool coherent_changes_pending();

  /// Starts a coherent change set; should only be called once.
  void begin_coherent_changes();

  /// Ends a coherent change set; should only be called once.
  void end_coherent_changes(const GroupCoherentSamples& group_samples);

#endif

  /**
   * Get associated topic type name.
   */
  char const* get_type_name() const;

  /**
   * This mothod is called by transport to notify the instance
   * sample is dropped and it delegates to WriteDataContainer
   * to update the internal list.
   */
  void data_dropped(const DataSampleElement* element,
                    bool dropped_by_transport);

  /**
   * This is called by transport to notify that the control
   * message is dropped.
   */
  void control_dropped(const Message_Block_Ptr& sample,
                       bool dropped_by_transport);

  /**
   * Accessor of the WriterDataContainer's lock.
   */
  // ciju: Seems this is no longer being used.
  // Was wrong. Still required.
  ACE_INLINE
  ACE_Recursive_Thread_Mutex& get_lock() {
    return data_container_->lock_;
  }

  /**
   * This is used to retrieve the listener for a certain status
   * change.
   *
   * If this datawriter has a registered listener and the status
   * kind is in the listener mask then the listener is returned.
   * Otherwise, the query for the listener is propagated up to the
   * factory/publisher.
   */
  DDS::DataWriterListener_ptr listener_for(DDS::StatusKind kind);

  /// Handle the assert liveliness timeout.
  virtual int handle_timeout(const ACE_Time_Value &tv,
                             const void *arg);

  /// Called by the PublisherImpl to indicate that the Publisher is now
  /// resumed and any data collected while it was suspended should now be sent.
  void send_suspended_data();

  void remove_all_associations();

  virtual void register_for_reader(const RepoId& participant,
                                   const RepoId& writerid,
                                   const RepoId& readerid,
                                   const TransportLocatorSeq& locators,
                                   DiscoveryListener* listener);

  virtual void unregister_for_reader(const RepoId& participant,
                                     const RepoId& writerid,
                                     const RepoId& readerid);

  virtual void update_locators(const RepoId& remote,
                               const TransportLocatorSeq& locators);

  void notify_publication_disconnected(const ReaderIdSeq& subids);
  void notify_publication_reconnected(const ReaderIdSeq& subids);
  void notify_publication_lost(const ReaderIdSeq& subids);

  /// Statistics counter.
  int         data_dropped_count_;
  int         data_delivered_count_;

  MessageTracker controlTracker;

  /**
   * This method create a header message block and chain with
   * the sample data. The header contains the information
   * needed. e.g. message id, length of whole message...
   * The fast allocator is used to allocate the message block,
   * data block and header.
   */
  DDS::ReturnCode_t
  create_sample_data_message(Message_Block_Ptr data,
                             DDS::InstanceHandle_t instance_handle,
                             DataSampleHeader& header_data,
                             Message_Block_Ptr& message,
                             const DDS::Time_t& source_timestamp,
                             bool content_filter);

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  /// Make sent data available beyond the lifetime of this
  /// @c DataWriter.
  bool persist_data();
#endif

  // Reset time interval for each instance.
  void reschedule_deadline();

  /// Wait for pending samples to drain.
  void wait_pending();

  /**
   * Get an instance handle for a new instance.
   */
  DDS::InstanceHandle_t get_next_handle();

  virtual RcHandle<EntityImpl> parent() const;

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  bool filter_out(const DataSampleElement& elt,
                  const OPENDDS_STRING& filterClassName,
                  const FilterEvaluator& evaluator,
                  const DDS::StringSeq& expression_params) const;
#endif

  /**
   * Wait until pending control elements have either been delivered
   * or dropped.
   */
  void wait_control_pending();

  DataBlockLockPool::DataBlockLock* get_db_lock() {
    return db_lock_pool_->get_lock();
  }

 /**
  *  Attempt to locate an existing instance for the given handle.
  */
 PublicationInstance_rch get_handle_instance(
   DDS::InstanceHandle_t handle);

 virtual ICE::Endpoint* get_ice_endpoint();

protected:

  DDS::ReturnCode_t wait_for_specific_ack(const AckToken& token);

  void prepare_to_delete();

  // type specific DataWriter's part of enable.
  virtual DDS::ReturnCode_t enable_specific() = 0;

  /// The number of chunks for the cached allocator.
  size_t                     n_chunks_;

  /// The multiplier for allocators affected by associations
  size_t                     association_chunk_multiplier_;


  /// The type name of associated topic.
  CORBA::String_var               type_name_;

  /// The qos policy list of this datawriter.
  DDS::DataWriterQos              qos_;

  /// The participant servant which creats the publisher that
  /// creates this datawriter.
  WeakRcHandle<DomainParticipantImpl>          participant_servant_;

  //This lock should be used to protect access to reader_info_
  ACE_Thread_Mutex reader_info_lock_;

  struct ReaderInfo {
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    WeakRcHandle<DomainParticipantImpl> participant_;
    OPENDDS_STRING filter_class_name_;
    OPENDDS_STRING filter_;
    DDS::StringSeq expression_params_;
    RcHandle<FilterEvaluator> eval_;
#endif
    SequenceNumber expected_sequence_;
    bool durable_;
    ReaderInfo(const char* filter_class_name, const char* filter, const DDS::StringSeq& params,
               WeakRcHandle<DomainParticipantImpl> participant, bool durable);
    ~ReaderInfo();
  };

  typedef OPENDDS_MAP_CMP(RepoId, ReaderInfo, GUID_tKeyLessThan) RepoIdToReaderInfoMap;
  RepoIdToReaderInfoMap reader_info_;

  struct AckCustomization {
    GUIDSeq customized_;
    AckToken& token_;
    explicit AckCustomization(AckToken& at) : token_(at) {}
  };

  virtual SendControlStatus send_control(const DataSampleHeader& header,
                                         Message_Block_Ptr msg);

private:

  void track_sequence_number(GUIDSeq* filter_out);

  void notify_publication_lost(const DDS::InstanceHandleSeq& handles);

  DDS::ReturnCode_t dispose_and_unregister(DDS::InstanceHandle_t handle,
                                           const DDS::Time_t& timestamp);

  /**
   * This method create a header message block and chain with
   * the registered sample. The header contains the information
   * needed. e.g. message id, length of whole message...
   * The fast allocator is not used for the header.
   */
  ACE_Message_Block*
  create_control_message(MessageId message_id,
                         DataSampleHeader& header,
                         Message_Block_Ptr data,
                         const DDS::Time_t& source_timestamp);

  /// Send the liveliness message.
  bool send_liveliness(const MonotonicTimePoint& now);

  /// Lookup the instance handles by the subscription repo ids
  void lookup_instance_handles(const ReaderIdSeq& ids,
                               DDS::InstanceHandleSeq& hdls);

  const RepoId& get_repo_id() const {
    return this->publication_id_;
  }

  DDS::DomainId_t domain_id() const {
    return this->domain_id_;
  }

  CORBA::Long get_priority_value(const AssociationData&) const {
    return this->qos_.transport_priority.value;
  }

#if defined(OPENDDS_SECURITY)
  DDS::Security::ParticipantCryptoHandle get_crypto_handle() const;
#endif

  void association_complete_i(const RepoId& remote_id);

  friend class ::DDS_TEST; // allows tests to get at privates


  // Data block local pool for this data writer.
  unique_ptr<DataBlockLockPool>  db_lock_pool_;

  /// The name of associated topic.
  CORBA::String_var               topic_name_;
  /// The associated topic repository id.
  RepoId                          topic_id_;
  /// The topic servant.
  TopicDescriptionPtr<TopicImpl>                 topic_servant_;

  /// The StatusKind bit mask indicates which status condition change
  /// can be notified by the listener of this entity.
  DDS::StatusMask                 listener_mask_;
  /// Used to notify the entity for relevant events.
  DDS::DataWriterListener_var     listener_;
  /// The domain id.
  DDS::DomainId_t                 domain_id_;
  RepoId                          dp_id_;
  /// The publisher servant which creates this datawriter.
  WeakRcHandle<PublisherImpl>     publisher_servant_;
  /// The repository id of this datawriter/publication.
  PublicationId                   publication_id_;
  /// The sequence number unique in DataWriter scope.
  SequenceNumber                  sequence_number_;
  /// Flag indicating DataWriter current belongs to
  /// a coherent change set.
  bool                            coherent_;
  /// The number of samples belonging to the current
  /// coherent change set.
  ACE_UINT32                      coherent_samples_;
  /// The sample data container.
  unique_ptr<WriteDataContainer>  data_container_;
  /// The lock to protect the activate subscriptions
  /// and status changes.
  ACE_Recursive_Thread_Mutex      lock_;

  typedef OPENDDS_MAP_CMP(RepoId, DDS::InstanceHandle_t, GUID_tKeyLessThan) RepoIdToHandleMap;

  RepoIdToHandleMap               id_to_handle_map_;

  RepoIdSet readers_;

  /// Status conditions.
  DDS::LivelinessLostStatus           liveliness_lost_status_ ;
  DDS::OfferedDeadlineMissedStatus    offered_deadline_missed_status_ ;
  DDS::OfferedIncompatibleQosStatus   offered_incompatible_qos_status_ ;
  DDS::PublicationMatchedStatus       publication_match_status_ ;

  /// True if the writer failed to actively signal its liveliness within
  /// its offered liveliness period.
  bool liveliness_lost_;

  /**
   * @todo The publication_lost_status_ and
   *       publication_reconnecting_status_ are left here for
   *       future use when we add get_publication_lost_status()
   *       and get_publication_reconnecting_status() methods.
   */
  // Statistics of the lost publications due to lost connection.
  // PublicationLostStatus               publication_lost_status_;
  // Statistics of the publications that associates with a
  // reconnecting datalink.
  // PublicationReconnectingStatus       publication_reconnecting_status_;

  /// The message block allocator.
  unique_ptr<MessageBlockAllocator>     mb_allocator_;
  /// The data block allocator.
  unique_ptr<DataBlockAllocator>        db_allocator_;
  /// The header data allocator.
  unique_ptr<DataSampleHeaderAllocator> header_allocator_;

  /// The orb's reactor to be used to register the liveliness
  /// timer.
  ACE_Reactor_Timer_Interface* reactor_;
  /// The time interval for sending liveliness message.
  TimeDuration liveliness_check_interval_;
  /// Timestamp of last write/dispose/assert_liveliness.
  MonotonicTimePoint last_liveliness_activity_time_;
  /// Total number of offered deadlines missed during last offered
  /// deadline status check.
  CORBA::Long last_deadline_missed_total_count_;
  /// Watchdog responsible for reporting missed offered
  /// deadlines.
  RcHandle<OfferedDeadlineWatchdog> watchdog_;

  /// Flag indicates that this datawriter is a builtin topic
  /// datawriter.
  bool is_bit_;

  RepoIdSet pending_readers_, assoc_complete_readers_;

  /// The cached available data while suspending and associated transaction ids.
  ACE_UINT64 min_suspended_transaction_id_;
  ACE_UINT64 max_suspended_transaction_id_;
  SendStateDataSampleList             available_data_list_;

  /// Monitor object for this entity
  unique_ptr<Monitor> monitor_;

  /// Periodic Monitor object for this entity
  unique_ptr<Monitor> periodic_monitor_;


  // Do we need to set the sequence repair header bit?
  //   must call prior to incrementing sequence number
  bool need_sequence_repair();
  bool need_sequence_repair_i() const;

  DDS::ReturnCode_t send_end_historic_samples(const RepoId& readerId);
  DDS::ReturnCode_t send_request_ack();

  bool liveliness_asserted_;

  // Lock used to synchronize remove_associations calls from discovery
  // and unregister_instances during deletion of datawriter from application
  ACE_Thread_Mutex sync_unreg_rem_assocs_lock_;
  RcHandle<LivenessTimer> liveness_timer_;
};

typedef RcHandle<DataWriterImpl> DataWriterImpl_rch;


class LivenessTimer : public RcEventHandler
{
public:
  LivenessTimer(DataWriterImpl& writer)
    : writer_(writer)
  {
  }

  /// Handle the assert liveliness timeout.
  virtual int handle_timeout(const ACE_Time_Value &tv,
                             const void *arg);

private:
  WeakRcHandle<DataWriterImpl> writer_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
