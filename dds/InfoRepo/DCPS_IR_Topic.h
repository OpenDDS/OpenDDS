/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_IR_TOPIC_H
#define DCPS_IR_TOPIC_H

#include  "inforepo_export.h"
#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DdsDcpsTopicC.h"
#include /**/ "dds/DCPS/InfoRepoDiscovery/InfoC.h"
#include /**/ "ace/Unbounded_Set.h"

#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

// forward declarations
class DCPS_IR_Publication;
typedef ACE_Unbounded_Set<DCPS_IR_Publication*> DCPS_IR_Publication_Set;

class DCPS_IR_Subscription;
typedef ACE_Unbounded_Set<DCPS_IR_Subscription*> DCPS_IR_Subscription_Set;

class DCPS_IR_Domain;
class DCPS_IR_Participant;
class DCPS_IR_Topic_Description;
class DCPS_IR_Subscription;

/**
 * @class DCPS_IR_Topic
 *
 * @brief Representative of a Topic
 *
 */
class OpenDDS_InfoRepoLib_Export DCPS_IR_Topic {
public:
  DCPS_IR_Topic(const OpenDDS::DCPS::RepoId& id,
                const DDS::TopicQos& qos,
                DCPS_IR_Domain* domain,
                DCPS_IR_Participant* creator,
                DCPS_IR_Topic_Description* description);

  ~DCPS_IR_Topic();

  /// Delete the topic object upon last topic associated sub/pub and topic
  /// object deletion. It's kind of reference counting.
  /// The removing true indicates it's called upon delete_topic, otherwise
  /// it's upon remove_publication/remove_subcription.
  void release(bool removing);

  /// Adds the publication to the list of publications
  /// Calls the topic description's try associate if successfully added
  /// 'associate' switch toggles association attempt.
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_publication_reference(DCPS_IR_Publication* publication
                                , bool associate = true);

  /// Removes the publication from the list of publications
  /// Returns 0 if successful
  int remove_publication_reference(DCPS_IR_Publication* publication);

  /// Adds the subscription to the list of subscriptions
  /// and let description handle the association.
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_subscription_reference(DCPS_IR_Subscription* subscription
                                 , bool associate = true);

  /// Removes the subscription from the list of subscriptions
  /// Returns 0 if successful
  int remove_subscription_reference(DCPS_IR_Subscription* subscription);

  /// Called by the DCPS_IR_Topic_Description
  /// Find any compatible publications and associate
  ///  them using the DCPS_IR_Topic_Description's
  ///  associate method.
  /// This method does not check the subscription's incompatible
  ///  qos status.
  void try_associate(DCPS_IR_Subscription* subscription);

  /// Called by the DCPS_IR_Topic_Description to re-evaluate the
  /// association between the publications of this topic and the
  /// provided subscription.
  void reevaluate_associations(DCPS_IR_Subscription* subscription);

  OpenDDS::DCPS::RepoId get_id() const;
  OpenDDS::DCPS::RepoId get_participant_id() const;

  /// Return pointer to the Topic Description
  /// Domain retains ownership
  DCPS_IR_Topic_Description* get_topic_description();

  /// Return pointer to the Topic qos
  /// Topic retains ownership
  DDS::TopicQos * get_topic_qos();

  /// Reset topic qos and also propogate the qos change to related BITs
  /// that has the qos copy.
  /// Return false if the provided QoS makes the DataWriter and DataReader
  /// QoS incompatible. Currently supported changeable QoS in TopicQos do
  /// not affect.
  bool set_topic_qos(const DDS::TopicQos& qos);

  DDS::InstanceHandle_t get_handle();
  void set_handle(DDS::InstanceHandle_t handle);

  CORBA::Boolean is_bit();
  void set_bit_status(CORBA::Boolean isBIT);

  /// Try to associate all the current publications.
  /// This is used to reconnect Built in Topics after a persistent restart.
  void reassociate_all_publications();

  std::string dump_to_string(const std::string& prefix, int depth) const;

private:
  OpenDDS::DCPS::RepoId id_;
  DDS::TopicQos qos_;
  DCPS_IR_Domain* domain_;
  DCPS_IR_Participant* participant_;
  DCPS_IR_Topic_Description* description_;
  DDS::InstanceHandle_t handle_;
  CORBA::Boolean isBIT_;

  DCPS_IR_Publication_Set publicationRefs_;
  /// Keep track the subscriptions of this topic so the TopicQos
  /// change can be published for those subscriptions.
  DCPS_IR_Subscription_Set subscriptionRefs_;

  /// True means release() is called upon delete_topic, but topic object is
  /// not deleted because there are still pub/sub associated.
  bool removed_;
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_IR_TOPIC_H */
