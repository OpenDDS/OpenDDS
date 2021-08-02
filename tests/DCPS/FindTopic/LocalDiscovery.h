#include <dds/DCPS/Discovery.h>
#include <dds/DCPS/GuidUtils.h>

#include <map>
#include <string>

using namespace OpenDDS::DCPS;

class LocalDiscovery : public Discovery {
public:
  LocalDiscovery();

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

  DDS::Subscriber* init_bit(DomainParticipantImpl* participant);

  void fini_bit(DomainParticipantImpl* participant);

  bool attach_participant(
    DDS::DomainId_t domainId,
    const RepoId& participantId);

  RepoId generate_participant_guid();

  AddDomainStatus add_domain_participant(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos);

#ifdef OPENDDS_SECURITY
  AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    const RepoId& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto);
#endif

  bool remove_domain_participant(
    DDS::DomainId_t domainId,
    const RepoId& participantId);

  bool ignore_domain_participant(
    DDS::DomainId_t domainId,
    const RepoId& myParticipantId,
    const RepoId& ignoreId);

  bool update_domain_participant_qos(
    DDS::DomainId_t domain,
    const RepoId& participantId,
    const DDS::DomainParticipantQos& qos);

  TopicStatus assert_topic(
    RepoId_out topicId,
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey,
    TopicCallbacks* topic_callbacks);

  TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    RepoId_out topicId);

  TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& topicId);

  bool ignore_topic(
    DDS::DomainId_t domainId,
    const RepoId& myParticipantId,
    const RepoId& ignoreId);

  bool update_topic_qos(
    const RepoId& topicId,
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const DDS::TopicQos& qos);

  RepoId add_publication(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& topicId,
    DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const OpenDDS::XTypes::TypeInformation& type_info);

  bool remove_publication(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& publicationId);

  bool ignore_publication(
    DDS::DomainId_t domainId,
    const RepoId& myParticipantId,
    const RepoId& ignoreId);

  bool update_publication_qos(
    DDS::DomainId_t domainId,
    const RepoId& partId,
    const RepoId& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos);

  RepoId add_subscription(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& topicId,
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
    const RepoId& participantId,
    const RepoId& subscriptionId);

  bool ignore_subscription(
    DDS::DomainId_t domainId,
    const RepoId& myParticipantId,
    const RepoId& ignoreId);

  bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const RepoId& partId,
    const RepoId& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos);

  bool update_subscription_params(
    DDS::DomainId_t domainId,
    const RepoId& participantId,
    const RepoId& subscriptionId,
    const DDS::StringSeq& params);


};

