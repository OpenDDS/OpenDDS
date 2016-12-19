/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_IR_PARTICIPANT_H
#define DCPS_IR_PARTICIPANT_H

#include  "inforepo_export.h"
#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DCPS/InfoRepoDiscovery/InfoS.h"

#include "dds/DCPS/RepoIdGenerator.h"
#include "dds/DCPS/GuidUtils.h"

#include /**/ "ace/Map_Manager.h"

#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Topic.h"

#include <map>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

// forward declarations
class DCPS_IR_Domain;
class TAO_DDS_DCPSFederationId;

namespace Update {

class Manager;

} // namespace Update

typedef std::map<OpenDDS::DCPS::RepoId, DCPS_IR_Subscription*,
  OpenDDS::DCPS::GUID_tKeyLessThan> DCPS_IR_Subscription_Map;
typedef std::map<OpenDDS::DCPS::RepoId, DCPS_IR_Publication*,
  OpenDDS::DCPS::GUID_tKeyLessThan> DCPS_IR_Publication_Map;
typedef std::map<OpenDDS::DCPS::RepoId, DCPS_IR_Topic*,
  OpenDDS::DCPS::GUID_tKeyLessThan> DCPS_IR_Topic_Map;

typedef ACE_Unbounded_Set<OpenDDS::DCPS::RepoId> TAO_DDS_RepoId_Set;

/**
 * @class DCPS_IR_Participant
 *
 * @brief Representative of the Domain Participant
 *
 *
 */
class OpenDDS_InfoRepoLib_Export DCPS_IR_Participant {
public:
  /// Special owner to enforce no callbacks.
  enum { OWNER_NONE};

  DCPS_IR_Participant(const TAO_DDS_DCPSFederationId& federationId,
                      OpenDDS::DCPS::RepoId id,
                      DCPS_IR_Domain* domain,
                      DDS::DomainParticipantQos qos,
                      Update::Manager* um);

  virtual ~DCPS_IR_Participant();

  /// Take local ownership of this participant and publish an update.
  void takeOwnership();

  /// Process an incoming update that changes ownership.
  void changeOwner(long sender, long owner);

  /// Value of the owner for this participant.
  long owner() const;

  /// Indication of whether the current repository is the owner of this participant.
  bool isOwner() const;

  /// Flag to discriminate the built-in topic publishing participant within a domain.
  bool& isBitPublisher();
  bool  isBitPublisher() const;

  /// Add a publication
  /// This takes ownership of the memory pointed to by pub
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_publication(DCPS_IR_Publication* pub);

  /// Return the publication object.
  int find_publication_reference(OpenDDS::DCPS::RepoId pubId,
                                 DCPS_IR_Publication*& pub);

  /// Removes the publication with the id
  /// Deletes the publication object if returns successful
  /// Returns 0 if successful
  int remove_publication(OpenDDS::DCPS::RepoId pubId);

  /// Add a subscription
  /// This takes ownership of the memory pointed to by aub
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_subscription(DCPS_IR_Subscription* sub);

  /// Return the subscription object.
  int find_subscription_reference(OpenDDS::DCPS::RepoId subId,
                                  DCPS_IR_Subscription*& sub);

  /// Removes the subscription with the id
  /// Deletes the subscription object if returns successful
  /// Returns 0 if successful
  int remove_subscription(OpenDDS::DCPS::RepoId subId);

  /// Add a topic
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_topic_reference(DCPS_IR_Topic* topic);

  /// Remove a topic reference
  /// Does not change or take ownership of topic
  /// Returns 0 if successful
  int remove_topic_reference(OpenDDS::DCPS::RepoId topicId,
                             DCPS_IR_Topic*& topic);

  /// Find topic reference with id
  /// Does NOT give ownership of memory
  /// Returns 0 if successful
  int find_topic_reference(OpenDDS::DCPS::RepoId topicId,
                           DCPS_IR_Topic*& topic);

  /// Removes all topics, publications and
  // subscriptions for this participant
  void remove_all_dependents(CORBA::Boolean notify_lost);

  // called by publications and subscriptions when the writer
  // or reader throws an exception during a remote invocation
  //
  /// Changes aliveStatus to false then adds itself to the
  ///  domain's list of dead participants for removal
  void mark_dead();

  OpenDDS::DCPS::RepoId get_id();

  CORBA::Boolean is_alive();
  void set_alive(CORBA::Boolean alive);

  /// Ignore the participant with the id
  void ignore_participant(OpenDDS::DCPS::RepoId id);
  /// Ignore the topic with the id
  void ignore_topic(OpenDDS::DCPS::RepoId id);
  /// Ignore the publication with the id
  void ignore_publication(OpenDDS::DCPS::RepoId id);
  /// Ignore the subscription with the id
  void ignore_subscription(OpenDDS::DCPS::RepoId id);

  /// Return pointer to the participant qos
  /// Participant retains ownership
  const DDS::DomainParticipantQos* get_qos();

  /// Update qos and also propogate the qos change to related BITs
  /// that has the qos copy.
  /// Return false if the provided QoS makes the DataWriter and DataReader
  /// QoS incompatible. Currently supported changeable QoS in DomainParticipantQos
  /// do not affect.
  bool set_qos(const DDS::DomainParticipantQos & qos);

  CORBA::Boolean is_participant_ignored(OpenDDS::DCPS::RepoId id);
  CORBA::Boolean is_topic_ignored(OpenDDS::DCPS::RepoId id);
  CORBA::Boolean is_publication_ignored(OpenDDS::DCPS::RepoId id);
  CORBA::Boolean is_subscription_ignored(OpenDDS::DCPS::RepoId id);

  DDS::InstanceHandle_t get_handle();
  void set_handle(DDS::InstanceHandle_t handle);

  CORBA::Boolean is_bit();
  void set_bit_status(CORBA::Boolean isBIT);

  DCPS_IR_Domain* get_domain_reference() const;

  // Next Entity Id value in sequence.
  OpenDDS::DCPS::RepoId get_next_topic_id();
  OpenDDS::DCPS::RepoId get_next_publication_id();
  OpenDDS::DCPS::RepoId get_next_subscription_id();

  // Ensure no conflicts with sequence values from persistent storage.
  void last_topic_key(long key);
  void last_publication_key(long key);
  void last_subscription_key(long key);

  /// Expose a readable reference to the publication map.
  const DCPS_IR_Publication_Map& publications() const;

  /// Expose a readable reference to the subscription map.
  const DCPS_IR_Subscription_Map& subscriptions() const;

  /// Expose a readable reference to the topic map.
  const DCPS_IR_Topic_Map& topics() const;

  std::string dump_to_string(const std::string& prefix, int depth) const;

private:
  OpenDDS::DCPS::RepoId id_;
  DCPS_IR_Domain* domain_;
  DDS::DomainParticipantQos qos_;
  CORBA::Boolean aliveStatus_;
  DDS::InstanceHandle_t handle_;
  CORBA::Boolean isBIT_;

  const TAO_DDS_DCPSFederationId& federationId_;
  long owner_;

  /// Lock portions ownership processing.
  ACE_SYNCH_MUTEX ownerLock_;

  // Entity GUID Id generators.
  OpenDDS::DCPS::RepoIdGenerator topicIdGenerator_;
  OpenDDS::DCPS::RepoIdGenerator publicationIdGenerator_;
  OpenDDS::DCPS::RepoIdGenerator subscriptionIdGenerator_;

  DCPS_IR_Subscription_Map subscriptions_;
  DCPS_IR_Publication_Map publications_;
  DCPS_IR_Topic_Map topicRefs_;

  // list of ignored entity ids
  TAO_DDS_RepoId_Set ignoredParticipants_;
  TAO_DDS_RepoId_Set ignoredTopics_;
  TAO_DDS_RepoId_Set ignoredPublications_;
  TAO_DDS_RepoId_Set ignoredSubscriptions_;

  // The participant is the only entity that has and deals with
  //  dependencies (topics, actors). In handling dependencies it
  //  encompasses a bigger role. Therefore it needs to update
  //  other entities (specifically the Update::Manager) the
  //  changes it makes.
  Update::Manager* um_;

  /// Flag indicating this participant publishes built-in topics.
  bool isBitPublisher_;
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_IR_PARTICIPANT_H */
