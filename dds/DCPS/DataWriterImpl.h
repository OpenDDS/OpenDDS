/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAWRITER_H
#define OPENDDS_DCPS_DATAWRITER_H

#include "dds/DdsDcpsPublicationS.h"
#include "dds/DdsDcpsDataWriterRemoteS.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "WriteDataContainer.h"
#include "Definitions.h"
#include "DataSampleList.h"
#include "DataSampleHeader.h"
#include "TopicImpl.h"
#include "Qos_Helper.h"
#include "CoherentChangeControl.h"
#include "GuidUtils.h"

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#include "FilterEvaluator.h"
#endif

#include "ace/Event_Handler.h"
#include "ace/OS_NS_sys_time.h"
#include "ace/Condition_T.h"
#include "ace/Condition_Recursive_Thread_Mutex.h"

#include <map>
#include <memory>
#include <set>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

namespace OpenDDS {
namespace DCPS {

class PublisherImpl;
class DomainParticipantImpl;
class OfferedDeadlineWatchdog;
class Monitor;
struct AssociationData;

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
*        DataSampleListElement.
*        The data-type datawriter is responsible for allocating
*        memory for the sample data message block.
*        (e.g. MessageBlock + DataBlock + Foo data). But it gives
*        up ownership to this WriteDataContainer.
*/
class OpenDDS_Dcps_Export DataWriterImpl
  : public virtual DDS::DataWriter,
    public virtual EntityImpl,
    public virtual TransportClient,
    public virtual TransportSendListener,
    public virtual ACE_Event_Handler
{
public:
  friend class WriteDataContainer;
  friend class PublisherImpl;

  typedef std::map<RepoId, SequenceNumber, GUID_tKeyLessThan>
    RepoIdToSequenceMap;

  struct AckToken {
    ACE_Time_Value tstamp_;
    DDS::Duration_t max_wait_;
    SequenceNumber sequence_;
    RepoIdToSequenceMap custom_;

    AckToken(const DDS::Duration_t& max_wait,
             const SequenceNumber& sequence)
        : tstamp_(ACE_OS::gettimeofday()),
        max_wait_(max_wait),
        sequence_(sequence) {}

    ~AckToken() {}

    ACE_Time_Value deadline() const {
      return duration_to_absolute_time_value(this->max_wait_, this->tstamp_);
    }

    DDS::Time_t timestamp() const {
      return time_value_to_time(this->tstamp_);
    }

    SequenceNumber expected(const RepoId& subscriber) const;

    bool marshal(ACE_Message_Block*& mblock, bool swap_bytes) const;
  };

  ///Constructor
  DataWriterImpl();

  ///Destructor
  virtual ~DataWriterImpl();

  virtual DDS::InstanceHandle_t get_instance_handle()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_qos(const DDS::DataWriterQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_qos(DDS::DataWriterQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_listener(
    DDS::DataWriterListener_ptr a_listener,
    DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::DataWriterListener_ptr get_listener()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::Topic_ptr get_topic()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t wait_for_acknowledgments(
    const DDS::Duration_t & max_wait)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::Publisher_ptr get_publisher()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_liveliness_lost_status(
    DDS::LivelinessLostStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_offered_deadline_missed_status(
    DDS::OfferedDeadlineMissedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_offered_incompatible_qos_status(
    DDS::OfferedIncompatibleQosStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_publication_matched_status(
    DDS::PublicationMatchedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t assert_liveliness()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t assert_liveliness_by_participant();

  typedef std::vector<DDS::InstanceHandle_t> InstanceHandleVec;
  void get_instance_handles(InstanceHandleVec& instance_handles);

  typedef std::set<RepoId, GUID_tKeyLessThan> IdSet;
  void get_readers(IdSet& readers);

  virtual DDS::ReturnCode_t get_matched_subscriptions(
    DDS::InstanceHandleSeq & subscription_handles)
  ACE_THROW_SPEC((CORBA::SystemException));

#if !defined (DDS_HAS_MINIMUM_BIT)
  virtual DDS::ReturnCode_t get_matched_subscription_data(
    DDS::SubscriptionBuiltinTopicData & subscription_data,
    DDS::InstanceHandle_t subscription_handle)
  ACE_THROW_SPEC((CORBA::SystemException));
#endif // !defined (DDS_HAS_MINIMUM_BIT)

  virtual DDS::ReturnCode_t enable()
  ACE_THROW_SPEC((CORBA::SystemException));

  void add_association(const RepoId& yourId,
                       const ReaderAssociation& reader,
                       bool active);

  void association_complete(const RepoId& remote_id);

  void remove_associations(const ReaderIdSeq & readers,
                           bool callback);

  void update_incompatible_qos(const IncompatibleQosStatus& status);

  void update_subscription_params(const RepoId& readerId,
                                  const DDS::StringSeq& params);

  /**
   * cleanup the DataWriter.
   */
  void cleanup();

  /**
   * Initialize the data members.
   */
  virtual void init(
    DDS::Topic_ptr                        topic,
    TopicImpl*                            topic_servant,
    const DDS::DataWriterQos &            qos,
    DDS::DataWriterListener_ptr           a_listener,
    const DDS::StatusMask &               mask,
    OpenDDS::DCPS::DomainParticipantImpl* participant_servant,
    OpenDDS::DCPS::PublisherImpl*         publisher_servant,
    DDS::DataWriter_ptr                   dw_local,
    OpenDDS::DCPS::DataWriterRemote_ptr   dw_remote)
  ACE_THROW_SPEC((CORBA::SystemException));

  /**
   * Delegate to the WriteDataContainer to register and tell
   * the transport to broadcast the registered instance.
   */
  DDS::ReturnCode_t
  register_instance_i(
    DDS::InstanceHandle_t& handle,
    DataSample* data,
    const DDS::Time_t & source_timestamp)
  ACE_THROW_SPEC((CORBA::SystemException));

  /**
   * Delegate to the WriteDataContainer to unregister and tell
   * the transport to broadcast the unregistered instance.
   */
  DDS::ReturnCode_t
  unregister_instance_i(
    DDS::InstanceHandle_t handle,
    const DDS::Time_t & source_timestamp)
  ACE_THROW_SPEC((CORBA::SystemException));

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
  DDS::ReturnCode_t write(DataSample* sample,
                          DDS::InstanceHandle_t handle,
                          const DDS::Time_t& source_timestamp,
                          GUIDSeq* filter_out);

  /**
   * Delegate to the WriteDataContainer to dispose all data
   * samples for a given instance and tell the transport to
   * broadcast the disposed instance.
   */
  DDS::ReturnCode_t dispose(DDS::InstanceHandle_t handle,
                            const DDS::Time_t & source_timestamp)
  ACE_THROW_SPEC((CORBA::SystemException));

  /**
   * Return the number of samples for a given instance.
   */
  DDS::ReturnCode_t num_samples(DDS::InstanceHandle_t handle,
                                size_t&               size);

  /**
   * Retrieve the unsent data from the WriteDataContainer.
   */
  DataSampleList get_unsent_data();
  DataSampleList get_resend_data();

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
  void data_delivered(const DataSampleListElement* sample);

  /**
   * This is called by transport to notify that the control
   * message is delivered.
   */
  void control_delivered(ACE_Message_Block* sample);

  SendControlStatus send_control_customized(const DataLinkSet_rch& links,
                                            const DataSampleHeader& header,
                                            ACE_Message_Block* msg,
                                            void* extra);

  /// Does this writer have samples to be acknowledged?
  bool should_ack() const;

  /// Create an AckToken for ack operations.
  AckToken create_ack_token(DDS::Duration_t max_wait) const;

  /// Send SAMPLE_ACK messages to associated readers.
  DDS::ReturnCode_t send_ack_requests(AckToken& token);

  /// Wait for SAMPLE_ACK responses from associated readers.
  DDS::ReturnCode_t wait_for_ack_responses(const AckToken& token);

  /// Deliver a requested SAMPLE_ACK message to this writer.
  virtual void deliver_ack(const DataSampleHeader& header, DataSample* data);

  virtual void retrieve_inline_qos_data(TransportSendListener::InlineQosData& qos_data) const;

  virtual bool check_transport_qos(const TransportInst& inst);

  /// Are coherent changes pending?
  bool coherent_changes_pending();

  /// Starts a coherent change set; should only be called once.
  void begin_coherent_changes();

  /// Ends a coherent change set; should only be called once.
  void end_coherent_changes(const GroupCoherentSamples& group_samples);

  /**
   * Accessor of the associated topic name.
   */
  const char* get_topic_name();

  /**
   * Get associated topic type name.
   */
  char const* get_type_name() const;

  /**
   * This mothod is called by transport to notify the instance
   * sample is dropped and it delegates to WriteDataContainer
   * to update the internal list.
   */
  void data_dropped(const DataSampleListElement* element,
                    bool dropped_by_transport);

  /**
   * This is called by transport to notify that the control
   * message is dropped.
   */
  void control_dropped(ACE_Message_Block* sample,
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
   * This method is called when an instance is unregistered from
   * the WriteDataContainer.
   *
   * The subclass must provide the implementation to unregister
   * the instance from its own map.
   */
  virtual void unregistered(DDS::InstanceHandle_t instance_handle);

  /**
   * This is used to retrieve the listener for a certain status
   * change.
   *
   * If this datawriter has a registered listener and the status
   * kind is in the listener mask then the listener is returned.
   * Otherwise, the query for the listener is propagated up to the
   * factory/publisher.
   */
  DDS::DataWriterListener* listener_for(DDS::StatusKind kind);

  /// Handle the assert liveliness timeout.
  virtual int handle_timeout(const ACE_Time_Value &tv,
                             const void *arg);

  virtual int handle_close(ACE_HANDLE,
                           ACE_Reactor_Mask);

  /// Called by the PublisherImpl to indicate that the Publisher is now
  /// resumed and any data collected while it was suspended should now be sent.
  void send_suspended_data();

  void remove_all_associations();

  void notify_publication_disconnected(const ReaderIdSeq& subids);
  void notify_publication_reconnected(const ReaderIdSeq& subids);
  void notify_publication_lost(const ReaderIdSeq& subids);

  void notify_connection_deleted();

  /// Statistics counter.
  int         data_dropped_count_;
  int         data_delivered_count_;
  int         control_dropped_count_;
  int         control_delivered_count_;

  /**
   * This method create a header message block and chain with
   * the sample data. The header contains the information
   * needed. e.g. message id, length of whole message...
   * The fast allocator is used to allocate the message block,
   * data block and header.
   */
  DDS::ReturnCode_t
  create_sample_data_message(DataSample* data,
                             DDS::InstanceHandle_t instance_handle,
                             DataSampleHeader& header_data,
                             ACE_Message_Block*& message,
                             const DDS::Time_t& source_timestamp,
                             bool content_filter);

  /// Make sent data available beyond the lifetime of this
  /// @c DataWriter.
  bool persist_data();

  // Reset time interval for each instance.
  void reschedule_deadline();

  /// Wait for pending samples to drain.
  void wait_pending();

  /**
   * Get an instance handle for a new instance.
   */
  DDS::InstanceHandle_t get_next_handle();

  virtual EntityImpl* parent() const;

protected:

  /**
   * Accessor of the cached publisher servant.
   */
  PublisherImpl* get_publisher_servant();

  // type specific DataWriter's part of enable.
  virtual DDS::ReturnCode_t enable_specific()
  ACE_THROW_SPEC((CORBA::SystemException)) = 0;

  /// The number of chunks for the cached allocator.
  size_t                     n_chunks_;

  /// The multiplier for allocators affected by associations
  size_t                     association_chunk_multiplier_;

  /**
   *  Attempt to locate an existing instance for the given handle.
   */
  PublicationInstance* get_handle_instance(
    DDS::InstanceHandle_t handle);

  /// The type name of associated topic.
  CORBA::String_var               type_name_;

  /// The qos policy list of this datawriter.
  DDS::DataWriterQos              qos_;

  /// The participant servant which creats the publisher that
  /// creates this datawriter.
  DomainParticipantImpl*          participant_servant_;

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

  struct ReaderInfo {
    DomainParticipantImpl* participant_;
    DDS::StringSeq expression_params_;
    std::string filter_;
    RcHandle<FilterEvaluator> eval_;
    SequenceNumber expected_sequence_;
    ReaderInfo(const char* filter, const DDS::StringSeq& params,
               DomainParticipantImpl* participant);
    ~ReaderInfo();
  };

  typedef std::map<RepoId, ReaderInfo, GUID_tKeyLessThan> RepoIdToReaderInfoMap;
  RepoIdToReaderInfoMap reader_info_;

  struct AckCustomization {
    GUIDSeq customized_;
    AckToken& token_;
    explicit AckCustomization(AckToken& at) : token_(at) {}
  };

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

private:

  void notify_publication_lost(const DDS::InstanceHandleSeq& handles);

  /**
   * This method create a header message block and chain with
   * the registered sample. The header contains the information
   * needed. e.g. message id, length of whole message...
   * The fast allocator is not used for the header.
   */
  ACE_Message_Block*
  create_control_message(MessageId message_id,
                         DataSampleHeader& header,
                         ACE_Message_Block* data,
                         const DDS::Time_t& source_timestamp);

  /// Send the liveliness message.
  bool send_liveliness(const ACE_Time_Value& now);

  /// Lookup the instance handles by the subscription repo ids
  bool lookup_instance_handles(const ReaderIdSeq& ids,
                               DDS::InstanceHandleSeq& hdls);


  const RepoId& get_repo_id() const { return this->publication_id_; }

  CORBA::Long get_priority_value(const AssociationData&) const {
    return this->qos_.transport_priority.value;
  }

  friend class ::DDS_TEST; // allows tests to get at dw_remote_objref_

  /// The name of associated topic.
  CORBA::String_var               topic_name_;
  /// The associated topic repository id.
  RepoId                          topic_id_;
  /// The object reference of the associated topic.
  DDS::Topic_var                  topic_objref_;
  /// The topic servant.
  TopicImpl*                      topic_servant_;

  /// The StatusKind bit mask indicates which status condition change
  /// can be notified by the listener of this entity.
  DDS::StatusMask                 listener_mask_;
  /// Used to notify the entity for relevant events.
  DDS::DataWriterListener_var     listener_;
  /// The datawriter listener servant.
  DDS::DataWriterListener*        fast_listener_;
  /// The domain id.
  DDS::DomainId_t                 domain_id_;
  /// The publisher servant which creates this datawriter.
  PublisherImpl*                  publisher_servant_;
  /// the object reference of the local datawriter
  DDS::DataWriter_var             dw_local_objref_;
  /// The object reference of the remote datawriter.
  OpenDDS::DCPS::DataWriterRemote_var dw_remote_objref_;
  /// The repository id of this datawriter/publication.
  PublicationId                   publication_id_;
  /// The sequence number unique in DataWriter scope.
  /// Not used in first implementation.
  SequenceNumber                  sequence_number_;
  /// Flag indicating DataWriter current belongs to
  /// a coherent change set.
  bool                            coherent_;
  /// The number of samples belonging to the current
  /// coherent change set.
  ACE_UINT32                      coherent_samples_;
  /// The sample data container.
  WriteDataContainer*             data_container_;
  /// The lock to protect the activate subscriptions
  /// and status changes.
  ACE_Recursive_Thread_Mutex      lock_;

  typedef std::map<RepoId, DDS::InstanceHandle_t, GUID_tKeyLessThan> RepoIdToHandleMap;

  RepoIdToHandleMap               id_to_handle_map_;

  IdSet                           readers_;

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
  MessageBlockAllocator*     mb_allocator_;
  /// The data block allocator.
  DataBlockAllocator*        db_allocator_;
  /// The header data allocator.
  DataSampleHeaderAllocator* header_allocator_;

  /// The orb's reactor to be used to register the liveliness
  /// timer.
  ACE_Reactor*               reactor_;
  /// The time interval for sending liveliness message.
  ACE_Time_Value             liveliness_check_interval_;
  /// Timestamp of last write/dispose/assert_liveliness.
  ACE_Time_Value             last_liveliness_activity_time_;
  /// Total number of offered deadlines missed during last offered
  /// deadline status check.
  CORBA::Long last_deadline_missed_total_count_;
  /// Watchdog responsible for reporting missed offered
  /// deadlines.
  std::auto_ptr<OfferedDeadlineWatchdog> watchdog_;
  /// The flag indicates whether the liveliness timer is scheduled and
  /// needs be cancelled.
  bool                       cancel_timer_;

  /// Flag indicates that this datawriter is a builtin topic
  /// datawriter.
  bool                       is_bit_;

  /// Flag indicates that the init() is called.
  bool                       initialized_;

  /// Lock used for wait_for_acks() processing.
  ACE_SYNCH_MUTEX wfaLock_;

  /// Used to block in wait_for_acks().
  ACE_Condition<ACE_SYNCH_MUTEX> wfaCondition_;

  RepoIdToSequenceMap idToSequence_;

  IdSet                      pending_readers_;

  /// The cached available data while suspending.
  DataSampleList             available_data_list_;

  /// Monitor object for this entity
  Monitor* monitor_;

  /// Periodic Monitor object for this entity
  Monitor* periodic_monitor_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif
