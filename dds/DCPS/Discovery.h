/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DISCOVERY_H
#define OPENDDS_DCPS_DISCOVERY_H

#include "ConditionVariable.h"
#include "DataReaderCallbacks.h"
#include "DataWriterCallbacks.h"
#include "PoolAllocationBase.h"
#include "PoolAllocator.h"
#include "RcHandle_T.h"
#include "RcObject.h"
#include "TopicCallbacks.h"
#include "unique_ptr.h"

#include "XTypes/TypeLookupService.h"
#include "XTypes/TypeObject.h"

#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY
#  include <dds/DdsSecurityCoreC.h>
#endif

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;
class DataWriterImpl;
class DataReaderImpl;
class BitSubscriber;

/**
 * This is used by get_dynamic_type on the service participant to wait for a
 * TypeObject request to complete.
 */
struct OpenDDS_Dcps_Export TypeObjReqCond {
  typedef ACE_Thread_Mutex LockType;
  LockType lock;
  ConditionVariable<LockType> cond;
  bool waiting;
  DDS::ReturnCode_t rc;

  TypeObjReqCond()
  : cond(lock)
  , waiting(true)
  , rc(DDS::RETCODE_OK)
  {
  }

  DDS::ReturnCode_t wait();
  void done(DDS::ReturnCode_t retcode);
};

/**
 * @class Discovery
 *
 * @brief Discovery Strategy interface class
 *
 * This class is an abstract class that acts as an interface for both
 * InfoRepo-based discovery and RTPS Discovery.
 *
 */
class OpenDDS_Dcps_Export Discovery : public virtual RcObject {
public:
  /// Key type for storing discovery objects.
  /// Probably should just be Discovery::Key
  typedef OPENDDS_STRING RepoKey;

  /// Key value for the default repository IOR.
  static const char* DEFAULT_REPO;
  static const char* DEFAULT_RTPS;
  static const char* DEFAULT_STATIC;

  // TODO: NEED TO REMOVE THIS once Service_Participant::repository_lost has been refactored
  virtual bool active() { return true; }

  virtual RcHandle<BitSubscriber> init_bit(DomainParticipantImpl* participant) = 0;

  virtual void fini_bit(DCPS::DomainParticipantImpl* participant) = 0;

  virtual RepoKey key() const = 0;

  class OpenDDS_Dcps_Export Config
    : public PoolAllocationBase
    , public EnableContainerSupportedUniquePtr<Config> {
  public:
    virtual ~Config();
    virtual int discovery_config() = 0;
  };

  virtual bool attach_participant(
    DDS::DomainId_t domainId,
    const GUID_t& participantId) = 0;

  virtual OpenDDS::DCPS::GUID_t generate_participant_guid() = 0;

  virtual AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls) = 0;

#if OPENDDS_CONFIG_SECURITY
  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls,
    const OpenDDS::DCPS::GUID_t& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto) = 0;
#endif

  virtual bool remove_domain_participant(
    DDS::DomainId_t domainId,
    const GUID_t& participantId) = 0;

  virtual bool ignore_domain_participant(
    DDS::DomainId_t domainId,
    const GUID_t& myParticipantId,
    const GUID_t& ignoreId) = 0;

  virtual bool update_domain_participant_qos(
    DDS::DomainId_t domain,
    const GUID_t& participantId,
    const DDS::DomainParticipantQos& qos) = 0;


  // Topic operations:

  virtual TopicStatus assert_topic(
    GUID_t_out topicId,
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey,
    TopicCallbacks* topic_callbacks) = 0;

  virtual TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    GUID_t_out topicId) = 0;

  virtual TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId) = 0;

  virtual bool ignore_topic(
    DDS::DomainId_t domainId,
    const GUID_t& myParticipantId,
    const GUID_t& ignoreId) = 0;

  virtual bool update_topic_qos(
    const GUID_t& topicId,
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const DDS::TopicQos& qos) = 0;


  // Publication operations:

  virtual void pre_writer(DataWriterImpl*) {}

  /// add the passed in publication into discovery.
  /// Discovery does not participate in memory management
  /// for the publication pointer, so it requires that
  /// the publication pointer remain valid until
  /// remove_publication is called.
  /// Return true on success.
  virtual bool add_publication(DDS::DomainId_t domainId,
                               const GUID_t& participantId,
                               const GUID_t& topicId,
                               DataWriterCallbacks_rch publication,
                               const DDS::DataWriterQos& qos,
                               const TransportLocatorSeq& transInfo,
                               const DDS::PublisherQos& publisherQos,
                               const XTypes::TypeInformation& type_info) = 0;

  virtual bool remove_publication(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& publicationId) = 0;

  virtual bool ignore_publication(
    DDS::DomainId_t domainId,
    const GUID_t& myParticipantId,
    const GUID_t& ignoreId) = 0;

  virtual bool update_publication_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos) = 0;

  virtual void update_publication_locators(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const TransportLocatorSeq& transInfo);

  // Subscription operations:

  virtual void pre_reader(DataReaderImpl*) {}

  /// add the passed in subscription into discovery.
  /// Discovery does not participate in memory management
  /// for the subscription pointer, so it requires that
  /// the subscription pointer remain valid until
  /// remove_subscription is called.
  /// Return true on success.
  virtual bool add_subscription(DDS::DomainId_t domainId,
                                const GUID_t& participantId,
                                const GUID_t& topicId,
                                DataReaderCallbacks_rch subscription,
                                const DDS::DataReaderQos& qos,
                                const TransportLocatorSeq& transInfo,
                                const DDS::SubscriberQos& subscriberQos,
                                const char* filterClassName,
                                const char* filterExpression,
                                const DDS::StringSeq& exprParams,
                                const XTypes::TypeInformation& type_info) = 0;

  virtual bool remove_subscription(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& subscriptionId) = 0;

  virtual bool ignore_subscription(
    DDS::DomainId_t domainId,
    const GUID_t& myParticipantId,
    const GUID_t& ignoreId) = 0;

  virtual bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos) = 0;

  virtual bool update_subscription_params(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& subscriptionId,
    const DDS::StringSeq& params) = 0;

  virtual void update_subscription_locators(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& drId,
    const TransportLocatorSeq& transInfo);

  // Managing reader/writer associations:

  virtual bool supports_liveliness() const { return false; }

  virtual void signal_liveliness(const DDS::DomainId_t /*domain_id*/,
                                 const GUID_t& /*part_id*/,
                                 DDS::LivelinessQosPolicyKind /*kind*/) { }

  virtual void request_remote_complete_type_objects(
    DDS::DomainId_t /*domain*/, const GUID_t& /*local_participant*/,
    const GUID_t& /*remote_entity*/, const XTypes::TypeInformation& /*remote_type_info*/,
    TypeObjReqCond& cond)
  {
    cond.done(DDS::RETCODE_UNSUPPORTED);
  }

protected:
  DDS::ReturnCode_t create_bit_topics(DomainParticipantImpl* participant);
};

typedef RcHandle<Discovery> Discovery_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DISCOVERY_H  */
