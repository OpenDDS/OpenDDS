/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_IR_PUBLICATION_H
#define DCPS_IR_PUBLICATION_H

#include  "inforepo_export.h"
#include /**/ "UpdateDataTypes.h"
#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DdsDcpsPublicationC.h"
#include /**/ "dds/DCPS/InfoRepoDiscovery/InfoC.h"
#include /**/ "dds/DCPS/InfoRepoDiscovery/DataWriterRemoteC.h"
#include /**/ "ace/Unbounded_Set.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

// forward declarations
class DCPS_IR_Participant;
class DCPS_IR_Topic;
class DCPS_IR_Topic_Description;

class DCPS_IR_Subscription;
typedef ACE_Unbounded_Set<DCPS_IR_Subscription*> DCPS_IR_Subscription_Set;

/**
 * @class DCPS_IR_Publication
 *
 * @brief Representative of a Publication
 *
 *
 */
class OpenDDS_InfoRepoLib_Export DCPS_IR_Publication {
public:
  DCPS_IR_Publication(const OpenDDS::DCPS::RepoId& id,
                      DCPS_IR_Participant* participant,
                      DCPS_IR_Topic* topic,
                      OpenDDS::DCPS::DataWriterRemote_ptr writer,
                      const DDS::DataWriterQos& qos,
                      const OpenDDS::DCPS::TransportLocatorSeq& info,
                      const DDS::PublisherQos& publisherQos);

  ~DCPS_IR_Publication();

  /// Associate with the subscription
  /// Adds the subscription to the list of associated
  ///  subscriptions and notifies datawriter if successfully added
  /// This method can mark the participant dead
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_associated_subscription(DCPS_IR_Subscription* sub, bool active);

  /// The service participant that contains this Publication has indicated
  /// that the assocation to peer "remote" is complete.  This method will
  /// locate the Subscription object for "remote" in order to inform it
  /// of the completed association.
  void association_complete(const OpenDDS::DCPS::RepoId& remote);

  /// Invoke the DataWriterRemote::association_complete() callback, passing
  /// the "remote" parameter (Subscription) to the service participant.
  void call_association_complete(const OpenDDS::DCPS::RepoId& remote);

  /// Remove the associated subscription
  /// Removes the subscription from the list of associated
  ///  subscriptions if return successful
  /// sendNotify indicates whether to tell the datawriter about
  ///  removing the subscription
  /// The notify_lost parameter is passed to the remove_associations()
  /// The notify_both_side parameter indicates if it needs call sub to remove
  /// association as well.
  /// See the comments of remove_associations() in DataWriterRemote.idl
  /// or DataReaderRemote.idl.
  /// This method can mark the participant dead
  /// Returns 0 if successful
  int remove_associated_subscription(DCPS_IR_Subscription* sub,
                                     CORBA::Boolean sendNotify,
                                     CORBA::Boolean notify_lost,
                                     bool notify_both_side = false);

  /// Removes all the associated subscriptions
  /// This method can mark the participant dead
  /// The notify_lost flag true indicates this remove_associations is called
  /// when the InfoRepo detects this publication is lost because of the failure
  /// of invocation on this publication.
  /// Returns 0 if successful
  int remove_associations(CORBA::Boolean notify_lost);

  /// Remove any subscriptions whose participant has the id
  void disassociate_participant(OpenDDS::DCPS::RepoId id,
                                bool reassociate = false);

  /// Remove any subscriptions whose topic has the id
  void disassociate_topic(OpenDDS::DCPS::RepoId id);

  /// Remove any subscriptions with the id
  void disassociate_subscription(OpenDDS::DCPS::RepoId id,
                                 bool reassociate = false);

  /// Notify the writer of incompatible qos status
  ///  and reset the status' count_since_last_send to 0
  void update_incompatible_qos();

  /// Check that none of the ids given are ones that
  ///  this publication should ignore.
  /// returns 1 if one of these ids is an ignored id
  CORBA::Boolean is_subscription_ignored(OpenDDS::DCPS::RepoId partId,
                                         OpenDDS::DCPS::RepoId topicId,
                                         OpenDDS::DCPS::RepoId subId);

  /// Return pointer to the DataWriter qos
  /// Publication retains ownership
  DDS::DataWriterQos* get_datawriter_qos();

  /// Return pointer to the Publisher qos
  /// Publication retains ownership
  DDS::PublisherQos* get_publisher_qos();

  /// Update the DataWriter or Publisher qos and also publish the qos changes
  /// to datawriter BIT.
  bool set_qos(const DDS::DataWriterQos & qos,
               const DDS::PublisherQos & publisherQos,
               Update::SpecificQos& specificQos);

  /// Update DataWriterQos only.
  void set_qos(const DDS::DataWriterQos& qos);

  /// Update PublisherQos only.
  void set_qos(const DDS::PublisherQos& qos);

  OpenDDS::DCPS::TransportLocatorSeq get_transportLocatorSeq() const;

  /// Return pointer to the incompatible qos status
  /// Publication retains ownership
  OpenDDS::DCPS::IncompatibleQosStatus* get_incompatibleQosStatus();

  OpenDDS::DCPS::RepoId get_id();
  OpenDDS::DCPS::RepoId get_topic_id();
  OpenDDS::DCPS::RepoId get_participant_id();

  DCPS_IR_Topic* get_topic();
  DCPS_IR_Topic_Description* get_topic_description();

  DDS::InstanceHandle_t get_handle();
  void set_handle(DDS::InstanceHandle_t handle);

  CORBA::Boolean is_bit();
  void set_bit_status(CORBA::Boolean isBIT);

  // Expose the datawriter.
  OpenDDS::DCPS::DataWriterRemote_ptr writer();

  void reevaluate_defunct_associations();

  // Verify the existing associations. This may result removal of
  // associations. The existing associations have to be removed before
  // adding new association and may need some delay. Otherwise, if
  // two DataReaders uses same Datalink and add an association happens
  // before remove an association then the new association will fail to
  // connect.
  void reevaluate_existing_associations();

  // Re-evaluate the association between this publication and the provided
  // subscription. If they are already associated and not compatible then
  // they will be dis-associated. If they are not already associated then
  // the new association will be added.
  bool reevaluate_association(DCPS_IR_Subscription* subscription);

  void update_expr_params(OpenDDS::DCPS::RepoId readerId,
                          const DDS::StringSeq& params);

  std::string dump_to_string(const std::string& prefix, int depth) const;

private:

  OpenDDS::DCPS::RepoId id_;
  DCPS_IR_Participant* participant_;
  DCPS_IR_Topic* topic_;
  DDS::InstanceHandle_t handle_;
  CORBA::Boolean isBIT_;

  /// the corresponding DataWriterRemote object
  OpenDDS::DCPS::DataWriterRemote_var writer_;
  DDS::DataWriterQos qos_;
  OpenDDS::DCPS::TransportLocatorSeq info_;
  DDS::PublisherQos publisherQos_;

  DCPS_IR_Subscription_Set associations_;
  DCPS_IR_Subscription_Set defunct_;

  OpenDDS::DCPS::IncompatibleQosStatus incompatibleQosStatus_;
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_IR_PUBLICATION_H */
