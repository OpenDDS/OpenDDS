/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Sedp.h"
#include "Spdp.h"
#include "RtpsDiscovery.h"

#include "RtpsMessageTypesC.h"
#include "RtpsBaseMessageTypesTypeSupportImpl.h"

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"

#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"

#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>


namespace OpenDDS {
namespace DCPS {
  typedef RcHandle<RtpsUdpInst> RtpsUdpInst_rch;
}

namespace RTPS {
using DCPS::RepoId;

Sedp::Sedp(const RepoId& participant_id, Spdp& owner)
  : participant_id_(participant_id)
  , spdp_(owner)
  , publications_writer_(make_id(participant_id,
                                 ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER),
                         owner)
  , subscriptions_writer_(make_id(participant_id,
                                  ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER),
                          owner)
  , publications_reader_(make_id(participant_id,
                                 ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER),
                         owner)
  , subscriptions_reader_(make_id(participant_id,
                                  ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER),
                          owner)
  , publication_counter_(0), subscription_counter_(0), topic_counter_(0)
{}

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
  std::string key = OpenDDS::DCPS::GuidConverter(guid).uniqueId();

  // allocate one transport
  transport_ = TheTransportRegistry->create_inst(
                   DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                   "_SEDPTransportInst_" + key, "rtps_udp");
  // Use a static cast to avoid dependency on the RtpsUdp library
  DCPS::RtpsUdpInst_rch rtps_inst = 
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_);

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

  // Todo: TransportConfig
  // Todo: reader/writer -> transport_config()
  // Todo: reader/writer -> enable(true)
  return DDS::RETCODE_OK;
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
  if (topics_.count(topicName)) { // types must match, RtpsInfo checked for us
    topics_[topicName].qos_ = qos;
    topicId = topics_[topicName].repo_id_;
    return DCPS::FOUND;
  }

  TopicDetails& td = topics_[topicName];
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
               ACE_TEXT("(%P|%t) ERROR: Spdp::assert_topic: ")
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
  if (topic_names_.count(topicId)) {
    name = topic_names_[topicId];
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

  typedef std::map<std::string, TopicDetails> TDMap;
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

  // TODO: use SEDP to advertise the new publication

  return rid;
}

void
Sedp::remove_publication(const RepoId& publicationId)
{
  // TODO: use SEDP to dispose the publication
  local_publications_.erase(publicationId);
}

bool
Sedp::update_publication_qos(const RepoId& publicationId,
                             const DDS::DataWriterQos& qos,
                             const DDS::PublisherQos& publisherQos)
{
  if (local_publications_.count(publicationId)) {
    LocalPublication& pb = local_publications_[publicationId];
    pb.qos_ = qos;
    pb.publisher_qos_ = publisherQos;
    // TODO: tell the world about the change with SEDP

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
  if (local_subscriptions_.count(subscriptionId)) {
    LocalSubscription& sb = local_subscriptions_[subscriptionId];
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
  if (local_subscriptions_.count(subId)) {
    LocalSubscription& sb = local_subscriptions_[subId];
    sb.params_ = params;
    // TODO: tell the world about the change with SEDP

    return true;
  }
  return false;
}

void
Sedp::association_complete(const RepoId& localId, const RepoId& remoteId)
{
}

void
Sedp::disassociate_participant(const RepoId& remoteId)
{
}

void
Sedp::disassociate_publication(const RepoId& localId, const RepoId& remoteId)
{
}

void
Sedp::disassociate_subscription(const RepoId& localId, const RepoId& remoteId)
{
}

Sedp::Endpoint::~Endpoint()
{
}

//---------------------------------------------------------------

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
  case DCPS::SAMPLE_DATA: {
    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::ULong encap;
    ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
    ParameterList data;
    ok &= (ser >> data);
    if (!ok) {
      ACE_DEBUG((LM_ERROR, "ERROR: failed to deserialize data\n"));
      return;
    }

//TODO:    spdp_.add_discovered_endpoint(data);

    break;
  }
  case DCPS::DISPOSE_INSTANCE:
  case DCPS::UNREGISTER_INSTANCE:
  case DCPS::DISPOSE_UNREGISTER_INSTANCE: {
    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::ULong encap;
    ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
    DiscoveredWriterData data;
    ok &= (ser >> data);

    if (!ok) {
      ACE_DEBUG((LM_ERROR, "ERROR: failed to deserialize key data\n"));
      return;
    }

    //TODO: process dispose/unregister
    break;
  }
  default:
    break;
  }
}


}
}
