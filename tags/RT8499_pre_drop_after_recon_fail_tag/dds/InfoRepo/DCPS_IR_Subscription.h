// ============================================================================
/**
 *  @file   DCPS_IR_Subscription.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================
#ifndef DCPS_IR_SUBSCRIPTION_H
#define DCPS_IR_SUBSCRIPTION_H

#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "dds/DdsDcpsSubscriptionC.h"
#include /**/ "dds/DdsDcpsInfoC.h"
#include /**/ "dds/DdsDcpsDataReaderRemoteC.h"
#include /**/ "ace/Unbounded_Set.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


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
class DCPS_IR_Subscription
{
public:
  DCPS_IR_Subscription (TAO::DCPS::RepoId id,
                        DCPS_IR_Participant* participant,
                        DCPS_IR_Topic* topic,
                        TAO::DCPS::DataReaderRemote_ptr reader,
                        ::DDS::DataReaderQos qos,
                        TAO::DCPS::TransportInterfaceInfo info,
                        ::DDS::SubscriberQos subscriberQos);

  ~DCPS_IR_Subscription ();

  /// Associate with the publication
  /// Adds the publication to the list of associated
  ///  publications and notifies datareader if successfully added
  /// This method can mark the participant dead
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_associated_publication (DCPS_IR_Publication* pub);

  /// Remove the associated publication
  /// Removes the publication from the list of associated
  ///  publications if return successful
  /// sendNotify indicates whether to tell the datareader about
  ///  removing the publication
  /// The notify_lost flag true indicates this remove_associations is called
  /// when the InfoRepo detects this subscription is lost because of the failure
  /// of invocation on this subscription.
  /// This method can mark the participant dead
  /// Returns 0 if successful
  int remove_associated_publication (DCPS_IR_Publication* pub,
                                     CORBA::Boolean sendNotify,
                                     CORBA::Boolean notify_lost);

  /// Removes all the associated publications
  /// This method can mark the participant dead
  /// The notify_lost flag true indicates this remove_associations is called
  /// when the InfoRepo detects this subscription is lost because of the failure
  /// of invocation on this subscription.
  /// Returns 0 if successful
  int remove_associations (CORBA::Boolean notify_lost);

  /// Remove any publications whose participant has the id
  void disassociate_participant (TAO::DCPS::RepoId id);

  /// Remove any publications whose topic has the id
  void disassociate_topic (TAO::DCPS::RepoId id);

  /// Remove any publications with id
  void disassociate_publication (TAO::DCPS::RepoId id);

  /// Notify the reader of incompatible qos status
  ///  and reset the status' count_since_last_send to 0
  void update_incompatible_qos ();

  /// Check that none of the ids given are ones that
  ///  this subscription should ignore.
  /// returns 1 if one of these ids is an ignored id
  CORBA::Boolean is_publication_ignored (TAO::DCPS::RepoId partId,
                                         TAO::DCPS::RepoId topicId,
                                         TAO::DCPS::RepoId pubId);

  /// Return pointer to the DataReader qos
  /// Subscription retains ownership
  const ::DDS::DataReaderQos* get_datareader_qos ();

  /// Return pointer to the Subscriber qos
  /// Subscription retains ownership
  const ::DDS::SubscriberQos* get_subscriber_qos ();

  /// get the transport ID of the transport implementation type.
  TAO::DCPS::TransportInterfaceId   get_transport_id () const;

  /// Returns a copy of the TransportInterfaceInfo object
  TAO::DCPS::TransportInterfaceInfo get_transportInterfaceInfo () const;

  /// Return pointer to the incompatible qos status
  /// Subscription retains ownership
  TAO::DCPS::IncompatibleQosStatus* get_incompatibleQosStatus (); 

  TAO::DCPS::RepoId get_id ();
  TAO::DCPS::RepoId get_topic_id ();
  TAO::DCPS::RepoId get_participant_id ();

  DCPS_IR_Topic_Description* get_topic_description();

  DCPS_IR_Topic* get_topic ();

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

  /// the corresponding DataReaderRemote object
  TAO::DCPS::DataReaderRemote_var reader_;
  ::DDS::DataReaderQos qos_;
  TAO::DCPS::TransportInterfaceInfo info_;
  ::DDS::SubscriberQos subscriberQos_;

  DCPS_IR_Publication_Set associations_;

  TAO::DCPS::IncompatibleQosStatus incompatibleQosStatus_;
};

#endif /* DCPS_IR_SUBSCRIPTION_H */
