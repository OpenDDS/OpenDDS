/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INFOREPODISCOVERY_INFOREPODISCOVERY_H
#define OPENDDS_DCPS_INFOREPODISCOVERY_INFOREPODISCOVERY_H

#include "dds/DCPS/Discovery.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"
#include "DataReaderRemoteC.h"
#include "InfoC.h"
#include "dds/DCPS/transport/framework/TransportConfig_rch.h"
#include "dds/DCPS/XTypes/TypeObject.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "ace/Task.h"

#include "InfoRepoDiscovery_Export.h"

#include "ace/Thread_Mutex.h"
#ifdef ACE_HAS_CPP11
#  include <atomic>
#else
#  include <ace/Atomic_Op_T.h>
#endif /* ACE_HAS_CPP11 */

#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class InfoRepoDiscovery
 *
 * @brief Discovery Strategy class that implements InfoRepo discovery
 *
 * This class implements the Discovery interface for InfoRepo-based
 * discovery.
 *
 */
class OpenDDS_InfoRepoDiscovery_Export InfoRepoDiscovery : public Discovery {
public:
  InfoRepoDiscovery(const RepoKey& key, const std::string& ior);
  InfoRepoDiscovery(const RepoKey& key, const DCPSInfo_var& info);

  virtual ~InfoRepoDiscovery();

  std::string get_stringified_dcps_info_ior();
  DCPSInfo_var get_dcps_info();

  virtual bool active();

  int bit_transport_port() const { return bit_transport_port_; }
  void bit_transport_port(int port) {
    bit_transport_port_ = port;
    use_local_bit_config_ = true;
  }

  std::string bit_transport_ip() const { return bit_transport_ip_; }
  void bit_transport_ip(const std::string& ip) {
    bit_transport_ip_ = ip;
    use_local_bit_config_ = true;
  }

  /// User provides an ORB for OpenDDS to use.
  /// @note The user is responsible for running the ORB.
  /// @Returns true if the operation succeeds
  bool set_ORB(CORBA::ORB_ptr orb);

  virtual DDS::Subscriber_ptr init_bit(DomainParticipantImpl* participant);

  virtual void fini_bit(DCPS::DomainParticipantImpl* participant);

  virtual bool attach_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId);

  virtual OpenDDS::DCPS::RepoId generate_participant_guid();

  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls);

#if defined(OPENDDS_SECURITY)
  virtual OpenDDS::DCPS::AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls,
    const OpenDDS::DCPS::RepoId& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto);
#endif

  virtual bool remove_domain_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId);

  virtual bool ignore_domain_participant(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_domain_participant_qos(
    DDS::DomainId_t domain,
    const OpenDDS::DCPS::RepoId& participantId,
    const DDS::DomainParticipantQos& qos);


  // Topic operations:

  virtual OpenDDS::DCPS::TopicStatus assert_topic(
    OpenDDS::DCPS::RepoId_out topicId,
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey,
    TopicCallbacks* topic_callbacks);

  virtual OpenDDS::DCPS::TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    OpenDDS::DCPS::RepoId_out topicId);

  virtual OpenDDS::DCPS::TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId);

  virtual bool ignore_topic(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_topic_qos(
    const OpenDDS::DCPS::RepoId& topicId,
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const DDS::TopicQos& qos);


  // Publication operations:

  virtual OpenDDS::DCPS::RepoId add_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const XTypes::TypeInformation& type_info);

  virtual bool remove_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& publicationId);

  virtual bool ignore_publication(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_publication_qos(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& partId,
    const OpenDDS::DCPS::RepoId& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos);


  // Subscription operations:

  virtual OpenDDS::DCPS::RepoId add_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataReaderCallbacks_rch subscription,
    const DDS::DataReaderQos& qos,
    const OpenDDS::DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpression,
    const DDS::StringSeq& exprParams,
    const XTypes::TypeInformation& type_info);

  virtual bool remove_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId);

  virtual bool ignore_subscription(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    const OpenDDS::DCPS::RepoId& ignoreId);

  virtual bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& partId,
    const OpenDDS::DCPS::RepoId& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos);

  virtual bool update_subscription_params(
    DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId,
    const DDS::StringSeq& params);

private:
  TransportConfig_rch bit_config();

  void removeDataReaderRemote(const RepoId& subscriptionId);

  void removeDataWriterRemote(const RepoId& publicationId);

  std::string    ior_;
  DCPSInfo_var   info_;

  /// The builtin topic transport address.
  std::string bit_transport_ip_;

  /// The builtin topic transport port number.
  int bit_transport_port_;

  bool use_local_bit_config_;
  TransportConfig_rch bit_config_;

  CORBA::ORB_var orb_;
  bool orb_from_user_;

  struct OrbRunner : ACE_Task_Base {
    OrbRunner() {}
    int svc();
    void shutdown();

    CORBA::ORB_var orb_;
#ifdef ACE_HAS_CPP11
    std::atomic<unsigned long> use_count_;
#else
    ACE_Atomic_Op<ACE_Thread_Mutex, unsigned long> use_count_;
#endif
  private:
    OrbRunner(const OrbRunner&);
    OrbRunner& operator=(const OrbRunner&);
  };

  static OrbRunner* orb_runner_;
  static ACE_Thread_Mutex mtx_orb_runner_;

  typedef OPENDDS_MAP_CMP(RepoId, DataReaderRemote_var, DCPS::GUID_tKeyLessThan) DataReaderMap;

  DataReaderMap dataReaderMap_;

  typedef OPENDDS_MAP_CMP(RepoId, DataWriterRemote_var, DCPS::GUID_tKeyLessThan) DataWriterMap;

  DataWriterMap dataWriterMap_;

  mutable ACE_Thread_Mutex lock_;

public:
  class Config : public Discovery::Config {
  public:
    int discovery_config(ACE_Configuration_Heap& cf);
  };

  class OpenDDS_InfoRepoDiscovery_Export StaticInitializer {
  public:
    StaticInitializer();
  };
};

typedef RcHandle<InfoRepoDiscovery> InfoRepoDiscovery_rch;

static InfoRepoDiscovery::StaticInitializer initialize_inforepodisco;

// Support loading this library using the ACE Service Configurator:
// this is used by TransportRegistry (from Service_Participant).

class OpenDDS_InfoRepoDiscovery_Export IRDiscoveryLoader
  : public ACE_Service_Object {
public:
  virtual int init(int argc, ACE_TCHAR* argv[]);
};

ACE_STATIC_SVC_DECLARE_EXPORT(OpenDDS_InfoRepoDiscovery, IRDiscoveryLoader)
ACE_FACTORY_DECLARE(OpenDDS_InfoRepoDiscovery, IRDiscoveryLoader)

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_INFOREPODISCOVERY_H  */
