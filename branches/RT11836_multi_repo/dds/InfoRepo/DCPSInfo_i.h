// -*- C++ -*-
// ============================================================================
/**
 *  @file   DCPSInfo_i.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef DCPSINFO_I_H
#define DCPSINFO_I_H



#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Domain.h"
#include "UpdateManager.h"

#include /**/ "dds/DdsDcpsInfoS.h"
#include /**/ "dds/DdsDcpsDataReaderRemoteC.h"
#include /**/ "dds/DdsDcpsDataWriterRemoteC.h"

#include "tao/ORB_Core.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// typedef declarations
typedef ACE_Map_Manager< ::DDS::DomainId_t, DCPS_IR_Domain*, ACE_Null_Mutex> DCPS_IR_Domain_Map;

// Forward declaration
class UpdateManager;

/**
 * @class TAO_DDS_DCPSInfo_i
 *
 * @brief Implementation of the DCPSInfo
 *
 * This is the Information Repository object.  Clients of
 * the system will use the CORBA reference of this object.
 */
class  TAO_DDS_DCPSInfo_i : public virtual POA_OpenDDS::DCPS::DCPSInfo
{
public:
  //Constructor
  TAO_DDS_DCPSInfo_i (CORBA::ORB_ptr orb, bool reincarnate);

  //Destructor
  virtual ~TAO_DDS_DCPSInfo_i (void);

  virtual OpenDDS::DCPS::TopicStatus assert_topic (
      OpenDDS::DCPS::RepoId_out topicId,
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId participantId,
      const char * topicName,
      const char * dataTypeName,
      const ::DDS::TopicQos & qos
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
    ));

  bool add_topic (OpenDDS::DCPS::RepoId topicId,
                  ::DDS::DomainId_t domainId,
                  OpenDDS::DCPS::RepoId participantId,
                  const char* topicName,
                  const char* dataTypeName,
                  const ::DDS::TopicQos& qos);

  virtual OpenDDS::DCPS::TopicStatus find_topic (
      ::DDS::DomainId_t domainId,
      const char * topicName,
      CORBA::String_out dataTypeName,
      ::DDS::TopicQos_out qos,
      OpenDDS::DCPS::RepoId_out topicId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
    ));

  virtual OpenDDS::DCPS::TopicStatus remove_topic (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId participantId,
      OpenDDS::DCPS::RepoId topicId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Topic
    ));

  virtual OpenDDS::DCPS::TopicStatus enable_topic (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId participantId,
      OpenDDS::DCPS::RepoId topicId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Topic
    ));

  virtual OpenDDS::DCPS::RepoId add_publication (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId participantId,
      OpenDDS::DCPS::RepoId topicId,
      OpenDDS::DCPS::DataWriterRemote_ptr publication,
      const ::DDS::DataWriterQos & qos,
      const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
      const ::DDS::PublisherQos & publisherQos
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Topic
    ));

  bool add_publication (::DDS::DomainId_t domainId,
                        OpenDDS::DCPS::RepoId participantId,
                        OpenDDS::DCPS::RepoId topicId,
                        OpenDDS::DCPS::RepoId pubId,
                        const char* pub_str,
                        const ::DDS::DataWriterQos & qos,
                        const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
                        const ::DDS::PublisherQos & publisherQos);

    virtual void remove_publication (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId participantId,
      OpenDDS::DCPS::RepoId publicationId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Publication
    ));

  virtual OpenDDS::DCPS::RepoId add_subscription (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId participantId,
      OpenDDS::DCPS::RepoId topicId,
      OpenDDS::DCPS::DataReaderRemote_ptr subscription,
      const ::DDS::DataReaderQos & qos,
      const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
      const ::DDS::SubscriberQos & subscriberQos
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Topic
    ));

  bool add_subscription (::DDS::DomainId_t domainId,
                         OpenDDS::DCPS::RepoId participantId,
                         OpenDDS::DCPS::RepoId topicId,
                         OpenDDS::DCPS::RepoId subId,
                         const char* sub_str,
                         const ::DDS::DataReaderQos & qos,
                         const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
                         const ::DDS::SubscriberQos & subscriberQos);

    virtual void remove_subscription (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId participantId,
      OpenDDS::DCPS::RepoId subscriptionId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Subscription
    ));

  virtual OpenDDS::DCPS::RepoId add_domain_participant (
      ::DDS::DomainId_t domain,
      const ::DDS::DomainParticipantQos & qos
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
    ));

  bool add_domain_participant (::DDS::DomainId_t domainId
                               , OpenDDS::DCPS::RepoId participantId
                               , const ::DDS::DomainParticipantQos & qos);

    virtual void remove_domain_participant (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId participantId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
    ));

  virtual void ignore_domain_participant (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId myParticipantId,
      OpenDDS::DCPS::RepoId otherParticipantId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
    ));

  virtual void ignore_topic (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId myParticipantId,
      OpenDDS::DCPS::RepoId topicId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Topic
    ));

  virtual void ignore_subscription (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId myParticipantId,
      OpenDDS::DCPS::RepoId subscriptionId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Subscription
    ));

  virtual void ignore_publication (
      ::DDS::DomainId_t domainId,
      OpenDDS::DCPS::RepoId myParticipantId,
      OpenDDS::DCPS::RepoId publicationId
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
      , OpenDDS::DCPS::Invalid_Domain
      , OpenDDS::DCPS::Invalid_Participant
      , OpenDDS::DCPS::Invalid_Publication
    ));

  /// Called to load the domains
  /// returns the number of domains that were loaded.
  /// Currently after loading domains, it
  ///  invoke 'init_persistence'
  int load_domains (const ACE_TCHAR* filename, bool use_bit);

  /// Initialize the transport for the Built-In Topics
  /// Returns 0 (zero) if succeeds
  int init_transport (int listen_address_given,
                      const ACE_INET_Addr listen);

  bool receive_image (const UpdateManager::UImage& image);

private:

  bool init_persistence (void);

private:
  DCPS_IR_Domain_Map domains_;
  CORBA::ORB_var orb_;

  UpdateManager* um_;
  bool reincarnate_;
};


#endif /* DCPSINFO_I_H_  */
