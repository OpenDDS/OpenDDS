#include "LocalDiscovery.h"

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/GuidUtils.h>

#include <dds/OpenDDSConfigWrapper.h>

LocalDiscovery::LocalDiscovery()
  : Discovery()
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

RcHandle<BitSubscriber> LocalDiscovery::init_bit(DomainParticipantImpl*)
{
  return RcHandle<BitSubscriber>();
}

void LocalDiscovery::fini_bit(DomainParticipantImpl*)
{}

bool LocalDiscovery::attach_participant(
  DDS::DomainId_t,
  const GUID_t&)
{
  return true;
}

GUID_t LocalDiscovery::generate_participant_guid()
{
  return GUID_UNKNOWN;
}

AddDomainStatus LocalDiscovery::add_domain_participant(
  DDS::DomainId_t,
  const DDS::DomainParticipantQos&,
  OpenDDS::XTypes::TypeLookupService_rch)
{
  const AddDomainStatus ads = {make_guid(ENTITYID_PARTICIPANT), false};
  return ads;
}

#if OPENDDS_CONFIG_SECURITY
AddDomainStatus LocalDiscovery::add_domain_participant_secure(
  DDS::DomainId_t,
  const DDS::DomainParticipantQos&,
  OpenDDS::XTypes::TypeLookupService_rch,
  const GUID_t&,
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
  const GUID_t&)
{
  return true;
}

bool LocalDiscovery::ignore_domain_participant(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&)
{
  return true;
}

bool LocalDiscovery::update_domain_participant_qos(
  DDS::DomainId_t,
  const GUID_t&,
  const DDS::DomainParticipantQos&)
{
  return true;
}

TopicStatus LocalDiscovery::assert_topic(
  GUID_t_out guid,
  DDS::DomainId_t,
  const GUID_t&,
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
  const GUID_t&,
  const char* topicName,
  CORBA::String_out dataTypeName,
  DDS::TopicQos_out qos,
  GUID_t_out topicId)
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
  const GUID_t&,
  const GUID_t& topicId)
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
  const GUID_t&,
  const GUID_t&)
{
  return true;
}

bool LocalDiscovery::update_topic_qos(
  const GUID_t&,
  DDS::DomainId_t,
  const GUID_t&,
  const DDS::TopicQos&)
{
  return true;
}

bool LocalDiscovery::add_publication(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&,
  DataWriterCallbacks_rch writer,
  const DDS::DataWriterQos&,
  const TransportLocatorSeq&,
  const DDS::PublisherQos&,
  const OpenDDS::XTypes::TypeInformation&)
{
  GUID_t guid = GUID_UNKNOWN;
  guid.guidPrefix[0] = 1;
  guid.entityId.entityKind = ENTITYKIND_USER_WRITER_WITH_KEY;
  writer->set_publication_id(guid);
  return true;
}

bool LocalDiscovery::remove_publication(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&)
{
  return true;
}

bool LocalDiscovery::ignore_publication(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&)
{
  return true;
}

bool LocalDiscovery::update_publication_qos(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&,
  const DDS::DataWriterQos&,
  const DDS::PublisherQos&)
{
  return true;
}

bool LocalDiscovery::add_subscription(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&,
  DataReaderCallbacks_rch,
  const DDS::DataReaderQos&,
  const TransportLocatorSeq&,
  const DDS::SubscriberQos&,
  const char*,
  const char*,
  const DDS::StringSeq&,
  const OpenDDS::XTypes::TypeInformation&)
{
  return true;
}

bool LocalDiscovery::remove_subscription(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&)
{
  return true;
}

bool LocalDiscovery::ignore_subscription(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&)
{
  return true;
}

bool LocalDiscovery::update_subscription_qos(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&,
  const DDS::DataReaderQos&,
  const DDS::SubscriberQos&)
{
  return true;
}

bool LocalDiscovery::update_subscription_params(
  DDS::DomainId_t,
  const GUID_t&,
  const GUID_t&,
  const DDS::StringSeq&)
{
  return true;
}

