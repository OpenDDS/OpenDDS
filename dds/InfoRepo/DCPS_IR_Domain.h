/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_IR_DOMAIN_H
#define DCPS_IR_DOMAIN_H

#include  "inforepo_export.h"
#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DCPS/InfoRepoDiscovery/InfoS.h"

#include "dds/DCPS/RepoIdGenerator.h"

#include /**/ "dds/DdsDcpsDomainC.h"
#include /**/ "dds/DdsDcpsInfoUtilsC.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include /**/ "dds/DdsDcpsCoreTypeSupportImpl.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "dds/DCPS/transport/framework/TransportConfig.h"
#include "dds/DCPS/transport/tcp/TcpTransport.h"
#include "dds/DCPS/transport/framework/TransportConfig_rch.h"
#include /**/ "ace/Unbounded_Set.h"

#include <set>
#include <map>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

// forward declarations
class DCPS_IR_Topic_Description;
typedef std::set<DCPS_IR_Topic_Description*> DCPS_IR_Topic_Description_Set;

class DCPS_IR_Participant;
typedef ACE_Unbounded_Set<DCPS_IR_Participant*> DCPS_IR_Participant_Set;

typedef std::map<OpenDDS::DCPS::RepoId, DCPS_IR_Participant*,
  OpenDDS::DCPS::GUID_tKeyLessThan> DCPS_IR_Participant_Map;

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
class OpenDDS_InfoRepoLib_Export DCPS_IR_Domain {
public:
  DCPS_IR_Domain(DDS::DomainId_t id, OpenDDS::DCPS::RepoIdGenerator& generator);

  ~DCPS_IR_Domain();

  /// Add the participant
  /// This takes ownership of the memory if the particpant is added.
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_participant(DCPS_IR_Participant* participant);

  /// Remove the particpant
  /// The participant has been deleted if returns successful.
  /// Returns 0 if successful.
  /// The notify_lost parameter is passed to the remove_associations()
  /// See the comments of remove_associations() in DataWriterRemote.idl
  /// or DataReaderRemote.idl.
  int remove_participant(const OpenDDS::DCPS::RepoId& particpantId,
                         CORBA::Boolean    notify_lost);

  /// Find the participant with the id.
  DCPS_IR_Participant* participant(const OpenDDS::DCPS::RepoId& id) const;

  /// Add a topic to the domain
  /// Returns OpenDDS::DCPS::CREATED if successfull
  OpenDDS::DCPS::TopicStatus add_topic(OpenDDS::DCPS::RepoId_out topicId,
                                       const char * topicName,
                                       const char * dataTypeName,
                                       const DDS::TopicQos & qos,
                                       DCPS_IR_Participant* participantPtr);

  OpenDDS::DCPS::TopicStatus force_add_topic(const OpenDDS::DCPS::RepoId& topicId,
                                             const char* topicName,
                                             const char* dataTypeName,
                                             const DDS::TopicQos & qos,
                                             DCPS_IR_Participant* participantPtr);

  /// Find the topic with the topic name
  /// Does NOT take ownership of any initial memory pointed to by topic
  /// Returns OpenDDS::DCPS::FOUND if exists and topic is changed, -1 otherwise
  OpenDDS::DCPS::TopicStatus find_topic(const char * topicName,
                                        DCPS_IR_Topic*& topic);

  /// Find a topic object reference using the topic Id value.
  DCPS_IR_Topic* find_topic(const OpenDDS::DCPS::RepoId& id);

  /// Remove the topic
  /// The topic has been deleted if returns successful
  /// Returns OpenDDS::DCPS::REMOVED if successful
  OpenDDS::DCPS::TopicStatus remove_topic(DCPS_IR_Participant* part,
                                          DCPS_IR_Topic*& topic);

  /// Remove the topic from the id to topic map.
  /// This method should only be called by the DCPS_IR_Topic
  /// when deleting the topic.
  void remove_topic_id_mapping(const OpenDDS::DCPS::RepoId& topicId);

  /// Mark a participant as being unresponsive (dead) and
  ///  schedule it to be removed next time
  ///  remove_dead_participants is called.
  void add_dead_participant(DCPS_IR_Participant* participant);

  /// Remove any participants currently marked as dead
  void remove_dead_participants();

  DDS::DomainId_t get_id();

  // Next Entity Id value in sequence.
  OpenDDS::DCPS::RepoId get_next_participant_id();

  // Ensure no conflicts with sequence values from persistent storage.
  void last_participant_key(long key);

  /// Initialize the Built-In Topic structures
  /// This needs to be called before the run begins
  /// Returns 0 (zero) if successful
  int init_built_in_topics(bool federated = false);

  /// Cleans up the Built-In Topic structures
  int cleanup_built_in_topics();

  /// Reassociate the Built-In Topic datawriters
  /// This needs to be called after reincarnating from persistence and
  /// before the run begins
  /// Returns 0 (zero) if successful
  int reassociate_built_in_topic_pubs();

  /// Publish the Built-In Topic information
  void publish_participant_bit(DCPS_IR_Participant* participant);
  void publish_topic_bit(DCPS_IR_Topic* topic);
  void publish_subscription_bit(DCPS_IR_Subscription* subscription);
  void publish_publication_bit(DCPS_IR_Publication* publication);

  /// Remove the Built-In Topic information
  void dispose_participant_bit(DCPS_IR_Participant* participant);
  void dispose_topic_bit(DCPS_IR_Topic* topic);
  void dispose_subscription_bit(DCPS_IR_Subscription* subscription);
  void dispose_publication_bit(DCPS_IR_Publication* publication);

  /// Expose a readable reference to the participant map.
  const DCPS_IR_Participant_Map& participants() const;

  std::string dump_to_string(const std::string& prefix, int depth) const;

  bool useBIT() const { return useBIT_; }

private:
  OpenDDS::DCPS::TopicStatus add_topic_i(OpenDDS::DCPS::RepoId& topicId,
                                         const char * topicName,
                                         const char * dataTypeName,
                                         const DDS::TopicQos & qos,
                                         DCPS_IR_Participant* participantPtr);

  /// Takes ownership of the memory pointed to by desc if successful
  /// returns 0 if successful,
  /// 1 if description already exists
  /// -1 unknown error
  /// 2 if confliciting dataTypeName
  int add_topic_description(DCPS_IR_Topic_Description*& desc);

  /// Find the topic description with the name and data type name
  /// Does NOT take ownership of any initial memory pointed to by desc
  /// Returns 0 if found and desc is changed,
  ///  -1 if not found and 1 if conflicting dataTypeName
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
  int init_built_in_topics_topics();
  int init_built_in_topics_datawriters(bool federated);
  int init_built_in_topics_transport();

  DDS::DomainId_t id_;

  // Participant GUID Id generator.  The remaining Entities have their
  // values generated within the containing Participant.
  OpenDDS::DCPS::RepoIdGenerator& participantIdGenerator_;

  /// all the participants
  DCPS_IR_Participant_Map participants_;

  /// the dead participants
  /// dead participants exist in both this and participants_
  DCPS_IR_Participant_Set deadParticipants_;

  /// all the topics
  DCPS_IR_Topic_Description_Set topicDescriptions_;

  /// Mapping from RepoId values to Topic object references.
  typedef std::map<OpenDDS::DCPS::RepoId, DCPS_IR_Topic*,
    OpenDDS::DCPS::GUID_tKeyLessThan> IdToTopicMap;

  /// Actual mapping of Id values to Topic object references.
  IdToTopicMap idToTopicMap_;

  /// indicates if the BuiltIn Topics are enabled
  bool useBIT_;

  /// Built-in Topic variables
  DDS::DomainParticipantFactory_var                bitParticipantFactory_;
  DDS::DomainParticipant_var                       bitParticipant_;
  DDS::DomainParticipantListener_var               bitParticipantListener_;
  DDS::Publisher_var                               bitPublisher_;

#if !defined (DDS_HAS_MINIMUM_BIT)
  OpenDDS::DCPS::TransportConfig_rch               transportConfig_;

  DDS::Topic_var                                   bitParticipantTopic_;
  DDS::ParticipantBuiltinTopicDataDataWriter_var   bitParticipantDataWriter_;

  DDS::Topic_var                                   bitTopicTopic_;
  DDS::TopicBuiltinTopicDataDataWriter_var         bitTopicDataWriter_;

  DDS::Topic_var                                   bitSubscriptionTopic_;
  DDS::SubscriptionBuiltinTopicDataDataWriter_var  bitSubscriptionDataWriter_;

  DDS::Topic_var                                   bitPublicationTopic_;
  DDS::PublicationBuiltinTopicDataDataWriter_var   bitPublicationDataWriter_;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_IR_DOMAIN_H */
