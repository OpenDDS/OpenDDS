#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "StaticDiscovery.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/ConfigUtils.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include <ctype.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void EndpointRegistry::match()
{
  for (WriterMapType::iterator wp = writer_map.begin(), wp_limit = writer_map.end();
       wp != wp_limit;
       ++wp) {
    const RepoId& writerid = wp->first;
    Writer& writer = wp->second;
    for (ReaderMapType::iterator rp = reader_map.begin(), rp_limit = reader_map.end();
         rp != rp_limit;
         ++rp) {
      const RepoId& readerid = rp->first;
      Reader& reader = rp->second;

      if (StaticDiscGuidDomainEqual()(readerid.guidPrefix, writerid.guidPrefix) &&
          !StaticDiscGuidPartEqual()(readerid.guidPrefix, writerid.guidPrefix) &&
          reader.topic_name == writer.topic_name) {
        // Different participants, same topic.
        IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
        IncompatibleQosStatus readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
        const TransportLocatorSeq& writer_trans_info = writer.trans_info;
        const TransportLocatorSeq& reader_trans_info = reader.trans_info;
        const DDS::DataWriterQos& writer_qos = writer.qos;
        const DDS::DataReaderQos& reader_qos = reader.qos;
        const DDS::PublisherQos& publisher_qos = writer.publisher_qos;
        const DDS::SubscriberQos& subscriber_qos = reader.subscriber_qos;

        if (compatibleQOS(&writerStatus, &readerStatus, writer_trans_info, reader_trans_info,
                          &writer_qos, &reader_qos, &publisher_qos, &subscriber_qos)) {
          switch (reader.qos.reliability.kind) {
          case DDS::BEST_EFFORT_RELIABILITY_QOS:
            writer.best_effort_readers.insert(readerid);
            reader.best_effort_writers.insert(writerid);
            break;
          case DDS::RELIABLE_RELIABILITY_QOS:
            writer.reliable_readers.insert(readerid);
            reader.reliable_writers.insert(writerid);
            break;
          }
        }
      }
    }
  }
}

StaticEndpointManager::StaticEndpointManager(const RepoId& participant_id,
                                             ACE_Thread_Mutex& lock,
                                             const EndpointRegistry& registry,
                                             StaticParticipant& participant)
  : EndpointManager<StaticDiscoveredParticipantData>(participant_id, lock)
  , registry_(registry)
  , participant_(participant)
{
  pub_bit_key_.value[0] = pub_bit_key_.value[1] = pub_bit_key_.value[2] = 0;
  sub_bit_key_.value[0] = sub_bit_key_.value[1] = sub_bit_key_.value[2] = 0;
}

void StaticEndpointManager::init_bit()
{
  // Discover all remote publications and subscriptions.

  for (EndpointRegistry::WriterMapType::const_iterator pos = registry_.writer_map.begin(),
         limit = registry_.writer_map.end();
       pos != limit;
       ++pos) {
    const RepoId& remoteid = pos->first;
    const EndpointRegistry::Writer& writer = pos->second;

    if (!GuidPrefixEqual()(participant_id_.guidPrefix, remoteid.guidPrefix)) {
      increment_key(pub_bit_key_);
      pub_key_to_id_[pub_bit_key_] = remoteid;

      // pos represents a remote.
      // Populate data.
      DDS::PublicationBuiltinTopicData data;

      data.key = pub_bit_key_;
      OPENDDS_STRING topic_name = writer.topic_name;
      data.topic_name = topic_name.c_str();
      const EndpointRegistry::Topic& topic = registry_.topic_map.find(topic_name)->second;
      data.type_name = topic.type_name.c_str();
      data.durability = writer.qos.durability;
      data.durability_service = writer.qos.durability_service;
      data.deadline = writer.qos.deadline;
      data.latency_budget = writer.qos.latency_budget;
      data.liveliness = writer.qos.liveliness;
      data.reliability = writer.qos.reliability;
      data.lifespan = writer.qos.lifespan;
      data.user_data = writer.qos.user_data;
      data.ownership = writer.qos.ownership;
      data.ownership_strength = writer.qos.ownership_strength;
      data.destination_order = writer.qos.destination_order;
      data.presentation = writer.publisher_qos.presentation;
      data.partition = writer.publisher_qos.partition;
      // If the TopicQos becomes available, this can be populated.
      //data.topic_data = topic_details.qos_.topic_data;
      data.group_data = writer.publisher_qos.group_data;
#ifndef DDS_HAS_MINIMUM_BIT
      OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
      if (bit) { // bit may be null if the DomainParticipant is shutting down
        bit->store_synthetic_data(data, DDS::NEW_VIEW_STATE);
      }
#endif /* DDS_HAS_MINIMUM_BIT */
    }
  }

  for (EndpointRegistry::ReaderMapType::const_iterator pos = registry_.reader_map.begin(),
         limit = registry_.reader_map.end();
       pos != limit;
       ++pos) {
    const RepoId& remoteid = pos->first;
    const EndpointRegistry::Reader& reader = pos->second;

    if (!GuidPrefixEqual()(participant_id_.guidPrefix, remoteid.guidPrefix)) {
      increment_key(sub_bit_key_);
      sub_key_to_id_[sub_bit_key_] = remoteid;

      // pos represents a remote.
      // Populate data.
      DDS::SubscriptionBuiltinTopicData data;

      data.key = sub_bit_key_;
      OPENDDS_STRING topic_name = reader.topic_name;
      data.topic_name = topic_name.c_str();
      const EndpointRegistry::Topic& topic = registry_.topic_map.find(topic_name)->second;
      data.type_name = topic.type_name.c_str();
      data.durability = reader.qos.durability;
      data.deadline = reader.qos.deadline;
      data.latency_budget = reader.qos.latency_budget;
      data.liveliness = reader.qos.liveliness;
      data.reliability = reader.qos.reliability;
      data.ownership = reader.qos.ownership;
      data.destination_order = reader.qos.destination_order;
      data.user_data = reader.qos.user_data;
      data.time_based_filter = reader.qos.time_based_filter;
      data.presentation = reader.subscriber_qos.presentation;
      data.partition = reader.subscriber_qos.partition;
      // // If the TopicQos becomes available, this can be populated.
      //data.topic_data = topic_details.qos_.topic_data;
      data.group_data = reader.subscriber_qos.group_data;

#ifndef DDS_HAS_MINIMUM_BIT
      OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
      if (bit) { // bit may be null if the DomainParticipant is shutting down
        bit->store_synthetic_data(data, DDS::NEW_VIEW_STATE);
      }
#endif /* DDS_HAS_MINIMUM_BIT */
    }
  }
}

void StaticEndpointManager::assign_publication_key(RepoId& rid,
                                                   const RepoId& /*topicId*/,
                                                   const DDS::DataWriterQos& qos)
{
  if (qos.user_data.value.length() != 3) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_publication_key: no user data to identify writer\n")));
    return;
  }

  rid.entityId.entityKey[0] = qos.user_data.value[0];
  rid.entityId.entityKey[1] = qos.user_data.value[1];
  rid.entityId.entityKey[2] = qos.user_data.value[2];
  rid.entityId.entityKind = ENTITYKIND_USER_WRITER_WITH_KEY;

  if (DCPS_debug_level > 8) {
    ACE_DEBUG((LM_INFO, "(%P|%t) looking up writer ID %s\n",
               LogGuid(rid).c_str()));
  }

  EndpointRegistry::WriterMapType::const_iterator pos = registry_.writer_map.find(rid);
  if (pos == registry_.writer_map.end()) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: StaticEndpointManager::assign_publication_key: unknown writer: %s\n"), LogGuid(rid).c_str()));
    return;
  }

  DDS::DataWriterQos qos2(qos);
  // Qos in registry will not have the user data so overwrite.
  qos2.user_data = pos->second.qos.user_data;

  DDS::DataWriterQos qos3(pos->second.qos);

  if (qos2 != qos3) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) WARNING: StaticEndpointManager::assign_publication_key: dynamic and static QoS differ\n")));
  }
}

void StaticEndpointManager::assign_subscription_key(RepoId& rid,
                                                    const RepoId& /*topicId*/,
                                                    const DDS::DataReaderQos& qos)
{
  if (qos.user_data.value.length() != 3) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_subscription_key: no user data to identify reader\n")));
    return;
  }

  rid.entityId.entityKey[0] = qos.user_data.value[0];
  rid.entityId.entityKey[1] = qos.user_data.value[1];
  rid.entityId.entityKey[2] = qos.user_data.value[2];
  rid.entityId.entityKind = ENTITYKIND_USER_READER_WITH_KEY;

  EndpointRegistry::ReaderMapType::const_iterator pos = registry_.reader_map.find(rid);
  if (pos == registry_.reader_map.end()) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: StaticEndpointManager::assign_subscription_key: unknown reader: %s\n"), LogGuid(rid).c_str()));
    return;
  }

  DDS::DataReaderQos qos2(qos);
  // Qos in registry will not have the user data so overwrite.
  qos2.user_data = pos->second.qos.user_data;

  if (qos2 != pos->second.qos) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) WARNING: StaticEndpointManager::assign_subscription_key: dynamic and static QoS differ\n")));
  }
}

bool
StaticEndpointManager::update_topic_qos(const RepoId& /*topicId*/,
                                        const DDS::TopicQos& /*qos*/,
                                        OPENDDS_STRING& /*name*/)
{
  ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::update_topic_qos - ")
             ACE_TEXT("Not allowed\n")));
  return false;
}

bool
StaticEndpointManager::update_publication_qos(const RepoId& /*publicationId*/,
                                              const DDS::DataWriterQos& /*qos*/,
                                              const DDS::PublisherQos& /*publisherQos*/)
{
  ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::update_publication_qos - ")
             ACE_TEXT("Not allowed\n")));
  return false;
}

bool
StaticEndpointManager::update_subscription_qos(const RepoId& /*subscriptionId*/,
                                               const DDS::DataReaderQos& /*qos*/,
                                               const DDS::SubscriberQos& /*subscriberQos*/)
{
  ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::update_subscription_qos - ")
             ACE_TEXT("Not allowed\n")));
  return false;
}

bool
StaticEndpointManager::update_subscription_params(const RepoId& /*subId*/,
                                                  const DDS::StringSeq& /*params*/)
{
  ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::update_subscription_qos - ")
             ACE_TEXT("Not allowed\n")));
  return false;
}

void
StaticEndpointManager::association_complete(const RepoId& /*localId*/,
                                            const RepoId& /*remoteId*/)
{
  // Do nothing.
}

bool
StaticEndpointManager::disassociate(const StaticDiscoveredParticipantData& /*pdata*/)
{
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::disassociate TODO\n")));
  // TODO
  return false;
}

DDS::ReturnCode_t
StaticEndpointManager::add_publication_i(const RepoId& writerid,
                                         LocalPublication& pub)
{
  /*
    Find all matching remote readers.
    If the reader is best effort, then associate immediately.
    If the reader is reliable (we are reliable by implication), register with the transport to receive notification that the remote reader is up.
    */
  EndpointRegistry::WriterMapType::const_iterator pos = registry_.writer_map.find(writerid);
  if (pos == registry_.writer_map.end()) {
    return DDS::RETCODE_ERROR;
  }
  const EndpointRegistry::Writer& writer = pos->second;

  for (RepoIdSet::const_iterator pos = writer.best_effort_readers.begin(), limit = writer.best_effort_readers.end();
       pos != limit;
       ++pos) {
    const RepoId& readerid = *pos;
    const EndpointRegistry::Reader& reader = registry_.reader_map.find(readerid)->second;

#ifdef __SUNPRO_CC
    ReaderAssociation ra;
    ra.readerTransInfo = reader.trans_info;
    ra.readerId = readerid;
    ra.subQos = reader.subscriber_qos;
    ra.readerQos = reader.qos;
    ra.filterClassName = "";
    ra.filterExpression = "";
    ra.exprParams = 0;
#else
    const ReaderAssociation ra =
      {reader.trans_info, readerid, reader.subscriber_qos, reader.qos, "", "", 0};
#endif
    pub.publication_->add_association(writerid, ra, true);
    pub.publication_->association_complete(readerid);
  }

  for (RepoIdSet::const_iterator pos = writer.reliable_readers.begin(), limit = writer.reliable_readers.end();
       pos != limit;
       ++pos) {
    const RepoId& readerid = *pos;
    const EndpointRegistry::Reader& reader = registry_.reader_map.find(readerid)->second;
    pub.publication_->register_for_reader(participant_id_, writerid, readerid, reader.trans_info, this);
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
StaticEndpointManager::remove_publication_i(const RepoId& writerid)
{
  LocalPublicationMap::const_iterator lp_pos = local_publications_.find(writerid);
  if (lp_pos == local_publications_.end()) {
    return DDS::RETCODE_ERROR;
  }

  const LocalPublication& pub = lp_pos->second;

  EndpointRegistry::WriterMapType::const_iterator pos = registry_.writer_map.find(writerid);
  if (pos == registry_.writer_map.end()) {
    return DDS::RETCODE_ERROR;
  }

  const EndpointRegistry::Writer& writer = pos->second;

  ReaderIdSeq ids;
  ids.length((CORBA::ULong)writer.reliable_readers.size());
  CORBA::ULong idx = 0;
  for (RepoIdSet::const_iterator pos = writer.reliable_readers.begin(), limit = writer.reliable_readers.end();
        pos != limit;
        ++pos, ++idx) {
    const RepoId& readerid = *pos;
    ids[idx] = readerid;
    pub.publication_->unregister_for_reader(participant_id_, writerid, readerid);
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
StaticEndpointManager::add_subscription_i(const RepoId& readerid,
                                          LocalSubscription& sub)
{
  /*
    Find all matching remote writers.
    If we (the reader) is best effort, then associate immediately.
    If we (the reader) are reliable, then register with the transport to receive notification that the remote writer is up.
    */
  EndpointRegistry::ReaderMapType::const_iterator pos = registry_.reader_map.find(readerid);
  if (pos == registry_.reader_map.end()) {
    return DDS::RETCODE_ERROR;
  }
  const EndpointRegistry::Reader& reader = pos->second;

  for (RepoIdSet::const_iterator pos = reader.best_effort_writers.begin(), limit = reader.best_effort_writers.end();
       pos != limit;
       ++pos) {
    const RepoId& writerid = *pos;
    const EndpointRegistry::Writer& writer = registry_.writer_map.find(writerid)->second;

#ifdef __SUNPRO_CC
    WriterAssociation wa;
    wa.writerTransInfo = writer.trans_info;
    wa.writerId = writerid;
    wa.pubQos = writer.publisher_qos;
    wa.writerQos = writer.qos;
#else
    const WriterAssociation wa =
      {writer.trans_info, writerid, writer.publisher_qos, writer.qos};
#endif
    sub.subscription_->add_association(readerid, wa, false);
  }

  for (RepoIdSet::const_iterator pos = reader.reliable_writers.begin(), limit = reader.reliable_writers.end();
       pos != limit;
       ++pos) {
    const RepoId& writerid = *pos;
    const EndpointRegistry::Writer& writer = registry_.writer_map.find(writerid)->second;
    sub.subscription_->register_for_writer(participant_id_, readerid, writerid, writer.trans_info, this);
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
StaticEndpointManager::remove_subscription_i(const RepoId& readerid)
{
  LocalSubscriptionMap::const_iterator ls_pos = local_subscriptions_.find(readerid);
  if (ls_pos == local_subscriptions_.end()) {
    return DDS::RETCODE_ERROR;
  }

  const LocalSubscription& sub = ls_pos->second;

  EndpointRegistry::ReaderMapType::const_iterator pos = registry_.reader_map.find(readerid);
  if (pos == registry_.reader_map.end()) {
    return DDS::RETCODE_ERROR;
  }

  const EndpointRegistry::Reader& reader = pos->second;

  WriterIdSeq ids;
  ids.length((CORBA::ULong)reader.reliable_writers.size());
  CORBA::ULong idx = 0;
  for (RepoIdSet::const_iterator pos = reader.reliable_writers.begin(), limit = reader.reliable_writers.end();
        pos != limit;
        ++pos, ++idx) {
    const RepoId& writerid = *pos;
    ids[idx] = writerid;
    sub.subscription_->unregister_for_writer(participant_id_, readerid, writerid);
  }

  return DDS::RETCODE_OK;
}

bool
StaticEndpointManager::shutting_down() const
{
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::shutting_down TODO\n")));
  // TODO
  return false;
}

void
StaticEndpointManager::populate_transport_locator_sequence(TransportLocatorSeq*& /*tls*/,
                                                           DiscoveredSubscriptionIter& /*iter*/,
                                                           const RepoId& /*reader*/)
{
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::populate_transport_locator_sequence TODO\n")));
  // TODO
}

void
StaticEndpointManager::populate_transport_locator_sequence(TransportLocatorSeq*& /*tls*/,
                                                           DiscoveredPublicationIter& /*iter*/,
                                                           const RepoId& /*reader*/)
{
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::populate_transport_locator_sequence TODO\n")));
  // TODO
}

bool
StaticEndpointManager::defer_writer(const RepoId& /*writer*/,
                                    const RepoId& /*writer_participant*/)
{
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::defer_writer TODO\n")));
  // TODO
  return false;
}

bool
StaticEndpointManager::defer_reader(const RepoId& /*writer*/,
                                    const RepoId& /*writer_participant*/)
{
  // TODO
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::defer_reader TODO\n")));
  return false;
}

void
StaticEndpointManager::reader_exists(const RepoId& readerid, const RepoId& writerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationMap::const_iterator lp_pos = local_publications_.find(writerid);
  EndpointRegistry::ReaderMapType::const_iterator reader_pos = registry_.reader_map.find(readerid);
  if (lp_pos != local_publications_.end() &&
      reader_pos != registry_.reader_map.end()) {
    DataWriterCallbacks* dwr = lp_pos->second.publication_;
#ifdef __SUNPRO_CC
    ReaderAssociation ra;
    ra.readerTransInfo = reader_pos->second.trans_info;
    ra.readerId = readerid;
    ra.subQos = reader_pos->second.subscriber_qos;
    ra.readerQos = reader_pos->second.qos;
    ra.filterClassName = "";
    ra.filterExpression = "";
    ra.exprParams = 0;
#else
    const ReaderAssociation ra =
      {reader_pos->second.trans_info, readerid, reader_pos->second.subscriber_qos, reader_pos->second.qos, "", "", 0};

#endif
    dwr->add_association(writerid, ra, true);
    dwr->association_complete(readerid);
  }
}

void
StaticEndpointManager::reader_does_not_exist(const RepoId& readerid, const RepoId& writerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationMap::const_iterator lp_pos = local_publications_.find(writerid);
  EndpointRegistry::ReaderMapType::const_iterator reader_pos = registry_.reader_map.find(readerid);
  if (lp_pos != local_publications_.end() &&
      reader_pos != registry_.reader_map.end()) {
    DataWriterCallbacks* dwr = lp_pos->second.publication_;
    ReaderIdSeq ids;
    ids.length(1);
    ids[0] = readerid;
    dwr->remove_associations(ids, true);
  }
}

void
StaticEndpointManager::writer_exists(const RepoId& writerid, const RepoId& readerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionMap::const_iterator ls_pos = local_subscriptions_.find(readerid);
  EndpointRegistry::WriterMapType::const_iterator writer_pos = registry_.writer_map.find(writerid);
  if (ls_pos != local_subscriptions_.end() &&
      writer_pos != registry_.writer_map.end()) {
    DataReaderCallbacks* drr = ls_pos->second.subscription_;
#ifdef __SUNPRO_CC
    WriterAssociation wa;
    wa.writerTransInfo = writer_pos->second.trans_info;
    wa.writerId = writerid;
    wa.pubQos = writer_pos->second.publisher_qos;
    wa.writerQos = writer_pos->second.qos;
#else
    const WriterAssociation wa =
      {writer_pos->second.trans_info, writerid, writer_pos->second.publisher_qos, writer_pos->second.qos};
#endif
    drr->add_association(readerid, wa, false);
  }
}

void
StaticEndpointManager::writer_does_not_exist(const RepoId& writerid, const RepoId& readerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionMap::const_iterator ls_pos = local_subscriptions_.find(readerid);
  EndpointRegistry::WriterMapType::const_iterator writer_pos = registry_.writer_map.find(writerid);
  if (ls_pos != local_subscriptions_.end() &&
      writer_pos != registry_.writer_map.end()) {
    DataReaderCallbacks* drr = ls_pos->second.subscription_;
    WriterIdSeq ids;
    ids.length(1);
    ids[0] = writerid;
    drr->remove_associations(ids, true);
  }
}

#ifndef DDS_HAS_MINIMUM_BIT
OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl*
StaticEndpointManager::pub_bit()
{
  DDS::Subscriber_var sub = participant_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d = sub->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);
  return dynamic_cast<OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl*>(d.in());
}

OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl*
StaticEndpointManager::sub_bit()
{
  DDS::Subscriber_var sub = participant_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d = sub->lookup_datareader(BUILT_IN_SUBSCRIPTION_TOPIC);
  return dynamic_cast<OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl*>(d.in());
}
#endif /* DDS_HAS_MINIMUM_BIT */

StaticDiscovery::StaticDiscovery(const RepoKey& key)
  : PeerDiscovery<StaticParticipant>(key)
{}

namespace {
  unsigned char hextobyte(unsigned char c)
  {
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
      return 10 + c - 'a';
    }
    if (c >= 'A' && c <= 'F') {
      return 10 + c - 'A';
    }
    return c;
  }

  unsigned char
  fromhex(const OPENDDS_STRING& x, size_t idx)
  {
    return (hextobyte(x[idx * 2]) << 4) | (hextobyte(x[idx * 2 + 1]));
  }
}

EntityId_t
EndpointRegistry::build_id(const unsigned char* entity_key,
                           const unsigned char entity_kind)
{
  EntityId_t retval;
  retval.entityKey[0] = entity_key[0];
  retval.entityKey[1] = entity_key[1];
  retval.entityKey[2] = entity_key[2];
  retval.entityKind = entity_kind;
  return retval;
}

RepoId
EndpointRegistry::build_id(DDS::DomainId_t domain,
                           const unsigned char* participant_id,
                           const EntityId_t& entity_id)
{
  RepoId id;
  id.guidPrefix[0] = VENDORID_OCI[0];
  id.guidPrefix[1] = VENDORID_OCI[1];
  // id.guidPrefix[2] = domain[0]
  // id.guidPrefix[3] = domain[1]
  // id.guidPrefix[4] = domain[2]
  // id.guidPrefix[5] = domain[3]
  DDS::DomainId_t netdom = ACE_HTONL(domain);
  ACE_OS::memcpy(&id.guidPrefix[2], &netdom, sizeof(DDS::DomainId_t));
  // id.guidPrefix[6] = participant[0]
  // id.guidPrefix[7] = participant[1]
  // id.guidPrefix[8] = participant[2]
  // id.guidPrefix[9] = participant[3]
  // id.guidPrefix[10] = participant[4]
  // id.guidPrefix[11] = participant[5]
  ACE_OS::memcpy(&id.guidPrefix[6], participant_id, 6);
  id.entityId = entity_id;
  return id;
}

AddDomainStatus
StaticDiscovery::add_domain_participant(DDS::DomainId_t domain,
                                        const DDS::DomainParticipantQos& qos)
{
  AddDomainStatus ads = {RepoId(), false /*federated*/};

  if (qos.user_data.value.length() != 6) {
    ACE_DEBUG((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::add_domain_participant ")
                ACE_TEXT("No userdata to identify participant\n")));
    return ads;
  }

  RepoId id = EndpointRegistry::build_id(domain,
                                         qos.user_data.value.get_buffer(),
                                         ENTITYID_PARTICIPANT);
  if (!get_part(domain, id).is_nil()) {
    ACE_DEBUG((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::add_domain_participant ")
                ACE_TEXT("Duplicate participant\n")));
    return ads;
  }

  const RcHandle<StaticParticipant> participant (make_rch<StaticParticipant>(ref(id), qos, registry));

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ads);
    participants_[domain][id] = participant;
  }

  ads.id = id;
  return ads;
}

namespace {
  const ACE_TCHAR TOPIC_SECTION_NAME[] = ACE_TEXT("topic");
  const ACE_TCHAR DATAWRITERQOS_SECTION_NAME[] = ACE_TEXT("datawriterqos");
  const ACE_TCHAR DATAREADERQOS_SECTION_NAME[] = ACE_TEXT("datareaderqos");
  const ACE_TCHAR PUBLISHERQOS_SECTION_NAME[]  = ACE_TEXT("publisherqos");
  const ACE_TCHAR SUBSCRIBERQOS_SECTION_NAME[] = ACE_TEXT("subscriberqos");
  const ACE_TCHAR ENDPOINT_SECTION_NAME[] = ACE_TEXT("endpoint");

  void parse_second(CORBA::Long& x, const OPENDDS_STRING& value)
  {
    if (value == "DURATION_INFINITE_SEC") {
      x = DDS::DURATION_INFINITE_SEC;
    } else {
      x = atoi(value.c_str());
    }
  }

  void parse_nanosecond(CORBA::ULong& x, const OPENDDS_STRING& value)
  {
    if (value == "DURATION_INFINITE_NANOSEC") {
      x = DDS::DURATION_INFINITE_NSEC;
    } else {
      x = atoi(value.c_str());
    }
  }

  bool parse_bool(CORBA::Boolean& x, const OPENDDS_STRING& value)
  {
    if (value == "true") {
      x = true;
      return true;
    } else if (value == "false") {
      x = false;
      return true;
    }
    return false;
  }

  void parse_list(DDS::PartitionQosPolicy& x, const OPENDDS_STRING& value)
  {
    // Value can be a comma-separated list
    const char* start = value.c_str();
    char buffer[128];
    std::memset(buffer, 0, sizeof(buffer));
    while (const char* next_comma = std::strchr(start, ',')) {
      // Copy into temp buffer, won't have null
      std::strncpy(buffer, start, next_comma - start);
      // Append null
      buffer[next_comma - start] = '\0';
      // Add to QOS
      x.name.length(x.name.length() + 1);
      x.name[x.name.length() - 1] = static_cast<const char*>(buffer);
      // Advance pointer
      start = next_comma + 1;
    }
    // Append everything after last comma
    x.name.length(x.name.length() + 1);
    x.name[x.name.length() - 1] = start;
  }
}

int
StaticDiscovery::load_configuration(ACE_Configuration_Heap& cf)
{
  if (parse_topics(cf) ||
      parse_datawriterqos(cf) ||
      parse_datareaderqos(cf) ||
      parse_publisherqos(cf) ||
      parse_subscriberqos(cf) ||
      parse_endpoints(cf)) {
    return -1;
  }

  registry.match();

  return 0;
}

int
StaticDiscovery::parse_topics(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key section;

  if (cf.open_section(root, TOPIC_SECTION_NAME, 0, section) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any topic (sub)section.
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_topics ")
                  ACE_TEXT("no [%s] sections.\n"),
                  TOPIC_SECTION_NAME));
    }
    return 0;
  }

  // Ensure there are no key/values in the [topic] section.
  // Every key/value must be in a [topic/*] sub-section.
  ValueMap vm;
  if (pullValues(cf, section, vm) > 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                      ACE_TEXT("[topic] sections must have a subsection name\n")),
                      -1);
  }
  // Process the subsections of this section
  KeyList keys;
  if (processSections(cf, section, keys) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                      ACE_TEXT("too many nesting layers in the [topic] section.\n")),
                      -1);
  }

  // Loop through the [topic/*] sections
  for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
    OPENDDS_STRING topic_name = it->first;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_topics ")
                  ACE_TEXT("processing [topic/%C] section.\n"),
                  topic_name.c_str()));
    }

    ValueMap values;
    pullValues(cf, it->second, values);

    EndpointRegistry::Topic topic;
    bool name_Specified = false,
      type_name_Specified = false;

    for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
      OPENDDS_STRING name = it->first;
      OPENDDS_STRING value = it->second;

      if (name == "name") {
        topic.name = value;
        name_Specified = true;
      } else if (name == "type_name") {
        if (value.size() >= 128) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                            ACE_TEXT("type_name (%C) must be less than 128 characters in [topic/%C] section.\n"),
                            value.c_str(), topic_name.c_str()),
                            -1);
        }
        topic.type_name = value;
        type_name_Specified = true;
      } else {
        // Typos are ignored to avoid parsing FACE-specific keys.
      }
    }

    if (!name_Specified) {
      topic.name = topic_name;
    }

    if (!type_name_Specified) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                        ACE_TEXT("No type_name specified for [topic/%C] section.\n"),
                        topic_name.c_str()),
                       -1);
    }

    registry.topic_map[topic_name] = topic;
  }

  return 0;
}

int
StaticDiscovery::parse_datawriterqos(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key section;

  if (cf.open_section(root, DATAWRITERQOS_SECTION_NAME, 0, section) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any datawriterqos (sub)section.
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_datawriterqos ")
                  ACE_TEXT("no [%s] sections.\n"),
                  DATAWRITERQOS_SECTION_NAME));
    }
    return 0;
  }

  // Ensure there are no key/values in the [datawriterqos] section.
  // Every key/value must be in a [datawriterqos/*] sub-section.
  ValueMap vm;
  if (pullValues(cf, section, vm) > 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                      ACE_TEXT("[datawriterqos] sections must have a subsection name\n")),
                      -1);
  }
  // Process the subsections of this section
  KeyList keys;
  if (processSections(cf, section, keys) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                      ACE_TEXT("too many nesting layers in the [datawriterqos] section.\n")),
                      -1);
  }

  // Loop through the [datawriterqos/*] sections
  for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
    OPENDDS_STRING datawriterqos_name = it->first;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_datawriterqos ")
                  ACE_TEXT("processing [datawriterqos/%C] section.\n"),
                  datawriterqos_name.c_str()));
    }

    ValueMap values;
    pullValues(cf, it->second, values);

    DDS::DataWriterQos datawriterqos(TheServiceParticipant->initial_DataWriterQos());

    for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
      OPENDDS_STRING name = it->first;
      OPENDDS_STRING value = it->second;

      if (name == "durability.kind") {
        if (value == "VOLATILE") {
          datawriterqos.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
        } else if (value == "TRANSIENT_LOCAL") {
          datawriterqos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
        } else if (value == "TRANSIENT") {
          datawriterqos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
        } else if (value == "PERSISTENT") {
          datawriterqos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
#endif
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                            ACE_TEXT("Illegal value for durability.kind (%C) in [datawriterqos/%C] section.\n"),
                            value.c_str(), datawriterqos_name.c_str()),
                            -1);
        }
      } else if (name == "deadline.period.sec") {
        parse_second(datawriterqos.deadline.period.sec, value);
      } else if (name == "deadline.period.nanosec") {
        parse_nanosecond(datawriterqos.deadline.period.nanosec, value);
      } else if (name == "latency_budget.duration.sec") {
        parse_second(datawriterqos.latency_budget.duration.sec, value);
      } else if (name == "latency_budget.duration.nanosec") {
        parse_nanosecond(datawriterqos.latency_budget.duration.nanosec, value);
      } else if (name == "liveliness.kind") {
        if (value == "AUTOMATIC") {
          datawriterqos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
        } else if (value == "MANUAL_BY_TOPIC") {
          datawriterqos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
        } else if (value == "MANUAL_BY_PARTICIPANT") {
          datawriterqos.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                            ACE_TEXT("Illegal value for liveliness.kind (%C) in [datawriterqos/%C] section.\n"),
                            value.c_str(), datawriterqos_name.c_str()),
                            -1);
        }
      } else if (name == "liveliness.lease_duration.sec") {
        parse_second(datawriterqos.liveliness.lease_duration.sec, value);
      } else if (name == "liveliness.lease_duration.nanosec") {
        parse_nanosecond(datawriterqos.liveliness.lease_duration.nanosec, value);
      } else if (name == "reliability.kind") {
        if (value == "BEST_EFFORT") {
          datawriterqos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
        } else if (value == "RELIABLE") {
          datawriterqos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                            ACE_TEXT("Illegal value for reliability.kind (%C) in [datawriterqos/%C] section.\n"),
                            value.c_str(), datawriterqos_name.c_str()),
                            -1);
        }
      } else if (name == "reliability.max_blocking_time.sec") {
        parse_second(datawriterqos.reliability.max_blocking_time.sec, value);
      } else if (name == "reliability.max_blocking_time.nanosec") {
        parse_nanosecond(datawriterqos.reliability.max_blocking_time.nanosec, value);
      } else if (name == "destination_order.kind") {
        if (value == "BY_RECEPTION_TIMESTAMP") {
          datawriterqos.destination_order.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
        } else if (value == "BY_SOURCE_TIMESTAMP") {
          datawriterqos.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                            ACE_TEXT("Illegal value for destination_order.kind (%C) in [datawriterqos/%C] section.\n"),
                            value.c_str(), datawriterqos_name.c_str()),
                            -1);
        }
      } else if (name == "history.kind") {
        if (value == "KEEP_ALL") {
          datawriterqos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        } else if (value == "KEEP_LAST") {
          datawriterqos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                            ACE_TEXT("Illegal value for history.kind (%C) in [datawriterqos/%C] section.\n"),
                            value.c_str(), datawriterqos_name.c_str()),
                            -1);
        }
      } else if (name == "history.depth") {
        datawriterqos.history.depth = atoi(value.c_str());
      } else if (name == "resource_limits.max_samples") {
        datawriterqos.resource_limits.max_samples = atoi(value.c_str());
      } else if (name == "resource_limits.max_instances") {
        datawriterqos.resource_limits.max_instances = atoi(value.c_str());
      } else if (name == "resource_limits.max_samples_per_instance") {
        datawriterqos.resource_limits.max_samples_per_instance = atoi(value.c_str());
      } else if (name == "transport_priority.value") {
        datawriterqos.transport_priority.value = atoi(value.c_str());
      } else if (name == "lifespan.duration.sec") {
        parse_second(datawriterqos.lifespan.duration.sec, value);
      } else if (name == "lifespan.duration.nanosec") {
        parse_nanosecond(datawriterqos.lifespan.duration.nanosec, value);
      } else if (name == "ownership.kind") {
        if (value == "SHARED") {
          datawriterqos.ownership.kind = DDS::SHARED_OWNERSHIP_QOS;
        } else if (value == "EXCLUSIVE") {
          datawriterqos.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                            ACE_TEXT("Illegal value for ownership.kind (%C) in [datawriterqos/%C] section.\n"),
                            value.c_str(), datawriterqos_name.c_str()),
                            -1);
        }
      } else if (name == "ownership_strength.value") {
        datawriterqos.ownership_strength.value = atoi(value.c_str());
      } else {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_datawriterqos ")
                          ACE_TEXT("Unexpected entry (%C) in [datawriterqos/%C] section.\n"),
                          name.c_str(), datawriterqos_name.c_str()),
                          -1);
      }
    }

    registry.datawriterqos_map[datawriterqos_name] = datawriterqos;
  }

  return 0;
}

int
StaticDiscovery::parse_datareaderqos(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key section;

  if (cf.open_section(root, DATAREADERQOS_SECTION_NAME, 0, section) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any datareaderqos (sub)section.
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_datareaderqos ")
                  ACE_TEXT("no [%s] sections.\n"),
                  DATAREADERQOS_SECTION_NAME));
    }
    return 0;
  }

  // Ensure there are no key/values in the [datareaderqos] section.
  // Every key/value must be in a [datareaderqos/*] sub-section.
  ValueMap vm;
  if (pullValues(cf, section, vm) > 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                      ACE_TEXT("[datareaderqos] sections must have a subsection name\n")),
                      -1);
  }
  // Process the subsections of this section
  KeyList keys;
  if (processSections(cf, section, keys) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                      ACE_TEXT("too many nesting layers in the [datareaderqos] section.\n")),
                      -1);
  }

  // Loop through the [datareaderqos/*] sections
  for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
    OPENDDS_STRING datareaderqos_name = it->first;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_datareaderqos ")
                  ACE_TEXT("processing [datareaderqos/%C] section.\n"),
                  datareaderqos_name.c_str()));
    }

    ValueMap values;
    pullValues(cf, it->second, values);

    DDS::DataReaderQos datareaderqos(TheServiceParticipant->initial_DataReaderQos());

    for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
      OPENDDS_STRING name = it->first;
      OPENDDS_STRING value = it->second;

      if (name == "durability.kind") {
        if (value == "VOLATILE") {
          datareaderqos.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
        } else if (value == "TRANSIENT_LOCAL") {
          datareaderqos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
        } else if (value == "TRANSIENT") {
          datareaderqos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
        } else if (value == "PERSISTENT") {
          datareaderqos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
#endif
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                            ACE_TEXT("Illegal value for durability.kind (%C) in [datareaderqos/%C] section.\n"),
                            value.c_str(), datareaderqos_name.c_str()),
                            -1);
        }
      } else if (name == "deadline.period.sec") {
        parse_second(datareaderqos.deadline.period.sec, value);
      } else if (name == "deadline.period.nanosec") {
        parse_nanosecond(datareaderqos.deadline.period.nanosec, value);
      } else if (name == "latency_budget.duration.sec") {
        parse_second(datareaderqos.latency_budget.duration.sec, value);
      } else if (name == "latency_budget.duration.nanosec") {
        parse_nanosecond(datareaderqos.latency_budget.duration.nanosec, value);
      } else if (name == "liveliness.kind") {
        if (value == "AUTOMATIC") {
          datareaderqos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
        } else if (value == "MANUAL_BY_TOPIC") {
          datareaderqos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
        } else if (value == "MANUAL_BY_PARTICIPANT") {
          datareaderqos.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                            ACE_TEXT("Illegal value for liveliness.kind (%C) in [datareaderqos/%C] section.\n"),
                            value.c_str(), datareaderqos_name.c_str()),
                            -1);
        }
      } else if (name == "liveliness.lease_duration.sec") {
        parse_second(datareaderqos.liveliness.lease_duration.sec, value);
      } else if (name == "liveliness.lease_duration.nanosec") {
        parse_nanosecond(datareaderqos.liveliness.lease_duration.nanosec, value);
      } else if (name == "reliability.kind") {
        if (value == "BEST_EFFORT") {
          datareaderqos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
        } else if (value == "RELIABLE") {
          datareaderqos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                            ACE_TEXT("Illegal value for reliability.kind (%C) in [datareaderqos/%C] section.\n"),
                            value.c_str(), datareaderqos_name.c_str()),
                            -1);
        }
      } else if (name == "reliability.max_blocking_time.sec") {
        parse_second(datareaderqos.reliability.max_blocking_time.sec, value);
      } else if (name == "reliability.max_blocking_time.nanosec") {
        parse_nanosecond(datareaderqos.reliability.max_blocking_time.nanosec, value);
      } else if (name == "destination_order.kind") {
        if (value == "BY_RECEPTION_TIMESTAMP") {
          datareaderqos.destination_order.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
        } else if (value == "BY_SOURCE_TIMESTAMP") {
          datareaderqos.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                            ACE_TEXT("Illegal value for destination_order.kind (%C) in [datareaderqos/%C] section.\n"),
                            value.c_str(), datareaderqos_name.c_str()),
                            -1);
        }
      } else if (name == "history.kind") {
        if (value == "KEEP_ALL") {
          datareaderqos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        } else if (value == "KEEP_LAST") {
          datareaderqos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                            ACE_TEXT("Illegal value for history.kind (%C) in [datareaderqos/%C] section.\n"),
                            value.c_str(), datareaderqos_name.c_str()),
                            -1);
        }
      } else if (name == "history.depth") {
        datareaderqos.history.depth = atoi(value.c_str());
      } else if (name == "resource_limits.max_samples") {
        datareaderqos.resource_limits.max_samples = atoi(value.c_str());
      } else if (name == "resource_limits.max_instances") {
        datareaderqos.resource_limits.max_instances = atoi(value.c_str());
      } else if (name == "resource_limits.max_samples_per_instance") {
        datareaderqos.resource_limits.max_samples_per_instance = atoi(value.c_str());
      } else if (name == "time_based_filter.minimum_separation.sec") {
        parse_second(datareaderqos.time_based_filter.minimum_separation.sec, value);
      } else if (name == "time_based_filter.minimum_separation.nanosec") {
        parse_nanosecond(datareaderqos.time_based_filter.minimum_separation.nanosec, value);
      } else if (name == "reader_data_lifecycle.autopurge_nowriter_samples_delay.sec") {
        parse_second(datareaderqos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec, value);
      } else if (name == "reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec") {
        parse_nanosecond(datareaderqos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec, value);
      } else if (name == "reader_data_lifecycle.autopurge_disposed_samples_delay.sec") {
        parse_second(datareaderqos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec, value);
      } else if (name == "reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec") {
        parse_nanosecond(datareaderqos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec, value);
      } else {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_datareaderqos ")
                          ACE_TEXT("Unexpected entry (%C) in [datareaderqos/%C] section.\n"),
                          name.c_str(), datareaderqos_name.c_str()),
                          -1);
      }
    }

    registry.datareaderqos_map[datareaderqos_name] = datareaderqos;
  }

  return 0;
}

int
StaticDiscovery::parse_publisherqos(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key section;

  if (cf.open_section(root, PUBLISHERQOS_SECTION_NAME, 0, section) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any publisherqos (sub)section.
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_publisherqos ")
                  ACE_TEXT("no [%s] sections.\n"),
                  PUBLISHERQOS_SECTION_NAME));
    }
    return 0;
  }

  // Ensure there are no key/values in the [publisherqos] section.
  // Every key/value must be in a [publisherqos/*] sub-section.
  ValueMap vm;
  if (pullValues(cf, section, vm) > 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                      ACE_TEXT("[publisherqos] sections must have a subsection name\n")),
                      -1);
  }
  // Process the subsections of this section
  KeyList keys;
  if (processSections(cf, section, keys) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                      ACE_TEXT("too many nesting layers in the [publisherqos] section.\n")),
                      -1);
  }

  // Loop through the [publisherqos/*] sections
  for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
    OPENDDS_STRING publisherqos_name = it->first;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_publisherqos ")
                  ACE_TEXT("processing [publisherqos/%C] section.\n"),
                  publisherqos_name.c_str()));
    }

    ValueMap values;
    pullValues(cf, it->second, values);

    DDS::PublisherQos publisherqos(TheServiceParticipant->initial_PublisherQos());

    for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
      OPENDDS_STRING name = it->first;
      OPENDDS_STRING value = it->second;

      if (name == "presentation.access_scope") {
        if (value == "INSTANCE") {
          publisherqos.presentation.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
        } else if (value == "TOPIC") {
          publisherqos.presentation.access_scope = DDS::TOPIC_PRESENTATION_QOS;
        } else if (value == "GROUP") {
          publisherqos.presentation.access_scope = DDS::GROUP_PRESENTATION_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                            ACE_TEXT("Illegal value for presentation.access_scope (%C) in [publisherqos/%C] section.\n"),
                            value.c_str(), publisherqos_name.c_str()),
                            -1);
        }
      } else if (name == "presentation.coherent_access") {
        if (parse_bool(publisherqos.presentation.coherent_access, value)) {
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                            ACE_TEXT("Illegal value for presentation.coherent_access (%C) in [publisherqos/%C] section.\n"),
                            value.c_str(), publisherqos_name.c_str()),
                            -1);
        }
      } else if (name == "presentation.ordered_access") {
        if (parse_bool(publisherqos.presentation.ordered_access, value)) {
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                            ACE_TEXT("Illegal value for presentation.ordered_access (%C)")
                            ACE_TEXT("in [publisherqos/%C] section.\n"),
                            value.c_str(), publisherqos_name.c_str()),
                            -1);
        }
      } else if (name == "partition.name") {
        try {
          parse_list(publisherqos.partition, value);
        }
        catch (const CORBA::Exception& ex) {
          ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
            ACE_TEXT("Exception caught while parsing partition.name (%C) ")
            ACE_TEXT("in [publisherqos/%C] section: %C.\n"),
            value.c_str(), publisherqos_name.c_str(), ex._info().c_str()),
            -1);
        }
      } else {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_publisherqos ")
                          ACE_TEXT("Unexpected entry (%C) in [publisherqos/%C] section.\n"),
                          name.c_str(), publisherqos_name.c_str()),
                          -1);
      }
    }

    registry.publisherqos_map[publisherqos_name] = publisherqos;
  }

  return 0;
}

int
StaticDiscovery::parse_subscriberqos(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key section;

  if (cf.open_section(root, SUBSCRIBERQOS_SECTION_NAME, 0, section) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any subscriberqos (sub)section.
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_subscriberqos ")
                  ACE_TEXT("no [%s] sections.\n"),
                  SUBSCRIBERQOS_SECTION_NAME));
    }
    return 0;
  }

  // Ensure there are no key/values in the [subscriberqos] section.
  // Every key/value must be in a [subscriberqos/*] sub-section.
  ValueMap vm;
  if (pullValues(cf, section, vm) > 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                      ACE_TEXT("[subscriberqos] sections must have a subsection name\n")),
                      -1);
  }
  // Process the subsections of this section
  KeyList keys;
  if (processSections(cf, section, keys) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                      ACE_TEXT("too many nesting layers in the [subscriberqos] section.\n")),
                      -1);
  }

  // Loop through the [subscriberqos/*] sections
  for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
    OPENDDS_STRING subscriberqos_name = it->first;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_subscriberqos ")
                  ACE_TEXT("processing [subscriberqos/%C] section.\n"),
                  subscriberqos_name.c_str()));
    }

    ValueMap values;
    pullValues(cf, it->second, values);

    DDS::SubscriberQos subscriberqos(TheServiceParticipant->initial_SubscriberQos());

    for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
      OPENDDS_STRING name = it->first;
      OPENDDS_STRING value = it->second;

      if (name == "presentation.access_scope") {
        if (value == "INSTANCE") {
          subscriberqos.presentation.access_scope = DDS::INSTANCE_PRESENTATION_QOS;
        } else if (value == "TOPIC") {
          subscriberqos.presentation.access_scope = DDS::TOPIC_PRESENTATION_QOS;
        } else if (value == "GROUP") {
          subscriberqos.presentation.access_scope = DDS::GROUP_PRESENTATION_QOS;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                            ACE_TEXT("Illegal value for presentation.access_scope (%C) in [subscriberqos/%C] section.\n"),
                            value.c_str(), subscriberqos_name.c_str()),
                            -1);
        }
      } else if (name == "presentation.coherent_access") {
        if (parse_bool(subscriberqos.presentation.coherent_access, value)) {
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                            ACE_TEXT("Illegal value for presentation.coherent_access (%C) in [subscriberqos/%C] section.\n"),
                            value.c_str(), subscriberqos_name.c_str()),
                            -1);
        }
      } else if (name == "presentation.ordered_access") {
        if (parse_bool(subscriberqos.presentation.ordered_access, value)) {
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                            ACE_TEXT("Illegal value for presentation.ordered_access (%C) in [subscriberqos/%C] section.\n"),
                            value.c_str(), subscriberqos_name.c_str()),
                            -1);
        }
      } else if (name == "partition.name") {
        try {
          parse_list(subscriberqos.partition, value);
        }
        catch (const CORBA::Exception& ex) {
          ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
            ACE_TEXT("Exception caught while parsing partition.name (%C) ")
            ACE_TEXT("in [subscriberqos/%C] section: %C.\n"),
            value.c_str(), subscriberqos_name.c_str(), ex._info().c_str()),
            -1);
        }
      } else {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) StaticDiscovery::parse_subscriberqos ")
                          ACE_TEXT("Unexpected entry (%C) in [subscriberqos/%C] section.\n"),
                          name.c_str(), subscriberqos_name.c_str()),
                          -1);
      }
    }

   registry.subscriberqos_map[subscriberqos_name] = subscriberqos;
  }

  return 0;
}

int
StaticDiscovery::parse_endpoints(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key section;

  if (cf.open_section(root, ENDPOINT_SECTION_NAME, 0, section) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any endpoint (sub)section.
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_endpoints ")
                  ACE_TEXT("no [%s] sections.\n"),
                  ENDPOINT_SECTION_NAME));
    }
    return 0;
  }

  // Ensure there are no key/values in the [endpoint] section.
  // Every key/value must be in a [endpoint/*] sub-section.
  ValueMap vm;
  if (pullValues(cf, section, vm) > 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                      ACE_TEXT("[endpoint] sections must have a subsection name\n")),
                      -1);
  }
  // Process the subsections of this section
  KeyList keys;
  if (processSections(cf, section, keys) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                      ACE_TEXT("too many nesting layers in the [endpoint] section.\n")),
                      -1);
  }

  // Loop through the [endpoint/*] sections
  for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
    OPENDDS_STRING endpoint_name = it->first;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_endpoints ")
                  ACE_TEXT("processing [endpoint/%C] section.\n"),
                  endpoint_name.c_str()));
    }

    ValueMap values;
    pullValues(cf, it->second, values);
    int domain = 0;
    unsigned char participant[6];
    unsigned char entity[3];
    enum Type {
      Reader,
      Writer
    };
    Type type = Reader; // avoid warning
    OPENDDS_STRING topic_name;
    DDS::DataWriterQos datawriterqos(TheServiceParticipant->initial_DataWriterQos());
    DDS::DataReaderQos datareaderqos(TheServiceParticipant->initial_DataReaderQos());
    DDS::PublisherQos publisherqos(TheServiceParticipant->initial_PublisherQos());
    DDS::SubscriberQos subscriberqos(TheServiceParticipant->initial_SubscriberQos());
    TransportLocatorSeq trans_info;
    OPENDDS_STRING config_name;

    bool domainSpecified = false,
      participantSpecified = false,
      entitySpecified = false,
      typeSpecified = false,
      topic_name_Specified = false,
      config_name_Specified = false;

    for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
      OPENDDS_STRING name = it->first;
      OPENDDS_STRING value = it->second;

      if (name == "domain") {
        if (convertToInteger(value, domain)) {
          domainSpecified = true;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("Illegal integer value for domain (%C) in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }
      } else if (name == "participant") {
#ifdef __SUNPRO_CC
        int count = 0;
        std::count_if(value.begin(), value.end(), isxdigit, count);
        if (value.size() != 12 || count != 12) {
#else
        if (value.size() != 12 ||
            std::count_if(value.begin(), value.end(), isxdigit) != 12) {
#endif
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("participant (%C) must be 12 hexadecimal digits in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }

        for (size_t idx = 0; idx != 6; ++idx) {
          participant[idx] = fromhex(value, idx);
        }
        participantSpecified = true;
      } else if (name == "entity") {
#ifdef __SUNPRO_CC
        int count = 0;
        std::count_if(value.begin(), value.end(), isxdigit, count);
        if (value.size() != 6 || count != 6) {
#else
        if (value.size() != 6 ||
            std::count_if(value.begin(), value.end(), isxdigit) != 6) {
#endif
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("entity (%C) must be 6 hexadecimal digits in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }

        for (size_t idx = 0; idx != 3; ++idx) {
          entity[idx] = fromhex(value, idx);
        }
        entitySpecified = true;
      } else if (name == "type") {
        if (value == "reader") {
          type = Reader;
          typeSpecified = true;
        } else if (value == "writer") {
          type = Writer;
          typeSpecified = true;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("Illegal string value for type (%C) in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }
      } else if (name == "topic") {
        EndpointRegistry::TopicMapType::const_iterator pos = this->registry.topic_map.find(value);
        if (pos != this->registry.topic_map.end()) {
          topic_name = pos->second.name;
          topic_name_Specified = true;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("Illegal topic reference (%C) in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }
      } else if (name == "datawriterqos") {
        EndpointRegistry::DataWriterQosMapType::const_iterator pos = this->registry.datawriterqos_map.find(value);
        if (pos != this->registry.datawriterqos_map.end()) {
          datawriterqos = pos->second;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("Illegal datawriterqos reference (%C) in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }
      } else if (name == "publisherqos") {
        EndpointRegistry::PublisherQosMapType::const_iterator pos = this->registry.publisherqos_map.find(value);
        if (pos != this->registry.publisherqos_map.end()) {
          publisherqos = pos->second;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("Illegal publisherqos reference (%C) in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }
      } else if (name == "datareaderqos") {
        EndpointRegistry::DataReaderQosMapType::const_iterator pos = this->registry.datareaderqos_map.find(value);
        if (pos != this->registry.datareaderqos_map.end()) {
          datareaderqos = pos->second;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("Illegal datareaderqos reference (%C) in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }
      } else if (name == "subscriberqos") {
        EndpointRegistry::SubscriberQosMapType::const_iterator pos = this->registry.subscriberqos_map.find(value);
        if (pos != this->registry.subscriberqos_map.end()) {
          subscriberqos = pos->second;
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                            ACE_TEXT("Illegal subscriberqos reference (%C) in [endpoint/%C] section.\n"),
                            value.c_str(), endpoint_name.c_str()),
                            -1);
        }
      } else if (name == "config") {
        config_name = value;
        config_name_Specified = true;
      } else {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                          ACE_TEXT("Unexpected entry (%C) in [endpoint/%C] section.\n"),
                          name.c_str(), endpoint_name.c_str()),
                          -1);
      }
    }

    if (!domainSpecified) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                        ACE_TEXT("No domain specified for [endpoint/%C] section.\n"),
                        endpoint_name.c_str()),
                        -1);
    }

    if (!participantSpecified) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                        ACE_TEXT("No participant specified for [endpoint/%C] section.\n"),
                        endpoint_name.c_str()),
                        -1);
    }

    if (!entitySpecified) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                        ACE_TEXT("No entity specified for [endpoint/%C] section.\n"),
                        endpoint_name.c_str()),
                        -1);
    }

    if (!typeSpecified) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR:StaticDiscovery::parse_endpoints ")
                        ACE_TEXT("No type specified for [endpoint/%C] section.\n"),
                        endpoint_name.c_str()),
                        -1);
    }

    if (!topic_name_Specified) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                        ACE_TEXT("No topic specified for [endpoint/%C] section.\n"),
                        endpoint_name.c_str()),
                        -1);
    }

    TransportConfig_rch config;

    if (config_name_Specified) {
      config = TheTransportRegistry->get_config(config_name);
      if (config.is_nil()) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                          ACE_TEXT("Illegal config reference (%C) in [endpoint/%C] section.\n"),
                          config_name.c_str(), endpoint_name.c_str()),
                          -1);
      }
    }

    if (config.is_nil() && domainSpecified) {
      config = TheTransportRegistry->domain_default_config(domain);
    }

    if (config.is_nil()) {
      config = TheTransportRegistry->global_config();
    }

    try {
      config->populate_locators(trans_info);
    }
    catch (const CORBA::Exception& ex) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                        ACE_TEXT("Exception caught while populating locators for [endpoint/%C] section. %C\n"),
                        endpoint_name.c_str(), ex._info().c_str()),
                        -1);
    }
    if (trans_info.length() == 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                          ACE_TEXT("No locators for [endpoint/%C] section.\n"),
                          endpoint_name.c_str()),
                          -1);
    }

    EntityId_t entity_id = EndpointRegistry::build_id(entity,
      (type == Reader) ? ENTITYKIND_USER_READER_WITH_KEY : ENTITYKIND_USER_WRITER_WITH_KEY);

    RepoId id = EndpointRegistry::build_id(domain, participant, entity_id);

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: StaticDiscovery::parse_endpoints adding entity with id %C\n"), LogGuid(id).c_str()));
    }

    switch (type) {
    case Reader:
      // Populate the userdata.
      datareaderqos.user_data.value.length(3);
      datareaderqos.user_data.value[0] = entity_id.entityKey[0];
      datareaderqos.user_data.value[1] = entity_id.entityKey[1];
      datareaderqos.user_data.value[2] = entity_id.entityKey[2];

      if (!registry.reader_map.insert(std::make_pair(id,
            EndpointRegistry::Reader(topic_name, datareaderqos, subscriberqos, config_name, trans_info))).second) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                          ACE_TEXT("Section [endpoint/%C] ignored - duplicate reader.\n"),
                          endpoint_name.c_str()),
                          -1);
      }
      break;
    case Writer:
      // Populate the userdata.
      datawriterqos.user_data.value.length(3);
      datawriterqos.user_data.value[0] = entity_id.entityKey[0];
      datawriterqos.user_data.value[1] = entity_id.entityKey[1];
      datawriterqos.user_data.value[2] = entity_id.entityKey[2];

      if (!registry.writer_map.insert(std::make_pair(id,
            EndpointRegistry::Writer(topic_name, datawriterqos, publisherqos, config_name, trans_info))).second) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                          ACE_TEXT("Section [endpoint/%C] ignored - duplicate writer.\n"),
                          endpoint_name.c_str()),
                          -1);
      }
      break;
    }
  }

  return 0;
}

void StaticDiscovery::pre_writer(DataWriterImpl* writer)
{
  const DDS::Publisher_var pub = writer->get_publisher();
  const DDS::DomainParticipant_var part = pub->get_participant();
  const DDS::DomainId_t dom = part->get_domain_id();

  DDS::DomainParticipantQos partQos;
  part->get_qos(partQos);
  if (partQos.user_data.value.length() < 6)
    return;
  const unsigned char* const partId = partQos.user_data.value.get_buffer();

  DDS::DataWriterQos qos;
  writer->get_qos(qos);
  if (qos.user_data.value.length() < 3)
    return;
  const unsigned char* const dwId = qos.user_data.value.get_buffer();

  const EntityId_t entId =
    EndpointRegistry::build_id(dwId, ENTITYKIND_USER_WRITER_WITH_KEY);
  const RepoId rid = EndpointRegistry::build_id(dom, partId, entId);

  const EndpointRegistry::WriterMapType::const_iterator iter =
    registry.writer_map.find(rid);

  if (iter != registry.writer_map.end() && !iter->second.trans_cfg.empty()) {
    TransportRegistry::instance()->bind_config(iter->second.trans_cfg, writer);
  }
}

void StaticDiscovery::pre_reader(DataReaderImpl* reader)
{
  const DDS::Subscriber_var sub = reader->get_subscriber();
  const DDS::DomainParticipant_var part = sub->get_participant();
  const DDS::DomainId_t dom = part->get_domain_id();

  DDS::DomainParticipantQos partQos;
  part->get_qos(partQos);
  if (partQos.user_data.value.length() < 6)
    return;
  const unsigned char* const partId = partQos.user_data.value.get_buffer();

  DDS::DataReaderQos qos;
  reader->get_qos(qos);
  if (qos.user_data.value.length() < 3)
    return;
  const unsigned char* const drId = qos.user_data.value.get_buffer();

  const EntityId_t entId =
    EndpointRegistry::build_id(drId, ENTITYKIND_USER_READER_WITH_KEY);
  const RepoId rid = EndpointRegistry::build_id(dom, partId, entId);

  const EndpointRegistry::ReaderMapType::const_iterator iter =
    registry.reader_map.find(rid);

  if (iter != registry.reader_map.end() && !iter->second.trans_cfg.empty()) {
    TransportRegistry::instance()->bind_config(iter->second.trans_cfg, reader);
  }
}

StaticDiscovery_rch StaticDiscovery::instance_(make_rch<StaticDiscovery>(Discovery::DEFAULT_STATIC));

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
