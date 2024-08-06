#include <dds/DCPS/Discovery.h>
#include <dds/DCPS/GuidUtils.h>

#include <dds/DCPS/XTypes/TypeLookupService.h>

#include <dds/OpenDDSConfigWrapper.h>

#include <map>
#include <string>

using namespace OpenDDS::DCPS;

class LocalDiscovery : public Discovery {
public:
  LocalDiscovery();

  virtual RepoKey key() const { return "LocalDiscovery"; }

private:
  EntityId_t next_topic_id_;

  struct TopicDetails {
    TopicDetails()
      : entity_id_(ENTITYID_UNKNOWN)
      , refcount_(1)
    {}
    EntityId_t entity_id_;
    std::string type_;
    int refcount_;
  };

  std::map<std::string, TopicDetails> topics_;

  static GUID_t make_guid(EntityId_t eid);

  // Discovery implementation:

  RcHandle<BitSubscriber> init_bit(DomainParticipantImpl* participant);

  void fini_bit(DomainParticipantImpl* participant);

  bool attach_participant(
    DDS::DomainId_t domainId,
    const GUID_t& participantId);

  GUID_t generate_participant_guid();

  AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    OpenDDS::XTypes::TypeLookupService_rch tls);

#if OPENDDS_CONFIG_SECURITY
  AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    OpenDDS::XTypes::TypeLookupService_rch tls,
    const GUID_t& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto);
#endif

  bool remove_domain_participant(
    DDS::DomainId_t domainId,
    const GUID_t& participantId);

  bool ignore_domain_participant(
    DDS::DomainId_t domainId,
    const GUID_t& myParticipantId,
    const GUID_t& ignoreId);

  bool update_domain_participant_qos(
    DDS::DomainId_t domain,
    const GUID_t& participantId,
    const DDS::DomainParticipantQos& qos);

  TopicStatus assert_topic(
    GUID_t_out topicId,
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey,
    TopicCallbacks* topic_callbacks);

  TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    GUID_t_out topicId);

  TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId);

  bool ignore_topic(
    DDS::DomainId_t domainId,
    const GUID_t& myParticipantId,
    const GUID_t& ignoreId);

  bool update_topic_qos(
    const GUID_t& topicId,
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const DDS::TopicQos& qos);

  bool add_publication(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId,
    DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const OpenDDS::XTypes::TypeInformation& type_info);

  bool remove_publication(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& publicationId);

  bool ignore_publication(
    DDS::DomainId_t domainId,
    const GUID_t& myParticipantId,
    const GUID_t& ignoreId);

  bool update_publication_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos);

  bool add_subscription(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId,
    DataReaderCallbacks_rch subscription,
    const DDS::DataReaderQos& qos,
    const TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpression,
    const DDS::StringSeq& exprParams,
    const OpenDDS::XTypes::TypeInformation& type_info);

  bool remove_subscription(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& subscriptionId);

  bool ignore_subscription(
    DDS::DomainId_t domainId,
    const GUID_t& myParticipantId,
    const GUID_t& ignoreId);

  bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos);

  bool update_subscription_params(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& subscriptionId,
    const DDS::StringSeq& params);


};

