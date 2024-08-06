#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "StaticDiscovery.h"

#include "BuiltInTopicUtils.h"
#include "DataWriterImpl.h"
#include "DcpsUpcalls.h"
#include "DomainParticipantImpl.h"
#include "Marked_Default_Qos.h"
#include "Qos_Helper.h"
#include "Registered_Data_Types.h"
#include "SubscriberImpl.h"
#include "debug.h"

#include "transport/framework/TransportRegistry.h"

#include "XTypes/TypeAssignability.h"

#include <dds/OpenDDSConfigWrapper.h>

#include <ctype.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace {
  const size_t BYTES_IN_PARTICIPANT = 6;
  const size_t HEX_DIGITS_IN_PARTICIPANT = 2 * BYTES_IN_PARTICIPANT;
  const size_t BYTES_IN_ENTITY = 3;
  const size_t HEX_DIGITS_IN_ENTITY = 2 * BYTES_IN_ENTITY;
  const size_t TYPE_NAME_MAX = 128;
}

void EndpointRegistry::match()
{
  for (WriterMapType::iterator wp = writer_map.begin(), wp_limit = writer_map.end();
       wp != wp_limit;
       ++wp) {
    const GUID_t& writerid = wp->first;
    Writer& writer = wp->second;
    for (ReaderMapType::iterator rp = reader_map.begin(), rp_limit = reader_map.end();
         rp != rp_limit;
         ++rp) {
      const GUID_t& readerid = rp->first;
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

StaticEndpointManager::StaticEndpointManager(const GUID_t& participant_id,
                                             ACE_Thread_Mutex& lock,
                                             const EndpointRegistry& registry,
                                             StaticParticipant& participant)
  : lock_(lock)
  , participant_id_(participant_id)
  , topic_counter_(0)
  , registry_(registry)
#ifndef DDS_HAS_MINIMUM_BIT
  , participant_(participant)
#endif
  , max_type_lookup_service_reply_period_(0)
  , type_lookup_service_sequence_number_(0)
{
#ifdef DDS_HAS_MINIMUM_BIT
  ACE_UNUSED_ARG(participant);
#endif
  type_lookup_init(TheServiceParticipant->interceptor());
}

StaticEndpointManager::~StaticEndpointManager()
{
  type_lookup_fini();
}

void StaticEndpointManager::init_bit()
{
  // Discover all remote publications and subscriptions.

  for (EndpointRegistry::WriterMapType::const_iterator pos = registry_.writer_map.begin(),
         limit = registry_.writer_map.end();
       pos != limit;
       ++pos) {
    const GUID_t& remoteid = pos->first;
    const EndpointRegistry::Writer& writer = pos->second;

    if (!equal_guid_prefixes(participant_id_, remoteid)) {
      const DDS::BuiltinTopicKey_t key = guid_to_bit_key(remoteid);

      // pos represents a remote.
      // Populate data.
      DDS::PublicationBuiltinTopicData data = DDS::PublicationBuiltinTopicData();

      data.key = key;
      OPENDDS_STRING topic_name = writer.topic_name;
      data.topic_name = topic_name.c_str();

      EndpointRegistry::TopicMapType::const_iterator iter = registry_.topic_map.find(topic_name);
      if (iter == registry_.topic_map.end()) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: StaticEndpointManager::init_bit: no topic named %C\n",
                     topic_name.c_str()));
        }
        continue;
      }

      const EndpointRegistry::Topic& topic = iter->second;
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
      data.representation = writer.qos.representation;

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
    const GUID_t& remoteid = pos->first;
    const EndpointRegistry::Reader& reader = pos->second;

    if (!equal_guid_prefixes(participant_id_, remoteid)) {
      const DDS::BuiltinTopicKey_t key = guid_to_bit_key(remoteid);

      // pos represents a remote.
      // Populate data.
      DDS::SubscriptionBuiltinTopicData data = DDS::SubscriptionBuiltinTopicData();

      data.key = key;
      OPENDDS_STRING topic_name = reader.topic_name;
      data.topic_name = topic_name.c_str();

      EndpointRegistry::TopicMapType::const_iterator iter = registry_.topic_map.find(topic_name);
      if (iter == registry_.topic_map.end()) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: StaticEndpointManager::init_bit: no topic named %C\n",
                     topic_name.c_str()));
        }
        continue;
      }

      const EndpointRegistry::Topic& topic = iter->second;
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
      data.representation = reader.qos.representation;

#ifndef DDS_HAS_MINIMUM_BIT
      OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
      if (bit) { // bit may be null if the DomainParticipant is shutting down
        bit->store_synthetic_data(data, DDS::NEW_VIEW_STATE);
      }
#endif /* DDS_HAS_MINIMUM_BIT */
    }
  }
}

void StaticEndpointManager::assign_publication_key(GUID_t& rid,
                                                   const GUID_t& /*topicId*/,
                                                   const DDS::DataWriterQos& qos)
{
  if (qos.user_data.value.length() != BYTES_IN_ENTITY) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_publication_key: no user data to identify writer\n")));
    return;
  }

  rid.entityId.entityKey[0] = qos.user_data.value[0];
  rid.entityId.entityKey[1] = qos.user_data.value[1];
  rid.entityId.entityKey[2] = qos.user_data.value[2];
  rid.entityId.entityKind = ENTITYKIND_USER_WRITER_WITH_KEY;

  if (DCPS_debug_level > 8) {
    ACE_DEBUG((LM_INFO, "(%P|%t) looking up writer ID %C\n",
               LogGuid(rid).c_str()));
  }

  EndpointRegistry::WriterMapType::const_iterator pos = registry_.writer_map.find(rid);
  if (pos == registry_.writer_map.end()) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: StaticEndpointManager::assign_publication_key: unknown writer: %C\n"), LogGuid(rid).c_str()));
    return;
  }

  DDS::DataWriterQos qos2(qos);
  // Qos in registry will not have the user data so overwrite.
  qos2.user_data = pos->second.qos.user_data;

  DDS::DataWriterQos qos3(pos->second.qos);

  if (qos2 != qos3) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_publication_key: dynamic and static QoS differ\n")));
  }
}

void StaticEndpointManager::assign_subscription_key(GUID_t& rid,
                                                    const GUID_t& /*topicId*/,
                                                    const DDS::DataReaderQos& qos)
{
  if (qos.user_data.value.length() != BYTES_IN_ENTITY) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_subscription_key: no user data to identify reader\n")));
    return;
  }

  rid.entityId.entityKey[0] = qos.user_data.value[0];
  rid.entityId.entityKey[1] = qos.user_data.value[1];
  rid.entityId.entityKey[2] = qos.user_data.value[2];
  rid.entityId.entityKind = ENTITYKIND_USER_READER_WITH_KEY;

  EndpointRegistry::ReaderMapType::const_iterator pos = registry_.reader_map.find(rid);
  if (pos == registry_.reader_map.end()) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: StaticEndpointManager::assign_subscription_key: unknown reader: %C\n"), LogGuid(rid).c_str()));
    return;
  }

  DDS::DataReaderQos qos2(qos);
  // Qos in registry will not have the user data so overwrite.
  qos2.user_data = pos->second.qos.user_data;

  DDS::DataReaderQos qos3(pos->second.qos);

  if (qos2 != qos3) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::assign_subscription_key: dynamic and static QoS differ\n")));
  }
}

bool
StaticEndpointManager::update_topic_qos(const GUID_t& /*topicId*/,
                                        const DDS::TopicQos& /*qos*/)
{
  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::update_topic_qos - ")
             ACE_TEXT("Not allowed\n")));
  return false;
}

bool
StaticEndpointManager::update_publication_qos(const GUID_t& /*publicationId*/,
                                              const DDS::DataWriterQos& /*qos*/,
                                              const DDS::PublisherQos& /*publisherQos*/)
{
  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::update_publication_qos - ")
             ACE_TEXT("Not allowed\n")));
  return false;
}

bool
StaticEndpointManager::update_subscription_qos(const GUID_t& /*subscriptionId*/,
                                               const DDS::DataReaderQos& /*qos*/,
                                               const DDS::SubscriberQos& /*subscriberQos*/)
{
  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::update_subscription_qos - ")
             ACE_TEXT("Not allowed\n")));
  return false;
}

bool
StaticEndpointManager::update_subscription_params(const GUID_t& /*subId*/,
                                                  const DDS::StringSeq& /*params*/)
{
  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::update_subscription_qos - ")
             ACE_TEXT("Not allowed\n")));
  return false;
}

bool
StaticEndpointManager::disassociate()
{
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::disassociate TODO\n")));
  // TODO
  return false;
}

DDS::ReturnCode_t
StaticEndpointManager::add_publication_i(const GUID_t& writerid,
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
    const GUID_t& readerid = *pos;
    const EndpointRegistry::Reader& reader = registry_.reader_map.find(readerid)->second;

    const ReaderAssociation ra =
      {reader.trans_info, TransportLocator(), 0, readerid, reader.subscriber_qos, reader.qos, "", "", 0, 0, {0, 0}};
    DataWriterCallbacks_rch pl = pub.publication_.lock();
    if (pl) {
      pl->add_association(ra, true);
    }
  }

  for (RepoIdSet::const_iterator pos = writer.reliable_readers.begin(), limit = writer.reliable_readers.end();
       pos != limit;
       ++pos) {
    const GUID_t& readerid = *pos;
    const EndpointRegistry::Reader& reader = registry_.reader_map.find(readerid)->second;
    DataWriterCallbacks_rch pl = pub.publication_.lock();
    if (pl) {
      pl->register_for_reader(participant_id_, writerid, readerid, reader.trans_info, this);
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
StaticEndpointManager::remove_publication_i(const GUID_t& writerid, LocalPublication& pub)
{
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
    const GUID_t& readerid = *pos;
    ids[idx] = readerid;
    DataWriterCallbacks_rch pl = pub.publication_.lock();
    if (pl) {
      pl->unregister_for_reader(participant_id_, writerid, readerid);
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
StaticEndpointManager::add_subscription_i(const GUID_t& readerid,
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
    const GUID_t& writerid = *pos;
    const EndpointRegistry::Writer& writer = registry_.writer_map.find(writerid)->second;

    DDS::OctetSeq type_info;
    const WriterAssociation wa = {
      writer.trans_info, TransportLocator(), 0, writerid, writer.publisher_qos, writer.qos, type_info, {0, 0}
    };
    DataReaderCallbacks_rch sl = sub.subscription_.lock();
    if (sl) {
      sl->add_association(wa, false);
    }
  }

  for (RepoIdSet::const_iterator pos = reader.reliable_writers.begin(), limit = reader.reliable_writers.end();
       pos != limit;
       ++pos) {
    const GUID_t& writerid = *pos;
    const EndpointRegistry::Writer& writer = registry_.writer_map.find(writerid)->second;
    DataReaderCallbacks_rch sl = sub.subscription_.lock();
    if (sl) {
      sl->register_for_writer(participant_id_, readerid, writerid, writer.trans_info, this);
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t StaticEndpointManager::remove_subscription_i(
  const GUID_t& readerid, LocalSubscription& sub)
{
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
    const GUID_t& writerid = *pos;
    ids[idx] = writerid;
    DataReaderCallbacks_rch sl = sub.subscription_.lock();
    if (sl) {
      sl->unregister_for_writer(participant_id_, readerid, writerid);
    }
  }

  return DDS::RETCODE_OK;
}

bool
StaticEndpointManager::is_expectant_opendds(const GUID_t& /*endpoint*/) const
{
  // We can't propagate associated writers via SEDP announcments if we're
  // using static discovery, so nobody ought to be "expecting" them
  return false;
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
                                                           const GUID_t& /*reader*/)
{
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::populate_transport_locator_sequence TODO\n")));
  // TODO
}

void
StaticEndpointManager::populate_transport_locator_sequence(TransportLocatorSeq*& /*tls*/,
                                                           DiscoveredPublicationIter& /*iter*/,
                                                           const GUID_t& /*reader*/)
{
  ACE_DEBUG((LM_NOTICE, ACE_TEXT("(%P|%t) StaticEndpointManager::populate_transport_locator_sequence TODO\n")));
  // TODO
}

void
StaticEndpointManager::reader_exists(const GUID_t& readerid, const GUID_t& writerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationMap::const_iterator lp_pos = local_publications_.find(writerid);
  EndpointRegistry::ReaderMapType::const_iterator reader_pos = registry_.reader_map.find(readerid);
  if (lp_pos != local_publications_.end() &&
      reader_pos != registry_.reader_map.end()) {
    DataWriterCallbacks_rch dwr = lp_pos->second.publication_.lock();
    if (dwr) {
      const ReaderAssociation ra =
        {reader_pos->second.trans_info, TransportLocator(), 0, readerid, reader_pos->second.subscriber_qos, reader_pos->second.qos,
         "", "", DDS::StringSeq(), DDS::OctetSeq(), {0, 0}};
      dwr->add_association(ra, true);
    }
  }
}

void
StaticEndpointManager::reader_does_not_exist(const GUID_t& readerid, const GUID_t& writerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationMap::const_iterator lp_pos = local_publications_.find(writerid);
  EndpointRegistry::ReaderMapType::const_iterator reader_pos = registry_.reader_map.find(readerid);
  if (lp_pos != local_publications_.end() &&
      reader_pos != registry_.reader_map.end()) {
    DataWriterCallbacks_rch dwr = lp_pos->second.publication_.lock();
    if (dwr) {
      ReaderIdSeq ids;
      ids.length(1);
      ids[0] = readerid;
      dwr->remove_associations(ids, true);
    }
  }
}

void
StaticEndpointManager::writer_exists(const GUID_t& writerid, const GUID_t& readerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionMap::const_iterator ls_pos = local_subscriptions_.find(readerid);
  EndpointRegistry::WriterMapType::const_iterator writer_pos = registry_.writer_map.find(writerid);
  if (ls_pos != local_subscriptions_.end() &&
      writer_pos != registry_.writer_map.end()) {
    DataReaderCallbacks_rch drr = ls_pos->second.subscription_.lock();
    if (drr) {
      const WriterAssociation wa =
        {writer_pos->second.trans_info, TransportLocator(), 0, writerid, writer_pos->second.publisher_qos, writer_pos->second.qos, DDS::OctetSeq(), {0,0}};
      drr->add_association(wa, false);
    }
  }
}

void
StaticEndpointManager::writer_does_not_exist(const GUID_t& writerid, const GUID_t& readerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionMap::const_iterator ls_pos = local_subscriptions_.find(readerid);
  EndpointRegistry::WriterMapType::const_iterator writer_pos = registry_.writer_map.find(writerid);
  if (ls_pos != local_subscriptions_.end() &&
      writer_pos != registry_.writer_map.end()) {
    DataReaderCallbacks_rch drr = ls_pos->second.subscription_.lock();
    if (drr) {
      WriterIdSeq ids;
      ids.length(1);
      ids[0] = writerid;
      drr->remove_associations(ids, true);
    }
  }
}

void StaticEndpointManager::cleanup_type_lookup_data(const GuidPrefix_t& /*guid_prefix*/,
                                                     const XTypes::TypeIdentifier& /*ti*/,
                                                     bool /*secure*/)
{
  // Do nothing.
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

void StaticEndpointManager::type_lookup_init(ReactorInterceptor_rch reactor_interceptor)
{
  if (!type_lookup_reply_deadline_processor_) {
    type_lookup_reply_deadline_processor_ =
      DCPS::make_rch<StaticEndpointManagerSporadic>(TheServiceParticipant->time_source(), reactor_interceptor,
                                                    rchandle_from(this), &StaticEndpointManager::remove_expired_endpoints);
  }
}

void StaticEndpointManager::type_lookup_fini()
{
  if (type_lookup_reply_deadline_processor_) {
    type_lookup_reply_deadline_processor_->cancel();
    type_lookup_reply_deadline_processor_.reset();
  }
}

void StaticEndpointManager::type_lookup_service(
  const XTypes::TypeLookupService_rch type_lookup_service)
{
  type_lookup_service_ = type_lookup_service;
}

void StaticEndpointManager::purge_dead_topic(const String& topic_name)
{
  TopicDetailsMap::iterator top_it = topics_.find(topic_name);
  topic_names_.erase(top_it->second.topic_id());
  topics_.erase(top_it);
}

void StaticEndpointManager::ignore(const GUID_t& to_ignore)
{
  // Locked prior to call from Spdp.
  ignored_guids_.insert(to_ignore);
  {
    const DiscoveredPublicationIter iter = discovered_publications_.find(to_ignore);
    if (iter != discovered_publications_.end()) {
      // clean up tracking info
      const String topic_name = iter->second.get_topic_name();
      TopicDetails& td = topics_[topic_name];
      td.remove_discovered_publication(to_ignore);
      remove_from_bit(iter->second);
      discovered_publications_.erase(iter);
      // break associations
      match_endpoints(to_ignore, td, true /*remove*/);
      if (td.is_dead()) {
        purge_dead_topic(topic_name);
      }
      return;
    }
  }
  {
    const DiscoveredSubscriptionIter iter =
      discovered_subscriptions_.find(to_ignore);
    if (iter != discovered_subscriptions_.end()) {
      // clean up tracking info
      const String topic_name = iter->second.get_topic_name();
      TopicDetails& td = topics_[topic_name];
      td.remove_discovered_publication(to_ignore);
      remove_from_bit(iter->second);
      discovered_subscriptions_.erase(iter);
      // break associations
      match_endpoints(to_ignore, td, true /*remove*/);
      if (td.is_dead()) {
        purge_dead_topic(topic_name);
      }
      return;
    }
  }
  {
    const OPENDDS_MAP_CMP(GUID_t, OPENDDS_STRING, GUID_tKeyLessThan)::iterator
      iter = topic_names_.find(to_ignore);
    if (iter != topic_names_.end()) {
      ignored_topics_.insert(iter->second);
      // Remove all publications and subscriptions on this topic
      TopicDetails& td = topics_[iter->second];
      {
        const RepoIdSet ids = td.discovered_publications();
        for (RepoIdSet::const_iterator ep = ids.begin(); ep!= ids.end(); ++ep) {
          match_endpoints(*ep, td, true /*remove*/);
          td.remove_discovered_publication(*ep);
          // TODO: Do we need to remove from discovered_subscriptions?
          if (shutting_down()) { return; }
        }
      }
      {
        const RepoIdSet ids = td.discovered_subscriptions();
        for (RepoIdSet::const_iterator ep = ids.begin(); ep!= ids.end(); ++ep) {
          match_endpoints(*ep, td, true /*remove*/);
          td.remove_discovered_subscription(*ep);
          // TODO: Do we need to remove from discovered_publications?
          if (shutting_down()) { return; }
        }
      }
      if (td.is_dead()) {
        purge_dead_topic(iter->second);
      }
    }
  }
}

bool StaticEndpointManager::ignoring(const GUID_t& guid) const
{
  return ignored_guids_.count(guid);
}
bool StaticEndpointManager::ignoring(const char* topic_name) const
{
  return ignored_topics_.count(topic_name);
}

TopicStatus StaticEndpointManager::assert_topic(
  GUID_t& topicId, const char* topicName,
  const char* dataTypeName, const DDS::TopicQos& qos,
  bool hasDcpsKey, TopicCallbacks* topic_callbacks)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
  TopicDetailsMap::iterator iter = topics_.find(topicName);
  if (iter != topics_.end()) {
    if (iter->second.local_is_set() && iter->second.local_data_type_name() != dataTypeName) {
      return CONFLICTING_TYPENAME;
    }
    topicId = iter->second.topic_id();
    iter->second.set_local(dataTypeName, qos, hasDcpsKey, topic_callbacks);
    return FOUND;
  }

  TopicDetails& td = topics_[topicName];
  topicId = make_topic_guid();
  td.init(topicName, topicId);
  topic_names_[topicId] = topicName;
  td.set_local(dataTypeName, qos, hasDcpsKey, topic_callbacks);

  return CREATED;
}

TopicStatus StaticEndpointManager::find_topic(
  const char* topicName,
  CORBA::String_out dataTypeName,
  DDS::TopicQos_out qos,
  GUID_t& topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
  TopicDetailsMap::const_iterator iter = topics_.find(topicName);
  if (iter == topics_.end()) {
    return NOT_FOUND;
  }

  const TopicDetails& td = iter->second;

  dataTypeName = td.local_data_type_name().c_str();
  qos = new DDS::TopicQos(td.local_qos());
  topicId = td.topic_id();
  return FOUND;
}

TopicStatus StaticEndpointManager::remove_topic(const GUID_t& topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, INTERNAL_ERROR);
  TopicNameMap::iterator name_iter = topic_names_.find(topicId);
  if (name_iter == topic_names_.end()) {
    return NOT_FOUND;
  }
  const String& name = name_iter->second;
  TopicDetails& td = topics_[name];
  td.unset_local();
  if (td.is_dead()) {
    purge_dead_topic(name);
  }

  return REMOVED;
}

bool StaticEndpointManager::add_publication(const GUID_t& topicId,
                                            DataWriterCallbacks_rch publication,
                                            const DDS::DataWriterQos& qos,
                                            const TransportLocatorSeq& transInfo,
                                            const DDS::PublisherQos& publisherQos,
                                            const XTypes::TypeInformation& type_info)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);

  GUID_t rid = participant_id_;
  assign_publication_key(rid, topicId, qos);
  publication->set_publication_id(rid);
  LocalPublication& pb = local_publications_[rid];
  pb.topic_id_ = topicId;
  pb.publication_ = publication;
  pb.qos_ = qos;
  pb.trans_info_ = transInfo;
  pb.publisher_qos_ = publisherQos;
  pb.type_info_ = type_info;
  const OPENDDS_STRING& topic_name = topic_names_[topicId];

  TopicDetails& td = topics_[topic_name];
  td.add_local_publication(rid);

  if (DDS::RETCODE_OK != add_publication_i(rid, pb)) {
    return false;
  }

  if (DDS::RETCODE_OK != write_publication_data(rid, pb)) {
    return false;
  }

  if (DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) StaticEndpointManager::add_publication - ")
               ACE_TEXT("calling match_endpoints\n")));
  }
  match_endpoints(rid, td);

  return true;
}

void StaticEndpointManager::remove_publication(const GUID_t& publicationId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationIter iter = local_publications_.find(publicationId);
  if (iter != local_publications_.end()) {
    if (DDS::RETCODE_OK == remove_publication_i(publicationId, iter->second)) {
      OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
      local_publications_.erase(publicationId);
      TopicDetailsMap::iterator top_it = topics_.find(topic_name);
      if (top_it != topics_.end()) {
        match_endpoints(publicationId, top_it->second, true /*remove*/);
        top_it->second.remove_local_publication(publicationId);
        // Local, no need to check for dead topic.
      }
    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::remove_publication - ")
                 ACE_TEXT("Failed to publish dispose msg\n")));
    }
  }
}

void StaticEndpointManager::update_publication_locators(
  const GUID_t& publicationId, const TransportLocatorSeq& transInfo)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationIter iter = local_publications_.find(publicationId);
  if (iter != local_publications_.end()) {
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_INFO,
        ACE_TEXT("(%P|%t) StaticEndpointManager::update_publication_locators - updating locators for %C\n"),
        LogGuid(publicationId).c_str()));
    }
    iter->second.trans_info_ = transInfo;
    write_publication_data(publicationId, iter->second);
  }
}

bool StaticEndpointManager::add_subscription(const GUID_t& topicId,
                                             DataReaderCallbacks_rch subscription,
                                             const DDS::DataReaderQos& qos,
                                             const TransportLocatorSeq& transInfo,
                                             const DDS::SubscriberQos& subscriberQos,
                                             const char* filterClassName,
                                             const char* filterExpr,
                                             const DDS::StringSeq& params,
                                             const XTypes::TypeInformation& type_info)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);

  GUID_t rid = participant_id_;
  assign_subscription_key(rid, topicId, qos);
  subscription->set_subscription_id(rid);
  LocalSubscription& sb = local_subscriptions_[rid];
  sb.topic_id_ = topicId;
  sb.subscription_ = subscription;
  sb.qos_ = qos;
  sb.trans_info_ = transInfo;
  sb.subscriber_qos_ = subscriberQos;
  sb.filterProperties.filterClassName = filterClassName;
  sb.filterProperties.filterExpression = filterExpr;
  sb.filterProperties.expressionParameters = params;
  sb.type_info_ = type_info;
  const OPENDDS_STRING& topic_name = topic_names_[topicId];

  TopicDetails& td = topics_[topic_name];
  td.add_local_subscription(rid);

  if (DDS::RETCODE_OK != add_subscription_i(rid, sb)) {
    return false;
  }

  if (DDS::RETCODE_OK != write_subscription_data(rid, sb)) {
    return false;
  }

  if (DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) StaticEndpointManager::add_subscription - ")
               ACE_TEXT("calling match_endpoints\n")));
  }
  match_endpoints(rid, td);

  return true;
}

void StaticEndpointManager::remove_subscription(const GUID_t& subscriptionId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
  if (iter != local_subscriptions_.end()) {
    if (DDS::RETCODE_OK == remove_subscription_i(subscriptionId, iter->second)) {
      OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
      local_subscriptions_.erase(subscriptionId);
      TopicDetailsMap::iterator top_it = topics_.find(topic_name);
      if (top_it != topics_.end()) {
        match_endpoints(subscriptionId, top_it->second, true /*remove*/);
        top_it->second.remove_local_subscription(subscriptionId);
        // Local, no need to check for dead topic.
      }
    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::remove_subscription - ")
                 ACE_TEXT("Failed to publish dispose msg\n")));
    }
  }
}

void StaticEndpointManager::update_subscription_locators(
  const GUID_t& subscriptionId,
  const TransportLocatorSeq& transInfo)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
  if (iter != local_subscriptions_.end()) {
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_INFO,
        ACE_TEXT("(%P|%t) StaticEndpointManager::update_subscription_locators updating locators for %C\n"),
        LogGuid(subscriptionId).c_str()));
    }
    iter->second.trans_info_ = transInfo;
    write_subscription_data(subscriptionId, iter->second);
  }
}

// TODO: This is perhaps too generic since the context probably has the details this function computes.
void StaticEndpointManager::match_endpoints(
  GUID_t repoId, const TopicDetails& td, bool remove)
{
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) StaticEndpointManager::match_endpoints %C%C\n",
      remove ? "remove " : "", LogGuid(repoId).c_str()));
  }

  const bool reader = GuidConverter(repoId).isReader();
  // Copy the endpoint set - lock can be released in match()
  RepoIdSet local_endpoints;
  RepoIdSet discovered_endpoints;
  if (reader) {
    local_endpoints = td.local_publications();
    discovered_endpoints = td.discovered_publications();
  } else {
    local_endpoints = td.local_subscriptions();
    discovered_endpoints = td.discovered_subscriptions();
  }

  const bool is_remote = !equal_guid_prefixes(repoId, participant_id_);
  if (is_remote && local_endpoints.empty()) {
    // Nothing to match.
    return;
  }

  for (RepoIdSet::const_iterator iter = local_endpoints.begin();
       iter != local_endpoints.end(); ++iter) {
    // check to make sure it's a Reader/Writer or Writer/Reader match
    if (GuidConverter(*iter).isReader() != reader) {
      if (remove) {
        remove_assoc(*iter, repoId);
      } else {
        match(reader ? *iter : repoId, reader ? repoId : *iter);
      }
    }
  }

  // Remote/remote matches are a waste of time
  if (is_remote) {
    return;
  }

  for (RepoIdSet::const_iterator iter = discovered_endpoints.begin();
       iter != discovered_endpoints.end(); ++iter) {
    // check to make sure it's a Reader/Writer or Writer/Reader match
    if (GuidConverter(*iter).isReader() != reader) {
      if (remove) {
        remove_assoc(*iter, repoId);
      } else {
        match(reader ? *iter : repoId, reader ? repoId : *iter);
      }
    }
  }
}

void StaticEndpointManager::remove_assoc(const GUID_t& remove_from, const GUID_t& removing)
{
  if (GuidConverter(remove_from).isReader()) {
    const LocalSubscriptionIter lsi = local_subscriptions_.find(remove_from);
    if (lsi != local_subscriptions_.end()) {
      lsi->second.matched_endpoints_.erase(removing);
      const DiscoveredPublicationIter dpi = discovered_publications_.find(removing);
      if (dpi != discovered_publications_.end()) {
        dpi->second.matched_endpoints_.erase(remove_from);
      }
      WriterIdSeq writer_seq(1);
      writer_seq.length(1);
      writer_seq[0] = removing;
      const size_t count = lsi->second.remote_expectant_opendds_associations_.erase(removing);
      DataReaderCallbacks_rch drr = lsi->second.subscription_.lock();
      if (drr) {
        drr->remove_associations(writer_seq, false /*notify_lost*/);
      }
      remove_assoc_i(remove_from, lsi->second, removing);
      // Update writer
      if (count) {
        write_subscription_data(remove_from, lsi->second);
      }
    }

  } else {
    const LocalPublicationIter lpi = local_publications_.find(remove_from);
    if (lpi != local_publications_.end()) {
      lpi->second.matched_endpoints_.erase(removing);
      const DiscoveredSubscriptionIter dsi = discovered_subscriptions_.find(removing);
      if (dsi != discovered_subscriptions_.end()) {
        dsi->second.matched_endpoints_.erase(remove_from);
      }
      ReaderIdSeq reader_seq(1);
      reader_seq.length(1);
      reader_seq[0] = removing;
      lpi->second.remote_expectant_opendds_associations_.erase(removing);
      DataWriterCallbacks_rch dwr = lpi->second.publication_.lock();
      if (dwr) {
        dwr->remove_associations(reader_seq, false /*notify_lost*/);
      }
      remove_assoc_i(remove_from, lpi->second, removing);
    }
  }
}

void StaticEndpointManager::match(const GUID_t& writer, const GUID_t& reader)
{
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) StaticEndpointManager::match: w: %C r: %C\n",
      LogGuid(writer).c_str(), LogGuid(reader).c_str()));
  }

  match_continue(writer, reader);
}

void StaticEndpointManager::remove_expired_endpoints(
  const MonotonicTimePoint& /*now*/)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  const MonotonicTimePoint now = MonotonicTimePoint::now();

  // Clean up internal data used by getTypeDependencies
  for (OrigSeqNumberMap::iterator it = orig_seq_numbers_.begin(); it != orig_seq_numbers_.end();) {
    if (now - it->second.time_started >= max_type_lookup_service_reply_period_) {
      if (DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) StaticEndpointManager::remove_expired_endpoints: "
          "clean up type lookup data for %C\n",
          LogGuid(it->second.participant).c_str()));
      }
      cleanup_type_lookup_data(it->second.participant, it->second.type_id, it->second.secure);
      orig_seq_numbers_.erase(it++);
    } else {
      ++it;
    }
  }
}

void StaticEndpointManager::match_continue(const GUID_t& writer, const GUID_t& reader)
{
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) StaticEndpointManager::match_continue: w: %C r: %C\n",
      LogGuid(writer).c_str(), LogGuid(reader).c_str()));
  }

  // 0. For discovered endpoints, we'll have the QoS info in the form of the
  // publication or subscription BIT data which doesn't use the same structures
  // for QoS.  In those cases we can copy the individual QoS policies to temp
  // QoS structs:
  DDS::DataWriterQos tempDwQos;
  DDS::PublisherQos tempPubQos;
  DDS::DataReaderQos tempDrQos;
  DDS::SubscriberQos tempSubQos;
  ContentFilterProperty_t tempCfp;

  DiscoveredPublicationIter dpi = discovered_publications_.find(writer);
  DiscoveredSubscriptionIter dsi = discovered_subscriptions_.find(reader);
  if (dpi != discovered_publications_.end() && dsi != discovered_subscriptions_.end()) {
    // This is a discovered/discovered match, nothing for us to do
    return;
  }

  // 1. Collect details about the writer, which may be local or discovered
  const DDS::DataWriterQos* dwQos = 0;
  const DDS::PublisherQos* pubQos = 0;
  TransportLocatorSeq* wTls = 0;
  ACE_CDR::ULong wTransportContext = 0;
  XTypes::TypeInformation* writer_type_info = 0;
  OPENDDS_STRING topic_name;
  MonotonicTime_t writer_participant_discovered_at;

  const LocalPublicationIter lpi = local_publications_.find(writer);
  bool writer_local = false, already_matched = false;
  if (lpi != local_publications_.end()) {
    writer_local = true;
    dwQos = &lpi->second.qos_;
    pubQos = &lpi->second.publisher_qos_;
    wTls = &lpi->second.trans_info_;
    wTransportContext = lpi->second.transport_context_;
    already_matched = lpi->second.matched_endpoints_.count(reader);
    writer_type_info = &lpi->second.type_info_;
    topic_name = topic_names_[lpi->second.topic_id_];
    writer_participant_discovered_at = lpi->second.participant_discovered_at_;
  } else if (dpi != discovered_publications_.end()) {
    wTls = &dpi->second.writer_data_.writerProxy.allLocators;
    wTransportContext = dpi->second.transport_context_;
    writer_type_info = &dpi->second.type_info_;
    topic_name = dpi->second.get_topic_name();
    writer_participant_discovered_at = dpi->second.participant_discovered_at_;

    const DDS::PublicationBuiltinTopicData& bit =
      dpi->second.writer_data_.ddsPublicationData;
    tempDwQos.durability = bit.durability;
    tempDwQos.durability_service = bit.durability_service;
    tempDwQos.deadline = bit.deadline;
    tempDwQos.latency_budget = bit.latency_budget;
    tempDwQos.liveliness = bit.liveliness;
    tempDwQos.reliability = bit.reliability;
    tempDwQos.destination_order = bit.destination_order;
    tempDwQos.history = TheServiceParticipant->initial_HistoryQosPolicy();
    tempDwQos.resource_limits =
      TheServiceParticipant->initial_ResourceLimitsQosPolicy();
    tempDwQos.transport_priority =
      TheServiceParticipant->initial_TransportPriorityQosPolicy();
    tempDwQos.lifespan = bit.lifespan;
    tempDwQos.user_data = bit.user_data;
    tempDwQos.ownership = bit.ownership;
    tempDwQos.ownership_strength = bit.ownership_strength;
    tempDwQos.writer_data_lifecycle =
      TheServiceParticipant->initial_WriterDataLifecycleQosPolicy();
    tempDwQos.representation = bit.representation;
    dwQos = &tempDwQos;

    tempPubQos.presentation = bit.presentation;
    tempPubQos.partition = bit.partition;
    tempPubQos.group_data = bit.group_data;
    tempPubQos.entity_factory =
      TheServiceParticipant->initial_EntityFactoryQosPolicy();
    pubQos = &tempPubQos;

    populate_transport_locator_sequence(wTls, dpi, writer);
  } else {
    return; // Possible and ok, since lock is released
  }

  // 2. Collect details about the reader, which may be local or discovered
  const DDS::DataReaderQos* drQos = 0;
  const DDS::SubscriberQos* subQos = 0;
  TransportLocatorSeq* rTls = 0;
  ACE_CDR::ULong rTransportContext = 0;
  const ContentFilterProperty_t* cfProp = 0;
  XTypes::TypeInformation* reader_type_info = 0;
  MonotonicTime_t reader_participant_discovered_at;

  const LocalSubscriptionIter lsi = local_subscriptions_.find(reader);
  bool reader_local = false;
  if (lsi != local_subscriptions_.end()) {
    reader_local = true;
    drQos = &lsi->second.qos_;
    subQos = &lsi->second.subscriber_qos_;
    rTls = &lsi->second.trans_info_;
    rTransportContext = lsi->second.transport_context_;
    reader_type_info = &lsi->second.type_info_;
    if (lsi->second.filterProperties.filterExpression[0] != 0) {
      tempCfp.filterExpression = lsi->second.filterProperties.filterExpression;
      tempCfp.expressionParameters = lsi->second.filterProperties.expressionParameters;
    }
    cfProp = &tempCfp;
    if (!already_matched) {
      already_matched = lsi->second.matched_endpoints_.count(writer);
    }
    reader_participant_discovered_at = lsi->second.participant_discovered_at_;
  } else if (dsi != discovered_subscriptions_.end()) {
    rTls = &dsi->second.reader_data_.readerProxy.allLocators;

    populate_transport_locator_sequence(rTls, dsi, reader);
    rTransportContext = dsi->second.transport_context_;

    const DDS::SubscriptionBuiltinTopicData& bit =
      dsi->second.reader_data_.ddsSubscriptionData;
    tempDrQos.durability = bit.durability;
    tempDrQos.deadline = bit.deadline;
    tempDrQos.latency_budget = bit.latency_budget;
    tempDrQos.liveliness = bit.liveliness;
    tempDrQos.reliability = bit.reliability;
    tempDrQos.destination_order = bit.destination_order;
    tempDrQos.history = TheServiceParticipant->initial_HistoryQosPolicy();
    tempDrQos.resource_limits =
      TheServiceParticipant->initial_ResourceLimitsQosPolicy();
    tempDrQos.user_data = bit.user_data;
    tempDrQos.ownership = bit.ownership;
    tempDrQos.time_based_filter = bit.time_based_filter;
    tempDrQos.reader_data_lifecycle =
      TheServiceParticipant->initial_ReaderDataLifecycleQosPolicy();
    tempDrQos.representation = bit.representation;
    tempDrQos.type_consistency = bit.type_consistency;
    drQos = &tempDrQos;

    tempSubQos.presentation = bit.presentation;
    tempSubQos.partition = bit.partition;
    tempSubQos.group_data = bit.group_data;
    tempSubQos.entity_factory =
      TheServiceParticipant->initial_EntityFactoryQosPolicy();
    subQos = &tempSubQos;

    cfProp = &dsi->second.reader_data_.contentFilterProperty;
    reader_type_info = &dsi->second.type_info_;
    reader_participant_discovered_at = dsi->second.participant_discovered_at_;
  } else {
    return; // Possible and ok, since lock is released
  }

  // 3. Perform type consistency check (XTypes 1.3, Section 7.6.3.4.2)
  bool consistent = false;

  TopicDetailsMap::iterator td_iter = topics_.find(topic_name);
  if (td_iter == topics_.end()) {
    ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) StaticEndpointManager::match_continue - ERROR ")
              ACE_TEXT("Didn't find topic for consistency check\n")));
    return;
  } else {
    const XTypes::TypeIdentifier& writer_type_id = writer_type_info->minimal.typeid_with_size.type_id;
    const XTypes::TypeIdentifier& reader_type_id = reader_type_info->minimal.typeid_with_size.type_id;
    if (writer_type_id.kind() != XTypes::TK_NONE && reader_type_id.kind() != XTypes::TK_NONE) {
      if (!writer_local || !reader_local) {
        Encoding::Kind encoding_kind;
        if (tempDwQos.representation.value.length() > 0 &&
            repr_to_encoding_kind(tempDwQos.representation.value[0], encoding_kind) &&
            encoding_kind == Encoding::KIND_XCDR1) {
          const XTypes::TypeFlag extensibility_mask = XTypes::IS_APPENDABLE;
          if (type_lookup_service_->extensibility(extensibility_mask, writer_type_id)) {
            if (DCPS_debug_level) {
              ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("StaticEndpointManager::match_continue: ")
                ACE_TEXT("Encountered unsupported combination of XCDR1 encoding and appendable extensibility\n")));
            }
          }
        }
      }

      XTypes::TypeConsistencyAttributes type_consistency;
      type_consistency.ignore_sequence_bounds = drQos->type_consistency.ignore_sequence_bounds;
      type_consistency.ignore_string_bounds = drQos->type_consistency.ignore_string_bounds;
      type_consistency.ignore_member_names = drQos->type_consistency.ignore_member_names;
      type_consistency.prevent_type_widening = drQos->type_consistency.prevent_type_widening;
      XTypes::TypeAssignability ta(type_lookup_service_, type_consistency);

      if (drQos->type_consistency.kind == DDS::ALLOW_TYPE_COERCION) {
        consistent = ta.assignable(reader_type_id, writer_type_id);
      } else {
        // The two types must be equivalent for DISALLOW_TYPE_COERCION
        consistent = reader_type_id == writer_type_id;
      }
    } else {
      if (drQos->type_consistency.force_type_validation) {
        // Cannot do type validation since not both TypeObjects are available
        consistent = false;
      } else {
        // Fall back to matching type names
        OPENDDS_STRING writer_type_name;
        OPENDDS_STRING reader_type_name;
        if (writer_local) {
          writer_type_name = td_iter->second.local_data_type_name();
        } else {
          writer_type_name = dpi->second.get_type_name();
        }
        if (reader_local) {
          reader_type_name = td_iter->second.local_data_type_name();
        } else {
          reader_type_name = dsi->second.get_type_name();
        }
        consistent = writer_type_name == reader_type_name;
      }
    }

    if (!consistent) {
      td_iter->second.increment_inconsistent();
      if (DCPS::DCPS_debug_level) {
        ACE_DEBUG((LM_WARNING,
                  ACE_TEXT("(%P|%t) StaticEndpointManager::match_continue - WARNING ")
                  ACE_TEXT("Data types of topic %C does not match (inconsistent)\n"),
                  topic_name.c_str()));
      }
      return;
    }
  }

  // Need to release lock, below, for callbacks into DCPS which could
  // call into Spdp/Sedp.  Note that this doesn't unlock, it just constructs
  // an ACE object which will be used below for unlocking.
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  // 4. Check transport and QoS compatibility

  // Copy entries from local publication and local subscription maps
  // prior to releasing lock
  DataWriterCallbacks_wrch dwr;
  DataReaderCallbacks_wrch drr;
  if (writer_local) {
    dwr = lpi->second.publication_;
    OPENDDS_ASSERT(lpi->second.publication_);
    OPENDDS_ASSERT(dwr);
  }
  if (reader_local) {
    drr = lsi->second.subscription_;
    OPENDDS_ASSERT(lsi->second.subscription_);
    OPENDDS_ASSERT(drr);
  }

  IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
  IncompatibleQosStatus readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};

  if (compatibleQOS(&writerStatus, &readerStatus, *wTls, *rTls,
      dwQos, drQos, pubQos, subQos)) {

    bool call_writer = false, call_reader = false;

    if (writer_local) {
      call_writer = lpi->second.matched_endpoints_.insert(reader).second;
      dwr = lpi->second.publication_;
      if (!reader_local) {
        dsi->second.matched_endpoints_.insert(writer);
      }
    }
    if (reader_local) {
      call_reader = lsi->second.matched_endpoints_.insert(writer).second;
      drr = lsi->second.subscription_;
      if (!writer_local) {
        dpi->second.matched_endpoints_.insert(reader);
      }
    }

    if (writer_local && !reader_local) {
      add_assoc_i(writer, lpi->second, reader, dsi->second);
    }
    if (reader_local && !writer_local) {
      add_assoc_i(reader, lsi->second, writer, dpi->second);
    }

    if (!call_writer && !call_reader) {
      return; // nothing more to do
    }

    // Copy reader and writer association data prior to releasing lock
    DDS::OctetSeq octet_seq_type_info_reader;
    XTypes::serialize_type_info(*reader_type_info, octet_seq_type_info_reader);
    const ReaderAssociation ra = {
      *rTls, TransportLocator(), rTransportContext, reader, *subQos, *drQos,
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
      cfProp->filterClassName, cfProp->filterExpression,
#else
      "", "",
#endif
      cfProp->expressionParameters,
      octet_seq_type_info_reader,
      reader_participant_discovered_at
    };

    DDS::OctetSeq octet_seq_type_info_writer;
    XTypes::serialize_type_info(*writer_type_info, octet_seq_type_info_writer);
    const WriterAssociation wa = {
      *wTls, TransportLocator(), wTransportContext, writer, *pubQos, *dwQos,
      octet_seq_type_info_writer,
      writer_participant_discovered_at
    };

    ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
    static const bool writer_active = true;

    if (call_writer) {
      if (DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) StaticEndpointManager::match_continue - ")
          ACE_TEXT("adding writer %C association for reader %C\n"), LogGuid(writer).c_str(),
          LogGuid(reader).c_str()));
      }
      DataWriterCallbacks_rch dwr_lock = dwr.lock();
      if (dwr_lock) {
        if (call_reader) {
          DataReaderCallbacks_rch drr_lock = drr.lock();
          if (drr_lock) {
            DcpsUpcalls thr(drr_lock, wa, !writer_active, dwr_lock);
            thr.activate();
            dwr_lock->add_association(ra, writer_active);
            thr.writer_done();
          }
        } else {
          dwr_lock->add_association(ra, writer_active);
        }
      }
    } else if (call_reader) {
      if (DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) StaticEndpointManager::match_continue - ")
          ACE_TEXT("adding reader %C association for writer %C\n"),
          LogGuid(reader).c_str(), LogGuid(writer).c_str()));
      }
      DataReaderCallbacks_rch drr_lock = drr.lock();
      if (drr_lock) {
        drr_lock->add_association(wa, !writer_active);
      }
    }

  } else if (already_matched) { // break an existing associtaion
    if (writer_local) {
      lpi->second.matched_endpoints_.erase(reader);
      lpi->second.remote_expectant_opendds_associations_.erase(reader);
      if (dsi != discovered_subscriptions_.end()) {
        dsi->second.matched_endpoints_.erase(writer);
      }
    }
    if (reader_local) {
      lsi->second.matched_endpoints_.erase(writer);
      lsi->second.remote_expectant_opendds_associations_.erase(writer);
      if (dpi != discovered_publications_.end()) {
        dpi->second.matched_endpoints_.erase(reader);
      }
    }
    if (writer_local && !reader_local) {
      remove_assoc_i(writer, lpi->second, reader);
    }
    if (reader_local && !writer_local) {
      remove_assoc_i(reader, lsi->second, writer);
    }
    ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
    if (writer_local) {
      ReaderIdSeq reader_seq(1);
      reader_seq.length(1);
      reader_seq[0] = reader;
      DataWriterCallbacks_rch dwr_lock = dwr.lock();
      if (dwr_lock) {
        dwr_lock->remove_associations(reader_seq, false /*notify_lost*/);
      }
    }
    if (reader_local) {
      WriterIdSeq writer_seq(1);
      writer_seq.length(1);
      writer_seq[0] = writer;
      DataReaderCallbacks_rch drr_lock = drr.lock();
      if (drr_lock) {
        drr_lock->remove_associations(writer_seq, false /*notify_lost*/);
      }
    }
  } else { // something was incompatible
    ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
    if (writer_local && writerStatus.count_since_last_send) {
      if (DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) StaticEndpointManager::match - ")
                   ACE_TEXT("writer incompatible\n")));
      }
      DataWriterCallbacks_rch dwr_lock = dwr.lock();
      if (dwr_lock) {
        dwr_lock->update_incompatible_qos(writerStatus);
      }
    }
    if (reader_local && readerStatus.count_since_last_send) {
      if (DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) StaticEndpointManager::match - ")
                   ACE_TEXT("reader incompatible\n")));
      }
      DataReaderCallbacks_rch drr_lock = drr.lock();
      if (drr_lock) {
        drr_lock->update_incompatible_qos(readerStatus);
      }
    }
  }
}

GUID_t StaticEndpointManager::make_topic_guid()
{
  EntityId_t entity_id;
  assign(entity_id.entityKey, topic_counter_);
  ++topic_counter_;
  entity_id.entityKind = ENTITYKIND_OPENDDS_TOPIC;

  if (topic_counter_ == 0x1000000) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: StaticEndpointManager::make_topic_guid: ")
               ACE_TEXT("Exceeded Maximum number of topic entity keys!")
               ACE_TEXT("Next key will be a duplicate!\n")));
    topic_counter_ = 0;
  }

  return make_id(participant_id_, entity_id);
}

bool StaticEndpointManager::has_dcps_key(const GUID_t& topicId) const
{
  typedef OPENDDS_MAP_CMP(GUID_t, OPENDDS_STRING, GUID_tKeyLessThan) TNMap;
  TNMap::const_iterator tn = topic_names_.find(topicId);
  if (tn == topic_names_.end()) return false;

  TopicDetailsMap::const_iterator td = topics_.find(tn->second);
  if (td == topics_.end()) return false;

  return td->second.has_dcps_key();
}

StaticDiscovery::StaticDiscovery(const RepoKey& key)
  : key_(key)
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

GUID_t
EndpointRegistry::build_id(DDS::DomainId_t domain,
                           const unsigned char* participant_id,
                           const EntityId_t& entity_id)
{
  GUID_t id;
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

OpenDDS::DCPS::GUID_t
StaticDiscovery::generate_participant_guid()
{
  return GUID_UNKNOWN;
}

AddDomainStatus
StaticDiscovery::add_domain_participant(DDS::DomainId_t domain,
                                        const DDS::DomainParticipantQos& qos,
                                        XTypes::TypeLookupService_rch tls)
{
  AddDomainStatus ads = {GUID_t(), false /*federated*/};

  if (qos.user_data.value.length() != BYTES_IN_PARTICIPANT) {
    ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::add_domain_participant ")
                ACE_TEXT("No userdata to identify participant\n")));
    return ads;
  }

  GUID_t id = EndpointRegistry::build_id(domain,
                                         qos.user_data.value.get_buffer(),
                                         ENTITYID_PARTICIPANT);
  if (!get_part(domain, id).is_nil()) {
    ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::add_domain_participant ")
                ACE_TEXT("Duplicate participant\n")));
    return ads;
  }

  const RcHandle<StaticParticipant> participant (make_rch<StaticParticipant>(ref(id), qos, registry));

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ads);
    participants_[domain][id] = participant;
  }

  participant->type_lookup_service(tls);

  ads.id = id;
  return ads;
}

#if OPENDDS_CONFIG_SECURITY
AddDomainStatus
StaticDiscovery::add_domain_participant_secure(
  DDS::DomainId_t /*domain*/,
  const DDS::DomainParticipantQos& /*qos*/,
  XTypes::TypeLookupService_rch /*tls*/,
  const OpenDDS::DCPS::GUID_t& /*guid*/,
  DDS::Security::IdentityHandle /*id*/,
  DDS::Security::PermissionsHandle /*perm*/,
  DDS::Security::ParticipantCryptoHandle /*part_crypto*/)
{
  const DCPS::AddDomainStatus ads = {OpenDDS::DCPS::GUID_UNKNOWN, false /*federated*/};
  ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::add_domain_participant_secure ")
              ACE_TEXT("Security not supported for static discovery.\n")));
  return ads;
}
#endif

namespace {

  void string_list_to_partitions(DDS::PartitionQosPolicy& x,
                                 const ConfigStoreImpl::StringList& value)
  {
    for (ConfigStoreImpl::StringList::const_iterator pos = value.begin(), limit = value.end();
         pos != limit; ++pos) {
      x.name.length(x.name.length() + 1);
      x.name[x.name.length() - 1] = pos->c_str();
    }
  }

  const EnumList<DDS::DurabilityQosPolicyKind> durability_kinds[] =
    {
      { DDS::VOLATILE_DURABILITY_QOS, "VOLATILE" },
      { DDS::TRANSIENT_LOCAL_DURABILITY_QOS, "TRANSIENT_LOCAL" }
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
      ,
      { DDS::TRANSIENT_DURABILITY_QOS, "TRANSIENT" },
      { DDS::PERSISTENT_DURABILITY_QOS, "PERSISTENT" }
#endif
    };

  const EnumList<DDS::LivelinessQosPolicyKind> liveliness_kinds[] =
    {
      { DDS::AUTOMATIC_LIVELINESS_QOS, "AUTOMATIC" },
      { DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS, "MANUAL_BY_TOPIC" },
      { DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, "MANUAL_BY_PARTICIPANT" }
    };

  const EnumList<DDS::ReliabilityQosPolicyKind> reliability_kinds[] =
    {
      { DDS::BEST_EFFORT_RELIABILITY_QOS, "BEST_EFFORT" },
      { DDS::RELIABLE_RELIABILITY_QOS, "RELIABLE" }
    };

  const EnumList<DDS::DestinationOrderQosPolicyKind> destination_order_kinds[] =
    {
      { DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS, "BY_RECEPTION_TIMESTAMP" },
      { DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS, "BY_SOURCE_TIMESTAMP" }
    };

  const EnumList<DDS::HistoryQosPolicyKind> history_kinds[] =
    {
      { DDS::KEEP_ALL_HISTORY_QOS, "KEEP_ALL" },
      { DDS::KEEP_LAST_HISTORY_QOS, "KEEP_LAST" }
    };

  const EnumList<DDS::OwnershipQosPolicyKind> ownership_kinds[] =
    {
      { DDS::SHARED_OWNERSHIP_QOS, "SHARED" },
      { DDS::EXCLUSIVE_OWNERSHIP_QOS, "EXCLUSIVE" }
    };

  const EnumList<DDS::PresentationQosPolicyAccessScopeKind> access_scope_kinds[] =
    {
      { DDS::INSTANCE_PRESENTATION_QOS, "INSTANCE" },
      { DDS::TOPIC_PRESENTATION_QOS, "TOPIC" },
      { DDS::GROUP_PRESENTATION_QOS, "GROUP" }
    };

  enum Type {
    Reader,
    Writer
  };
  const EnumList<Type> type_kinds[] =
    {
      { Reader, "reader" },
      { Writer, "writer" }
    };

}

int
StaticDiscovery::load_configuration()
{
  if (parse_topics() ||
      parse_datawriterqos() ||
      parse_datareaderqos() ||
      parse_publisherqos() ||
      parse_subscriberqos() ||
      parse_endpoints()) {
    return -1;
  }

  registry.match();

  return 0;
}

int
StaticDiscovery::parse_topics()
{
  RcHandle<ConfigStoreImpl> config_store = TheServiceParticipant->config_store();
  const ConfigStoreImpl::StringList sections = config_store->get_section_names("TOPIC");

  // Loop through the [topic/*] sections
  for (ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    const String& topic_name = *pos;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_topics ")
                 ACE_TEXT("processing [topic/%C] section.\n"),
                 topic_name.c_str()));
    }

    EndpointRegistry::Topic topic;
    topic.name = config_store->get((String("TOPIC_") + topic_name + "_NAME").c_str(), topic_name);
    topic.type_name = config_store->get((String("TOPIC_") + topic_name + "_TYPE_NAME").c_str(), "");
    if (topic.type_name.empty()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                 ACE_TEXT("No type_name specified for [topic/%C] section.\n"),
                 topic_name.c_str()));
      return -1;
    }
    if (topic.type_name.size() >= TYPE_NAME_MAX) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) StaticDiscovery::parse_topics ")
                 ACE_TEXT("type_name (%C) must be less than 128 characters in [topic/%C] section.\n"),
                 topic.type_name.c_str(), topic_name.c_str()));
      return -1;

    }

    registry.topic_map[topic_name] = topic;
  }

  return 0;
}

int
StaticDiscovery::parse_datawriterqos()
{
  RcHandle<ConfigStoreImpl> config_store = TheServiceParticipant->config_store();
  const ConfigStoreImpl::StringList sections = config_store->get_section_names("DATAWRITERQOS");

  // Loop through the [datawriterqos/*] sections
  for (ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    const String& datawriterqos_name = *pos;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_datawriterqos ")
                 ACE_TEXT("processing [datawriterqos/%C] section.\n"),
                 datawriterqos_name.c_str()));
    }

    const String prefix = String("DATAWRITERQOS_") + datawriterqos_name;

    DDS::DataWriterQos datawriterqos(TheServiceParticipant->initial_DataWriterQos());

    datawriterqos.durability.kind =
      config_store->get((prefix + "_DURABILITY_KIND").c_str(), datawriterqos.durability.kind, durability_kinds);

    datawriterqos.deadline.period.sec =
      config_store->get_int32((prefix + "_DEADLINE_PERIOD_SEC").c_str(), datawriterqos.deadline.period.sec);
    datawriterqos.deadline.period.nanosec =
      config_store->get_uint32((prefix + "_DEADLINE_PERIOD_NANOSEC").c_str(), datawriterqos.deadline.period.nanosec);

    datawriterqos.latency_budget.duration.sec =
      config_store->get_int32((prefix + "_LATENCY_BUDGET_DURATION_SEC").c_str(), datawriterqos.latency_budget.duration.sec);
    datawriterqos.latency_budget.duration.nanosec =
      config_store->get_uint32((prefix + "_LATENCY_BUDGET_DURATION_NANOSEC").c_str(), datawriterqos.latency_budget.duration.nanosec);

    datawriterqos.liveliness.kind =
      config_store->get((prefix + "_LIVELINESS_KIND").c_str(), datawriterqos.liveliness.kind, liveliness_kinds);
    datawriterqos.liveliness.lease_duration.sec =
      config_store->get_int32((prefix + "_LIVELINESS_LEASE_DURATION_SEC").c_str(), datawriterqos.liveliness.lease_duration.sec);
    datawriterqos.liveliness.lease_duration.nanosec =
      config_store->get_uint32((prefix + "_LIVELINESS_LEASE_DURATION_NANOSEC").c_str(), datawriterqos.liveliness.lease_duration.nanosec);

    datawriterqos.reliability.kind =
      config_store->get((prefix + "_RELIABILITY_KIND").c_str(), datawriterqos.reliability.kind, reliability_kinds);
    datawriterqos.reliability.max_blocking_time.sec =
      config_store->get_int32((prefix + "_RELIABILITY_MAX_BLOCKING_TIME_SEC").c_str(), datawriterqos.reliability.max_blocking_time.sec);
    datawriterqos.reliability.max_blocking_time.nanosec =
      config_store->get_uint32((prefix + "_RELIABILITY_MAX_BLOCKING_TIME_NANOSEC").c_str(), datawriterqos.reliability.max_blocking_time.nanosec);

    datawriterqos.destination_order.kind =
      config_store->get((prefix + "_DESTINATION_ORDER_KIND").c_str(), datawriterqos.destination_order.kind, destination_order_kinds);

    datawriterqos.history.kind =
      config_store->get((prefix + "_HISTORY_KIND").c_str(), datawriterqos.history.kind, history_kinds);
    datawriterqos.history.depth =
      config_store->get_int32((prefix + "_HISTORY_DEPTH").c_str(), datawriterqos.history.depth);

    datawriterqos.resource_limits.max_samples =
      config_store->get_int32((prefix + "_RESOURCE_LIMITS_MAX_SAMPLES").c_str(), datawriterqos.resource_limits.max_samples);
    datawriterqos.resource_limits.max_instances =
      config_store->get_int32((prefix + "_RESOURCE_LIMITS_MAX_INSTANCES").c_str(), datawriterqos.resource_limits.max_instances);
    datawriterqos.resource_limits.max_samples_per_instance =
      config_store->get_int32((prefix + "_RESOURCE_LIMITS_MAX_SAMPLES_PER_INSTANCE").c_str(), datawriterqos.resource_limits.max_samples_per_instance);

    datawriterqos.transport_priority.value =
      config_store->get_int32((prefix + "_TRANSPORT_PRIORITY_VALUE").c_str(), datawriterqos.transport_priority.value);

    datawriterqos.lifespan.duration.sec =
      config_store->get_int32((prefix + "_LIFESPAN_DURATION_SEC").c_str(), datawriterqos.lifespan.duration.sec);
    datawriterqos.lifespan.duration.nanosec =
      config_store->get_uint32((prefix + "_LIFESPAN_DURATION_NANOSEC").c_str(), datawriterqos.lifespan.duration.nanosec);

    datawriterqos.ownership.kind =
      config_store->get((prefix + "_OWNERSHIP_KIND").c_str(), datawriterqos.ownership.kind, ownership_kinds);
    datawriterqos.ownership_strength.value =
      config_store->get_int32((prefix + "_OWNERSHIP_STRENGTH_VALUE").c_str(), datawriterqos.ownership_strength.value);

    registry.datawriterqos_map[datawriterqos_name] = datawriterqos;
  }

  return 0;
}

int
StaticDiscovery::parse_datareaderqos()
{
  RcHandle<ConfigStoreImpl> config_store = TheServiceParticipant->config_store();
  const ConfigStoreImpl::StringList sections = config_store->get_section_names("DATAREADERQOS");

  // Loop through the [datareaderqos/*] sections
  for (ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    const String& datareaderqos_name = *pos;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_datareaderqos ")
                  ACE_TEXT("processing [datareaderqos/%C] section.\n"),
                  datareaderqos_name.c_str()));
    }

    const String prefix = String("DATAREADERQOS_") + datareaderqos_name;

    DDS::DataReaderQos datareaderqos(TheServiceParticipant->initial_DataReaderQos());

    datareaderqos.durability.kind =
      config_store->get((prefix + "_DURABILITY_KIND").c_str(), datareaderqos.durability.kind, durability_kinds);

    datareaderqos.deadline.period.sec =
      config_store->get_int32((prefix + "_DEADLINE_PERIOD_SEC").c_str(), datareaderqos.deadline.period.sec);
    datareaderqos.deadline.period.nanosec =
      config_store->get_uint32((prefix + "_DEADLINE_PERIOD_NANOSEC").c_str(), datareaderqos.deadline.period.nanosec);

    datareaderqos.latency_budget.duration.sec =
      config_store->get_int32((prefix + "_LATENCY_BUDGET_DURATION_SEC").c_str(), datareaderqos.latency_budget.duration.sec);
    datareaderqos.latency_budget.duration.nanosec =
      config_store->get_uint32((prefix + "_LATENCY_BUDGET_DURATION_NANOSEC").c_str(), datareaderqos.latency_budget.duration.nanosec);

    datareaderqos.liveliness.kind =
      config_store->get((prefix + "_LIVELINESS_KIND").c_str(), datareaderqos.liveliness.kind, liveliness_kinds);
    datareaderqos.liveliness.lease_duration.sec =
      config_store->get_int32((prefix + "_LIVELINESS_LEASE_DURATION_SEC").c_str(), datareaderqos.liveliness.lease_duration.sec);
    datareaderqos.liveliness.lease_duration.nanosec =
      config_store->get_uint32((prefix + "_LIVELINESS_LEASE_DURATION_NANOSEC").c_str(), datareaderqos.liveliness.lease_duration.nanosec);

    datareaderqos.reliability.kind =
      config_store->get((prefix + "_RELIABILITY_KIND").c_str(), datareaderqos.reliability.kind, reliability_kinds);
    datareaderqos.reliability.max_blocking_time.sec =
      config_store->get_int32((prefix + "_RELIABILITY_MAX_BLOCKING_TIME_SEC").c_str(), datareaderqos.reliability.max_blocking_time.sec);
    datareaderqos.reliability.max_blocking_time.nanosec =
      config_store->get_uint32((prefix + "_RELIABILITY_MAX_BLOCKING_TIME_NANOSEC").c_str(), datareaderqos.reliability.max_blocking_time.nanosec);

    datareaderqos.destination_order.kind =
      config_store->get((prefix + "_DESTINATION_ORDER_KIND").c_str(), datareaderqos.destination_order.kind, destination_order_kinds);

    datareaderqos.history.kind =
      config_store->get((prefix + "_HISTORY_KIND").c_str(), datareaderqos.history.kind, history_kinds);
    datareaderqos.history.depth =
      config_store->get_int32((prefix + "_HISTORY_DEPTH").c_str(), datareaderqos.history.depth);

    datareaderqos.resource_limits.max_samples =
      config_store->get_int32((prefix + "_RESOURCE_LIMITS_MAX_SAMPLES").c_str(), datareaderqos.resource_limits.max_samples);
    datareaderqos.resource_limits.max_instances =
      config_store->get_int32((prefix + "_RESOURCE_LIMITS_MAX_INSTANCES").c_str(), datareaderqos.resource_limits.max_instances);
    datareaderqos.resource_limits.max_samples_per_instance =
      config_store->get_int32((prefix + "_RESOURCE_LIMITS_MAX_SAMPLES_PER_INSTANCE").c_str(), datareaderqos.resource_limits.max_samples_per_instance);

    datareaderqos.time_based_filter.minimum_separation.sec =
      config_store->get_int32((prefix + "_TIME_BASED_FILTER_MINIMUM_SEPARATION_SEC").c_str(), datareaderqos.time_based_filter.minimum_separation.sec);
    datareaderqos.time_based_filter.minimum_separation.nanosec =
      config_store->get_uint32((prefix + "_TIME_BASED_FILTER_MINIMUM_SEPARATION_NANOSEC").c_str(), datareaderqos.time_based_filter.minimum_separation.nanosec);

    datareaderqos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec =
      config_store->get_int32((prefix + "_READER_DATA_LIFECYCLE_AUTOPURGE_NOWRITER_SAMPLES_DELAY_SEC").c_str(), datareaderqos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec);
    datareaderqos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec =
      config_store->get_uint32((prefix + "_READER_DATA_LIFECYCLE_AUTOPURGE_NOWRITER_SAMPLES_DELAY_NANOSEC").c_str(), datareaderqos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec);

    datareaderqos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec =
      config_store->get_int32((prefix + "_READER_DATA_LIFECYCLE_AUTOPURGE_DISPOSED_SAMPLES_DELAY_SEC").c_str(), datareaderqos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec);
    datareaderqos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec =
      config_store->get_uint32((prefix + "_READER_DATA_LIFECYCLE_AUTOPURGE_DISPOSED_SAMPLES_DELAY_NANOSEC").c_str(), datareaderqos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec);

    registry.datareaderqos_map[datareaderqos_name] = datareaderqos;
  }

  return 0;
}

int
StaticDiscovery::parse_publisherqos()
{
  RcHandle<ConfigStoreImpl> config_store = TheServiceParticipant->config_store();
  const ConfigStoreImpl::StringList sections = config_store->get_section_names("PUBLISHERQOS");

  // Loop through the [publisherqos/*] sections
  for (ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    const String& publisherqos_name = *pos;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_publisherqos ")
                  ACE_TEXT("processing [publisherqos/%C] section.\n"),
                  publisherqos_name.c_str()));
    }

    const String prefix = String("PUBLISHERQOS_") + publisherqos_name;

    DDS::PublisherQos publisherqos(TheServiceParticipant->initial_PublisherQos());

    publisherqos.presentation.access_scope =
      config_store->get((prefix + "_PRESENTATION_ACCESS_SCOPE").c_str(), publisherqos.presentation.access_scope, access_scope_kinds);
    publisherqos.presentation.coherent_access =
      config_store->get_boolean((prefix + "_PRESENTATION_COHERENT_ACCESS").c_str(), publisherqos.presentation.coherent_access);
    publisherqos.presentation.ordered_access =
      config_store->get_boolean((prefix + "_PRESENTATION_ORDERED_ACCESS").c_str(), publisherqos.presentation.ordered_access);

    const ConfigStoreImpl::StringList partitions =
      config_store->get((prefix + "_PARTITION_NAME").c_str(), ConfigStoreImpl::StringList());
    string_list_to_partitions(publisherqos.partition, partitions);

    registry.publisherqos_map[publisherqos_name] = publisherqos;
  }

  return 0;
}

int
StaticDiscovery::parse_subscriberqos()
{
  RcHandle<ConfigStoreImpl> config_store = TheServiceParticipant->config_store();
  const ConfigStoreImpl::StringList sections = config_store->get_section_names("SUBSCRIBERQOS");

  // Loop through the [subscriberqos/*] sections
  for (ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    const String& subscriberqos_name = *pos;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_subscriberqos ")
                  ACE_TEXT("processing [subscriberqos/%C] section.\n"),
                  subscriberqos_name.c_str()));
    }

    const String prefix = String("SUBSCRIBERQOS_") + subscriberqos_name;

    DDS::SubscriberQos subscriberqos(TheServiceParticipant->initial_SubscriberQos());

    subscriberqos.presentation.access_scope =
      config_store->get((prefix + "_PRESENTATION_ACCESS_SCOPE").c_str(), subscriberqos.presentation.access_scope, access_scope_kinds);
    subscriberqos.presentation.coherent_access =
      config_store->get_boolean((prefix + "_PRESENTATION_COHERENT_ACCESS").c_str(), subscriberqos.presentation.coherent_access);
    subscriberqos.presentation.ordered_access =
      config_store->get_boolean((prefix + "_PRESENTATION_ORDERED_ACCESS").c_str(), subscriberqos.presentation.ordered_access);

    const ConfigStoreImpl::StringList partitions =
      config_store->get((prefix + "_PARTITION_NAME").c_str(), ConfigStoreImpl::StringList());
    string_list_to_partitions(subscriberqos.partition, partitions);

    registry.subscriberqos_map[subscriberqos_name] = subscriberqos;
  }

  return 0;
}

int
StaticDiscovery::parse_endpoints()
{
  RcHandle<ConfigStoreImpl> config_store = TheServiceParticipant->config_store();
  const ConfigStoreImpl::StringList sections = config_store->get_section_names("ENDPOINT");

  // Loop through the [endpoint/*] sections
  for (ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    const String& endpoint_name = *pos;

    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                  ACE_TEXT("(%P|%t) NOTICE: StaticDiscovery::parse_endpoints ")
                  ACE_TEXT("processing [endpoint/%C] section.\n"),
                  endpoint_name.c_str()));
    }

    const String prefix = String("ENDPOINT_") + endpoint_name;

    int domain;
    {
      if (!config_store->has((prefix + "_DOMAIN").c_str())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("No domain specified for [endpoint/%C] section.\n"),
                   endpoint_name.c_str()));
        return -1;
      }

      domain = config_store->get_int32((prefix + "_DOMAIN").c_str(), -1);
    }

    unsigned char participant[6] = { 0 };
    {
      if (!config_store->has((prefix + "_PARTICIPANT").c_str())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("No participant specified for [endpoint/%C] section.\n"),
                   endpoint_name.c_str()));
        return -1;
      }

      const String value = config_store->get((prefix + "_PARTICIPANT").c_str(), "");
      const OPENDDS_STRING::difference_type count = std::count_if(value.begin(), value.end(), isxdigit);
      if (value.size() != HEX_DIGITS_IN_PARTICIPANT || static_cast<size_t>(count) != HEX_DIGITS_IN_PARTICIPANT) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("participant (%C) must be 12 hexadecimal digits in [endpoint/%C] section.\n"),
                   value.c_str(), endpoint_name.c_str()));
        return -1;
      }

      for (size_t idx = 0; idx != BYTES_IN_PARTICIPANT; ++idx) {
        participant[idx] = fromhex(value, idx);
      }
    }

    unsigned char entity[3] = { 0 };
    {
      if (!config_store->has((prefix + "_ENTITY").c_str())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("No entity specified for [endpoint/%C] section.\n"),
                   endpoint_name.c_str()));
        return -1;
      }

      const String value = config_store->get((prefix + "_ENTITY").c_str(), "");
      const OPENDDS_STRING::difference_type count = std::count_if(value.begin(), value.end(), isxdigit);
      if (value.size() != HEX_DIGITS_IN_ENTITY || static_cast<size_t>(count) != HEX_DIGITS_IN_ENTITY) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("entity (%C) must be 6 hexadecimal digits in [endpoint/%C] section.\n"),
                   value.c_str(), endpoint_name.c_str()));
        return -1;
      }

      for (size_t idx = 0; idx != BYTES_IN_ENTITY; ++idx) {
        entity[idx] = fromhex(value, idx);
      }
    }

    Type type = Reader;
    {
      if (!config_store->has((prefix + "_TYPE").c_str())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("No type specified for [endpoint/%C] section.\n"),
                   endpoint_name.c_str()));
        return -1;
      }
      type = config_store->get((prefix + "_TYPE").c_str(), Reader, type_kinds);
    }

    String topic_name;
    {
      if (!config_store->has((prefix + "_TOPIC").c_str())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("No topic specified for [endpoint/%C] section.\n"),
                   endpoint_name.c_str()));
        return -1;
      }

      const String value = config_store->get((prefix + "_TOPIC").c_str(), "");
      EndpointRegistry::TopicMapType::const_iterator pos = this->registry.topic_map.find(value);
      if (pos == this->registry.topic_map.end()) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("Illegal topic reference (%C) in [endpoint/%C] section.\n"),
                   value.c_str(), endpoint_name.c_str()));
        return -1;
      }

      topic_name = pos->second.name;
    }

    DDS::DataWriterQos datawriterqos(TheServiceParticipant->initial_DataWriterQos());
    if (config_store->has((prefix + "_DATAWRITERQOS").c_str())) {
      const String value = config_store->get((prefix + "_DATAWRITERQOS").c_str(), "");
      EndpointRegistry::DataWriterQosMapType::const_iterator pos = this->registry.datawriterqos_map.find(value);
      if (pos == this->registry.datawriterqos_map.end()) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("Illegal datawriterqos reference (%C) in [endpoint/%C] section.\n"),
                   value.c_str(), endpoint_name.c_str()));
        return -1;
      }

      datawriterqos = pos->second;
    }

    DDS::DataReaderQos datareaderqos(TheServiceParticipant->initial_DataReaderQos());
    if (config_store->has((prefix + "_DATAREADERQOS").c_str())) {
      const String value = config_store->get((prefix + "_DATAREADERQOS").c_str(), "");
      EndpointRegistry::DataReaderQosMapType::const_iterator pos = this->registry.datareaderqos_map.find(value);
      if (pos == this->registry.datareaderqos_map.end()) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("Illegal datareaderqos reference (%C) in [endpoint/%C] section.\n"),
                   value.c_str(), endpoint_name.c_str()));
        return -1;
      }

      datareaderqos = pos->second;
    }

    DDS::PublisherQos publisherqos(TheServiceParticipant->initial_PublisherQos());
    if (config_store->has((prefix + "_PUBLISHERQOS").c_str())) {
      const String value = config_store->get((prefix + "_PUBLISHERQOS").c_str(), "");
      EndpointRegistry::PublisherQosMapType::const_iterator pos = this->registry.publisherqos_map.find(value);
      if (pos == this->registry.publisherqos_map.end()) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("Illegal publisherqos reference (%C) in [endpoint/%C] section.\n"),
                   value.c_str(), endpoint_name.c_str()));
        return -1;
      }

      publisherqos = pos->second;
    }

    DDS::SubscriberQos subscriberqos(TheServiceParticipant->initial_SubscriberQos());
    if (config_store->has((prefix + "_SUBSCRIBERQOS").c_str())) {
      const String value = config_store->get((prefix + "_SUBSCRIBERQOS").c_str(), "");
      EndpointRegistry::SubscriberQosMapType::const_iterator pos = this->registry.subscriberqos_map.find(value);
      if (pos == this->registry.subscriberqos_map.end()) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("Illegal subscriberqos reference (%C) in [endpoint/%C] section.\n"),
                   value.c_str(), endpoint_name.c_str()));
        return -1;
      }

      subscriberqos = pos->second;
    }

    String config_name;
    TransportConfig_rch config;
    if (config_store->has((prefix + "_CONFIG").c_str())) {
      config_name = config_store->get((prefix + "_CONFIG").c_str(), "");
      config = TheTransportRegistry->get_config(config_name);
      if (config.is_nil()) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: StaticDiscovery::parse_endpoints ")
                   ACE_TEXT("Illegal config reference (%C) in [endpoint/%C] section.\n"),
                   config_name.c_str(), endpoint_name.c_str()));
      }
    }

    if (config.is_nil()) {
      config = TheTransportRegistry->domain_default_config(domain);
    }

    if (config.is_nil()) {
      config = TheTransportRegistry->global_config();
    }

    TransportLocatorSeq trans_info;
    try {
      config->populate_locators(trans_info, domain);
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

    GUID_t id = EndpointRegistry::build_id(domain, participant, entity_id);

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
      set_reader_effective_data_rep_qos(datareaderqos.representation.value);
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
      bool encapsulated_only = false;
      for (CORBA::ULong i = 0; i < trans_info.length(); ++i) {
        if (0 == std::strcmp(trans_info[i].transport_type, "rtps_udp")) {
          encapsulated_only = true;
          break;
        }
      }
      set_writer_effective_data_rep_qos(datawriterqos.representation.value, encapsulated_only);

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
  const GUID_t rid = EndpointRegistry::build_id(dom, partId, entId);

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
  const GUID_t rid = EndpointRegistry::build_id(dom, partId, entId);

  const EndpointRegistry::ReaderMapType::const_iterator iter =
    registry.reader_map.find(rid);

  if (iter != registry.reader_map.end() && !iter->second.trans_cfg.empty()) {
    TransportRegistry::instance()->bind_config(iter->second.trans_cfg, reader);
  }
}

StaticDiscovery_rch StaticDiscovery::instance_(make_rch<StaticDiscovery>(Discovery::DEFAULT_STATIC));

RcHandle<BitSubscriber> StaticDiscovery::init_bit(DomainParticipantImpl* participant)
{
  DDS::Subscriber_var bit_subscriber;
#ifndef DDS_HAS_MINIMUM_BIT
  if (!TheServiceParticipant->get_BIT()) {
    get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber);
    return RcHandle<BitSubscriber>();
  }

  if (create_bit_topics(participant) != DDS::RETCODE_OK) {
    return RcHandle<BitSubscriber>();
  }

  bit_subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil(),
                                   DEFAULT_STATUS_MASK);
  SubscriberImpl* sub = dynamic_cast<SubscriberImpl*>(bit_subscriber.in());
  if (sub == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::init_bit")
               ACE_TEXT(" - Could not cast Subscriber to SubscriberImpl\n")));
    return RcHandle<BitSubscriber>();
  }

  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

  dr_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay =
    TheServiceParticipant->bit_autopurge_nowriter_samples_delay();
  dr_qos.reader_data_lifecycle.autopurge_disposed_samples_delay =
    TheServiceParticipant->bit_autopurge_disposed_samples_delay();

  DDS::TopicDescription_var bit_part_topic =
    participant->lookup_topicdescription(BUILT_IN_PARTICIPANT_TOPIC);
  create_bit_dr(bit_part_topic, BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_topic_topic =
    participant->lookup_topicdescription(BUILT_IN_TOPIC_TOPIC);
  create_bit_dr(bit_topic_topic, BUILT_IN_TOPIC_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_pub_topic =
    participant->lookup_topicdescription(BUILT_IN_PUBLICATION_TOPIC);
  create_bit_dr(bit_pub_topic, BUILT_IN_PUBLICATION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_sub_topic =
    participant->lookup_topicdescription(BUILT_IN_SUBSCRIPTION_TOPIC);
  create_bit_dr(bit_sub_topic, BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_part_loc_topic =
    participant->lookup_topicdescription(BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
  create_bit_dr(bit_part_loc_topic, BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_connection_record_topic =
    participant->lookup_topicdescription(BUILT_IN_CONNECTION_RECORD_TOPIC);
  create_bit_dr(bit_connection_record_topic, BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_internal_thread_topic =
    participant->lookup_topicdescription(BUILT_IN_INTERNAL_THREAD_TOPIC);
  create_bit_dr(bit_internal_thread_topic, BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE,
                sub, dr_qos);

  const DDS::ReturnCode_t ret = bit_subscriber->enable();
  if (ret != DDS::RETCODE_OK) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) PeerDiscovery::init_bit")
                 ACE_TEXT(" - Error %d enabling subscriber\n"), ret));
    }
    return RcHandle<BitSubscriber>();
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber);

  return make_rch<BitSubscriber>(bit_subscriber);
}

void StaticDiscovery::fini_bit(DCPS::DomainParticipantImpl* participant)
{
  get_part(participant->get_domain_id(), participant->get_id())->fini_bit();
}

bool StaticDiscovery::attach_participant(
  DDS::DomainId_t /*domainId*/, const GUID_t& /*participantId*/)
{
  return false; // This is just for DCPSInfoRepo?
}

bool StaticDiscovery::remove_domain_participant(
  DDS::DomainId_t domain_id, const GUID_t& participantId)
{
  // Use reference counting to ensure participant
  // does not get deleted until lock as been released.
  ParticipantHandle participant;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  DomainParticipantMap::iterator domain = participants_.find(domain_id);
  if (domain == participants_.end()) {
    return false;
  }
  ParticipantMap::iterator part = domain->second.find(participantId);
  if (part == domain->second.end()) {
    return false;
  }
  participant = part->second;
  domain->second.erase(part);
  if (domain->second.empty()) {
    participants_.erase(domain);
  }

  participant->shutdown();
  return true;
}

bool StaticDiscovery::ignore_domain_participant(
  DDS::DomainId_t domain, const GUID_t& myParticipantId, const GUID_t& ignoreId)
{
  get_part(domain, myParticipantId)->ignore_domain_participant(ignoreId);
  return true;
}

bool StaticDiscovery::update_domain_participant_qos(
  DDS::DomainId_t domain, const GUID_t& participant, const DDS::DomainParticipantQos& qos)
{
  return get_part(domain, participant)->update_domain_participant_qos(qos);
}

DCPS::TopicStatus StaticDiscovery::assert_topic(
  GUID_t& topicId,
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const char* topicName,
  const char* dataTypeName,
  const DDS::TopicQos& qos,
  bool hasDcpsKey,
  DCPS::TopicCallbacks* topic_callbacks)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  // Verified its safe to hold lock during call to assert_topic
  return participants_[domainId][participantId]->assert_topic(topicId, topicName,
                                                              dataTypeName, qos,
                                                              hasDcpsKey, topic_callbacks);
}

DCPS::TopicStatus StaticDiscovery::find_topic(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const char* topicName,
  CORBA::String_out dataTypeName,
  DDS::TopicQos_out qos,
  GUID_t& topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  return participants_[domainId][participantId]->find_topic(topicName, dataTypeName, qos, topicId);
}

DCPS::TopicStatus StaticDiscovery::remove_topic(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const GUID_t& topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  // Safe to hold lock while calling remove topic
  return participants_[domainId][participantId]->remove_topic(topicId);
}

bool StaticDiscovery::ignore_topic(DDS::DomainId_t domainId, const GUID_t& myParticipantId,
                                 const GUID_t& ignoreId)
{
  get_part(domainId, myParticipantId)->ignore_topic(ignoreId);
  return true;
}

bool StaticDiscovery::update_topic_qos(const GUID_t& topicId, DDS::DomainId_t domainId,
                                    const GUID_t& participantId, const DDS::TopicQos& qos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  // Safe to hold lock while calling update_topic_qos
  return participants_[domainId][participantId]->update_topic_qos(topicId, qos);
}

bool StaticDiscovery::add_publication(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const GUID_t& topicId,
  DCPS::DataWriterCallbacks_rch publication,
  const DDS::DataWriterQos& qos,
  const DCPS::TransportLocatorSeq& transInfo,
  const DDS::PublisherQos& publisherQos,
  const XTypes::TypeInformation& type_info)
{
  return get_part(domainId, participantId)->add_publication(topicId, publication, qos, transInfo, publisherQos, type_info);
}

bool StaticDiscovery::remove_publication(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& publicationId)
{
  get_part(domainId, participantId)->remove_publication(publicationId);
  return true;
}

bool StaticDiscovery::ignore_publication(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& ignoreId)
{
  get_part(domainId, participantId)->ignore_publication(ignoreId);
  return true;
}

bool StaticDiscovery::update_publication_qos(
  DDS::DomainId_t domainId,
  const GUID_t& partId,
  const GUID_t& dwId,
  const DDS::DataWriterQos& qos,
  const DDS::PublisherQos& publisherQos)
{
  return get_part(domainId, partId)->update_publication_qos(dwId, qos,
                                                            publisherQos);
}

void StaticDiscovery::update_publication_locators(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& dwId,
  const DCPS::TransportLocatorSeq& transInfo)
{
  get_part(domainId, partId)->update_publication_locators(dwId, transInfo);
}

bool StaticDiscovery::add_subscription(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const GUID_t& topicId,
  DCPS::DataReaderCallbacks_rch subscription,
  const DDS::DataReaderQos& qos,
  const DCPS::TransportLocatorSeq& transInfo,
  const DDS::SubscriberQos& subscriberQos,
  const char* filterClassName,
  const char* filterExpr,
  const DDS::StringSeq& params,
  const XTypes::TypeInformation& type_info)
{
  return get_part(domainId, participantId)->add_subscription(topicId,
                                                             subscription,
                                                             qos,
                                                             transInfo,
                                                             subscriberQos,
                                                             filterClassName,
                                                             filterExpr,
                                                             params,
                                                             type_info);
}

bool StaticDiscovery::remove_subscription(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& subscriptionId)
{
  get_part(domainId, participantId)->remove_subscription(subscriptionId);
  return true;
}

bool StaticDiscovery::ignore_subscription(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& ignoreId)
{
  get_part(domainId, participantId)->ignore_subscription(ignoreId);
  return true;
}

bool StaticDiscovery::update_subscription_qos(
  DDS::DomainId_t domainId,
  const GUID_t& partId,
  const GUID_t& drId,
  const DDS::DataReaderQos& qos,
  const DDS::SubscriberQos& subQos)
{
  return get_part(domainId, partId)->update_subscription_qos(drId, qos, subQos);
}

bool StaticDiscovery::update_subscription_params(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& subId, const DDS::StringSeq& params)
{
  return get_part(domainId, partId)->update_subscription_params(subId, params);
}

void StaticDiscovery::update_subscription_locators(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& subId,
  const DCPS::TransportLocatorSeq& transInfo)
{
  get_part(domainId, partId)->update_subscription_locators(subId, transInfo);
}

StaticDiscovery::ParticipantHandle StaticDiscovery::get_part(
  const DDS::DomainId_t domain_id, const GUID_t& part_id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ParticipantHandle());
  DomainParticipantMap::const_iterator domain = participants_.find(domain_id);
  if (domain == participants_.end()) {
    return ParticipantHandle();
  }
  ParticipantMap::const_iterator part = domain->second.find(part_id);
  if (part == domain->second.end()) {
    return ParticipantHandle();
  }
  return part->second;
}

void StaticDiscovery::create_bit_dr(DDS::TopicDescription_ptr topic, const char* type,
  SubscriberImpl* sub, const DDS::DataReaderQos& qos)
{
  TopicDescriptionImpl* bit_topic_i =
    dynamic_cast<TopicDescriptionImpl*>(topic);
  if (bit_topic_i == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::create_bit_dr")
               ACE_TEXT(" - Could not cast TopicDescription to TopicDescriptionImpl\n")));
    return;
  }

  DDS::DomainParticipant_var participant = sub->get_participant();
  DomainParticipantImpl* participant_i =
    dynamic_cast<DomainParticipantImpl*>(participant.in());
  if (participant_i == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::create_bit_dr")
               ACE_TEXT(" - Could not cast DomainParticipant to DomainParticipantImpl\n")));
    return;
  }

  TypeSupport_var type_support =
    Registered_Data_Types->lookup(participant, type);

  DDS::DataReader_var dr = type_support->create_datareader();
  DataReaderImpl* dri = dynamic_cast<DataReaderImpl*>(dr.in());
  if (dri == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::create_bit_dr")
               ACE_TEXT(" - Could not cast DataReader to DataReaderImpl\n")));
    return;
  }

  dri->init(bit_topic_i, qos, 0 /*listener*/, 0 /*mask*/, participant_i, sub);
  dri->disable_transport();
  dri->enable();
}

void StaticParticipant::remove_discovered_participant(DiscoveredParticipantIter& iter)
{
  if (iter == participants_.end()) {
    return;
  }
  GUID_t part_id = iter->first;
  bool removed = endpoint_manager().disassociate();
  iter = participants_.find(part_id); // refresh iter after disassociate, which can unlock
  if (iter == participants_.end()) {
    return;
  }
  if (removed) {
#ifndef DDS_HAS_MINIMUM_BIT
    ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
    ParticipantLocationBuiltinTopicDataDataReaderImpl* loc_bit = part_loc_bit();
    // bit may be null if the DomainParticipant is shutting down
    if ((bit && iter->second.bit_ih_ != DDS::HANDLE_NIL) ||
        (loc_bit && iter->second.location_ih_ != DDS::HANDLE_NIL)) {
      {
        const DDS::InstanceHandle_t bit_ih = iter->second.bit_ih_;
        const DDS::InstanceHandle_t location_ih = iter->second.location_ih_;

        ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);
        ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
        if (bit && bit_ih != DDS::HANDLE_NIL) {
          bit->set_instance_state(bit_ih,
                                  DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
        }
        if (loc_bit && location_ih != DDS::HANDLE_NIL) {
          loc_bit->set_instance_state(location_ih,
                                      DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
        }
      }
      iter = participants_.find(part_id);
      if (iter == participants_.end()) {
        return;
      }
    }
#endif /* DDS_HAS_MINIMUM_BIT */
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) LocalParticipant::remove_discovered_participant")
                 ACE_TEXT(" - erasing %C (%B)\n"), LogGuid(iter->first).c_str(), participants_.size()));
    }

    remove_discovered_participant_i(iter);

    participants_.erase(iter);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
