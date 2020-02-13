/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DDS_DCPS_DISCOVERY_H
#define OPENDDS_DDS_DCPS_DISCOVERY_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "RcObject.h"
#include "RcHandle_T.h"
#include "unique_ptr.h"

#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DCPS/DataWriterCallbacks.h"
#include "dds/DCPS/TopicCallbacks.h"
#include "dds/DdsDcpsSubscriptionC.h"

#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/PoolAllocationBase.h"

#ifdef OPENDDS_SECURITY
#include "dds/DdsSecurityCoreC.h"
#endif

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Configuration_Heap;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;
class DataWriterImpl;
class DataReaderImpl;

/**
 * @class Discovery
 *
 * @brief Discovery Strategy interface class
 *
 * This class is an abstract class that acts as an interface for both
 * InfoRepo-based discovery and RTPS Discovery.
 *
 */
class OpenDDS_Dcps_Export Discovery : public RcObject {
public:
  /// Key type for storing discovery objects.
  /// Probably should just be Discovery::Key
  typedef OPENDDS_STRING RepoKey;

  explicit Discovery(const RepoKey& key) : key_(key) { }

  /// Key value for the default repository IOR.
  static const char* DEFAULT_REPO;
  static const char* DEFAULT_RTPS;
  static const char* DEFAULT_STATIC;

  // TODO: NEED TO REMOVE THIS once Service_Participant::repository_lost has been refactored
  virtual bool active() { return true; }

  virtual DDS::Subscriber_ptr init_bit(DomainParticipantImpl* participant) = 0;

  virtual void fini_bit(DCPS::DomainParticipantImpl* participant) = 0;

  virtual RepoId bit_key_to_repo_id(DomainParticipantImpl* participant,
                                    const char* bit_topic_name,
                                    const DDS::BuiltinTopicKey_t& key) const = 0;

  RepoKey key() const { return this->key_; }

  class OpenDDS_Dcps_Export Config
    : public PoolAllocationBase
    , public EnableContainerSupportedUniquePtr<Config> {
  public:
    virtual ~Config();
    virtual int discovery_config(ACE_Configuration_Heap& cf) = 0;
  };

  virtual bool attach_participant(
    DDS::DomainId_t domainId,
    const RepoId& participantId) = 0;

  virtual OpenDDS::DCPS::RepoId generate_participant_guid() = 0;

  virtual AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos) = 0;

#if defined(OPENDDS_SECURITY)
  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    const OpenDDS::DCPS::RepoId& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto) = 0;
#endif

  virtual bool remove_domain_participant(
    DDS::DomainId_t domainId,
    const RepoId& participantId) = 0;

  virtual bool ignore_domain_participant(
    DDS::DomainId_t domainId,
    const RepoId& myParticipantId,
    const RepoId& ignoreId) = 0;

  virtual bool update_domain_participant_qos(
    DDS::DomainId_t domain,
    const RepoId& participantId,
    const DDS::DomainParticipantQos& qos) = 0;


  // Topic operations:

  virtual TopicStatus assert_topic(
    RepoId_out topicId,
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey,
    TopicCallbacks* topic_callbacks) = 0;

  virtual TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    RepoId_out topicId) = 0;

  virtual TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& topicId) = 0;

  virtual bool ignore_topic(
    DDS::DomainId_t domainId,
    const RepoId& myParticipantId,
    const RepoId& ignoreId) = 0;

  virtual bool update_topic_qos(
    const RepoId& topicId,
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const DDS::TopicQos& qos) = 0;


  // Publication operations:

  virtual void pre_writer(DataWriterImpl*) {}

  /// add the passed in publication into discovery.
  /// Discovery does not participate in memory management
  /// for the publication pointer, so it requires that
  /// the publication pointer remain valid until
  /// remove_publication is called.
  virtual RepoId add_publication(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& topicId,
    DataWriterCallbacks* publication,
    const DDS::DataWriterQos& qos,
    const TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos) = 0;

  virtual bool remove_publication(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& publicationId) = 0;

  virtual bool ignore_publication(
    DDS::DomainId_t domainId,
    const RepoId& myParticipantId,
    const RepoId& ignoreId) = 0;

  virtual bool update_publication_qos(
    DDS::DomainId_t domainId,
    const RepoId& partId,
    const RepoId& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos) = 0;

  virtual void update_publication_locators(
    DDS::DomainId_t domainId,
    const RepoId& partId,
    const RepoId& dwId,
    const TransportLocatorSeq& transInfo);

  // Subscription operations:

  virtual void pre_reader(DataReaderImpl*) {}

  /// add the passed in subscription into discovery.
  /// Discovery does not participate in memory management
  /// for the subscription pointer, so it requires that
  /// the subscription pointer remain valid until
  /// remove_subscription is called.
  virtual RepoId add_subscription(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& topicId,
    DataReaderCallbacks* subscription,
    const DDS::DataReaderQos& qos,
    const TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpression,
    const DDS::StringSeq& exprParams) = 0;

  virtual bool remove_subscription(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& subscriptionId) = 0;

  virtual bool ignore_subscription(
    DDS::DomainId_t domainId,
    const RepoId& myParticipantId,
    const RepoId& ignoreId) = 0;

  virtual bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const RepoId& partId,
    const RepoId& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos) = 0;

  virtual bool update_subscription_params(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& subscriptionId,
    const DDS::StringSeq& params) = 0;

  virtual void update_subscription_locators(
    DDS::DomainId_t domainId,
    const RepoId& partId,
    const RepoId& drId,
    const TransportLocatorSeq& transInfo);

  // Managing reader/writer associations:

  virtual void association_complete(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& localId,
    const RepoId& remoteId) = 0;

  virtual bool supports_liveliness() const { return false; }

  virtual void signal_liveliness(const DDS::DomainId_t /*domain_id*/,
                                 const RepoId& /*part_id*/,
                                 DDS::LivelinessQosPolicyKind /*kind*/) { }

protected:
  DDS::ReturnCode_t create_bit_topics(DomainParticipantImpl* participant);

private:
  RepoKey        key_;

};

typedef RcHandle<Discovery> Discovery_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DISCOVERY_H  */
