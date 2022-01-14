/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REPLAYERIMPL_H
#define OPENDDS_DCPS_REPLAYERIMPL_H

#include "Replayer.h"
#include "DataWriterCallbacks.h"
#include "WriteDataContainer.h"
#include "Definitions.h"
#include "DataSampleHeader.h"
#include "TopicImpl.h"
#include "Time_Helper.h"
#include "CoherentChangeControl.h"
#include "GuidUtils.h"
#include "unique_ptr.h"
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#  include "FilterEvaluator.h"
#endif
#include "ConditionVariable.h"
#include "transport/framework/TransportSendListener.h"
#include "transport/framework/TransportClient.h"

#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsTopicC.h>

#include <ace/Event_Handler.h>
#include <ace/OS_NS_sys_time.h>
#include <ace/Recursive_Thread_Mutex.h>

#include <memory>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SendStateDataSampleList;
class DataSampleElement;

/**
 * @class ReplayerImpl
 *
 * @brief Implementation of Replayer functionality
 *
 * This class is the implementation of the Replayer.
 * Inheritance is used to limit the applications access to
 * underlying system methods.
 */

class OpenDDS_Dcps_Export ReplayerImpl : public Replayer,
  public TransportClient,
  public TransportSendListener,
  public DataWriterCallbacks,
  public EntityImpl
{
public:
  ReplayerImpl();
  ~ReplayerImpl();

  /**
   * cleanup the DataWriter.
   */
  DDS::ReturnCode_t cleanup();

  /**
   * Initialize the data members.
   */
  virtual void init(
    DDS::Topic_ptr                        topic,
    TopicImpl*                            topic_servant,
    const DDS::DataWriterQos &            qos,
    ReplayerListener_rch                  a_listener,
    const DDS::StatusMask &               mask,
    OpenDDS::DCPS::DomainParticipantImpl* participant_servant,
    const DDS::PublisherQos&              publisher_qos);

  // Implement Replayer
  virtual DDS::ReturnCode_t write (const RawDataSample& sample );
  virtual DDS::ReturnCode_t write_to_reader (DDS::InstanceHandle_t subscription,
                                             const RawDataSample&  sample );
  virtual DDS::ReturnCode_t write_to_reader (DDS::InstanceHandle_t    subscription,
                                             const RawDataSampleList& samples );
  virtual DDS::ReturnCode_t set_qos (const DDS::PublisherQos & publisher_qos,
                                     const DDS::DataWriterQos &  datawriter_qos);
  virtual DDS::ReturnCode_t get_qos (DDS::PublisherQos &  publisher_qos,
                                     DDS::DataWriterQos & datawriter_qos);
  virtual DDS::ReturnCode_t set_listener (const ReplayerListener_rch & a_listener,
                                          DDS::StatusMask              mask);
  virtual ReplayerListener_rch get_listener ();

  // Implement TransportClient
  virtual bool check_transport_qos(const TransportInst& inst);
  virtual RepoId get_repo_id() const;
  DDS::DomainId_t domain_id() const { return this->domain_id_; }
  virtual CORBA::Long get_priority_value(const AssociationData& data) const;
  SequenceNumber get_max_sn() const { return sequence_number_; }


  // Implement TransportSendListener
  virtual void data_delivered(const DataSampleElement* sample);
  virtual void data_dropped(const DataSampleElement* sample,
                            bool                         dropped_by_transport);

  virtual void control_delivered(const Message_Block_Ptr& sample);
  virtual void control_dropped(const Message_Block_Ptr& sample,
                               bool               dropped_by_transport);

  virtual void notify_publication_disconnected(const ReaderIdSeq& subids);
  virtual void notify_publication_reconnected(const ReaderIdSeq& subids);
  virtual void notify_publication_lost(const ReaderIdSeq& subids);

  /// Statistics counter.
  int data_dropped_count_;
  int data_delivered_count_;


  virtual void retrieve_inline_qos_data(InlineQosData& qos_data) const;

  // implement DataWriterCallbacks
  virtual void add_association(const RepoId&            yourId,
                               const ReaderAssociation& reader,
                               bool                     active);

  virtual void remove_associations(const ReaderIdSeq& readers,
                                   CORBA::Boolean     callback);

  virtual void replay_durable_data_for(const RepoId&) {}

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status);

  virtual void update_subscription_params(const RepoId&         readerId,
                                          const DDS::StringSeq& exprParams);

  void remove_all_associations();

  virtual void register_for_reader(const RepoId& participant,
                                   const RepoId& writerid,
                                   const RepoId& readerid,
                                   const TransportLocatorSeq& locators,
                                   DiscoveryListener* listener);

  virtual void unregister_for_reader(const RepoId& participant,
                                     const RepoId& writerid,
                                     const RepoId& readerid);

  virtual DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint() { return DCPS::WeakRcHandle<ICE::Endpoint>(); }

  DDS::ReturnCode_t enable();

  DomainParticipantImpl*          participant() {
    return participant_servant_;
  }

  virtual DDS::InstanceHandle_t get_instance_handle();

private:
  void notify_publication_lost(const DDS::InstanceHandleSeq& handles);

  DDS::ReturnCode_t write (const RawDataSample* sample_array, int array_size, DDS::InstanceHandle_t* reader);

  DDS::ReturnCode_t
  create_sample_data_message(Message_Block_Ptr   data,
                             DataSampleHeader&   header_data,
                             Message_Block_Ptr&  message,
                             const DDS::Time_t&  source_timestamp,
                             bool                content_filter);
  bool need_sequence_repair() const;

  /// Lookup the instance handles by the subscription repo ids
  void lookup_instance_handles(const ReaderIdSeq&      ids,
                               DDS::InstanceHandleSeq& hdls);
  /// The number of chunks for the cached allocator.
  size_t n_chunks_;

  /// The multiplier for allocators affected by associations
  size_t association_chunk_multiplier_;

  /// The type name of associated topic.
  CORBA::String_var type_name_;

  /// The qos policy list of this datawriter.
  DDS::DataWriterQos qos_;
  /// The qos policy passed in by the user.
  /// Differs from qos_ because representation has been interpreted.
  DDS::DataWriterQos passed_qos_;

  /// The participant servant which creats the publisher that
  /// creates this datawriter.
  DomainParticipantImpl*          participant_servant_;

  struct ReaderInfo {
    SequenceNumber expected_sequence_;
    bool durable_;
    ReaderInfo(const char* filter, const DDS::StringSeq& params,
               DomainParticipantImpl* participant, bool durable);
    ~ReaderInfo();
  };

  typedef OPENDDS_MAP_CMP(RepoId, ReaderInfo, GUID_tKeyLessThan) RepoIdToReaderInfoMap;
  RepoIdToReaderInfoMap reader_info_;

  void association_complete_i(const RepoId& remote_id);

  friend class ::DDS_TEST; // allows tests to get at privates

  /// The name of associated topic.
  CORBA::String_var topic_name_;
  /// The associated topic repository id.
  RepoId topic_id_;
  /// The object reference of the associated topic.
  DDS::Topic_var topic_objref_;
  /// The topic servant.
  TopicDescriptionPtr<TopicImpl> topic_servant_;

  /// The StatusKind bit mask indicates which status condition change
  /// can be notified by the listener of this entity.
  DDS::StatusMask listener_mask_;
  /// Used to notify the entity for relevant events.
  ReplayerListener_rch listener_;
  /// The domain id.
  DDS::DomainId_t domain_id_;
  /// The publisher servant which creates this datawriter.
  PublisherImpl*                  publisher_servant_;
  DDS::PublisherQos publisher_qos_;

  /// The repository id of this datawriter/publication.
  PublicationId publication_id_;
  /// The sequence number unique in DataWriter scope.
  SequenceNumber sequence_number_;

  /// The sample data container.
  // WriteDataContainer*             data_container_;
  /// The lock to protect the activate subscriptions
  /// and status changes.
  ACE_Recursive_Thread_Mutex lock_;

  typedef OPENDDS_MAP_CMP(RepoId, DDS::InstanceHandle_t, GUID_tKeyLessThan) RepoIdToHandleMap;

  RepoIdToHandleMap id_to_handle_map_;

  RepoIdSet readers_;

  /// Status conditions.
  // DDS::LivelinessLostStatus liveliness_lost_status_;
  // DDS::OfferedDeadlineMissedStatus offered_deadline_missed_status_;
  DDS::OfferedIncompatibleQosStatus offered_incompatible_qos_status_;
  DDS::PublicationMatchedStatus publication_match_status_;

  /// True if the writer failed to actively signal its liveliness within
  /// its offered liveliness period.
  // bool liveliness_lost_;

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

  // The message block allocator.
  unique_ptr<MessageBlockAllocator>     mb_allocator_;
  // The data block allocator.
  unique_ptr<DataBlockAllocator>        db_allocator_;
  // The header data allocator.
  unique_ptr<DataSampleHeaderAllocator> header_allocator_;

  /// The cached allocator to allocate DataSampleElement
  /// objects.
  unique_ptr<DataSampleElementAllocator> sample_list_element_allocator_;

  /// The orb's reactor to be used to register the liveliness
  /// timer.
  // ACE_Reactor_Timer_Interface* reactor_;
  /// The time interval for sending liveliness message.
  // ACE_Time_Value             liveliness_check_interval_;
  /// Timestamp of last write/dispose/assert_liveliness.
  // ACE_Time_Value             last_liveliness_activity_time_;
  /// Total number of offered deadlines missed during last offered
  /// deadline status check.
  // CORBA::Long last_deadline_missed_total_count_;
  /// Watchdog responsible for reporting missed offered
  /// deadlines.
  // unique_ptr<OfferedDeadlineWatchdog> watchdog_;
  /// The flag indicates whether the liveliness timer is scheduled and
  /// needs be cancelled.
  // bool                       cancel_timer_;

  /// Flag indicates that this datawriter is a builtin topic
  /// datawriter.
  bool is_bit_;

  typedef OPENDDS_MAP_CMP(RepoId, SequenceNumber, GUID_tKeyLessThan)
  RepoIdToSequenceMap;

  RepoIdToSequenceMap idToSequence_;

  ConditionVariable<ACE_Recursive_Thread_Mutex> empty_condition_;
  int pending_write_count_;
};

} // namespace DCPS
} // namespace

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* end of include guard: OPENDDS_DCPS_REPLAYERIMPL_H */
