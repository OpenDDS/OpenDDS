// ============================================================================
/**
 *  @file   DCPS_IR_Participant.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef DCPS_IR_PARTICIPANT_H
#define DCPS_IR_PARTICIPANT_H

#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DdsDcpsInfoS.h"
#include /**/ "DCPS_Entity_Id_Generator.h"
#include /**/ "ace/Map_Manager.h"
#include /**/ "ace/Null_Mutex.h"

#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Topic.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// forward declarations
class DCPS_IR_Domain;
class UpdateManager;

typedef ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex> DCPS_IR_Subscription_Map;
typedef ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex> DCPS_IR_Publication_Map;
typedef ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Topic*,ACE_Null_Mutex> DCPS_IR_Topic_Map;

typedef ACE_Unbounded_Set<TAO::DCPS::RepoId> TAO_DDS_RepoId_Set;


/**
 * @class DCPS_IR_Participant
 *
 * @brief Representative of the Domain Participant
 *
 *
 */
class DCPS_IR_Participant
{
public:
  DCPS_IR_Participant (TAO::DCPS::RepoId id,
                       DCPS_IR_Domain* domain,
                       ::DDS::DomainParticipantQos qos,
		       UpdateManager* um);

  virtual ~DCPS_IR_Participant();

  /// Add a publication
  /// This takes ownership of the memory pointed to by pub
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_publication (DCPS_IR_Publication* pub);

  /// Removes the publication with the id
  /// Deletes the publication object if returns successful
  /// Returns 0 if successful
  int remove_publication (long pubId);

  /// Add a subscription
  /// This takes ownership of the memory pointed to by aub
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_subscription (DCPS_IR_Subscription* sub);

  /// Removes the subscription with the id
  /// Deletes the subscription object if returns successful
  /// Returns 0 if successful
  int remove_subscription (long subId);

  /// Add a topic
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_topic_reference (DCPS_IR_Topic* topic);

  /// Remove a topic reference
  /// Does not change or take ownership of topic
  /// Returns 0 if successful
  int remove_topic_reference (long topicId,
                              DCPS_IR_Topic*& topic);

  /// Find topic reference with id
  /// Does NOT give ownership of memory
  /// Returns 0 if successful
  int find_topic_reference (long topicId,
                            DCPS_IR_Topic*& topic);

  /// Removes all topics, publications and
  // subscriptions for this participant
  void remove_all_dependents (CORBA::Boolean notify_lost);

  // called by publications and subscriptions when the writer
  // or reader throws an exception during a remote invocation
  //
  /// Changes aliveStatus to false then adds itself to the
  ///  domain's list of dead participants for removal
  void mark_dead ();

  TAO::DCPS::RepoId get_id ();

  CORBA::Boolean is_alive ();
  void set_alive (CORBA::Boolean alive);

  /// Ignore the participant with the id
  void ignore_participant (TAO::DCPS::RepoId id);
  /// Ignore the topic with the id
  void ignore_topic (TAO::DCPS::RepoId id);
  /// Ignore the publication with the id
  void ignore_publication (TAO::DCPS::RepoId id);
  /// Ignore the subscription with the id
  void ignore_subscription (TAO::DCPS::RepoId id);


  /// Return pointer to the participant qos
  /// Participant retains ownership
  const ::DDS::DomainParticipantQos* get_qos ();

  CORBA::Boolean is_participant_ignored (TAO::DCPS::RepoId id);
  CORBA::Boolean is_topic_ignored (TAO::DCPS::RepoId id);
  CORBA::Boolean is_publication_ignored (TAO::DCPS::RepoId id);
  CORBA::Boolean is_subscription_ignored (TAO::DCPS::RepoId id);

  ::DDS::InstanceHandle_t get_handle();
  void set_handle(::DDS::InstanceHandle_t handle);

  CORBA::Boolean is_bit ();
  void set_bit_status (CORBA::Boolean isBIT);

private:
  TAO::DCPS::RepoId id_;
  DCPS_IR_Domain* domain_;
  ::DDS::DomainParticipantQos qos_;
  CORBA::Boolean aliveStatus_;
  ::DDS::InstanceHandle_t handle_;
  CORBA::Boolean isBIT_;

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
  //  other entities (specifically the UpdateManager) the
  //  changes it makes.
  UpdateManager* um_;
};


#endif /* DCPS_IR_PARTICIPANT_H */
