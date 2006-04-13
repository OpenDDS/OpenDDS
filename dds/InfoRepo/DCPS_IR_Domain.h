// ============================================================================
/**
 *  @file   DCPS_IR_Domain.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef DCPS_IR_DOMAIN_H
#define DCPS_IR_DOMAIN_H

#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DdsDcpsInfoS.h"
#include /**/ "DCPS_Entity_Id_Generator.h"

#include /**/ "dds/DdsDcpsDomainC.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include /**/ "dds/ParticipantBuiltinTopicDataTypeSupportC.h"
#include /**/ "dds/TopicBuiltinTopicDataTypeSupportC.h"
#include /**/ "dds/SubscriptionBuiltinTopicDataTypeSupportC.h"
#include /**/ "dds/PublicationBuiltinTopicDataTypeSupportC.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpTransport.h"

#include /**/ "ace/Unbounded_Set.h"
#include /**/ "ace/Map_Manager.h"
#include /**/ "ace/Null_Mutex.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// forward declarations
class DCPS_IR_Topic_Description;
typedef ACE_Unbounded_Set<DCPS_IR_Topic_Description*> DCPS_IR_Topic_Description_Set;

class DCPS_IR_Participant;
typedef ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex> DCPS_IR_Participant_Map;
typedef ACE_Unbounded_Set<DCPS_IR_Participant*> DCPS_IR_Participant_Set;

class DCPS_IR_Topic;
class DCPS_IR_Subscription;
class DCPS_IR_Publication;


/**
 * @class DCPS_IR_Domain
 *
 * @brief Representation of a Domain in the system.
 *
 * This represents a Domain in the system.  It contains the
 * representatives of the entities that are in the corresponding
 * system's domain.
 */
class DCPS_IR_Domain
{
public:
  DCPS_IR_Domain(::DDS::DomainId_t id);

  ~DCPS_IR_Domain();

  /// Add the participant
  /// This takes ownership of the memory if the particpant is added.
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_participant(DCPS_IR_Participant* participant);

  /// Remove the particpant
  /// The participant has been deleted if returns successful.
  /// Returns 0 if successful.
  /// The notify_lost parameter is passed to the remove_associations()
  /// See the comments of remove_associations() in DdsDcpsDataWriterRemote.idl
  /// or DdsDcpsDataReaderRemote.idl.
  int remove_participant(TAO::DCPS::RepoId particpantId,
                         CORBA::Boolean    notify_lost);

  /// Find the participant with the particpant id
  /// Does NOT take ownership of any initial memory pointed to by participant
  /// Returns 0 if exists and participant is changed, -1 otherwise
  int find_participant(TAO::DCPS::RepoId particpantId,
                       DCPS_IR_Participant*& participant);

  /// Add a topic to the domain
  /// Returns TAO::DCPS::CREATED if successfull
  TAO::DCPS::TopicStatus add_topic(TAO::DCPS::RepoId_out topicId,
                                   const char * topicName,
                                   const char * dataTypeName,
                                   const ::DDS::TopicQos & qos,
                                   DCPS_IR_Participant* participantPtr);

  /// Find the topic with the topic name
  /// Does NOT take ownership of any initial memory pointed to by topic
  /// Returns TAO::DCPS::FOUND if exists and topic is changed, -1 otherwise
  TAO::DCPS::TopicStatus find_topic(const char * topicName,
                                    DCPS_IR_Topic*& topic);

  /// Remove the topic
  /// The topic has been deleted if returns successful
  /// Returns TAO::DCPS::REMOVED if successful
  TAO::DCPS::TopicStatus remove_topic(DCPS_IR_Participant* part,
                                      DCPS_IR_Topic*& topic);

  /// Mark a participant as being unresponsive (dead) and
  ///  schedule it to be removed next time
  ///  remove_dead_participants is called.
  void add_dead_participant(DCPS_IR_Participant* participant);

  /// Remove any participants currently marked as dead
  void remove_dead_participants();

  ::DDS::DomainId_t get_id ();

  TAO::DCPS::RepoId get_next_participant_id ();
  TAO::DCPS::RepoId get_next_topic_id ();
  TAO::DCPS::RepoId get_next_publication_id ();
  TAO::DCPS::RepoId get_next_subscription_id ();


  /// Initialize the Built-In Topic structures
  /// This needs to be called before the run begins
  /// Returns 0 (zero) if successful
  int init_built_in_topics();

  /// Cleans up the Built-In Topic structures
  int cleanup_built_in_topics();


  /// Publish the Built-In Topic information
  void publish_participant_bit (DCPS_IR_Participant* participant);
  void publish_topic_bit (DCPS_IR_Topic* topic);
  void publish_subscription_bit (DCPS_IR_Subscription* subscription);
  void publish_publication_bit (DCPS_IR_Publication* publication);

  /// Remove the Built-In Topic information
  void dispose_participant_bit (DCPS_IR_Participant* participant);
  void dispose_topic_bit (DCPS_IR_Topic* topic);
  void dispose_subscription_bit (DCPS_IR_Subscription* subscription);
  void dispose_publication_bit (DCPS_IR_Publication* publication);

private:
  /// Takes ownership of the memory pointed to by desc if successful
  /// returns 0 if successful,
  /// 1 if description already exists
  /// -1 unknown error
  /// 2 if confliciting dataTypeName
  int add_topic_description(DCPS_IR_Topic_Description*& desc);

  /// Find the topic description with the name and data type name
  /// Does NOT take ownership of any initial memory pointed to by desc
  /// Returns 0 if found and desc is changed,
  ///  -1 if not found and 1 if confliciting dataTypeName
  int find_topic_description(const char* name,
                             const char* dataTypeName,
                             DCPS_IR_Topic_Description*& desc);

  /// Caller is given ownership of the topic description and any memory
  ///  that it has ownership of if returns successful
  // Returns 0 if successful
  int remove_topic_description(DCPS_IR_Topic_Description*& desc);

  /// work of initializing the built in topics is
  /// done in these private methods.  They were
  /// broken up for readability.
  int init_built_in_topics_topics ();
  int init_built_in_topics_datawriters ();
  int init_built_in_topics_transport ();

  ::DDS::DomainId_t id_;

  // the id generators
  DCPS_Entity_Id_Generator participantIdGenerator_;
  DCPS_Entity_Id_Generator topicIdGenerator_;
  DCPS_Entity_Id_Generator pubsubIdGenerator_;

  /// all the participants
  DCPS_IR_Participant_Map participants_;

  /// the dead participants
  /// dead participants exist in both this and participants_
  DCPS_IR_Participant_Set deadParticipants_;

  /// all the topics
  DCPS_IR_Topic_Description_Set topicDescriptions_;


  /// indicates if the BuiltIn Topics are enabled
  bool useBIT_;

  /// Built-in Topic variables
  ::DDS::DomainParticipantFactory_var                bitParticipantFactory_;
  ::DDS::DomainParticipant_var                       bitParticipant_;
  ::DDS::DomainParticipantListener_var               bitParticipantListener_;
  ::DDS::Publisher_var                               bitPublisher_;
  TAO::DCPS::TransportImpl_rch                       transportImpl_;

#if !defined (DDS_HAS_MINIMUM_BIT)
  ::DDS::Topic_var                                   bitParticipantTopic_;
  ::DDS::ParticipantBuiltinTopicDataDataWriter_var   bitParticipantDataWriter_;

  ::DDS::Topic_var                                   bitTopicTopic_;
  ::DDS::TopicBuiltinTopicDataDataWriter_var         bitTopicDataWriter_;

  ::DDS::Topic_var                                   bitSubscriptionTopic_;
  ::DDS::SubscriptionBuiltinTopicDataDataWriter_var  bitSubscriptionDataWriter_;

  ::DDS::Topic_var                                   bitPublicationTopic_;
  ::DDS::PublicationBuiltinTopicDataDataWriter_var   bitPublicationDataWriter_;
#endif // !defined (DDS_HAS_MINIMUM_BIT)

};

#endif /* DCPS_IR_DOMAIN_H */
