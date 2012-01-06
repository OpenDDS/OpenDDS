/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Sedp.h"
#include "Spdp.h"
#include "MessageTypes.h"
#include "RtpsDiscovery.h"
#include "RtpsMessageTypesC.h"
#include "RtpsBaseMessageTypesTypeSupportImpl.h"
#include "ParameterListConverter.h"

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DCPS_Utils.h"

#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"



#include <cstring>


namespace OpenDDS {
namespace DCPS {
  typedef RcHandle<RtpsUdpInst> RtpsUdpInst_rch;
}

namespace RTPS {
using DCPS::RepoId;

const bool Sedp::host_is_bigendian_(!ACE_CDR_BYTE_ORDER);

Sedp::Sedp(const RepoId& participant_id, Spdp& owner, ACE_Thread_Mutex& lock)
  : participant_id_(participant_id)
  , spdp_(owner)
  , lock_(lock)
  , publications_writer_(make_id(participant_id,
                                 ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER),
                         *this)
  , subscriptions_writer_(make_id(participant_id,
                                  ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER),
                          *this)
  , publications_reader_(make_id(participant_id,
                                 ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER),
                         *this)
  , subscriptions_reader_(make_id(participant_id,
                                  ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER),
                          *this)
  , publication_counter_(0), subscription_counter_(0), topic_counter_(0)
{
  pub_bit_key_.value[0] = pub_bit_key_.value[1] = pub_bit_key_.value[2] = 0;
  sub_bit_key_.value[0] = sub_bit_key_.value[1] = sub_bit_key_.value[2] = 0;
}

RepoId
Sedp::make_id(const DCPS::RepoId& participant_id, const EntityId_t& entity)
{
  RepoId id = participant_id;
  id.entityId = entity;
  return id;
}

DDS::ReturnCode_t
Sedp::init(const RepoId& guid,
           RtpsDiscovery& disco,
           DDS::DomainId_t domainId)
{
  char domainStr[16];
  ACE_OS::snprintf(domainStr, 16, "%d", domainId);

  std::string key = OpenDDS::DCPS::GuidConverter(guid).uniqueId();

  // configure one transport
  transport_inst_ = TheTransportRegistry->create_inst(
                       DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                       "_SEDPTransportInst_" + key + domainStr, "rtps_udp");
  // Use a static cast to avoid dependency on the RtpsUdp library
  DCPS::RtpsUdpInst_rch rtps_inst = 
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);

  // Bind to a specific multicast group
  const u_short mc_port = disco.pb() +
                          disco.dg() * domainId +
                          disco.dx();

  const char mc_addr[] = "239.255.0.1" /*RTPS v2.1 9.6.1.4.1*/;
  if (rtps_inst->multicast_group_address_.set(mc_port, mc_addr)) {
    ACE_DEBUG((LM_ERROR, "(%P|%t) Sedp::init() - "
                         "failed setting multicast local_addr to port %hd\n",
                         mc_port));
    return DDS::RETCODE_ERROR;
  }

  // Crete a config
  std::string config_name = DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                            "_SEDP_TransportCfg_" + key + domainStr;
  transport_cfg_ = TheTransportRegistry->create_config(config_name);
  transport_cfg_->instances_.push_back(transport_inst_);

  // Configure and enable each reader/writer
  bool force_reliability = true;
  publications_writer_.enable_transport(force_reliability, transport_cfg_);
  publications_reader_.enable_transport(force_reliability, transport_cfg_);
  subscriptions_writer_.enable_transport(force_reliability, transport_cfg_);
  subscriptions_reader_.enable_transport(force_reliability, transport_cfg_);

  return DDS::RETCODE_OK;
}

const ACE_INET_Addr&
Sedp::local_address() const
{
  DCPS::RtpsUdpInst_rch rtps_inst = 
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  return rtps_inst->local_address_;
}

const ACE_INET_Addr&
Sedp::multicast_group() const
{
  DCPS::RtpsUdpInst_rch rtps_inst = 
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  return rtps_inst->multicast_group_address_;
}

void
Sedp::ignore(const DCPS::RepoId& to_ignore)
{
  ignored_guids_.insert(to_ignore);
  {
    const DiscoveredPublicationIter iter =
      discovered_publications_.find(to_ignore);
    if (iter != discovered_publications_.end()) {
      //TODO: break associations
      topics_[get_topic_name(iter->second)].endpoints_.erase(iter->first);
      remove_from_bit(iter->second);
      discovered_publications_.erase(iter);
      return;
    }
  }
  {
    const DiscoveredSubscriptionIter iter =
      discovered_subscriptions_.find(to_ignore);
    if (iter != discovered_subscriptions_.end()) {
      //TODO: break associations
      topics_[get_topic_name(iter->second)].endpoints_.erase(iter->first);
      remove_from_bit(iter->second);
      discovered_subscriptions_.erase(iter);
      return;
    }
  }
  {
    const std::map<RepoId, std::string, DCPS::GUID_tKeyLessThan>::iterator
      iter = topic_names_.find(to_ignore);
    if (iter != topic_names_.end()) {
      ignored_topics_.insert(iter->second);
      //TODO: if we know of any publication(s) and/or subscription(s) on this
      //      topic remove them and break any associations.
    }
  }
}

RepoId
Sedp::bit_key_to_repo_id(const char* bit_topic_name,
                         const DDS::BuiltinTopicKey_t& key)
{
  if (0 == std::strcmp(bit_topic_name, DCPS::BUILT_IN_PUBLICATION_TOPIC)) {
    return pub_key_to_id_[key];
  }
  if (0 == std::strcmp(bit_topic_name, DCPS::BUILT_IN_SUBSCRIPTION_TOPIC)) {
    return sub_key_to_id_[key];
  }
  return RepoId();
}

void
Sedp::assign_bit_key(DiscoveredPublication& pub)
{
  increment_key(pub_bit_key_);
  pub_key_to_id_[pub_bit_key_] = pub.writer_data_.writerProxy.remoteWriterGuid;
  pub.writer_data_.ddsPublicationData.key = pub_bit_key_;
}

void
Sedp::assign_bit_key(DiscoveredSubscription& sub)
{
  increment_key(sub_bit_key_);
  sub_key_to_id_[sub_bit_key_] = sub.reader_data_.readerProxy.remoteReaderGuid;
  sub.reader_data_.ddsSubscriptionData.key = sub_bit_key_;
}

void
Sedp::increment_key(DDS::BuiltinTopicKey_t& key)
{
  for (int idx = 0; idx < 3; ++idx) {
    CORBA::ULong ukey = static_cast<CORBA::ULong>(key.value[idx]);
    if (ukey == 0xFFFFFFFF) {
      key.value[idx] = 0;
    } else {
      ++ukey;
      key.value[idx] = ukey;
      return;
    }
  }
  //TODO: error, we ran out of keys (2^96 available)
}

void
Sedp::associate(const SPDPdiscoveredParticipantData& pdata)
{
  // First create a 'prototypical' instance of AssociationData.  It will
  // be copied and modified for each of the (up to) four SEDP Endpoints.
  DCPS::AssociationData proto;
  proto.publication_transport_priority_ = 0;
  std::memcpy(proto.remote_id_.guidPrefix, pdata.participantProxy.guidPrefix,
              sizeof(GuidPrefix_t));

  const LocatorSeq& mll =
    pdata.participantProxy.metatrafficMulticastLocatorList;
  const LocatorSeq& ull =
    pdata.participantProxy.metatrafficUnicastLocatorList;
  const CORBA::ULong locator_count = mll.length() + ull.length();

  ACE_Message_Block mb_locator(4 + locator_count * sizeof(Locator_t));
  using DCPS::Serializer;
  Serializer ser_loc(&mb_locator, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser_loc << locator_count;

  for (CORBA::ULong i = 0; i < mll.length(); ++i) {
    ser_loc << mll[i];
  }
  for (CORBA::ULong i = 0; i < ull.length(); ++i) {
    ser_loc << ull[i];
  }

  proto.remote_data_.length(1);
  proto.remote_data_[0].transport_type = "rtps_udp";
  proto.remote_data_[0].data.replace(
    static_cast<CORBA::ULong>(mb_locator.length()), &mb_locator);

  const BuiltinEndpointSet_t& avail =
    pdata.participantProxy.availableBuiltinEndpoints;

  // See RTPS v2.1 section 8.5.5.1
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    publications_writer_.assoc(peer);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    publications_reader_.assoc(peer);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    subscriptions_writer_.assoc(peer);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    subscriptions_reader_.assoc(peer);
  }
  //FUTURE: if/when topic propagation is supported, add it here

  // Write durable data
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    write_durable_publication_data();
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    write_durable_subscription_data();
  }

}

void
Sedp::disassociate(const SPDPdiscoveredParticipantData& pdata)
{
  RepoId part;
  std::memcpy(part.guidPrefix, pdata.participantProxy.guidPrefix,
              sizeof(GuidPrefix_t));
  const BuiltinEndpointSet_t& avail =
    pdata.participantProxy.availableBuiltinEndpoints;

  // See RTPS v2.1 section 8.5.5.2
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    RepoId id = part;
    id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    publications_writer_.disassociate(id);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    RepoId id = part;
    id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    publications_reader_.disassociate(id);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    RepoId id = part;
    id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    subscriptions_writer_.disassociate(id);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    RepoId id = part;
    id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    subscriptions_reader_.disassociate(id);
  }
  //FUTURE: if/when topic propagation is supported, add it here

  remove_entities_belonging_to(discovered_publications_, part);
  remove_entities_belonging_to(discovered_subscriptions_, part);
}

template<typename Map>
void
Sedp::remove_entities_belonging_to(Map& m, const RepoId& participant)
{
  for (typename Map::iterator i = m.lower_bound(participant);
       i != m.end() && 0 == std::memcmp(i->first.guidPrefix,
                                        participant.guidPrefix,
                                        sizeof(GuidPrefix_t));) {
    topics_[get_topic_name(i->second)].endpoints_.erase(i->first);
    remove_from_bit(i->second);
    m.erase(i++);
  }
}

void
Sedp::remove_from_bit(const DiscoveredPublication& pub)
{
  pub_key_to_id_.erase(pub.writer_data_.ddsPublicationData.key);
  DDS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
  if (bit) { // bit may be null if the DomainParticipant is shutting down
    bit->set_instance_state(pub.bit_ih_,
                            DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  }
}

void
Sedp::remove_from_bit(const DiscoveredSubscription& sub)
{
  sub_key_to_id_.erase(sub.reader_data_.ddsSubscriptionData.key);
  DDS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
  if (bit) { // bit may be null if the DomainParticipant is shutting down
    bit->set_instance_state(sub.bit_ih_,
                            DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  }
}

DDS::TopicBuiltinTopicDataDataReaderImpl*
Sedp::topic_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_TOPIC_TOPIC);
  return dynamic_cast<DDS::TopicBuiltinTopicDataDataReaderImpl*>(d.in());
}

DDS::PublicationBuiltinTopicDataDataReaderImpl*
Sedp::pub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_PUBLICATION_TOPIC);
  return dynamic_cast<DDS::PublicationBuiltinTopicDataDataReaderImpl*>(d.in());
}

DDS::SubscriptionBuiltinTopicDataDataReaderImpl*
Sedp::sub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  return dynamic_cast<DDS::SubscriptionBuiltinTopicDataDataReaderImpl*>(d.in());
}

DCPS::TopicStatus
Sedp::assert_topic(DCPS::RepoId_out topicId, const char* topicName,
                   const char* dataTypeName, const DDS::TopicQos& qos,
                   bool hasDcpsKey)
{
  std::map<std::string, TopicDetailsEx>::iterator iter =
    topics_.find(topicName);
  if (iter != topics_.end()) { // types must match, RtpsInfo checked for us
    iter->second.qos_ = qos;
    topicId = iter->second.repo_id_;
    return DCPS::FOUND;
  }

  TopicDetailsEx& td = topics_[topicName];
  td.data_type_ = dataTypeName;
  td.qos_ = qos;
  td.has_dcps_key_ = hasDcpsKey;
  td.repo_id_ = participant_id_;
  td.repo_id_.entityId.entityKind = DCPS::ENTITYKIND_OPENDDS_TOPIC;
  assign(td.repo_id_.entityId.entityKey, topic_counter_++);
  topic_names_[td.repo_id_] = topicName;
  topicId = td.repo_id_;

  if (topic_counter_ == 0x1000000) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::assert_topic: ")
               ACE_TEXT("Exceeded Maximum number of topic entity keys!")
               ACE_TEXT("Next key will be a duplicate!\n")));
    topic_counter_ = 0;
  }

  return DCPS::CREATED;
}

DCPS::TopicStatus
Sedp::remove_topic(const RepoId& topicId, std::string& name)
{
  name = topic_names_[topicId];
  topics_.erase(name);
  topic_names_.erase(topicId);
  return DCPS::REMOVED;
}

bool
Sedp::update_topic_qos(const RepoId& topicId, const DDS::TopicQos& qos,
                       std::string& name)
{
  std::map<DCPS::RepoId, std::string, DCPS::GUID_tKeyLessThan>::iterator iter =
    topic_names_.find(topicId);
  if (iter != topic_names_.end()) {
    name = iter->second;
    topics_[name].qos_ = qos;
    return true;
  }
  return false;
}

bool
Sedp::has_dcps_key(const RepoId& topicId) const
{
  typedef std::map<DCPS::RepoId, std::string, DCPS::GUID_tKeyLessThan> TNMap;
  TNMap::const_iterator tn = topic_names_.find(topicId);
  if (tn == topic_names_.end()) return false;

  typedef std::map<std::string, TopicDetailsEx> TDMap;
  TDMap::const_iterator td = topics_.find(tn->second);
  if (td == topics_.end()) return false;

  return td->second.has_dcps_key_;
}

RepoId
Sedp::add_publication(const RepoId& topicId,
                      DCPS::DataWriterRemote_ptr publication,
                      const DDS::DataWriterQos& qos,
                      const DCPS::TransportLocatorSeq& transInfo,
                      const DDS::PublisherQos& publisherQos)
{
  RepoId rid = participant_id_;
  rid.entityId.entityKind =
    has_dcps_key(topicId)
    ? DCPS::ENTITYKIND_USER_WRITER_WITH_KEY
    : DCPS::ENTITYKIND_USER_WRITER_NO_KEY;
  assign(rid.entityId.entityKey, publication_counter_++);
  LocalPublication& pb = local_publications_[rid];
  pb.topic_id_ = topicId;
  pb.publication_ = publication;
  pb.qos_ = qos;
  pb.trans_info_ = transInfo;
  pb.publisher_qos_ = publisherQos;
  TopicDetailsEx& td = topics_[topic_names_[topicId]];
  td.endpoints_.insert(rid);

  if (DDS::RETCODE_OK != write_publication_data(rid, pb))
  {
    // TODO: should this be removed from the local_publications map?
  }

/*
  DiscoveredWriterData dwd;
  if (DDS::RETCODE_OK == populate_discovered_writer_msg(dwd, rid, pb)) {
    ACE_DEBUG((LM_ERROR, "Successfully populated DiscoveredWriterData msg\n"));
    if (DDS::RETCODE_OK != publications_writer_.publish_sample(dwd)) {
      ACE_DEBUG((LM_ERROR, "Failed to publish DiscoveredWriterData msg\n"));
      // TODO: should this be removed from the local_publications map?
    }
    find_matching_endpoints(rid, td);
  } else {
    ACE_DEBUG((LM_ERROR, "(%P|%t) Failed to populate DiscoveredWriterData msg\n"));
    // TODO: should this be removed from the local_publications map?
  }
*/
  return rid;
}

void
Sedp::remove_publication(const RepoId& publicationId)
{
  LocalPublicationIter iter = local_publications_.find(publicationId);
  if (iter != local_publications_.end()) {
    if (DDS::RETCODE_OK == 
          publications_writer_.write_unregister_dispose(publicationId))
    {
      local_publications_.erase(publicationId);
    } else {
      ACE_DEBUG((LM_ERROR, "Failed to publish DiscoveredWriterData msg\n"));
    }
  }
}

bool
Sedp::update_publication_qos(const RepoId& publicationId,
                             const DDS::DataWriterQos& qos,
                             const DDS::PublisherQos& publisherQos)
{
  LocalPublicationIter iter = local_publications_.find(publicationId);
  if (iter != local_publications_.end()) {
    LocalPublication& pb = iter->second;
    pb.qos_ = qos;
    pb.publisher_qos_ = publisherQos;

    if (DDS::RETCODE_OK != write_publication_data(publicationId, pb))
    {
      return false;
    }
    return true;
  }
  return false;
}

RepoId
Sedp::add_subscription(const RepoId& topicId,
                       DCPS::DataReaderRemote_ptr subscription,
                       const DDS::DataReaderQos& qos,
                       const DCPS::TransportLocatorSeq& transInfo,
                       const DDS::SubscriberQos& subscriberQos,
                       const char* filterExpr,
                       const DDS::StringSeq& params)
{
  RepoId rid = participant_id_;
  rid.entityId.entityKind =
    has_dcps_key(topicId)
    ? DCPS::ENTITYKIND_USER_READER_WITH_KEY
    : DCPS::ENTITYKIND_USER_READER_NO_KEY;
  assign(rid.entityId.entityKey, subscription_counter_++);
  LocalSubscription& sb = local_subscriptions_[rid];
  sb.topic_id_ = topicId;
  sb.subscription_ = subscription;
  sb.qos_ = qos;
  sb.trans_info_ = transInfo;
  sb.subscriber_qos_ = subscriberQos;
  sb.params_ = params;
  topics_[topic_names_[topicId]].endpoints_.insert(rid);

  // TODO: use SEDP to advertise the new subscription

  return rid;

}

void
Sedp::remove_subscription(const RepoId& subscriptionId)
{
  // TODO: use SEDP to dispose the subscription
  local_subscriptions_.erase(subscriptionId);
}

bool
Sedp::update_subscription_qos(const RepoId& subscriptionId,
                              const DDS::DataReaderQos& qos,
                              const DDS::SubscriberQos& subscriberQos)
{
  LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
  if (iter != local_subscriptions_.end()) {
    LocalSubscription& sb = iter->second;
    sb.qos_ = qos;
    sb.subscriber_qos_ = subscriberQos;
    // TODO: tell the world about the change with SEDP

    return true;
  }
  return false;
}

bool
Sedp::update_subscription_params(const RepoId& subId,
                                 const DDS::StringSeq& params)
{
  LocalSubscriptionIter iter = local_subscriptions_.find(subId);
  if (iter != local_subscriptions_.end()) {
    LocalSubscription& sb = iter->second;
    sb.params_ = params;
    // TODO: tell the world about the change with SEDP

    return true;
  }
  return false;
}

void
Sedp::data_received(char message_id, const DiscoveredWriterData& wdata)
{
  const RepoId& guid = wdata.writerProxy.remoteWriterGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(wdata.ddsPublicationData.topic_name)) {
    return;
  }

  const DiscoveredPublicationIter iter = discovered_publications_.find(guid);

  if (message_id == DCPS::SAMPLE_DATA) {
    if (iter == discovered_publications_.end()) { // add new
      DiscoveredPublication& pub =
        discovered_publications_[guid] = DiscoveredPublication(wdata);
      std::memcpy(pub.writer_data_.ddsPublicationData.participant_key.value,
                  guid.guidPrefix, sizeof(DDS::BuiltinTopicKey_t));
      assign_bit_key(pub);
      pub.bit_ih_ =
        pub_bit()->store_synthetic_data(pub.writer_data_.ddsPublicationData,
                                        DDS::NEW_VIEW_STATE);
      topics_[get_topic_name(pub)].endpoints_.insert(guid);
      //TODO: match local subscription(s)
    } else if (qosChanged(iter->second.writer_data_.ddsPublicationData,
                          wdata.ddsPublicationData)) { // update existing
      pub_bit()->store_synthetic_data(iter->second.writer_data_.ddsPublicationData,
                                      DDS::NOT_NEW_VIEW_STATE);
      //TODO: match/unmatch local subscription(s)
    }

  } else { // this is some combination of dispose and/or unregister
    if (iter != discovered_publications_.end()) {
      topics_[get_topic_name(iter->second)].endpoints_.erase(guid);
      //TODO: unmatch local subscription(s)
      remove_from_bit(iter->second);
      discovered_publications_.erase(iter);
    }
  }
}

void
Sedp::data_received(char message_id, const DiscoveredReaderData& rdata)
{
  const RepoId& guid = rdata.readerProxy.remoteReaderGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(rdata.ddsSubscriptionData.topic_name)) {
    return;
  }

  const DiscoveredSubscriptionIter iter = discovered_subscriptions_.find(guid);

  if (message_id == DCPS::SAMPLE_DATA) {
    if (iter == discovered_subscriptions_.end()) { // add new
      DiscoveredSubscription& sub =
        discovered_subscriptions_[guid] = DiscoveredSubscription(rdata);
      std::memcpy(sub.reader_data_.ddsSubscriptionData.participant_key.value,
                  guid.guidPrefix, sizeof(DDS::BuiltinTopicKey_t));
      assign_bit_key(sub);
      sub.bit_ih_ =
        sub_bit()->store_synthetic_data(sub.reader_data_.ddsSubscriptionData,
                                        DDS::NEW_VIEW_STATE);
      topics_[get_topic_name(sub)].endpoints_.insert(guid);
      //TODO: match local publication(s)
    } else if (qosChanged(iter->second.reader_data_.ddsSubscriptionData,
                          rdata.ddsSubscriptionData)) { // update existing
      sub_bit()->store_synthetic_data(iter->second.reader_data_.ddsSubscriptionData,
                                      DDS::NOT_NEW_VIEW_STATE);
      //TODO: match/unmatch local publication(s)
    }

  } else { // this is some combination of dispose and/or unregister
    if (iter != discovered_subscriptions_.end()) {
      topics_[get_topic_name(iter->second)].endpoints_.erase(guid);
      //TODO: unmatch local publication(s)
      remove_from_bit(iter->second);
      discovered_subscriptions_.erase(iter);
    }
  }
}

bool
Sedp::qosChanged(DDS::PublicationBuiltinTopicData& dest,
                 const DDS::PublicationBuiltinTopicData& src)
{
  using OpenDDS::DCPS::operator!=;
  bool changed = false;

  // check each Changeable QoS policy value in Publication BIT Data

  if (dest.deadline != src.deadline) {
    changed = true;
    dest.deadline = src.deadline;
  }

  if (dest.latency_budget != src.latency_budget) {
    changed = true;
    dest.latency_budget = src.latency_budget;
  }

  if (dest.lifespan != src.lifespan) {
    changed = true;
    dest.lifespan = src.lifespan;
  }

  if (dest.user_data != src.user_data) {
    changed = true;
    dest.user_data = src.user_data;
  }

  if (dest.ownership_strength != src.ownership_strength) {
    changed = true;
    dest.ownership_strength = src.ownership_strength;
  }

  if (dest.partition != src.partition) {
    changed = true;
    dest.partition = src.partition;
  }

  if (dest.topic_data != src.topic_data) {
    changed = true;
    dest.topic_data = src.topic_data;
  }

  if (dest.group_data != src.group_data) {
    changed = true;
    dest.group_data = src.group_data;
  }

  return changed;
}

bool
Sedp::qosChanged(DDS::SubscriptionBuiltinTopicData& dest,
                 const DDS::SubscriptionBuiltinTopicData& src)
{
  using OpenDDS::DCPS::operator!=;
  bool changed = false;

  // check each Changeable QoS policy value in Subcription BIT Data

  if (dest.deadline != src.deadline) {
    changed = true;
    dest.deadline = src.deadline;
  }

  if (dest.latency_budget != src.latency_budget) {
    changed = true;
    dest.latency_budget = src.latency_budget;
  }

  if (dest.user_data != src.user_data) {
    changed = true;
    dest.user_data = src.user_data;
  }

  if (dest.time_based_filter != src.time_based_filter) {
    changed = true;
    dest.time_based_filter = src.time_based_filter;
  }

  if (dest.partition != src.partition) {
    changed = true;
    dest.partition = src.partition;
  }

  if (dest.topic_data != src.topic_data) {
    changed = true;
    dest.topic_data = src.topic_data;
  }

  if (dest.group_data != src.group_data) {
    changed = true;
    dest.group_data = src.group_data;
  }

  return changed;
}

void
Sedp::find_matching_endpoints(const DCPS::RepoId& repoId,
                              const TopicDetailsEx& td)
{
  const bool reader = repoId.entityId.entityKind & 4;
  for (RepoIdSet::const_iterator iter = td.endpoints_.begin();
       iter != td.endpoints_.end(); ++iter) {
    // check to make sure it's a Reader/Writer or Writer/Reader match
    if (bool(iter->entityId.entityKind & 4) != reader) {
      match(reader ? *iter : repoId, reader ? repoId : *iter, td);
    }
  }
}

void
Sedp::match(const DCPS::RepoId& writer, const DCPS::RepoId& reader,
            const TopicDetailsEx& td) //TODO: do we need td here?
{
  // 0. For discovered endpoints, we'll have the QoS info in the form of the
  // publication or subscription BIT data which doesn't use the same structures
  // for QoS.  In those cases we can copy the individual QoS policies to temp
  // QoS structs:
  DDS::DataWriterQos tempDwQos;
  DDS::PublisherQos tempPubQos;
  DDS::DataReaderQos tempDrQos;
  DDS::SubscriberQos tempSubQos;

  // 1. collect details about the writer, which may be local or discovered
  const DDS::DataWriterQos* dwQos = 0;
  const DDS::PublisherQos* pubQos = 0;
  const DCPS::TransportLocatorSeq* wTls = 0;

  const LocalPublicationIter lpi = local_publications_.find(writer);
  DiscoveredPublicationIter dpi;
  bool writer_local = false;
  if (lpi != local_publications_.end()) {
    writer_local = true;
    dwQos = &lpi->second.qos_;
    pubQos = &lpi->second.publisher_qos_;
    wTls = &lpi->second.trans_info_;
  } else if ((dpi = discovered_publications_.find(writer))
             != discovered_publications_.end()) {
    wTls = &dpi->second.writer_data_.writerProxy.allLocators;
  } else {
    //TODO: error
  }

  // 2. collect details about the reader, which may be local or discovered
  const DDS::DataReaderQos* drQos = 0;
  const DDS::SubscriberQos* subQos = 0;
  const DCPS::TransportLocatorSeq* rTls = 0;

  const LocalSubscriptionIter lsi = local_subscriptions_.find(reader);
  DiscoveredSubscriptionIter dsi;
  bool reader_local = false;
  if (lsi != local_subscriptions_.end()) {
    reader_local = true;
    drQos = &lsi->second.qos_;
    subQos = &lsi->second.subscriber_qos_;
    rTls = &lpi->second.trans_info_;
  } else if ((dsi = discovered_subscriptions_.find(writer))
             != discovered_subscriptions_.end()) {
    if (!writer_local) {
      // this is a discovered/discovered match, nothing for us to do
      return;
    }
    rTls = &dsi->second.reader_data_.readerProxy.allLocators;
    const DDS::SubscriptionBuiltinTopicData& bit =
      dsi->second.reader_data_.ddsSubscriptionData;
    tempDrQos.durability = bit.durability;
    tempDrQos.deadline = bit.deadline;
    tempDrQos.latency_budget = bit.latency_budget;
    tempDrQos.liveliness = bit.liveliness;
    tempDrQos.reliability = bit.reliability;
    tempDrQos.ownership = bit.ownership;
    tempDrQos.destination_order = bit.destination_order;
    tempDrQos.user_data = bit.user_data;
    tempDrQos.time_based_filter = bit.time_based_filter;
    drQos = &tempDrQos;
    tempSubQos.presentation = bit.presentation;
    tempSubQos.partition = bit.partition;
    tempSubQos.group_data = bit.group_data;
    subQos = &tempSubQos;
  } else {
    //TODO: error
  }

  // This is really part of step 1, but we're doing it here just in case we
  // are in the discovered/discovered match and we don't need the QoS data.
  if (!writer_local) {
    const DDS::PublicationBuiltinTopicData& bit =
      dpi->second.writer_data_.ddsPublicationData;
    tempDwQos.durability = bit.durability;
    tempDwQos.durability_service = bit.durability_service;
    tempDwQos.deadline = bit.deadline;
    tempDwQos.latency_budget = bit.latency_budget;
    tempDwQos.liveliness = bit.liveliness;
    tempDwQos.reliability = bit.reliability;
    tempDwQos.lifespan = bit.lifespan;
    tempDwQos.user_data = bit.user_data;
    tempDwQos.ownership = bit.ownership;
    tempDwQos.ownership_strength = bit.ownership_strength;
    tempDwQos.destination_order = bit.destination_order;
    dwQos = &tempDwQos;
    tempPubQos.presentation = bit.presentation;
    tempPubQos.partition = bit.partition;
    tempPubQos.group_data = bit.group_data;
    pubQos = &tempPubQos;
  }

  // 3. check transport and QoS compatibility
  DCPS::IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()},
    readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
  if (DCPS::compatibleQOS(&writerStatus, &readerStatus, *wTls, *rTls,
                          dwQos, drQos, pubQos, subQos)) {
    static const bool writer_active = true;
    if (writer_local) {
      DCPS::ReaderAssociation ra = {*rTls, reader, *subQos, *drQos}; //TODO: filtering info
      lpi->second.publication_->add_association(writer, ra, writer_active);
    }
    if (reader_local) {
      DCPS::WriterAssociation wa = {*rTls, writer, *pubQos, *dwQos};
      lsi->second.subscription_->add_association(reader, wa, !writer_active);
    }
  } else {
    if (writer_local && writerStatus.count_since_last_send) {
      lpi->second.publication_->update_incompatible_qos(writerStatus);
    }
    if (reader_local && readerStatus.count_since_last_send) {
      lsi->second.subscription_->update_incompatible_qos(readerStatus);
    }
  }
}

void
Sedp::association_complete(const RepoId& /*localId*/,
                           const RepoId& /*remoteId*/)
{
  //TODO
}

void
Sedp::disassociate_participant(const RepoId& /*remoteId*/)
{
  // no-op: not called from DCPS
}

void
Sedp::disassociate_publication(const RepoId& /*localId*/,
                               const RepoId& /*remoteId*/)
{
  // no-op: not called from DCPS
}

void
Sedp::disassociate_subscription(const RepoId& /*localId*/,
                                const RepoId& /*remoteId*/)
{
  // no-op: not called from DCPS
}

Sedp::Endpoint::~Endpoint()
{
}

//---------------------------------------------------------------
Sedp::Writer::Writer(const RepoId& pub_id, Sedp& sedp)
  : Endpoint(pub_id, sedp),
    alloc_(2, sizeof(DCPS::TransportSendElementAllocator))
{
  header_.prefix[0] = 'R';
  header_.prefix[1] = 'T';
  header_.prefix[2] = 'P';
  header_.prefix[3] = 'S';
  header_.version = PROTOCOLVERSION;
  header_.vendorId = VENDORID_OPENDDS;
  header_.guidPrefix[0] = pub_id.guidPrefix[0];
  header_.guidPrefix[1] = pub_id.guidPrefix[1],
  header_.guidPrefix[2] = pub_id.guidPrefix[2];
  header_.guidPrefix[3] = pub_id.guidPrefix[3];
  header_.guidPrefix[4] = pub_id.guidPrefix[4];
  header_.guidPrefix[5] = pub_id.guidPrefix[5];
  header_.guidPrefix[6] = pub_id.guidPrefix[6];
  header_.guidPrefix[7] = pub_id.guidPrefix[7];
  header_.guidPrefix[8] = pub_id.guidPrefix[8];
  header_.guidPrefix[9] = pub_id.guidPrefix[9];
  header_.guidPrefix[10] = pub_id.guidPrefix[10];
  header_.guidPrefix[11] = pub_id.guidPrefix[11];
}

Sedp::Writer::~Writer()
{
}

bool
Sedp::Writer::assoc(const DCPS::AssociationData& subscription)
{
  return associate(subscription, true);
}

void
Sedp::Writer::data_delivered(const DCPS::DataSampleListElement*)
{
}

void
Sedp::Writer::data_dropped(const DCPS::DataSampleListElement*, bool)
{
}

void
Sedp::Writer::control_delivered(ACE_Message_Block*)
{
}

void
Sedp::Writer::control_dropped(ACE_Message_Block*, bool)
{
}

DDS::ReturnCode_t
Sedp::Writer::write_sample(const ParameterList& plist, bool is_retransmission)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  // Determine message length
  size_t size = 0, padding = 0;
  DCPS::find_size_ulong(size, padding);
  DCPS::gen_find_size(plist, size, padding);

  // Build RTPS message
  ACE_Message_Block payload(DCPS::DataSampleHeader::max_marshaled_size(),
                            ACE_Message_Block::MB_DATA,
                            new ACE_Message_Block(size));
  using DCPS::Serializer;
  Serializer ser(payload.cont(), host_is_bigendian_, Serializer::ALIGN_CDR);
  bool ok = (ser << ACE_OutputCDR::from_octet(0)) &&  // PL_CDR_LE = 0x0003
            (ser << ACE_OutputCDR::from_octet(3)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << plist);
  if (!ok) {
    result = DDS::RETCODE_ERROR;
  }

  if (result == DDS::RETCODE_OK) {
    // Send sample
    DCPS::DataSampleListElement list_el(repo_id_, this, 0, &alloc_, 0);
    set_header_fields(list_el.header_, size, is_retransmission);

    DCPS::DataSampleList list;  // Container of list elements
    list.head_ = list.tail_ = &list_el;
    list.size_ = 1;
    list_el.sample_ = new ACE_Message_Block(size);
    *list_el.sample_ << list_el.header_;
    list_el.sample_->cont(payload.duplicate());

    send(list);
  }
  return result;
}

DDS::ReturnCode_t
Sedp::Writer::write_unregister_dispose(const DCPS::RepoId& rid)
{
  // Build param list for message
  Parameter param;
  param.guid(rid);
  param._d(PID_ENDPOINT_GUID);
  ParameterList plist;
  plist.length(1);
  plist[0] = param;

  // Determine message length
  size_t size = 0, padding = 0;
  DCPS::find_size_ulong(size, padding);
  DCPS::gen_find_size(plist, size, padding);

  ACE_Message_Block payload(DCPS::DataSampleHeader::max_marshaled_size(),
                            ACE_Message_Block::MB_DATA,
                            new ACE_Message_Block(size));

  using DCPS::Serializer;
  Serializer ser(payload.cont(), host_is_bigendian_, Serializer::ALIGN_CDR);
  ser << plist;

  // Send
  write_control_msg(payload, size, DCPS::DISPOSE_UNREGISTER_INSTANCE);
  return DDS::RETCODE_OK;
}

void
Sedp::Writer::write_control_msg(
    ACE_Message_Block& payload, 
    size_t size,
    DCPS::MessageId id)
{
  DCPS::DataSampleHeader header;
  set_header_fields(header, size, false, id);
  send_control(header, &payload);
}

void
Sedp::Writer::set_header_fields(DCPS::DataSampleHeader& dsh, 
                                size_t size,
                                bool is_retransmission,
                                DCPS::MessageId id) 
{
  dsh.message_id_ = DCPS::SAMPLE_DATA;
  dsh.byte_order_ = ACE_CDR_BYTE_ORDER;
  dsh.message_length_ = static_cast<ACE_UINT32>(size);
  dsh.publication_id_ = repo_id_;
  dsh.historic_sample_ = is_retransmission;

  const ACE_Time_Value now = ACE_OS::gettimeofday();
  dsh.source_timestamp_sec_ = now.sec();
  dsh.source_timestamp_nanosec_ = now.usec() * 1000;

  dsh.sequence_ = ++seq_;
}

//-------------------------------------------------------------------------

Sedp::Reader::~Reader()
{}

bool
Sedp::Reader::assoc(const DCPS::AssociationData& publication)
{
  return associate(publication, false);
}


// Implementing TransportReceiveListener

void
Sedp::Reader::data_received(const DCPS::ReceivedDataSample& sample)
{
  switch (sample.header_.message_id_) {
  case DCPS::SAMPLE_DATA:
  case DCPS::DISPOSE_INSTANCE:
  case DCPS::UNREGISTER_INSTANCE:
  case DCPS::DISPOSE_UNREGISTER_INSTANCE: {
    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::ULong encap;
    ok &= (ser >> encap);
    // Ignore the 'encap' header since we use sample.header_.byte_order_
    // to determine whether or not to swap bytes.
    ParameterList data;
    ok &= (ser >> data);
    if (!ok) {
      ACE_DEBUG((LM_ERROR,
        "ERROR: Sedp::Reader::data_received() failed to deserialize data\n"));
      return;
    }

    switch (repo_id_.entityId.entityKey[2]) {
    case 3: { // ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER.entityKey[2]
      DiscoveredWriterData wdata;
      if (ParameterListConverter::from_param_list(data, wdata) < 0) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR Sedp::Reader::data_received - "
          "failed to convert from ParameterList to DiscoveredWriterData\n"));
        return;
      }
      sedp_.data_received(sample.header_.message_id_, wdata);
      break;
    }
    case 4: { // ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER.entityKey[2]
      DiscoveredReaderData rdata;
      if (ParameterListConverter::from_param_list(data, rdata) < 0) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR Sedp::Reader::data_received - "
          "failed to convert from ParameterList to DiscoveredReaderData\n"));
        return;
      }
      sedp_.data_received(sample.header_.message_id_, rdata);
      break;
    }
    default:
      break;
    }
  }
  default:
    break;
  }
}

DDS::ReturnCode_t
Sedp::populate_discovered_writer_msg(
    DiscoveredWriterData& dwd,
    const RepoId& publication_id,
    const LocalPublication& pub)
{
  // Ignored on the wire dwd.ddsPublicationData.key
  // Ignored on the wire dwd.ddsPublicationData.participant_key
  std::string topic_name = topic_names_[pub.topic_id_];
  dwd.ddsPublicationData.topic_name = topic_name.c_str();
  TopicDetails& topic_details = topics_[topic_name];
  dwd.ddsPublicationData.type_name = topic_details.data_type_.c_str();
  dwd.ddsPublicationData.durability = pub.qos_.durability;
  dwd.ddsPublicationData.durability_service = pub.qos_.durability_service;
  dwd.ddsPublicationData.deadline = pub.qos_.deadline;
  dwd.ddsPublicationData.latency_budget = pub.qos_.latency_budget;
  dwd.ddsPublicationData.liveliness = pub.qos_.liveliness;
  dwd.ddsPublicationData.reliability = pub.qos_.reliability;
  dwd.ddsPublicationData.lifespan = pub.qos_.lifespan;
  dwd.ddsPublicationData.user_data = pub.qos_.user_data;
  dwd.ddsPublicationData.ownership = pub.qos_.ownership;
  dwd.ddsPublicationData.ownership_strength = pub.qos_.ownership_strength;
  dwd.ddsPublicationData.destination_order = pub.qos_.destination_order;
  dwd.ddsPublicationData.presentation = pub.publisher_qos_.presentation;
  dwd.ddsPublicationData.partition = pub.publisher_qos_.partition;
  dwd.ddsPublicationData.topic_data = topic_details.qos_.topic_data;
  dwd.ddsPublicationData.group_data = pub.publisher_qos_.group_data;
  dwd.writerProxy.remoteWriterGuid = publication_id;
  CORBA::ULong tx_length = pub.trans_info_.length();
  for (CORBA::ULong i = 0; i < tx_length; ++i) {
    ACE_DEBUG((LM_INFO, "got trans type of %C\n", 
               pub.trans_info_[i].transport_type.in()));
    std::string trans_type = pub.trans_info_[i].transport_type.in();
    if (trans_type == "rtps_udp") {
      LocatorSeq locators;
      DDS::ReturnCode_t result = blob_to_locators(pub.trans_info_[i].data, 
                                                  locators);
      if (result != DDS::RETCODE_OK) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) Failed to translate  ")
                             ACE_TEXT("blob to locators while ")
                             ACE_TEXT("populatingDiscoveredWriterData\n")));

        return result;
      } else {
        CORBA::ULong locators_len = locators.length();
        for (CORBA::ULong j = 0; j < locators_len; ++j) {
          CORBA::ULong unicast_len = 
                dwd.writerProxy.unicastLocatorList.length();
          // TODO Append to unicast locators or multicast locators?
          dwd.writerProxy.unicastLocatorList.length(unicast_len + 1);
          dwd.writerProxy.unicastLocatorList[unicast_len] = locators[j];
        }
      }
    }
    // Append to WriterProxy
    CORBA::ULong len = dwd.writerProxy.allLocators.length();
    dwd.writerProxy.allLocators.length(len + 1);
    dwd.writerProxy.allLocators[len].transport_type = 
          pub.trans_info_[i].transport_type;
    dwd.writerProxy.allLocators[len].data = 
          pub.trans_info_[i].data;
  }

  return DDS::RETCODE_OK;
}

void
Sedp::write_durable_publication_data()
{
  //ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  LocalPublicationIter pub, end = local_publications_.end();
  for (pub = local_publications_.begin(); pub != end; ++pub) {
    write_publication_data(pub->first, pub->second, true);
  }
}

void
Sedp::write_durable_subscription_data()
{
  //ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionIter sub, end = local_subscriptions_.end();

  for (sub = local_subscriptions_.begin(); sub != end; ++sub) {
    // Do nothing for now
  }
}

DDS::ReturnCode_t
Sedp::write_publication_data(
    const DCPS::RepoId& rid, 
    const LocalPublication& lp,
    bool is_retransmission)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  DiscoveredWriterData dwd;
  ParameterList plist;
  ACE_DEBUG((LM_INFO, "Writing publication data\n"));
  result = populate_discovered_writer_msg(dwd, rid, lp);
  if (DDS::RETCODE_OK == result) {
    // Convert to parameter list
    if (ParameterListConverter::to_param_list(dwd, plist)) {
      ACE_DEBUG((LM_INFO, 
          ACE_TEXT("(%P|%t) Failed to convert DiscoveredWriterData ")
          ACE_TEXT(" to ParameterList\n")));
      result = DDS::RETCODE_ERROR;
    }
  }
  if (DDS::RETCODE_OK == result) {
    result = publications_writer_.write_sample(plist, is_retransmission);
  }
  return result;
}

}
}
