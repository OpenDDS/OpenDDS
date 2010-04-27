/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PUBLISHER_IMPL_H
#define OPENDDS_DCPS_PUBLISHER_IMPL_H

#include "dds/DdsDcpsPublicationS.h"
#include "dds/DdsDcpsDataWriterRemoteC.h"
#include "dds/DdsDcpsInfoC.h"
#include "EntityImpl.h"
#include "DataWriterImpl.h"
#include "DataSampleList.h"
#include "dds/DCPS/transport/framework/TransportInterface.h"
#include "ace/Synch.h"
#include "ace/Reverse_Lock_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <map>
#include <list>

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;
class DataWriterImpl;
class Monitor;

/// Information about a DataWriter
struct OpenDDS_Dcps_Export PublisherDataWriterInfo {
  /// The remote datawriter object reference.
  DataWriterRemote_ptr  remote_writer_objref_;
  /// The local datawriter object reference.
  DDS::DataWriter_ptr          local_writer_objref_;
  /// The datawriter servant.
  DataWriterImpl*              local_writer_impl_;
  /// The topic id from repository.
  RepoId                       topic_id_;
  /// The datawriter/publication id from repository.
  PublicationId                publication_id_;
  /// The group id of the datawriter. - NOT USED IN FIRST IMPL
  CoherencyGroup               group_id_;
};

typedef std::multimap<ACE_CString, PublisherDataWriterInfo*>
DataWriterMap;

typedef std::map<PublicationId, PublisherDataWriterInfo*, GUID_tKeyLessThan>
PublicationMap;

// DataWriter id to qos map.
typedef std::map<RepoId, DDS::DataWriterQos, GUID_tKeyLessThan> DwIdToQosMap;

/**
* @class PublisherImpl
*
* @brief Implements the OpenDDS::DCPS::Publisher interfaces.
*
* This class acts as a factory and container of the datawriter.
* It is also an intermedia class which delegates the data from
* datawriter to transport for sending.
*
* See the DDS specification, OMG formal/04-12-02, for a description of
* the interface this class is implementing.
*/
class OpenDDS_Dcps_Export PublisherImpl
  : public virtual LocalObject<DDS::Publisher>,
    public virtual EntityImpl,
    public virtual TransportInterface {
public:

  typedef std::map<PublicationId, DataSampleList> DataSampleListMap;

  ///Constructor
  PublisherImpl(DDS::InstanceHandle_t handle,
                const DDS::PublisherQos& qos,
                DDS::PublisherListener_ptr a_listener,
                const DDS::StatusMask& mask,
                DomainParticipantImpl* participant);

  ///Destructor
  virtual ~PublisherImpl();

  virtual DDS::InstanceHandle_t get_instance_handle()
  ACE_THROW_SPEC((CORBA::SystemException));

  bool contains_writer(DDS::InstanceHandle_t a_handle);

  virtual DDS::DataWriter_ptr create_datawriter(
    DDS::Topic_ptr a_topic,
    const DDS::DataWriterQos & qos,
    DDS::DataWriterListener_ptr a_listener,
    DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t delete_datawriter(
    DDS::DataWriter_ptr a_datawriter)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::DataWriter_ptr lookup_datawriter(
    const char * topic_name)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t delete_contained_entities()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_qos(
    const DDS::PublisherQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_qos(
    DDS::PublisherQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_listener(
    DDS::PublisherListener_ptr a_listener,
    DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::PublisherListener_ptr get_listener()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t suspend_publications()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t resume_publications()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t begin_coherent_changes()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t end_coherent_changes()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t wait_for_acknowledgments(
    const DDS::Duration_t & max_wait)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::DomainParticipant_ptr get_participant()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_default_datawriter_qos(
    const DDS::DataWriterQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_default_datawriter_qos(
    DDS::DataWriterQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t copy_from_topic_qos(
    DDS::DataWriterQos & a_datawriter_qos,
    const DDS::TopicQos & a_topic_qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t enable()
  ACE_THROW_SPEC((CORBA::SystemException));

  ACE_INLINE
  ACE_Recursive_Thread_Mutex&      get_pi_lock() {
    return pi_lock_;
  }

  /** This method is not defined in the IDL and is defined for
  *  internal use.
  *  Check if there is any datawriter associated with this publisher.
  */
  int is_clean() const;

  /** This method is called when the datawriter created by this
  * publisher was enabled. It will notify the DCPSInfo that
  * a new datawriter/publication is associated with the topic.
  */
  DDS::ReturnCode_t writer_enabled(
    DataWriterRemote_ptr remote_writer,
    DDS::DataWriter_ptr local_writer,
    const char* topic_name,
    //BuiltinTopicKey_t topic_key
    RepoId topic_id);

  /**
  * This method is called by datawriter to tell transport
  * to add new subscriptions with the datawriter.
  */
  void add_associations(
    const ReaderAssociationSeq & readers,
    DataWriterImpl* writer,
    const DDS::DataWriterQos writer_qos);

  /**
  * This method is called by datawriter to tell transport
  * to remove subscriptions with the datawriter.
  */
  void remove_associations(
    // sequence of reader ids
    const ReaderIdSeq& readers,
    const RepoId& writer);

  /**
  * This is called by datawriter to notify the publisher to
  * collect the available data from the datawriter for
  * sending.
  */
  DDS::ReturnCode_t
  data_available(DataWriterImpl* writer, bool resend = false);

  /**
  * This is used to retrieve the listener for a certain status change.
  * If this publisher has a registered listener and the status kind
  * is in the listener mask then the listener is returned.
  * Otherwise, the query for listener is propagated up to the
  * factory/DomainParticipant.
  */
  DDS::PublisherListener* listener_for(::DDS::StatusKind kind);

  DDS::ReturnCode_t assert_liveliness_by_participant();

  typedef std::vector<PublicationId> PublicationIdVec;
  /// Populates a std::vector with the PublicationIds (GUIDs)
  /// of this Publisher's Data Writers
  void get_publication_ids(PublicationIdVec& pubs);

private:
  DDS::InstanceHandle_t        handle_;

  /// Publisher QoS policy list.
  DDS::PublisherQos            qos_;
  /// Default datawriter Qos policy list.
  DDS::DataWriterQos           default_datawriter_qos_;

  /// The StatusKind bit mask indicates which status condition change
  /// can be notified by the listener of this entity.
  DDS::StatusMask              listener_mask_;
  /// Used to notify the entity for relevant events.
  DDS::PublisherListener_var   listener_;
  /// The publisher listener servant.
  DDS::PublisherListener*      fast_listener_;
  /// This map is used to support datawriter lookup by topic name.
  DataWriterMap                datawriter_map_;
  /// This map is used to support datawriter lookup by datawriter
  /// repository id.
  PublicationMap               publication_map_;
  /// The number of times begin_coherent_changes as been called.
  std::size_t                  change_depth_;
  /// Domain in which we are contained.
  DDS::DomainId_t              domain_id_;
  /// The DomainParticipant servant that owns this Publisher.
  DomainParticipantImpl*       participant_;
  /// The suspend depth count.
  CORBA::Short                 suspend_depth_count_;
  /// Unique sequence number used when the scope_access = GROUP.
  /// -  NOT USED IN FIRST IMPL - not supporting GROUP scope
  SequenceNumber               sequence_number_;
  /// Start of current aggregation period. - NOT USED IN FIRST IMPL
  ACE_Time_Value               aggregation_period_start_;

  typedef ACE_Recursive_Thread_Mutex  lock_type;
  typedef ACE_Reverse_Lock<lock_type> reverse_lock_type;
  /// The recursive lock to protect datawriter map and suspend count.
  /// It also projects the TransportInterface (it must be held when
  /// calling any TransportInterface method).
  mutable lock_type                   pi_lock_;
  reverse_lock_type                   reverse_pi_lock_;

  /// The catched available data while suspending.
  DataSampleList                available_data_list_;

  /// Monitor object for this entity
  Monitor* monitor_;
};

} // namespace  DDS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_PUBLISHER_IMPL_H  */
