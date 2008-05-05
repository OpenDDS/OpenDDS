// ============================================================================
/**
 *  @file   DCPS_IR_Topic.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================
#ifndef DCPS_IR_TOPIC_H
#define DCPS_IR_TOPIC_H

#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DdsDcpsTopicC.h"
#include /**/ "dds/DdsDcpsInfoC.h"
#include /**/ "ace/Unbounded_Set.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// forward declarations
class DCPS_IR_Publication;
typedef ACE_Unbounded_Set<DCPS_IR_Publication*> DCPS_IR_Publication_Set;

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
class DCPS_IR_Topic
{
public:
  DCPS_IR_Topic(OpenDDS::DCPS::RepoId id,
                ::DDS::TopicQos qos,
                DCPS_IR_Domain* domain,
                DCPS_IR_Participant* creator,
                DCPS_IR_Topic_Description* description);

  ~DCPS_IR_Topic ();

  /// Adds the publication to the list of publications
  /// Calls the topic description's try associate if successfully added
  /// 'associate' switch toggles association attempt.
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_publication_reference (DCPS_IR_Publication* publication
				 , bool associate = true);

  /// Removes the publication from the list of publications
  /// Returns 0 if successful
  int remove_publication_reference (DCPS_IR_Publication* publication);

  /// Called by the DCPS_IR_Topic_Description
  /// Find any compatible publications and associate
  ///  them using the DCPS_IR_Topic_Description's
  ///  associate method.
  /// This method does not check the subscription's incompatible
  ///  qos status.
  void try_associate (DCPS_IR_Subscription* subscription);

  OpenDDS::DCPS::RepoId get_id () const;
  OpenDDS::DCPS::RepoId get_participant_id () const;

  /// Return pointer to the Topic Description
  /// Domain retains ownership
  DCPS_IR_Topic_Description* get_topic_description ();

  /// Return pointer to the Topic qos
  /// Topic retains ownership
  ::DDS::TopicQos * get_topic_qos ();

  /// Reset topic qos and also propogate the qos change to related BITs
  /// that has the qos copy.
  void set_topic_qos (const ::DDS::TopicQos& qos);

  ::DDS::InstanceHandle_t get_handle();
  void set_handle(::DDS::InstanceHandle_t handle);

  CORBA::Boolean is_bit ();
  void set_bit_status (CORBA::Boolean isBIT);

private:
  OpenDDS::DCPS::RepoId id_;
  ::DDS::TopicQos qos_;
  DCPS_IR_Domain* domain_;
  DCPS_IR_Participant* participant_;
  DCPS_IR_Topic_Description* description_;
  ::DDS::InstanceHandle_t handle_;
  CORBA::Boolean isBIT_;

  DCPS_IR_Publication_Set publicationRefs_;
};

#endif /* DCPS_IR_TOPIC_H */
