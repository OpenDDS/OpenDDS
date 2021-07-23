#include "LocalDiscovery.h"

#include <dds/DCPS/GuidUtils.h>

LocalDiscovery::LocalDiscovery()
  : Discovery("LocalDiscovery")
{
  next_topic_id_.entityKey[0] = 0;
  next_topic_id_.entityKey[1] = 0;
  next_topic_id_.entityKey[2] = 0;
  next_topic_id_.entityKind = ENTITYKIND_OPENDDS_TOPIC;
}

GUID_t LocalDiscovery::make_guid(EntityId_t eid)
{
  GUID_t guid = GUID_UNKNOWN;
  guid.guidPrefix[0] = 1;
  guid.entityId = eid;
  return guid;
}

DDS::Subscriber* LocalDiscovery::init_bit(DomainParticipantImpl*)
{
  return 0;
}

void LocalDiscovery::fini_bit(DomainParticipantImpl*)
{}

bool LocalDiscovery::attach_participant(
  DDS::DomainId_t,
  const RepoId&)
{
  return true;
}

RepoId LocalDiscovery::generate_participant_guid()
{
  return GUID_UNKNOWN;
}

AddDomainStatus LocalDiscovery::add_domain_participant(
  DDS::DomainId_t,
  const DDS::DomainParticipantQos&)
{
  const AddDomainStatus ads = {make_guid(ENTITYID_PARTICIPANT), false};
  return ads;
}

#ifdef OPENDDS_SECURITY
AddDomainStatus LocalDiscovery::add_domain_participant_secure(
  DDS::DomainId_t,
  const DDS::DomainParticipantQos&,
  const RepoId&,
  DDS::Security::IdentityHandle,
  DDS::Security::PermissionsHandle,
  DDS::Security::ParticipantCryptoHandle)
{
  const AddDomainStatus ads = {GUID_UNKNOWN, false};
  return ads;
}
#endif

bool LocalDiscovery::remove_domain_participant(
  DDS::DomainId_t,
  const RepoId&)
{
  return true;
}

bool LocalDiscovery::ignore_domain_participant(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&)
{
  return true;
}

bool LocalDiscovery::update_domain_participant_qos(
  DDS::DomainId_t,
  const RepoId&,
  const DDS::DomainParticipantQos&)
{
  return true;
}

TopicStatus LocalDiscovery::assert_topic(
  RepoId_out guid,
  DDS::DomainId_t,
  const RepoId&,
  const char* topicName,
  const char* typeName,
  const DDS::TopicQos&,
  bool,
  TopicCallbacks*)
{
  const std::map<std::string, TopicDetails>::iterator iter = topics_.find(topicName);
  if (iter == topics_.end()) {
    TopicDetails& td = topics_[topicName];
    guid = make_guid(td.entity_id_ = next_topic_id_);
    ++next_topic_id_.entityKey[0];
    td.type_ = typeName;
    return CREATED;
  }
  guid = make_guid(iter->second.entity_id_);
  ++iter->second.refcount_;
  return FOUND;
}

TopicStatus LocalDiscovery::find_topic(
  DDS::DomainId_t,
  const RepoId&,
  const char* topicName,
  CORBA::String_out dataTypeName,
  DDS::TopicQos_out qos,
  RepoId_out topicId)
{
  dataTypeName = "";
  qos = new DDS::TopicQos();
  topicId = GUID_UNKNOWN;
  const std::map<std::string, TopicDetails>::iterator iter = topics_.find(topicName);
  if (iter == topics_.end()) {
    return NOT_FOUND;
  }
  topicId = make_guid(iter->second.entity_id_);
  dataTypeName = iter->second.type_.c_str();
  ++iter->second.refcount_;
  return FOUND;
}

TopicStatus LocalDiscovery::remove_topic(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId& topicId)
{
  typedef std::map<std::string, TopicDetails>::iterator iter_t;
  for (iter_t iter = topics_.begin(); iter != topics_.end(); ++iter) {
    if (iter->second.entity_id_ == topicId.entityId) {
      if (--iter->second.refcount_ == 0) {
        topics_.erase(iter);
      }
      return REMOVED;
    }
  }
  return NOT_FOUND;
}

bool LocalDiscovery::ignore_topic(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&)
{
  return true;
}

bool LocalDiscovery::update_topic_qos(
  const RepoId&,
  DDS::DomainId_t,
  const RepoId&,
  const DDS::TopicQos&)
{
  return true;
}

RepoId LocalDiscovery::add_publication(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&,
  DataWriterCallbacks_rch,
  const DDS::DataWriterQos&,
  const TransportLocatorSeq&,
  const DDS::PublisherQos&,
  const OpenDDS::XTypes::TypeInformation&)
{
  GUID_t guid = GUID_UNKNOWN;
  guid.guidPrefix[0] = 1;
  guid.entityId.entityKind = ENTITYKIND_USER_WRITER_WITH_KEY;
  return guid;
}

bool LocalDiscovery::remove_publication(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&)
{
  return true;
}

bool LocalDiscovery::ignore_publication(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&)
{
  return true;
}

bool LocalDiscovery::update_publication_qos(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&,
  const DDS::DataWriterQos&,
  const DDS::PublisherQos&)
{
  return true;
}

RepoId LocalDiscovery::add_subscription(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&,
  DataReaderCallbacks_rch,
  const DDS::DataReaderQos&,
  const TransportLocatorSeq&,
  const DDS::SubscriberQos&,
  const char*,
  const char*,
  const DDS::StringSeq&,
  const OpenDDS::XTypes::TypeInformation&)
{
  return GUID_UNKNOWN;
}

bool LocalDiscovery::remove_subscription(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&)
{
  return true;
}

bool LocalDiscovery::ignore_subscription(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&)
{
  return true;
}

bool LocalDiscovery::update_subscription_qos(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&,
  const DDS::DataReaderQos&,
  const DDS::SubscriberQos&)
{
  return true;
}

bool LocalDiscovery::update_subscription_params(
  DDS::DomainId_t,
  const RepoId&,
  const RepoId&,
  const DDS::StringSeq&)
{
  return true;
}

