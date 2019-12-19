/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SUBSCRIBER_H
#define OPENDDS_DCPS_SUBSCRIBER_H

#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "EntityImpl.h"
#include "Definitions.h"
#include "DataCollector_T.h"
#include "DataReaderImpl.h"
#include "ace/Recursive_Thread_Mutex.h"
#include "dds/DCPS/PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;
class Monitor;

#ifndef OPENDDS_NO_MULTI_TOPIC
class MultiTopicImpl;
#endif

class OpenDDS_Dcps_Export SubscriberImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::Subscriber>
  , public virtual EntityImpl {
public:

  SubscriberImpl(DDS::InstanceHandle_t handle,
                 const DDS::SubscriberQos& qos,
                 DDS::SubscriberListener_ptr a_listener,
                 const DDS::StatusMask& mask,
                 DomainParticipantImpl* participant);

  virtual ~SubscriberImpl();

  virtual DDS::InstanceHandle_t get_instance_handle();

  bool contains_reader(DDS::InstanceHandle_t a_handle);

  virtual DDS::DataReader_ptr create_datareader(
    DDS::TopicDescription_ptr a_topic_desc,
    const DDS::DataReaderQos& qos,
    DDS::DataReaderListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::ReturnCode_t delete_datareader(
    DDS::DataReader_ptr a_datareader);

  virtual DDS::ReturnCode_t delete_contained_entities();

  virtual DDS::DataReader_ptr lookup_datareader(
    const char* topic_name);

  virtual DDS::ReturnCode_t get_datareaders(
    DDS::DataReaderSeq& readers,
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states);

  virtual DDS::ReturnCode_t notify_datareaders();

  virtual DDS::ReturnCode_t set_qos(
    const DDS::SubscriberQos& qos);

  virtual DDS::ReturnCode_t get_qos(
    DDS::SubscriberQos& qos);

  virtual DDS::ReturnCode_t set_listener(
    DDS::SubscriberListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::SubscriberListener_ptr get_listener();

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE

  virtual DDS::ReturnCode_t begin_access();

  virtual DDS::ReturnCode_t end_access();

#endif

  virtual DDS::DomainParticipant_ptr get_participant();

  virtual DDS::ReturnCode_t set_default_datareader_qos(
    const DDS::DataReaderQos& qos);

  virtual DDS::ReturnCode_t get_default_datareader_qos(
    DDS::DataReaderQos& qos);

  virtual DDS::ReturnCode_t copy_from_topic_qos(
    DDS::DataReaderQos& a_datareader_qos,
    const DDS::TopicQos& a_topic_qos);

  virtual DDS::ReturnCode_t enable();

  /** This method is not defined in the IDL and is defined for
  *  internal use.
  *  Check if there is any datareader associated with it.
  */
  bool is_clean() const;

  // called by DataReaderImpl::data_received
  void data_received(DataReaderImpl* reader);

  DDS::ReturnCode_t reader_enabled(const char* topic_name,
                                   DataReaderImpl* reader);

#ifndef OPENDDS_NO_MULTI_TOPIC
  DDS::ReturnCode_t multitopic_reader_enabled(DDS::DataReader_ptr reader);
  void remove_from_datareader_set(DataReaderImpl* reader);
#endif

  DDS::SubscriberListener_ptr listener_for(DDS::StatusKind kind);

  /// @name Raw Latency Statistics Configuration Interfaces
  /// @{

  /// Configure the size of the raw data collection buffer.
  unsigned int& raw_latency_buffer_size();

  /// Configure the type of the raw data collection buffer.
  DataCollector<double>::OnFull& raw_latency_buffer_type();

  /// @}

  typedef OPENDDS_VECTOR(RepoId) SubscriptionIdVec;
  /// Populates a std::vector with the SubscriptionIds (GUIDs)
  /// of this Subscriber's Data Readers
  void get_subscription_ids(SubscriptionIdVec& subs);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  void update_ownership_strength (const PublicationId& pub_id,
                                  const CORBA::Long& ownership_strength);
#endif

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  void coherent_change_received(RepoId& publisher_id,
                                DataReaderImpl* reader,
                                Coherent_State& group_state);
#endif

  virtual RcHandle<EntityImpl> parent() const;

  static bool validate_datareader_qos(const DDS::DataReaderQos & qos,
                                      const DDS::DataReaderQos & default_qos,
                                      DDS::Topic_ptr a_topic,
                                      DDS::DataReaderQos & result_qos, bool mt);

private:

  /// Keep track of all the DataReaders attached to this
  /// Subscriber: key is the topic_name
  typedef OPENDDS_MULTIMAP(OPENDDS_STRING, DataReaderImpl_rch) DataReaderMap;

  /// Keep track of DataReaders with data
  /// std::set for now, want to encapsulate
  /// this so we can switch between a set or
  /// list depending on Presentation QoS.
  typedef OPENDDS_SET(DataReaderImpl_rch) DataReaderSet;

  /// DataReader id to qos map.
  typedef OPENDDS_MAP_CMP(RepoId, DDS::DataReaderQos, GUID_tKeyLessThan) DrIdToQosMap;

  DDS::InstanceHandle_t        handle_;

  DDS::SubscriberQos           qos_;
  DDS::DataReaderQos           default_datareader_qos_;

  DDS::StatusMask              listener_mask_;
  DDS::SubscriberListener_var  listener_;

  DataReaderSet                readers_not_enabled_;
  DataReaderMap                datareader_map_;
  DataReaderSet                datareader_set_;

#ifndef OPENDDS_NO_MULTI_TOPIC
  OPENDDS_MAP(OPENDDS_STRING, DDS::DataReader_var) multitopic_reader_map_;
#endif

  WeakRcHandle<DomainParticipantImpl> participant_;

  DDS::DomainId_t              domain_id_;
  RepoId                       dp_id_;

  /// Bound (or initial reservation) of raw latency buffers.
  unsigned int raw_latency_buffer_size_;

  /// Type of raw latency data buffers.
  DataCollector<double>::OnFull raw_latency_buffer_type_;

  /// this lock protects the data structures in this class.
  ACE_Recursive_Thread_Mutex   si_lock_;

  /// Monitor object for this entity
  unique_ptr<Monitor> monitor_;

  int access_depth_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SUBSCRIBER_H  */
