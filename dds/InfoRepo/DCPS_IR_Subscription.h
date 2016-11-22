/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_IR_SUBSCRIPTION_H
#define DCPS_IR_SUBSCRIPTION_H

#include  "inforepo_export.h"
#include /**/ "UpdateDataTypes.h"
#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DdsDcpsSubscriptionC.h"
#include /**/ "dds/DCPS/InfoRepoDiscovery/InfoC.h"
#include /**/ "dds/DCPS/InfoRepoDiscovery/DataReaderRemoteC.h"
#include /**/ "ace/Unbounded_Set.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

// forward declarations
class DCPS_IR_Publication;
typedef ACE_Unbounded_Set<DCPS_IR_Publication*> DCPS_IR_Publication_Set;

class DCPS_IR_Participant;
class DCPS_IR_Topic_Description;
class DCPS_IR_Topic;

/**
 * @class DCPS_IR_Subscription
 *
 * @brief Representative of a Subscription
 *
 *
 */
class OpenDDS_InfoRepoLib_Export DCPS_IR_Subscription {
public:
  DCPS_IR_Subscription(const OpenDDS::DCPS::RepoId& id,
                       DCPS_IR_Participant* participant,
                       DCPS_IR_Topic* topic,
                       OpenDDS::DCPS::DataReaderRemote_ptr reader,
                       const DDS::DataReaderQos& qos,
                       const OpenDDS::DCPS::TransportLocatorSeq& info,
                       const DDS::SubscriberQos& subscriberQos,
                       const char* filterClassName,
                       const char* filterExpression,
                       const DDS::StringSeq& exprParams);

  ~DCPS_IR_Subscription();

  /// Associate with the publication
  /// Adds the publication to the list of associated
  ///  publications and notifies datareader if successfully added
  /// This method can mark the participant dead
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_associated_publication(DCPS_IR_Publication* pub, bool active);

  /// The service participant that contains this Subscription has indicated
  /// that the assocation to peer "remote" is complete.  This method will
  /// locate the Publication object for "remote" in order to inform it
  /// of the completed association.
  void association_complete(const OpenDDS::DCPS::RepoId& remote);

  /// Invoke the DataWriterRemote::association_complete() callback, passing
  /// the "remote" parameter (Subscription) to the service participant.
  void call_association_complete(const OpenDDS::DCPS::RepoId& remote);

  /// Remove the associated publication
  /// Removes the publication from the list of associated
  ///  publications if return successful
  /// sendNotify indicates whether to tell the datareader about
  ///  removing the publication
  /// The notify_lost flag true indicates this remove_associations is called
  /// when the InfoRepo detects this subscription is lost because of the failure
  /// of invocation on this subscription.
  /// The notify_both_side parameter indicates if it needs call pub to remove
  /// association as well.
  /// This method can mark the participant dead
  /// Returns 0 if successful
  int remove_associated_publication(DCPS_IR_Publication* pub,
                                    CORBA::Boolean sendNotify,
                                    CORBA::Boolean notify_lost,
                                    bool notify_both_side = false);

  /// Removes all the associated publications
  /// This method can mark the participant dead
  /// The notify_lost flag true indicates this remove_associations is called
  /// when the InfoRepo detects this subscription is lost because of the failure
  /// of invocation on this subscription.
  /// Returns 0 if successful
  int remove_associations(CORBA::Boolean notify_lost);

  /// Remove any publications whose participant has the id
  void disassociate_participant(OpenDDS::DCPS::RepoId id,
                                bool reassociate = false);

  /// Remove any publications whose topic has the id
  void disassociate_topic(OpenDDS::DCPS::RepoId id);

  /// Remove any publications with id
  void disassociate_publication(OpenDDS::DCPS::RepoId id,
                                bool reassociate = false);

  /// Notify the reader of incompatible qos status
  ///  and reset the status' count_since_last_send to 0
  void update_incompatible_qos();

  /// Check that none of the ids given are ones that
  ///  this subscription should ignore.
  /// returns 1 if one of these ids is an ignored id
  CORBA::Boolean is_publication_ignored(OpenDDS::DCPS::RepoId partId,
                                        OpenDDS::DCPS::RepoId topicId,
                                        OpenDDS::DCPS::RepoId pubId);

  /// Return pointer to the DataReader qos
  /// Subscription retains ownership
  const DDS::DataReaderQos* get_datareader_qos();

  /// Return pointer to the Subscriber qos
  /// Subscription retains ownership
  const DDS::SubscriberQos* get_subscriber_qos();

  /// Update the DataReader or Subscriber qos and also publish the qos
  /// changes to datereader BIT.
  bool set_qos(const DDS::DataReaderQos & qos,
               const DDS::SubscriberQos & subscriberQos,
               Update::SpecificQos& specificQos);

  /// Update DataReaderQos only.
  void set_qos(const DDS::DataReaderQos& qos);

  /// Update SubscriberQos only.
  void set_qos(const DDS::SubscriberQos& qos);

  void reevaluate_defunct_associations();

  // Verify the existing associations. This may result removal of
  // associations. The existing associations have to be removed before
  // adding new association and may need some delay. Otherwise, if
  // two DataWriters uses same Datalink and add an association happens
  // before remove an association then the new association will fail to
  // connect.
  void reevaluate_existing_associations();

  // Re-evaluate the association between this subscription and the provided
  // publication. If they are already associated and not compatible then
  // they will be dis-associated. If they are not already associated then
  // the new association will be added.
  bool reevaluate_association(DCPS_IR_Publication* publication);

  OpenDDS::DCPS::TransportLocatorSeq get_transportLocatorSeq() const;

  /// Return pointer to the incompatible qos status
  /// Subscription retains ownership
  OpenDDS::DCPS::IncompatibleQosStatus* get_incompatibleQosStatus();

  OpenDDS::DCPS::RepoId get_id();
  OpenDDS::DCPS::RepoId get_topic_id();
  OpenDDS::DCPS::RepoId get_participant_id();

  DCPS_IR_Topic_Description* get_topic_description();

  DCPS_IR_Topic* get_topic();

  DDS::InstanceHandle_t get_handle();
  void set_handle(DDS::InstanceHandle_t handle);

  CORBA::Boolean is_bit();
  void set_bit_status(CORBA::Boolean isBIT);

  // Expose the datareader.
  OpenDDS::DCPS::DataReaderRemote_ptr reader();

  std::string get_filter_class_name() const;
  std::string get_filter_expression() const;
  DDS::StringSeq get_expr_params() const;

  /// Calls associated Publications
  void update_expr_params(const DDS::StringSeq& params);

  std::string dump_to_string(const std::string& prefix, int depth) const;

private:
  OpenDDS::DCPS::RepoId id_;
  DCPS_IR_Participant* participant_;
  DCPS_IR_Topic* topic_;
  DDS::InstanceHandle_t handle_;
  CORBA::Boolean isBIT_;

  /// the corresponding DataReaderRemote object
  OpenDDS::DCPS::DataReaderRemote_var reader_;
  DDS::DataReaderQos qos_;
  OpenDDS::DCPS::TransportLocatorSeq info_;
  DDS::SubscriberQos subscriberQos_;
  std::string filterClassName_;
  std::string filterExpression_;
  DDS::StringSeq exprParams_;

  DCPS_IR_Publication_Set associations_;
  DCPS_IR_Publication_Set defunct_;

  OpenDDS::DCPS::IncompatibleQosStatus incompatibleQosStatus_;
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_IR_SUBSCRIPTION_H */
