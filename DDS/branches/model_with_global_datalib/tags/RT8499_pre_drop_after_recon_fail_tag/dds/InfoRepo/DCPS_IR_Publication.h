// ============================================================================
/**
 *  @file   DCPS_IR_Publication.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================
#ifndef DCPS_IR_PUBLICATION_H
#define DCPS_IR_PUBLICATION_H

#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DdsDcpsPublicationC.h"
#include /**/ "dds/DdsDcpsInfoC.h"
#include /**/ "dds/DdsDcpsDataWriterRemoteC.h"
#include /**/ "ace/Unbounded_Set.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

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
class DCPS_IR_Publication
{
public:
  DCPS_IR_Publication (TAO::DCPS::RepoId id,
                       DCPS_IR_Participant* participant,
                       DCPS_IR_Topic* topic,
                       TAO::DCPS::DataWriterRemote_ptr writer,
                       ::DDS::DataWriterQos qos,
                       TAO::DCPS::TransportInterfaceInfo info,
                       ::DDS::PublisherQos publisherQos);

  ~DCPS_IR_Publication ();

  /// Associate with the subscription
  /// Adds the subscription to the list of associated
  ///  subscriptions and notifies datawriter if successfully added
  /// This method can mark the participant dead
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_associated_subscription (DCPS_IR_Subscription* sub);

  /// Remove the associated subscription
  /// Removes the subscription from the list of associated
  ///  subscriptions if return successful
  /// sendNotify indicates whether to tell the datawriter about
  ///  removing the subscription
  /// The notify_lost flag true indicates this remove_associations is called
  /// when the InfoRepo detects this publication is lost because of the failure
  /// of invocation on this publication.
  /// This method can mark the participant dead
  /// Returns 0 if successful
  int remove_associated_subscription (DCPS_IR_Subscription* sub,
                                      CORBA::Boolean sendNotify,
                                      CORBA::Boolean notify_lost);

  /// Removes all the associated subscriptions
  /// This method can mark the participant dead
  /// The notify_lost flag true indicates this remove_associations is called
  /// when the InfoRepo detects this publication is lost because of the failure
  /// of invocation on this publication.
  /// Returns 0 if successful
  int remove_associations (CORBA::Boolean notify_lost);

  /// Remove any subscriptions whose participant has the id
  void disassociate_participant (TAO::DCPS::RepoId id);

  /// Remove any subscriptions whose topic has the id
  void disassociate_topic (TAO::DCPS::RepoId id);

  /// Remove any subscriptions with the id
  void disassociate_subscription (TAO::DCPS::RepoId id);

  /// Notify the writer of incompatible qos status
  ///  and reset the status' count_since_last_send to 0
  void update_incompatible_qos ();

  /// Check that none of the ids given are ones that
  ///  this publication should ignore.
  /// returns 1 if one of these ids is an ignored id
  CORBA::Boolean is_subscription_ignored (TAO::DCPS::RepoId partId,
                                          TAO::DCPS::RepoId topicId,
                                          TAO::DCPS::RepoId subId);

  /// Return pointer to the DataWriter qos
  /// Publication retains ownership
  ::DDS::DataWriterQos* get_datawriter_qos ();

  /// Return pointer to the Publisher qos
  /// Publication retains ownership
  ::DDS::PublisherQos* get_publisher_qos ();

  /// get the transport ID of the transport implementation type.
  TAO::DCPS::TransportInterfaceId   get_transport_id () const;

  /// Returns a copy of the TransportInterfaceInfo object
  TAO::DCPS::TransportInterfaceInfo get_transportInterfaceInfo () const;

  /// Return pointer to the incompatible qos status
  /// Publication retains ownership
  TAO::DCPS::IncompatibleQosStatus* get_incompatibleQosStatus (); 

  TAO::DCPS::RepoId get_id ();
  TAO::DCPS::RepoId get_topic_id ();
  TAO::DCPS::RepoId get_participant_id ();

  DCPS_IR_Topic* get_topic ();
  DCPS_IR_Topic_Description* get_topic_description();

  ::DDS::InstanceHandle_t get_handle();
  void set_handle(::DDS::InstanceHandle_t handle);

  CORBA::Boolean is_bit ();
  void set_bit_status (CORBA::Boolean isBIT);

private:
  TAO::DCPS::RepoId id_;
  DCPS_IR_Participant* participant_;
  DCPS_IR_Topic* topic_;
  ::DDS::InstanceHandle_t handle_;
  CORBA::Boolean isBIT_;

  /// the corresponding DataWriterRemote object
  TAO::DCPS::DataWriterRemote_var writer_;
  ::DDS::DataWriterQos qos_;
  TAO::DCPS::TransportInterfaceInfo info_;
  ::DDS::PublisherQos publisherQos_;

  DCPS_IR_Subscription_Set associations_;

  TAO::DCPS::IncompatibleQosStatus incompatibleQosStatus_;
};

#endif /* DCPS_IR_PUBLICATION_H */
