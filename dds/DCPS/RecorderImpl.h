/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RECORDERIMPL_H
#define OPENDDS_DCPS_RECORDERIMPL_H

#include "RcObject.h"
#include "WriterInfo.h"
#include "Definitions.h"
#include "DataReaderCallbacks.h"
#include "Recorder.h"
#include "EntityImpl.h"
#include "TopicImpl.h"
#include "OwnershipManager.h"
#include "transport/framework/ReceivedDataSample.h"
#include "transport/framework/TransportReceiveListener.h"
#include "transport/framework/TransportClient.h"

#include <dds/DdsDcpsTopicC.h>
#include <dds/DdsDcpsSubscriptionExtC.h>
#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsTopicC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class RecorderImpl
 *
 * @brief Implementation of Recorder functionality
 *
 * This class is the implementation of the Recorder.
 * Inheritance is used to limit the applications access to
 * underlying system methods.
 */

class OpenDDS_Dcps_Export RecorderImpl
  : public TransportClient
  , public TransportReceiveListener
  , public DataReaderCallbacks
  , public Recorder
  , public EntityImpl
  , private WriterInfoListener
{
public:
  RecorderImpl();

  virtual ~RecorderImpl();

  /**
   * cleanup the DataWriter.
   */
  DDS::ReturnCode_t cleanup();

  void init(
    TopicDescriptionImpl*      a_topic_desc,
    const DDS::DataReaderQos & qos,
    RecorderListener_rch       a_listener,
    const DDS::StatusMask &    mask,
    DomainParticipantImpl*     participant,
    DDS::SubscriberQos         subqos);

  DDS::ReturnCode_t enable();

  // Implement TransportClient
  virtual bool check_transport_qos(const TransportInst& inst);
  virtual RepoId get_repo_id() const;
  DDS::DomainId_t domain_id() const { return this->domain_id_; }
  virtual CORBA::Long get_priority_value(const AssociationData& data) const;

  //Implement TransportReceiveListener
  virtual void data_received(const ReceivedDataSample& sample);
  virtual void notify_subscription_disconnected(const WriterIdSeq& pubids);
  virtual void notify_subscription_reconnected(const WriterIdSeq& pubids);
  virtual void notify_subscription_lost(const WriterIdSeq& pubids);

  // Implement DataReaderCallbacks

  virtual void add_association(const RepoId&            yourId,
                               const WriterAssociation& writer,
                               bool                     active);

  virtual void remove_associations(const WriterIdSeq& writers,
                                   CORBA::Boolean     callback);

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status);

  virtual void signal_liveliness(const RepoId& remote_participant);

  void remove_all_associations();

#ifndef OPENDDS_SAFETY_PROFILE
  void add_to_dynamic_type_map(const PublicationId& pub_id, const XTypes::TypeIdentifier& ti);
#endif

#if !defined (DDS_HAS_MINIMUM_BIT)
  // implement Recoder
  virtual DDS::ReturnCode_t repoid_to_bit_key(const DCPS::RepoId&     id,
                                              DDS::BuiltinTopicKey_t& key);
#endif
  /**
   * Set the Quality of Service settings for the Recorder.
   *
   */
  DDS::ReturnCode_t set_qos (const DDS::SubscriberQos & subscriber_qos,
                             const DDS::DataReaderQos & datareader_qos);

  /**
   * Get the Quality of Service settings for the Recorder.
   *
   */
  DDS::ReturnCode_t get_qos (DDS::SubscriberQos & subscriber_qos,
                             DDS::DataReaderQos & datareader_qos);


  DDS::ReturnCode_t set_listener(const RecorderListener_rch& a_listener,
                                 DDS::StatusMask             mask);

  RecorderListener_rch get_listener();

  DomainParticipantImpl*          participant() {
    return participant_servant_;
  }

  virtual DDS::InstanceHandle_t get_instance_handle();

  virtual void register_for_writer(const RepoId& /*participant*/,
                                   const RepoId& /*readerid*/,
                                   const RepoId& /*writerid*/,
                                   const TransportLocatorSeq& /*locators*/,
                                   DiscoveryListener* /*listener*/);

  virtual void unregister_for_writer(const RepoId& /*participant*/,
                                     const RepoId& /*readerid*/,
                                     const RepoId& /*writerid*/);

  virtual WeakRcHandle<ICE::Endpoint> get_ice_endpoint() { return WeakRcHandle<ICE::Endpoint>(); }

protected:
  virtual void remove_associations_i(const WriterIdSeq& writers, bool callback);

private:

  void notify_subscription_lost(const DDS::InstanceHandleSeq& handles);

  /// Lookup the instance handles by the publication repo ids
  void lookup_instance_handles(const WriterIdSeq&      ids,
                               DDS::InstanceHandleSeq& hdls);

#ifndef OPENDDS_SAFETY_PROFILE
  DDS::DynamicData_ptr get_dynamic_data(const RawDataSample& sample);
#endif
  void check_encap(bool b) { check_encap_ = b; }
  bool check_encap() const { return check_encap_; }

  DDS::DataReaderQos qos_;
  DDS::DataReaderQos passed_qos_;

  /// lock protecting sample container as well as statuses.
  ACE_Recursive_Thread_Mutex sample_lock_;

  DomainParticipantImpl* participant_servant_;
  TopicDescriptionPtr<TopicImpl> topic_servant_;

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  bool is_exclusive_ownership_;

  OwnershipManager* owner_manager_;
#endif

  DDS::SubscriberQos subqos_;

  friend class ::DDS_TEST; //allows tests to get at private data

  DDS::TopicDescription_var topic_desc_;
  DDS::StatusMask listener_mask_;
  RecorderListener_rch listener_;
  DDS::DomainId_t domain_id_;

  ACE_Recursive_Thread_Mutex publication_handle_lock_;

  typedef OPENDDS_MAP_CMP(RepoId, DDS::InstanceHandle_t, GUID_tKeyLessThan) RepoIdToHandleMap;
  RepoIdToHandleMap id_to_handle_map_;

  DDS::RequestedIncompatibleQosStatus requested_incompatible_qos_status_;
  DDS::SubscriptionMatchedStatus subscription_match_status_;

  /// Flag indicates that this datareader is a builtin topic
  /// datareader.
  bool is_bit_;

  /// publications writing to this reader.
  typedef OPENDDS_MAP_CMP(PublicationId, RcHandle<WriterInfo>,
                   GUID_tKeyLessThan) WriterMapType;
  WriterMapType writers_;

  /// RW lock for reading/writing publications.
  ACE_RW_Thread_Mutex writers_lock_;

#ifndef OPENDDS_SAFETY_PROFILE
  typedef OPENDDS_MAP(PublicationId, DDS::DynamicType_var) DynamicTypeByPubId;
  DynamicTypeByPubId dt_map_;
#endif
  bool check_encap_;
};


} // namespace DCPS
} // namespace

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* end of include guard: OPENDDS_DCPS_RECORDERIMPL_H */
