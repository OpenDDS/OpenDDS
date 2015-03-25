/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DDS_HAS_MINIMUM_BIT
#include "Sedp.h"
#include "Spdp.h"
#include "MessageTypes.h"
#include "RtpsDiscovery.h"
#include "RtpsMessageTypesC.h"
#include "RtpsBaseMessageTypesTypeSupportImpl.h"
#include "ParameterListConverter.h"

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst_rch.h"

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/SendStateDataSampleList.h"
#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DCPS/DataWriterCallbacks.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DCPS_Utils.h"

#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"

#include <ace/Reverse_Lock_T.h>
#include <ace/Auto_Ptr.h>

#include <cstring>

namespace {
bool qosChanged(DDS::PublicationBuiltinTopicData& dest,
                const DDS::PublicationBuiltinTopicData& src)
{
#ifndef OPENDDS_GCC33
  using OpenDDS::DCPS::operator!=;
#endif
  bool changed = false;

  // check each Changeable QoS policy value in Publication BIT Data

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.deadline, src.deadline)) {
#else
  if (dest.deadline != src.deadline) {
#endif
    changed = true;
    dest.deadline = src.deadline;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.latency_budget, src.latency_budget)) {
#else
  if (dest.latency_budget != src.latency_budget) {
#endif
    changed = true;
    dest.latency_budget = src.latency_budget;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.lifespan, src.lifespan)) {
#else
  if (dest.lifespan != src.lifespan) {
#endif
    changed = true;
    dest.lifespan = src.lifespan;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.user_data, src.user_data)) {
#else
  if (dest.user_data != src.user_data) {
#endif
    changed = true;
    dest.user_data = src.user_data;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.ownership_strength,
                                src.ownership_strength)) {
#else
  if (dest.ownership_strength != src.ownership_strength) {
#endif
    changed = true;
    dest.ownership_strength = src.ownership_strength;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.partition, src.partition)) {
#else
  if (dest.partition != src.partition) {
#endif
    changed = true;
    dest.partition = src.partition;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.topic_data, src.topic_data)) {
#else
  if (dest.topic_data != src.topic_data) {
#endif
    changed = true;
    dest.topic_data = src.topic_data;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.group_data, src.group_data)) {
#else
  if (dest.group_data != src.group_data) {
#endif
    changed = true;
    dest.group_data = src.group_data;
  }

  return changed;
}

bool qosChanged(DDS::SubscriptionBuiltinTopicData& dest,
                const DDS::SubscriptionBuiltinTopicData& src)
{
#ifndef OPENDDS_GCC33
  using OpenDDS::DCPS::operator!=;
#endif
  bool changed = false;

  // check each Changeable QoS policy value in Subcription BIT Data

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.deadline, src.deadline)) {
#else
  if (dest.deadline != src.deadline) {
#endif
    changed = true;
    dest.deadline = src.deadline;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.latency_budget, src.latency_budget)) {
#else
  if (dest.latency_budget != src.latency_budget) {
#endif
    changed = true;
    dest.latency_budget = src.latency_budget;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.user_data, src.user_data)) {
#else
  if (dest.user_data != src.user_data) {
#endif
    changed = true;
    dest.user_data = src.user_data;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.time_based_filter,
                                src.time_based_filter)) {
#else
  if (dest.time_based_filter != src.time_based_filter) {
#endif
    changed = true;
    dest.time_based_filter = src.time_based_filter;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.partition, src.partition)) {
#else
  if (dest.partition != src.partition) {
#endif
    changed = true;
    dest.partition = src.partition;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.topic_data, src.topic_data)) {
#else
  if (dest.topic_data != src.topic_data) {
#endif
    changed = true;
    dest.topic_data = src.topic_data;
  }

#ifdef OPENDDS_GCC33
  if (OpenDDS::DCPS::operator!=(dest.group_data, src.group_data)) {
#else
  if (dest.group_data != src.group_data) {
#endif
    changed = true;
    dest.group_data = src.group_data;
  }

  return changed;
}

bool paramsChanged(OpenDDS::RTPS::ContentFilterProperty_t& dest,
                   const OpenDDS::RTPS::ContentFilterProperty_t& src)
{
  if (dest.expressionParameters.length() != src.expressionParameters.length()) {
    dest.expressionParameters = src.expressionParameters;
    return true;
  }
  for (CORBA::ULong i = 0; i < src.expressionParameters.length(); ++i) {
    if (0 != std::strcmp(dest.expressionParameters[i].in(),
                         src.expressionParameters[i].in())) {
      dest.expressionParameters = src.expressionParameters;
      return true;
    }
  }
  return false;
}

}

namespace OpenDDS {
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
  , participant_message_writer_(make_id(participant_id,
                                        ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER),
                        *this)
  , publications_reader_(new Reader(make_id(participant_id,
                                            ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER),
                                    *this))
  , subscriptions_reader_(new Reader(make_id(participant_id,
                                             ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER),
                                     *this))
  , participant_message_reader_(new Reader(make_id(participant_id,
                                                   ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER),
                                           *this))
  , task_(this)
  , publication_counter_(0), subscription_counter_(0), topic_counter_(0)
  , automatic_liveliness_seq_ (DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN())
  , manual_liveliness_seq_ (DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN())
{
  pub_bit_key_.value[0] = pub_bit_key_.value[1] = pub_bit_key_.value[2] = 0;
  sub_bit_key_.value[0] = sub_bit_key_.value[1] = sub_bit_key_.value[2] = 0;
}

RepoId
Sedp::make_id(const RepoId& participant_id, const EntityId_t& entity)
{
  RepoId id = participant_id;
  id.entityId = entity;
  return id;
}

DDS::ReturnCode_t
Sedp::init(const RepoId& guid, const RtpsDiscovery& disco,
           DDS::DomainId_t domainId)
{
  char domainStr[16];
  ACE_OS::snprintf(domainStr, 16, "%d", domainId);

  OPENDDS_STRING key = OpenDDS::DCPS::GuidConverter(guid).uniqueId();

  // configure one transport
  transport_inst_ = TheTransportRegistry->create_inst(
                       DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                       OPENDDS_STRING("_SEDPTransportInst_") + key.c_str() + domainStr,
                       "rtps_udp");
  // Use a static cast to avoid dependency on the RtpsUdp library
  DCPS::RtpsUdpInst_rch rtps_inst =
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  // The SEDP endpoints may need to wait at least one resend period before
  // the handshake completes (allows time for our SPDP multicast to be
  // received by the other side).  Arbitrary constant of 5 to account for
  // possible network lossiness.
  static const double HANDSHAKE_MULTIPLIER = 5;
  rtps_inst->handshake_timeout_ = disco.resend_period() * HANDSHAKE_MULTIPLIER;

  if (disco.sedp_multicast()) {
    // Bind to a specific multicast group
    const u_short mc_port = disco.pb() + disco.dg() * domainId + disco.dx();

    OPENDDS_STRING mc_addr = disco.default_multicast_group();
    if (rtps_inst->multicast_group_address_.set(mc_port, mc_addr.c_str())) {
      ACE_DEBUG((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::init - ")
                 ACE_TEXT("failed setting multicast local_addr to port %hd\n"),
                          mc_port));
      return DDS::RETCODE_ERROR;
    }

    rtps_inst->ttl_ = disco.ttl();
    rtps_inst->multicast_interface_ = disco.multicast_interface();

  } else {
    rtps_inst->use_multicast_ = false;
  }

  // Crete a config
  OPENDDS_STRING config_name = DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                            OPENDDS_STRING("_SEDP_TransportCfg_") + key +
                            domainStr;
  DCPS::TransportConfig_rch transport_cfg =
    TheTransportRegistry->create_config(config_name.c_str());
  transport_cfg->instances_.push_back(transport_inst_);

  // Configure and enable each reader/writer
  rtps_inst->opendds_discovery_default_listener_ = publications_reader_.in();
  rtps_inst->opendds_discovery_guid_ = guid;
  const bool reliability = true, durability = true;
  publications_writer_.enable_transport_using_config(reliability, durability,
                                                     transport_cfg);
  publications_reader_->enable_transport_using_config(reliability, durability,
                                                      transport_cfg);
  subscriptions_writer_.enable_transport_using_config(reliability, durability,
                                                      transport_cfg);
  subscriptions_reader_->enable_transport_using_config(reliability, durability,
                                                       transport_cfg);
  participant_message_writer_.enable_transport_using_config(reliability, durability,
                                                            transport_cfg);
  participant_message_reader_->enable_transport_using_config(reliability, durability,
                                                             transport_cfg);
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
Sedp::ignore(const RepoId& to_ignore)
{
  // Locked prior to call from Spdp.
  ignored_guids_.insert(to_ignore);
  {
    const DiscoveredPublicationIter iter =
      discovered_publications_.find(to_ignore);
    if (iter != discovered_publications_.end()) {
      // clean up tracking info
      topics_[get_topic_name(iter->second)].endpoints_.erase(iter->first);
      remove_from_bit(iter->second);
      OPENDDS_STRING topic_name = get_topic_name(iter->second);
      discovered_publications_.erase(iter);
      // break associations
      OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
      if (top_it != topics_.end()) {
        match_endpoints(to_ignore, top_it->second, true /*remove*/);
      }
      return;
    }
  }
  {
    const DiscoveredSubscriptionIter iter =
      discovered_subscriptions_.find(to_ignore);
    if (iter != discovered_subscriptions_.end()) {
      // clean up tracking info
      topics_[get_topic_name(iter->second)].endpoints_.erase(iter->first);
      remove_from_bit(iter->second);
      OPENDDS_STRING topic_name = get_topic_name(iter->second);
      discovered_subscriptions_.erase(iter);
      // break associations
      OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
      if (top_it != topics_.end()) {
        match_endpoints(to_ignore, top_it->second, true /*remove*/);
      }
      return;
    }
  }
  {
    const OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, DCPS::GUID_tKeyLessThan)::iterator
      iter = topic_names_.find(to_ignore);
    if (iter != topic_names_.end()) {
      ignored_topics_.insert(iter->second);
      // Remove all publications and subscriptions on this topic
      OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(iter->second);
      if (top_it != topics_.end()) {
        TopicDetailsEx& td = top_it->second;
        RepoIdSet::iterator ep;
        for (ep = td.endpoints_.begin(); ep!= td.endpoints_.end(); ++ep) {
          match_endpoints(*ep, td, true /*remove*/);
          if (spdp_.shutting_down()) { return; }
        }
      }
    }
  }
}

RepoId
Sedp::bit_key_to_repo_id(const char* bit_topic_name,
                         const DDS::BuiltinTopicKey_t& key)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, RepoId());
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
  ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) Sedp::increment_key - ")
                         ACE_TEXT("ran out of builtin topic keys\n")));
}

void
create_association_data_proto(DCPS::AssociationData& proto,
                              const SPDPdiscoveredParticipantData& pdata) {
  proto.publication_transport_priority_ = 0;
  proto.remote_reliable_ = true;
  proto.remote_durable_ = true;
  std::memcpy(proto.remote_id_.guidPrefix, pdata.participantProxy.guidPrefix,
              sizeof(GuidPrefix_t));

  const LocatorSeq& mll =
    pdata.participantProxy.metatrafficMulticastLocatorList;
  const LocatorSeq& ull =
    pdata.participantProxy.metatrafficUnicastLocatorList;
  const CORBA::ULong locator_count = mll.length() + ull.length();

  ACE_Message_Block mb_locator(4 + locator_count * sizeof(Locator_t) + 1);
  using DCPS::Serializer;
  Serializer ser_loc(&mb_locator, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser_loc << locator_count;

  for (CORBA::ULong i = 0; i < mll.length(); ++i) {
    ser_loc << mll[i];
  }
  for (CORBA::ULong i = 0; i < ull.length(); ++i) {
    ser_loc << ull[i];
  }
  ser_loc << ACE_OutputCDR::from_boolean(false); // requires_inline_qos

  proto.remote_data_.length(1);
  proto.remote_data_[0].transport_type = "rtps_udp";
  proto.remote_data_[0].data.replace(
    static_cast<CORBA::ULong>(mb_locator.length()), &mb_locator);
}

void
Sedp::associate(const SPDPdiscoveredParticipantData& pdata)
{
  // First create a 'prototypical' instance of AssociationData.  It will
  // be copied and modified for each of the (up to) four SEDP Endpoints.
  DCPS::AssociationData proto;
  create_association_data_proto(proto, pdata);

  const BuiltinEndpointSet_t& avail =
    pdata.participantProxy.availableBuiltinEndpoints;

  // See RTPS v2.1 section 8.5.5.1
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    publications_reader_->assoc(peer);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    subscriptions_reader_->assoc(peer);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    participant_message_reader_->assoc(peer);
  }

  SPDPdiscoveredParticipantData* dpd =
    new SPDPdiscoveredParticipantData(pdata);
  task_.enqueue(dpd);
}

void
Sedp::Task::svc_i(const SPDPdiscoveredParticipantData* ppdata)
{
  ACE_Auto_Basic_Ptr<const SPDPdiscoveredParticipantData> delete_the_data(ppdata);
  const SPDPdiscoveredParticipantData& pdata = *ppdata;
  // First create a 'prototypical' instance of AssociationData.  It will
  // be copied and modified for each of the (up to) four SEDP Endpoints.
  DCPS::AssociationData proto;
  create_association_data_proto(proto, pdata);

  const BuiltinEndpointSet_t& avail =
    pdata.participantProxy.availableBuiltinEndpoints;

  // See RTPS v2.1 section 8.5.5.1
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    sedp_->publications_writer_.assoc(peer);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    sedp_->subscriptions_writer_.assoc(peer);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    sedp_->participant_message_writer_.assoc(peer);
  }

  //FUTURE: if/when topic propagation is supported, add it here

  // Process deferred publications and subscriptions.
  for (DeferredSubscriptionMap::iterator pos = sedp_->deferred_subscriptions_.lower_bound (proto.remote_id_),
         limit = sedp_->deferred_subscriptions_.upper_bound (proto.remote_id_);
       pos != limit;
       /* Increment in body. */) {
    sedp_->data_received (pos->second.first, pos->second.second);
    sedp_->deferred_subscriptions_.erase (pos++);
  }
  for (DeferredPublicationMap::iterator pos = sedp_->deferred_publications_.lower_bound (proto.remote_id_),
         limit = sedp_->deferred_publications_.upper_bound (proto.remote_id_);
       pos != limit;
       /* Increment in body. */) {
    sedp_->data_received (pos->second.first, pos->second.second);
    sedp_->deferred_publications_.erase (pos++);
  }

  ACE_GUARD(ACE_Thread_Mutex, g, sedp_->lock_);
  if (spdp_->shutting_down()) { return; }

  proto.remote_id_.entityId = ENTITYID_PARTICIPANT;
  sedp_->associated_participants_.insert(proto.remote_id_);

  // Write durable data
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    proto.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    sedp_->write_durable_publication_data(proto.remote_id_);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    proto.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    sedp_->write_durable_subscription_data(proto.remote_id_);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) {
    proto.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    sedp_->write_durable_participant_message_data(proto.remote_id_);
  }

  for (RepoIdSet::iterator it = sedp_->defer_match_endpoints_.begin();
       it != sedp_->defer_match_endpoints_.end(); /*incremented in body*/) {
    if (0 == std::memcmp(it->guidPrefix, proto.remote_id_.guidPrefix,
                         sizeof(GuidPrefix_t))) {
      OPENDDS_STRING topic;
      if (it->entityId.entityKind & 4) {
        DiscoveredSubscriptionIter dsi =
          sedp_->discovered_subscriptions_.find(*it);
        if (dsi != sedp_->discovered_subscriptions_.end()) {
          topic = dsi->second.reader_data_.ddsSubscriptionData.topic_name;
        }
      } else {
        DiscoveredPublicationIter dpi =
          sedp_->discovered_publications_.find(*it);
        if (dpi != sedp_->discovered_publications_.end()) {
          topic = dpi->second.writer_data_.ddsPublicationData.topic_name;
        }
      }
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::AssociateTask::svc - ")
          ACE_TEXT("processing deferred endpoints for topic %C\n"),
          topic.c_str()));
      }
      if (!topic.empty()) {
        OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator ti =
          sedp_->topics_.find(topic);
        if (ti != sedp_->topics_.end()) {
          if (DCPS::DCPS_debug_level > 3) {
            DCPS::GuidConverter conv(*it);
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::AssociateTask::svc - ")
              ACE_TEXT("calling match_endpoints %C\n"),
              OPENDDS_STRING(conv).c_str()));
          }
          sedp_->match_endpoints(*it, ti->second);
          if (spdp_->shutting_down()) { return; }
        }
      }
      sedp_->defer_match_endpoints_.erase(it++);
    } else {
      ++it;
    }
  }
}

bool
Sedp::disassociate(const SPDPdiscoveredParticipantData& pdata)
{
  RepoId part;
  std::memcpy(part.guidPrefix, pdata.participantProxy.guidPrefix,
              sizeof(GuidPrefix_t));
  part.entityId = ENTITYID_PARTICIPANT;
  associated_participants_.erase(part);
  const BuiltinEndpointSet_t avail =
    pdata.participantProxy.availableBuiltinEndpoints;

  { // Release lock, so we can call into transport
    ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);
    ACE_GUARD_RETURN(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock, false);

    // See RTPS v2.1 section 8.5.5.2
    if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
      RepoId id = part;
      id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
      publications_writer_.disassociate(id);
    }
    if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
      RepoId id = part;
      id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
      publications_reader_->disassociate(id);
    }
    if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
      RepoId id = part;
      id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
      subscriptions_writer_.disassociate(id);
    }
    if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
      RepoId id = part;
      id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
      subscriptions_reader_->disassociate(id);
    }
    if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) {
      RepoId id = part;
      id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
      participant_message_writer_.disassociate(id);
    }
    if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
      RepoId id = part;
      id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
      participant_message_reader_->disassociate(id);
    }
    //FUTURE: if/when topic propagation is supported, add it here
  }
  if (spdp_.has_discovered_participant(part)) {
    remove_entities_belonging_to(discovered_publications_, part);
    remove_entities_belonging_to(discovered_subscriptions_, part);
    return true;
  } else {
    return false;
  }
}

template<typename Map>
void
Sedp::remove_entities_belonging_to(Map& m, RepoId participant)
{
  participant.entityId.entityKey[0] = 0;
  participant.entityId.entityKey[1] = 0;
  participant.entityId.entityKey[2] = 0;
  participant.entityId.entityKind = 0;
  for (typename Map::iterator i = m.lower_bound(participant);
       i != m.end() && 0 == std::memcmp(i->first.guidPrefix,
                                        participant.guidPrefix,
                                        sizeof(GuidPrefix_t));) {
    OPENDDS_STRING topic_name = get_topic_name(i->second);
    OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
      topics_.find(topic_name);
    if (top_it != topics_.end()) {
      top_it->second.endpoints_.erase(i->first);
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Sedp::remove_entities_belonging_to - ")
                   ACE_TEXT("calling match_endpoints remove\n")));
      }
      match_endpoints(i->first, top_it->second, true /*remove*/);
      if (spdp_.shutting_down()) { return; }
    }
    remove_from_bit(i->second);
    m.erase(i++);
  }
}

void
Sedp::remove_from_bit(const DiscoveredPublication& pub)
{
  pub_key_to_id_.erase(pub.writer_data_.ddsPublicationData.key);
  task_.enqueue(Msg::MSG_REMOVE_FROM_PUB_BIT,
                new DDS::InstanceHandle_t(pub.bit_ih_));
}

void
Sedp::remove_from_bit(const DiscoveredSubscription& sub)
{
  sub_key_to_id_.erase(sub.reader_data_.ddsSubscriptionData.key);
  task_.enqueue(Msg::MSG_REMOVE_FROM_SUB_BIT,
                new DDS::InstanceHandle_t(sub.bit_ih_));
}

void
Sedp::Task::svc_i(Msg::MsgType which_bit, const DDS::InstanceHandle_t* bit_ih)
{
  ACE_Auto_Basic_Ptr<const DDS::InstanceHandle_t> delete_the_ih(bit_ih);
  switch (which_bit) {
  case Msg::MSG_REMOVE_FROM_PUB_BIT: {
    DDS::PublicationBuiltinTopicDataDataReaderImpl* bit = sedp_->pub_bit();
    // bit may be null if the DomainParticipant is shutting down
    if (bit && *bit_ih != DDS::HANDLE_NIL) {
      bit->set_instance_state(*bit_ih,
                              DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
    }
    break;
  }
  case Msg::MSG_REMOVE_FROM_SUB_BIT: {
    DDS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sedp_->sub_bit();
    // bit may be null if the DomainParticipant is shutting down
    if (bit && *bit_ih != DDS::HANDLE_NIL) {
      bit->set_instance_state(*bit_ih,
                              DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
    }
    break;
  }
  default:
    break;
  }
}

DDS::TopicBuiltinTopicDataDataReaderImpl*
Sedp::topic_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_TOPIC_TOPIC);
  return dynamic_cast<DDS::TopicBuiltinTopicDataDataReaderImpl*>(d.in());
}

DDS::PublicationBuiltinTopicDataDataReaderImpl*
Sedp::pub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_PUBLICATION_TOPIC);
  return dynamic_cast<DDS::PublicationBuiltinTopicDataDataReaderImpl*>(d.in());
}

DDS::SubscriptionBuiltinTopicDataDataReaderImpl*
Sedp::sub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  return dynamic_cast<DDS::SubscriptionBuiltinTopicDataDataReaderImpl*>(d.in());
}

DCPS::TopicStatus
Sedp::assert_topic(DCPS::RepoId_out topicId, const char* topicName,
                   const char* dataTypeName, const DDS::TopicQos& qos,
                   bool hasDcpsKey)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator iter =
    topics_.find(topicName);
  if (iter != topics_.end()) { // types must match, RtpsDiscovery checked for us
    iter->second.qos_ = qos;
    iter->second.has_dcps_key_ = hasDcpsKey;
    topicId = iter->second.repo_id_;
    topic_names_[iter->second.repo_id_] = topicName;
    return DCPS::FOUND;
  }

  TopicDetailsEx& td = topics_[topicName];
  td.data_type_ = dataTypeName;
  td.qos_ = qos;
  td.has_dcps_key_ = hasDcpsKey;
  td.repo_id_ = make_topic_guid();
  topicId = td.repo_id_;
  topic_names_[td.repo_id_] = topicName;

  return DCPS::CREATED;
}

DCPS::TopicStatus
Sedp::remove_topic(const RepoId& topicId, OPENDDS_STRING& name)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  name = topic_names_[topicId];
  OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
      topics_.find(name);
  if (top_it != topics_.end()) {
    TopicDetailsEx& td = top_it->second;
    if (td.endpoints_.empty()) {
      topics_.erase(name);
    }
  }

  topic_names_.erase(topicId);
  return DCPS::REMOVED;
}

bool
Sedp::update_topic_qos(const RepoId& topicId, const DDS::TopicQos& qos,
                       OPENDDS_STRING& name)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, DCPS::GUID_tKeyLessThan)::iterator iter =
    topic_names_.find(topicId);
  if (iter == topic_names_.end()) {
    return false;
  }
  name = iter->second;
  TopicDetailsEx& topic = topics_[name];
  using namespace DCPS;
  // If the TOPIC_DATA QoS changed our local endpoints must be resent
  // with new QoS
  if (qos.topic_data != topic.qos_.topic_data) {
    topic.qos_ = qos;
    // For each endpoint associated on this topic
    for (RepoIdSet::iterator topic_endpoints = topic.endpoints_.begin();
         topic_endpoints != topic.endpoints_.end(); ++topic_endpoints) {

      const RepoId& rid = *topic_endpoints;
      EntityKind kind = GuidConverter(rid).entityKind();
      if (KIND_WRITER == kind) {
        // This may be our local publication, verify
        LocalPublicationIter lp = local_publications_.find(rid);
        if (lp != local_publications_.end()) {
          write_publication_data(rid, lp->second);
        }
      } else if (KIND_READER == kind) {
        // This may be our local subscription, verify
        LocalSubscriptionIter ls = local_subscriptions_.find(rid);
        if (ls != local_subscriptions_.end()) {
          write_subscription_data(rid, ls->second);
        }
      }
    }
  }

  return true;
}

void
Sedp::inconsistent_topic(const RepoIdSet& eps) const
{
  for (RepoIdSet::const_iterator iter(eps.begin()); iter != eps.end(); ++iter) {
    if (0 == std::memcmp(participant_id_.guidPrefix, iter->guidPrefix,
                         sizeof(GuidPrefix_t))) {
      const bool reader = iter->entityId.entityKind & 4;
      if (reader) {
        const LocalSubscriptionCIter lsi = local_subscriptions_.find(*iter);
        if (lsi != local_subscriptions_.end()) {
          lsi->second.subscription_->inconsistent_topic();
          // Only make one callback per inconsistent topic, even if we have
          // more than one reader/writer on the topic -- it's the Topic object
          // that will actually see the InconsistentTopicStatus change.
          return;
        }
      } else {
        const LocalPublicationCIter lpi = local_publications_.find(*iter);
        if (lpi != local_publications_.end()) {
          lpi->second.publication_->inconsistent_topic();
          return; // see comment above
        }
      }
    }
  }
}

bool
Sedp::has_dcps_key(const RepoId& topicId) const
{
  typedef OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, DCPS::GUID_tKeyLessThan) TNMap;
  TNMap::const_iterator tn = topic_names_.find(topicId);
  if (tn == topic_names_.end()) return false;

  typedef OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx) TDMap;
  TDMap::const_iterator td = topics_.find(tn->second);
  if (td == topics_.end()) return false;

  return td->second.has_dcps_key_;
}

RepoId
Sedp::add_publication(const RepoId& topicId,
                      DCPS::DataWriterCallbacks* publication,
                      const DDS::DataWriterQos& qos,
                      const DCPS::TransportLocatorSeq& transInfo,
                      const DDS::PublisherQos& publisherQos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, RepoId());
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

  if (DDS::RETCODE_OK != write_publication_data(rid, pb)) {
    return RepoId();
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::add_publication - ")
                         ACE_TEXT("calling match_endpoints\n")));
  }
  match_endpoints(rid, td);

  return rid;
}

void
Sedp::remove_publication(const RepoId& publicationId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationIter iter = local_publications_.find(publicationId);
  if (iter != local_publications_.end()) {
    if (DDS::RETCODE_OK ==
          publications_writer_.write_unregister_dispose(publicationId))
    {
      OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
      local_publications_.erase(publicationId);
      OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
            topics_.find(topic_name);
      if (top_it != topics_.end()) {
        match_endpoints(publicationId, top_it->second, true /*remove*/);
        top_it->second.endpoints_.erase(publicationId);
      }
    } else {
      ACE_DEBUG((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::remove_publication - ")
                 ACE_TEXT("Failed to publish dispose msg\n")));
    }
  }
}

bool
Sedp::update_publication_qos(const RepoId& publicationId,
                             const DDS::DataWriterQos& qos,
                             const DDS::PublisherQos& publisherQos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  LocalPublicationIter iter = local_publications_.find(publicationId);
  if (iter != local_publications_.end()) {
    LocalPublication& pb = iter->second;
    pb.qos_ = qos;
    pb.publisher_qos_ = publisherQos;

    if (DDS::RETCODE_OK != write_publication_data(publicationId, pb)) {
      return false;
    }
    // Match/unmatch with subscriptions
    OPENDDS_STRING topic_name = topic_names_[pb.topic_id_];
    OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
    if (top_it != topics_.end()) {
      match_endpoints(publicationId, top_it->second);
    }
    return true;
  }
  return false;
}

RepoId
Sedp::add_subscription(const RepoId& topicId,
                       DCPS::DataReaderCallbacks* subscription,
                       const DDS::DataReaderQos& qos,
                       const DCPS::TransportLocatorSeq& transInfo,
                       const DDS::SubscriberQos& subscriberQos,
                       const char* filterExpr,
                       const DDS::StringSeq& params)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, RepoId());
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
  sb.filter_ = filterExpr;
  sb.params_ = params;

  TopicDetailsEx& td = topics_[topic_names_[topicId]];
  td.endpoints_.insert(rid);

  if (DDS::RETCODE_OK != write_subscription_data(rid, sb)) {
    return RepoId();
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::add_subscription - ")
                         ACE_TEXT("calling match_endpoints\n")));
  }
  match_endpoints(rid, td);

  return rid;
}

void
Sedp::remove_subscription(const RepoId& subscriptionId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
  if (iter != local_subscriptions_.end()) {
    if (DDS::RETCODE_OK ==
          subscriptions_writer_.write_unregister_dispose(subscriptionId)) {
      OPENDDS_STRING topic_name = topic_names_[iter->second.topic_id_];
      local_subscriptions_.erase(subscriptionId);
      OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
            topics_.find(topic_name);
      if (top_it != topics_.end()) {
        match_endpoints(subscriptionId, top_it->second, true /*remove*/);
        top_it->second.endpoints_.erase(subscriptionId);
      }
    } else {
      ACE_DEBUG((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::remove_subscription - ")
                 ACE_TEXT("Failed to publish dispose msg\n")));
    }
  }
}

bool
Sedp::update_subscription_qos(const RepoId& subscriptionId,
                              const DDS::DataReaderQos& qos,
                              const DDS::SubscriberQos& subscriberQos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
  if (iter != local_subscriptions_.end()) {
    LocalSubscription& sb = iter->second;
    sb.qos_ = qos;
    sb.subscriber_qos_ = subscriberQos;

    if (DDS::RETCODE_OK != write_subscription_data(subscriptionId, sb)) {
      return false;
    }
    // Match/unmatch with subscriptions
    OPENDDS_STRING topic_name = topic_names_[sb.topic_id_];
    OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
    if (top_it != topics_.end()) {
      match_endpoints(subscriptionId, top_it->second);
    }
    return true;
  }
  return false;
}

bool
Sedp::update_subscription_params(const RepoId& subId,
                                 const DDS::StringSeq& params)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  const LocalSubscriptionIter iter = local_subscriptions_.find(subId);
  if (iter != local_subscriptions_.end()) {
    LocalSubscription& sb = iter->second;
    sb.params_ = params;

    if (DDS::RETCODE_OK != write_subscription_data(subId, sb)) {
      return false;
    }

    // Let any associated local publications know about the change
    for (RepoIdSet::iterator i = iter->second.matched_endpoints_.begin();
         i != iter->second.matched_endpoints_.end(); ++i) {
      const LocalPublicationIter lpi = local_publications_.find(*i);
      if (lpi != local_publications_.end()) {
        lpi->second.publication_->update_subscription_params(subId, params);
      }
    }

    return true;
  }
  return false;
}

RepoId
Sedp::make_topic_guid()
{
  RepoId guid;
  guid = participant_id_;
  guid.entityId.entityKind = DCPS::ENTITYKIND_OPENDDS_TOPIC;
  assign(guid.entityId.entityKey, topic_counter_++);

  if (topic_counter_ == 0x1000000) {
    ACE_DEBUG((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::make_topic_guid: ")
               ACE_TEXT("Exceeded Maximum number of topic entity keys!")
               ACE_TEXT("Next key will be a duplicate!\n")));
    topic_counter_ = 0;
  }
  return guid;
}

void
Sedp::shutdown()
{
  task_.shutdown();
  publications_reader_->shutting_down_ = 1;
  subscriptions_reader_->shutting_down_ = 1;
  participant_message_reader_->shutting_down_ = 1;
}

void
Sedp::Task::acknowledge()
{
  // id is really a don't care, but just set to REQUEST_ACK
  putq(new Msg(Msg::MSG_FINI_BIT, DCPS::REQUEST_ACK, 0));
}

void
Sedp::Task::shutdown()
{
  if (!shutting_down_) {
    shutting_down_ = true;
    putq(new Msg(Msg::MSG_STOP, DCPS::GRACEFUL_DISCONNECT, 0));
    wait();
  }
}

void
Sedp::Task::svc_i(DCPS::MessageId message_id,
                  const DiscoveredWriterData* pwdata)
{
  ACE_Auto_Basic_Ptr<const DiscoveredWriterData> delete_the_data(pwdata);
  sedp_->data_received(message_id, *pwdata);
}

void
Sedp::data_received(DCPS::MessageId message_id,
                    const DiscoveredWriterData& wdata)
{
  if (spdp_.shutting_down()) { return; }

  const RepoId& guid = wdata.writerProxy.remoteWriterGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(wdata.ddsPublicationData.topic_name)) {
    return;
  }

  if (!spdp_.has_discovered_participant (guid_participant)) {
    deferred_publications_[guid] = std::make_pair (message_id, wdata);
    return;
  }

  OPENDDS_STRING topic_name;
  // Find the publication  - iterator valid only as long as we hold the lock
  DiscoveredPublicationIter iter = discovered_publications_.find(guid);

  if (message_id == DCPS::SAMPLE_DATA) {
    DiscoveredWriterData wdata_copy;

    if (iter == discovered_publications_.end()) { // add new
      // Must unlock when calling into pub_bit() as it may call back into us
      ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

      { // Reduce scope of pub and td
        DiscoveredPublication& pub =
            discovered_publications_[guid] = DiscoveredPublication(wdata);

        topic_name = get_topic_name(pub);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
        if (top_it == topics_.end()) {
          top_it =
            topics_.insert(std::make_pair(topic_name, TopicDetailsEx())).first;
          top_it->second.data_type_ = wdata.ddsPublicationData.type_name;
          top_it->second.qos_.topic_data = wdata.ddsPublicationData.topic_data;
          top_it->second.repo_id_ = make_topic_guid();

        } else if (top_it->second.data_type_ !=
                   wdata.ddsPublicationData.type_name.in()) {
          inconsistent_topic(top_it->second.endpoints_);
          if (DCPS::DCPS_debug_level) {
            ACE_DEBUG((LM_WARNING,
                       ACE_TEXT("(%P|%t) Sedp::data_received(dwd) - WARNING ")
                       ACE_TEXT("topic %C discovered data type %C doesn't ")
                       ACE_TEXT("match known data type %C, ignoring ")
                       ACE_TEXT("discovered publication.\n"),
                       topic_name.c_str(),
                       wdata.ddsPublicationData.type_name.in(),
                       top_it->second.data_type_.c_str()));
          }
          return;
        }

        TopicDetailsEx& td = top_it->second;
        topic_names_[td.repo_id_] = topic_name;
        td.endpoints_.insert(guid);

        std::memcpy(pub.writer_data_.ddsPublicationData.participant_key.value,
                    guid.guidPrefix, sizeof(DDS::BuiltinTopicKey_t));
        assign_bit_key(pub);
        wdata_copy = pub.writer_data_;
      }

      // Iter no longer valid once lock released
      iter = discovered_publications_.end();

      DDS::InstanceHandle_t instance_handle = DDS::HANDLE_NIL;
      {
        // Release lock for call into pub_bit
        ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
        DDS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          instance_handle =
            bit->store_synthetic_data(wdata_copy.ddsPublicationData,
                                      DDS::NEW_VIEW_STATE);
        }
      }

      if (spdp_.shutting_down()) { return; }
      // Publication may have been removed while lock released
      iter = discovered_publications_.find(guid);
      if (iter != discovered_publications_.end()) {
        iter->second.bit_ih_ = instance_handle;
        OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
            topics_.find(topic_name);
        if (top_it != topics_.end()) {
          if (DCPS::DCPS_debug_level > 3) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::data_received(dwd) - ")
                                 ACE_TEXT("calling match_endpoints new\n")));
          }
          match_endpoints(guid, top_it->second);
        }
      }

    } else if (qosChanged(iter->second.writer_data_.ddsPublicationData,
                          wdata.ddsPublicationData)) { // update existing
      DDS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
      if (bit) { // bit may be null if the DomainParticipant is shutting down
        bit->store_synthetic_data(iter->second.writer_data_.ddsPublicationData,
                                  DDS::NOT_NEW_VIEW_STATE);
      }

      // Match/unmatch local subscription(s)
      topic_name = get_topic_name(iter->second);
      OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
      if (top_it != topics_.end()) {
        if (DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::data_received(dwd) - ")
                               ACE_TEXT("calling match_endpoints update\n")));
        }
        match_endpoints(guid, top_it->second);
      }
    }

  } else if (message_id == DCPS::UNREGISTER_INSTANCE ||
             message_id == DCPS::DISPOSE_INSTANCE ||
             message_id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
    if (iter != discovered_publications_.end()) {
      // Unmatch local subscription(s)
      topic_name = get_topic_name(iter->second);
      OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
      if (top_it != topics_.end()) {
        top_it->second.endpoints_.erase(guid);
        match_endpoints(guid, top_it->second, true /*remove*/);
        if (spdp_.shutting_down()) { return; }
      }
      remove_from_bit(iter->second);
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::data_received(dwd) - ")
                             ACE_TEXT("calling match_endpoints disp/unreg\n")));
      }
      discovered_publications_.erase(iter);
    }
  }
}

void
Sedp::Task::svc_i(DCPS::MessageId message_id,
                  const DiscoveredReaderData* prdata)
{
  ACE_Auto_Basic_Ptr<const DiscoveredReaderData> delete_the_data(prdata);
  sedp_->data_received(message_id, *prdata);
}

void
Sedp::data_received(DCPS::MessageId message_id,
                    const DiscoveredReaderData& rdata)
{
  if (spdp_.shutting_down()) { return; }

  const RepoId& guid = rdata.readerProxy.remoteReaderGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(rdata.ddsSubscriptionData.topic_name)) {
    return;
  }

  if (!spdp_.has_discovered_participant (guid_participant)) {
    deferred_subscriptions_[guid] = std::make_pair (message_id, rdata);
    return;
  }

  OPENDDS_STRING topic_name;
  // Find the publication  - iterator valid only as long as we hold the lock
  DiscoveredSubscriptionIter iter = discovered_subscriptions_.find(guid);

  // Must unlock when calling into sub_bit() as it may call back into us
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  if (message_id == DCPS::SAMPLE_DATA) {
    DiscoveredReaderData rdata_copy;

    if (iter == discovered_subscriptions_.end()) { // add new
      { // Reduce scope of sub and td
        DiscoveredSubscription& sub =
          discovered_subscriptions_[guid] = DiscoveredSubscription(rdata);

        topic_name = get_topic_name(sub);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
        if (top_it == topics_.end()) {
          top_it =
            topics_.insert(std::make_pair(topic_name, TopicDetailsEx())).first;
          top_it->second.data_type_ = rdata.ddsSubscriptionData.type_name;
          top_it->second.qos_.topic_data = rdata.ddsSubscriptionData.topic_data;
          top_it->second.repo_id_ = make_topic_guid();

        } else if (top_it->second.data_type_ !=
                   rdata.ddsSubscriptionData.type_name.in()) {
          inconsistent_topic(top_it->second.endpoints_);
          if (DCPS::DCPS_debug_level) {
            ACE_DEBUG((LM_WARNING,
                       ACE_TEXT("(%P|%t) Sedp::data_received(drd) - WARNING ")
                       ACE_TEXT("topic %C discovered data type %C doesn't ")
                       ACE_TEXT("match known data type %C, ignoring ")
                       ACE_TEXT("discovered subcription.\n"),
                       topic_name.c_str(),
                       rdata.ddsSubscriptionData.type_name.in(),
                       top_it->second.data_type_.c_str()));
          }
          return;
        }

        TopicDetailsEx& td = top_it->second;
        topic_names_[td.repo_id_] = topic_name;
        td.endpoints_.insert(guid);

        std::memcpy(sub.reader_data_.ddsSubscriptionData.participant_key.value,
                    guid.guidPrefix, sizeof(DDS::BuiltinTopicKey_t));
        assign_bit_key(sub);
        rdata_copy = sub.reader_data_;
      }

      // Iter no longer valid once lock released
      iter = discovered_subscriptions_.end();

      DDS::InstanceHandle_t instance_handle = DDS::HANDLE_NIL;
      {
        // Release lock for call into sub_bit
        ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
        DDS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          instance_handle =
            bit->store_synthetic_data(rdata_copy.ddsSubscriptionData,
                                      DDS::NEW_VIEW_STATE);
        }
      }

      if (spdp_.shutting_down()) { return; }
      // Subscription may have been removed while lock released
      iter = discovered_subscriptions_.find(guid);
      if (iter != discovered_subscriptions_.end()) {
        iter->second.bit_ih_ = instance_handle;
        OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
            topics_.find(topic_name);
        if (top_it != topics_.end()) {
          if (DCPS::DCPS_debug_level > 3) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::data_received(drd) - ")
                                 ACE_TEXT("calling match_endpoints new\n")));
          }
          match_endpoints(guid, top_it->second);
        }
      }

    } else { // update existing
      if (qosChanged(iter->second.reader_data_.ddsSubscriptionData,
                     rdata.ddsSubscriptionData)) {
        DDS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          bit->store_synthetic_data(
                iter->second.reader_data_.ddsSubscriptionData,
                DDS::NOT_NEW_VIEW_STATE);
        }

        // Match/unmatch local publication(s)
        topic_name = get_topic_name(iter->second);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
            topics_.find(topic_name);
        if (top_it != topics_.end()) {
          if (DCPS::DCPS_debug_level > 3) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::data_received(drd) - ")
                                 ACE_TEXT("calling match_endpoints update\n")));
          }
          match_endpoints(guid, top_it->second);
        }
      }

      if (paramsChanged(iter->second.reader_data_.contentFilterProperty,
                        rdata.contentFilterProperty)) {
        // Let any associated local publications know about the change
        topic_name = get_topic_name(iter->second);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
            topics_.find(topic_name);
        const RepoIdSet& assoc =
          (top_it == topics_.end()) ? RepoIdSet() : top_it->second.endpoints_;
        for (RepoIdSet::const_iterator i = assoc.begin(); i != assoc.end(); ++i) {
          if (i->entityId.entityKind & 4) continue; // subscription
          const LocalPublicationIter lpi = local_publications_.find(*i);
          if (lpi != local_publications_.end()) {
            lpi->second.publication_->update_subscription_params(guid,
              rdata.contentFilterProperty.expressionParameters);
          }
        }
      }
    }
    // For each associated opendds writer to this reader
    CORBA::ULong len = rdata.readerProxy.associatedWriters.length();
    for (CORBA::ULong writerIndex = 0; writerIndex < len; ++writerIndex)
    {
      GUID_t writerGuid = rdata.readerProxy.associatedWriters[writerIndex];

      // If the associated writer is in this participant
      LocalPublicationIter lp = local_publications_.find(writerGuid);
      if (lp != local_publications_.end()) {
        // If the local writer is not fully associated with the reader
        if (lp->second.remote_opendds_associations_.insert(guid).second) {
          // This is a new association
          lp->second.publication_->association_complete(guid);
        }
      }
    }

  } else if (message_id == DCPS::UNREGISTER_INSTANCE ||
             message_id == DCPS::DISPOSE_INSTANCE ||
             message_id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
    if (iter != discovered_subscriptions_.end()) {
      // Unmatch local publication(s)
      topic_name = get_topic_name(iter->second);
      OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx)::iterator top_it =
          topics_.find(topic_name);
      if (top_it != topics_.end()) {
        top_it->second.endpoints_.erase(guid);
        if (DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::data_received(drd) - ")
                               ACE_TEXT("calling match_endpoints disp/unreg\n")));
        }
        match_endpoints(guid, top_it->second, true /*remove*/);
        if (spdp_.shutting_down()) { return; }
      }
      remove_from_bit(iter->second);
      discovered_subscriptions_.erase(iter);
    }
  }
}

void
Sedp::Task::svc_i(DCPS::MessageId message_id,
                  const ParticipantMessageData* data)
{
  ACE_Auto_Basic_Ptr<const ParticipantMessageData> delete_the_data(data);
  sedp_->data_received(message_id, *data);
}

void
Sedp::data_received(DCPS::MessageId /*message_id*/,
                    const ParticipantMessageData& data)
{
  if (spdp_.shutting_down()) { return; }

  const RepoId& guid = data.participantGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;
  RepoId prefix = data.participantGuid;
  prefix.entityId = EntityId_t();

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)) {
    return;
  }

  if (!spdp_.has_discovered_participant (guid_participant)) {
    return;
  }

  // Clear the entityId so lower bound will work.
  for (LocalSubscriptionMap::const_iterator sub_pos = local_subscriptions_.begin(),
         sub_limit = local_subscriptions_.end();
       sub_pos != sub_limit;
       ++sub_pos) {
    for (RepoIdSet::const_iterator pos = sub_pos->second.matched_endpoints_.lower_bound(prefix),
           limit = sub_pos->second.matched_endpoints_.end();
         pos != limit;
         ++pos) {
      if (OpenDDS::DCPS::GuidPrefixEqual() (pos->guidPrefix, prefix.guidPrefix)) {
        sub_pos->second.subscription_->signal_liveliness(guid_participant);
        break;
      } else {
        break;
      }
    }
  }
}

// helper for match(), below
struct DcpsUpcalls : ACE_Task_Base {
  DcpsUpcalls(DCPS::DataReaderCallbacks* drr,
              const RepoId& reader,
              const DCPS::WriterAssociation& wa,
              bool active,
              DCPS::DataWriterCallbacks* dwr);
  int svc();
  void writer_done();
  DCPS::DataReaderCallbacks* const drr_;
  const RepoId& reader_;
  const DCPS::WriterAssociation& wa_;
  bool active_;
  DCPS::DataWriterCallbacks* const dwr_;
  bool reader_done_, writer_done_;
  ACE_Thread_Mutex mtx_;
  ACE_Condition_Thread_Mutex cnd_;
};

void
Sedp::match_endpoints(RepoId repoId, const TopicDetailsEx& td,
                      bool remove)
{
  const bool reader = repoId.entityId.entityKind & 4;
  // Copy the endpoint set - lock can be released in match()
  RepoIdSet endpoints_copy = td.endpoints_;

  for (RepoIdSet::const_iterator iter = endpoints_copy.begin();
       iter != endpoints_copy.end(); ++iter) {
    // check to make sure it's a Reader/Writer or Writer/Reader match
    if (bool(iter->entityId.entityKind & 4) != reader) {
      if (remove) {
        remove_assoc(*iter, repoId);
      } else {
        match(reader ? *iter : repoId, reader ? repoId : *iter);
      }
    }
  }
}

void
Sedp::match(const RepoId& writer, const RepoId& reader)
{
  // 0. For discovered endpoints, we'll have the QoS info in the form of the
  // publication or subscription BIT data which doesn't use the same structures
  // for QoS.  In those cases we can copy the individual QoS policies to temp
  // QoS structs:
  DDS::DataWriterQos tempDwQos;
  DDS::PublisherQos tempPubQos;
  DDS::DataReaderQos tempDrQos;
  DDS::SubscriberQos tempSubQos;
  ContentFilterProperty_t tempCfp;

  // 1. collect details about the writer, which may be local or discovered
  const DDS::DataWriterQos* dwQos = 0;
  const DDS::PublisherQos* pubQos = 0;
  DCPS::TransportLocatorSeq* wTls = 0;

  const LocalPublicationIter lpi = local_publications_.find(writer);
  DiscoveredPublicationIter dpi;
  bool writer_local = false, already_matched = false;
  if (lpi != local_publications_.end()) {
    writer_local = true;
    dwQos = &lpi->second.qos_;
    pubQos = &lpi->second.publisher_qos_;
    wTls = &lpi->second.trans_info_;
    already_matched = lpi->second.matched_endpoints_.count(reader);
  } else if ((dpi = discovered_publications_.find(writer))
             != discovered_publications_.end()) {
    wTls = &dpi->second.writer_data_.writerProxy.allLocators;
  } else {
    return; // Possible and ok, since lock is released
  }

  // 2. collect details about the reader, which may be local or discovered
  const DDS::DataReaderQos* drQos = 0;
  const DDS::SubscriberQos* subQos = 0;
  DCPS::TransportLocatorSeq* rTls = 0;
  const ContentFilterProperty_t* cfProp = 0;

  const LocalSubscriptionIter lsi = local_subscriptions_.find(reader);
  DiscoveredSubscriptionIter dsi;
  bool reader_local = false;
  if (lsi != local_subscriptions_.end()) {
    reader_local = true;
    drQos = &lsi->second.qos_;
    subQos = &lsi->second.subscriber_qos_;
    rTls = &lsi->second.trans_info_;
    if (!lsi->second.filter_.empty()) {
      tempCfp.filterExpression = lsi->second.filter_.c_str();
      tempCfp.expressionParameters = lsi->second.params_;
    }
    cfProp = &tempCfp;
    if (!already_matched) {
      already_matched = lsi->second.matched_endpoints_.count(writer);
    }
  } else if ((dsi = discovered_subscriptions_.find(reader))
             != discovered_subscriptions_.end()) {
    if (!writer_local) {
      // this is a discovered/discovered match, nothing for us to do
      return;
    }
    rTls = &dsi->second.reader_data_.readerProxy.allLocators;

    LocatorSeq locs;
    bool participantExpectsInlineQos = false;
    RepoId remote_participant = reader;
    remote_participant.entityId = ENTITYID_PARTICIPANT;
    const bool participant_found =
      spdp_.get_default_locators(remote_participant, locs,
                                 participantExpectsInlineQos);
    if (!rTls->length()) {     // if no locators provided, add the default
      if (!participant_found) {
        defer_match_endpoints_.insert(reader);
        return;
      } else if (locs.length()) {
        size_t size = 0, padding = 0;
        DCPS::gen_find_size(locs, size, padding);

        ACE_Message_Block mb_locator(size + 1);   // Add space for boolean
        using DCPS::Serializer;
        Serializer ser_loc(&mb_locator,
                           ACE_CDR_BYTE_ORDER,
                           Serializer::ALIGN_CDR);
        ser_loc << locs;
        const bool readerExpectsInlineQos =
          dsi->second.reader_data_.readerProxy.expectsInlineQos;
        ser_loc << ACE_OutputCDR::from_boolean(participantExpectsInlineQos
                                               || readerExpectsInlineQos);

        DCPS::TransportLocator tl;
        tl.transport_type = "rtps_udp";
        tl.data.replace(static_cast<CORBA::ULong>(mb_locator.length()),
                        &mb_locator);
        rTls->length(1);
        (*rTls)[0] = tl;
      } else {
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT("(%P|%t) Sedp::match - ")
                   ACE_TEXT("remote reader found with no locators ")
                   ACE_TEXT("and no default locators\n")));
      }
    }

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
    drQos = &tempDrQos;
    tempSubQos.presentation = bit.presentation;
    tempSubQos.partition = bit.partition;
    tempSubQos.group_data = bit.group_data;
    tempSubQos.entity_factory =
      TheServiceParticipant->initial_EntityFactoryQosPolicy();
    subQos = &tempSubQos;
    cfProp = &dsi->second.reader_data_.contentFilterProperty;
  } else {
    return; // Possible and ok, since lock is released
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
    dwQos = &tempDwQos;
    tempPubQos.presentation = bit.presentation;
    tempPubQos.partition = bit.partition;
    tempPubQos.group_data = bit.group_data;
    tempPubQos.entity_factory =
      TheServiceParticipant->initial_EntityFactoryQosPolicy();
    pubQos = &tempPubQos;

    LocatorSeq locs;
    bool participantExpectsInlineQos = false;
    RepoId remote_participant = writer;
    remote_participant.entityId = ENTITYID_PARTICIPANT;
    const bool participant_found =
      spdp_.get_default_locators(remote_participant, locs,
                                 participantExpectsInlineQos);
    if (!wTls->length()) {     // if no locators provided, add the default
      if (!participant_found) {
        defer_match_endpoints_.insert(writer);
        return;
      } else if (locs.length()) {
        size_t size = 0, padding = 0;
        DCPS::gen_find_size(locs, size, padding);

        ACE_Message_Block mb_locator(size + 1);   // Add space for boolean
        using DCPS::Serializer;
        Serializer ser_loc(&mb_locator,
                           ACE_CDR_BYTE_ORDER,
                           Serializer::ALIGN_CDR);
        ser_loc << locs;
        ser_loc << ACE_OutputCDR::from_boolean(participantExpectsInlineQos);

        DCPS::TransportLocator tl;
        tl.transport_type = "rtps_udp";
        tl.data.replace(static_cast<CORBA::ULong>(mb_locator.length()),
                        &mb_locator);
        wTls->length(1);
        (*wTls)[0] = tl;
      } else {
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT("(%P|%t) Sedp::match - ")
                   ACE_TEXT("remote writer found with no locators ")
                   ACE_TEXT("and no default locators\n")));
      }
    }
  }

  // Need to release lock, below, for callbacks into DCPS which could
  // call into Spdp/Sedp.  Note that this doesn't unlock, it just constructs
  // an ACE object which will be used below for unlocking.
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  // 3. check transport and QoS compatibility

  // Copy entries from local publication and local subscription maps
  // prior to releasing lock
  DCPS::DataWriterCallbacks* dwr = 0;
  DCPS::DataReaderCallbacks* drr = 0;
  if (writer_local) {
    dwr = lpi->second.publication_;
  }
  if (reader_local) {
    drr = lsi->second.subscription_;
  }

  DCPS::IncompatibleQosStatus
    writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()},
    readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};

  if (DCPS::compatibleQOS(&writerStatus, &readerStatus, *wTls, *rTls,
                          dwQos, drQos, pubQos, subQos)) {
    if (!writer_local) {
      RepoId writer_participant = writer;
      writer_participant.entityId = ENTITYID_PARTICIPANT;
      if (!associated_participants_.count(writer_participant)) {
        if (DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match - ")
                               ACE_TEXT("remote writer deferred\n")));
        }
        defer_match_endpoints_.insert(writer);
        return;
      }
    }
    if (!reader_local) {
      RepoId reader_participant = reader;
      reader_participant.entityId = ENTITYID_PARTICIPANT;
      if (!associated_participants_.count(reader_participant)) {
        if (DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match - ")
                               ACE_TEXT("remote reader deferred\n")));
        }
        defer_match_endpoints_.insert(reader);
        return;
      }
    }

    bool call_writer = false, call_reader = false;
    if (writer_local) {
      call_writer = lpi->second.matched_endpoints_.insert(reader).second;
    }
    if (reader_local) {
      call_reader = lsi->second.matched_endpoints_.insert(writer).second;
    }
    if (!call_writer && !call_reader) {
      return; // nothing more to do
    }
    // Copy reader and writer association data prior to releasing lock
#ifdef __SUNPRO_CC
    DCPS::ReaderAssociation ra;
    ra.readerTransInfo = *rTls;
    ra.readerId = reader;
    ra.subQos = *subQos;
    ra.readerQos = *drQos;
    ra.filterExpression = cfProp->filterExpression;
    ra.exprParams = cfProp->expressionParameters;
    DCPS::WriterAssociation wa;
    wa.writerTransInfo = *wTls;
    wa.writerId = writer;
    wa.pubQos = *pubQos;
    wa.writerQos = *dwQos;
#else
    const DCPS::ReaderAssociation ra =
        {*rTls, reader, *subQos, *drQos,
         cfProp->filterExpression, cfProp->expressionParameters};
    const DCPS::WriterAssociation wa = {*wTls, writer, *pubQos, *dwQos};
#endif

    ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
    static const bool writer_active = true;

    if (call_writer) {
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match - ")
                             ACE_TEXT("adding writer association\n")));
      }
      DcpsUpcalls thr(drr, reader, wa, !writer_active, dwr);
      if (call_reader) {
        thr.activate();
      }
      dwr->add_association(writer, ra, writer_active);
      if (call_reader) {
        thr.writer_done();
      }

    } else if (call_reader) {
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match - ")
                             ACE_TEXT("adding reader association\n")));
      }
      drr->add_association(reader, wa, !writer_active);
    }

    // change this if 'writer_active' (above) changes
    if (call_writer && !call_reader && !is_opendds(reader)) {
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Sedp::match - ")
                   ACE_TEXT("calling writer association_complete\n")));
      }
      dwr->association_complete(reader);
    }

  } else if (already_matched) { // break an existing associtaion
    if (writer_local) {
      lpi->second.matched_endpoints_.erase(reader);
      lpi->second.remote_opendds_associations_.erase(reader);
    }
    if (reader_local) {
      lsi->second.matched_endpoints_.erase(writer);
      lsi->second.remote_opendds_associations_.erase(writer);
    }
    ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
    if (writer_local) {
      DCPS::ReaderIdSeq reader_seq(1);
      reader_seq.length(1);
      reader_seq[0] = reader;
      dwr->remove_associations(reader_seq, false /*notify_lost*/);
    }
    if (reader_local) {
      DCPS::WriterIdSeq writer_seq(1);
      writer_seq.length(1);
      writer_seq[0] = writer;
      drr->remove_associations(writer_seq, false /*notify_lost*/);
    }

  } else { // something was incompatible
    ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
    if (writer_local && writerStatus.count_since_last_send) {
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match - ")
                             ACE_TEXT("writer incompatible\n")));
      }
      dwr->update_incompatible_qos(writerStatus);
    }
    if (reader_local && readerStatus.count_since_last_send) {
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match - ")
                             ACE_TEXT("reader incompatible\n")));
      }
      drr->update_incompatible_qos(readerStatus);
    }
  }
}

DcpsUpcalls::DcpsUpcalls(DCPS::DataReaderCallbacks* drr,
                         const RepoId& reader,
                         const DCPS::WriterAssociation& wa,
                         bool active,
                         DCPS::DataWriterCallbacks* dwr)
  : drr_(drr), reader_(reader), wa_(wa), active_(active), dwr_(dwr)
  , reader_done_(false), writer_done_(false), cnd_(mtx_)
{}

int
DcpsUpcalls::svc()
{
  drr_->add_association(reader_, wa_, active_);
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mtx_, -1);
    reader_done_ = true;
    cnd_.signal();
    while (!writer_done_) {
      cnd_.wait();
    }
  }
  dwr_->association_complete(reader_);
  return 0;
}

void
DcpsUpcalls::writer_done()
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mtx_);
    writer_done_ = true;
    cnd_.signal();
  }
  wait();
}

void
Sedp::remove_assoc(const RepoId& remove_from,
                   const RepoId& removing)
{
  const bool reader = remove_from.entityId.entityKind & 4;
  if (reader) {
    const LocalSubscriptionIter lsi = local_subscriptions_.find(remove_from);
    if (lsi != local_subscriptions_.end()) {
      lsi->second.matched_endpoints_.erase(removing);
      DCPS::WriterIdSeq writer_seq(1);
      writer_seq.length(1);
      writer_seq[0] = removing;
      lsi->second.remote_opendds_associations_.erase(removing);
      lsi->second.subscription_->remove_associations(writer_seq,
                                                     false /*notify_lost*/);
      // Update writer
      write_subscription_data(remove_from, lsi->second);
    }

  } else {
    const LocalPublicationIter lpi = local_publications_.find(remove_from);
    if (lpi != local_publications_.end()) {
      lpi->second.matched_endpoints_.erase(removing);
      DCPS::ReaderIdSeq reader_seq(1);
      reader_seq.length(1);
      reader_seq[0] = removing;
      lpi->second.remote_opendds_associations_.erase(removing);
      lpi->second.publication_->remove_associations(reader_seq,
                                                    false /*notify_lost*/);
    }
  }
}

void
Sedp::association_complete(const RepoId& localId,
                           const RepoId& remoteId)
{
  // If the remote endpoint is an opendds endpoint
  if (is_opendds(remoteId)) {
    LocalSubscriptionIter sub = local_subscriptions_.find(localId);
    // If the local endpoint is a reader
    if (sub != local_subscriptions_.end()) {
      std::pair<RepoIdSet::iterator, bool> result =
          sub->second.remote_opendds_associations_.insert(remoteId);
      // If this is a new association for the local reader
      if (result.second) {
        // Tell other participants
        write_subscription_data(localId, sub->second);
      }
    }
  }
}

void
Sedp::signal_liveliness(DDS::LivelinessQosPolicyKind kind)
{
  ParticipantMessageData pmd;
  pmd.participantGuid = participant_id_;
  switch (kind) {
  case DDS::AUTOMATIC_LIVELINESS_QOS:
    pmd.participantGuid.entityId = OpenDDS::DCPS::EntityIdConverter(PARTICIPANT_MESSAGE_DATA_KIND_AUTOMATIC_LIVELINESS_UPDATE);
    participant_message_writer_.write_sample(pmd, GUID_UNKNOWN, automatic_liveliness_seq_);
    break;
  case DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
    pmd.participantGuid.entityId = OpenDDS::DCPS::EntityIdConverter(PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE);
    participant_message_writer_.write_sample(pmd, GUID_UNKNOWN, manual_liveliness_seq_);
    break;
  case DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS:
    // Do nothing.
    break;
  }
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
Sedp::Writer::data_delivered(const DCPS::DataSampleElement* dsle)
{
  delete dsle;
}

void
Sedp::Writer::data_dropped(const DCPS::DataSampleElement* dsle, bool)
{
  delete dsle;
}

void
Sedp::Writer::control_delivered(ACE_Message_Block* mb)
{
  if (mb->flags() == ACE_Message_Block::DONT_DELETE) {
    // We allocated mb on stack, its continuation block on heap
    delete mb->cont();
  } else {
    mb->release();
  }
}

void
Sedp::Writer::control_dropped(ACE_Message_Block* mb, bool)
{
  if (mb->flags() == ACE_Message_Block::DONT_DELETE) {
    // We allocated mb on stack, its continuation block on heap
    delete mb->cont();
  } else {
    mb->release();
  }
}

DDS::ReturnCode_t
Sedp::Writer::write_sample(const ParameterList& plist,
                           const DCPS::RepoId& reader,
                           DCPS::SequenceNumber& sequence)
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
    DCPS::DataSampleElement* list_el =
      new DCPS::DataSampleElement(repo_id_, this, 0, &alloc_, 0);
    set_header_fields(list_el->get_header(), size, reader, sequence);

    list_el->set_sample(new ACE_Message_Block(size));
    *list_el->get_sample() << list_el->get_header();
    list_el->get_sample()->cont(payload.duplicate());

    if (reader != GUID_UNKNOWN) {
      list_el->set_sub_id(0, reader);
      list_el->set_num_subs(1);
    }

    DCPS::SendStateDataSampleList list;
    list.enqueue_tail(list_el);

    send(list);
  }
  delete payload.cont();
  return result;
}

DDS::ReturnCode_t
Sedp::Writer::write_sample(const ParticipantMessageData& pmd,
                           const DCPS::RepoId& reader,
                           DCPS::SequenceNumber& sequence)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  // Determine message length
  size_t size = 0, padding = 0;
  DCPS::find_size_ulong(size, padding);
  DCPS::gen_find_size(pmd, size, padding);

  // Build RTPS message
  ACE_Message_Block payload(DCPS::DataSampleHeader::max_marshaled_size(),
                            ACE_Message_Block::MB_DATA,
                            new ACE_Message_Block(size));
  using DCPS::Serializer;
  Serializer ser(payload.cont(), host_is_bigendian_, Serializer::ALIGN_CDR);
  bool ok = (ser << ACE_OutputCDR::from_octet(0)) &&  // CDR_LE = 0x0001
            (ser << ACE_OutputCDR::from_octet(1)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << pmd);
  if (!ok) {
    result = DDS::RETCODE_ERROR;
  }

  if (result == DDS::RETCODE_OK) {
    // Send sample
    DCPS::DataSampleElement* list_el =
      new DCPS::DataSampleElement(repo_id_, this, 0, &alloc_, 0);
    set_header_fields(list_el->get_header(), size, reader, sequence);

    list_el->set_sample(new ACE_Message_Block(size));
    *list_el->get_sample() << list_el->get_header();
    list_el->get_sample()->cont(payload.duplicate());

    if (reader != GUID_UNKNOWN) {
      list_el->set_sub_id(0, reader);
      list_el->set_num_subs(1);
    }

    DCPS::SendStateDataSampleList list;
    list.enqueue_tail(list_el);

    send(list);
  }
  delete payload.cont();
  return result;
}

DDS::ReturnCode_t
Sedp::Writer::write_unregister_dispose(const RepoId& rid)
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

  ACE_Message_Block* payload = new ACE_Message_Block(DCPS::DataSampleHeader::max_marshaled_size(),
                            ACE_Message_Block::MB_DATA,
                            new ACE_Message_Block(size));

  if (!payload) {
    ACE_DEBUG((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::Writer::write_unregister_dispose")
               ACE_TEXT(" - Failed to allocate message block message\n")));
    return DDS::RETCODE_ERROR;
  }

  using DCPS::Serializer;
  Serializer ser(payload->cont(), host_is_bigendian_, Serializer::ALIGN_CDR);
  bool ok = (ser << ACE_OutputCDR::from_octet(0)) &&  // PL_CDR_LE = 0x0003
            (ser << ACE_OutputCDR::from_octet(3)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << plist);

  if (ok) {
    // Send
    write_control_msg(*payload, size, DCPS::DISPOSE_UNREGISTER_INSTANCE);
    return DDS::RETCODE_OK;
  } else {
    // Error
    ACE_DEBUG((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::Writer::write_unregister_dispose")
               ACE_TEXT(" - Failed to serialize RTPS control message\n")));
    return DDS::RETCODE_ERROR;
  }
}

void
Sedp::Writer::end_historic_samples(const DCPS::RepoId& reader)
{
  const void* pReader = static_cast<const void*>(&reader);
  ACE_Message_Block mb(DCPS::DataSampleHeader::max_marshaled_size(),
                       ACE_Message_Block::MB_DATA,
                       new ACE_Message_Block(static_cast<const char*>(pReader),
                                             sizeof(reader)));
  mb.set_flags(ACE_Message_Block::DONT_DELETE);
  mb.cont()->wr_ptr(sizeof(reader));
  // 'mb' would contain the DSHeader, but we skip it. mb.cont() has the data
  write_control_msg(mb, sizeof(reader), DCPS::END_HISTORIC_SAMPLES,
                    DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN());
}

void
Sedp::Writer::write_control_msg(ACE_Message_Block& payload,
                                size_t size,
                                DCPS::MessageId id,
                                DCPS::SequenceNumber seq)
{
  DCPS::DataSampleHeader header;
  set_header_fields(header, size, GUID_UNKNOWN, seq, id);
  // no need to serialize header since rtps_udp transport ignores it
  send_control(header, &payload);
}

void
Sedp::Writer::set_header_fields(DCPS::DataSampleHeader& dsh,
                                size_t size,
                                const DCPS::RepoId& reader,
                                DCPS::SequenceNumber& sequence,
                                DCPS::MessageId id)
{
  dsh.message_id_ = id;
  dsh.byte_order_ = ACE_CDR_BYTE_ORDER;
  dsh.message_length_ = static_cast<ACE_UINT32>(size);
  dsh.publication_id_ = repo_id_;

  if (reader == GUID_UNKNOWN ||
      sequence == DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    sequence = seq_++;
  }

  if (reader != GUID_UNKNOWN) {
    // retransmit with same seq# for durability
    dsh.historic_sample_ = true;
  }

  dsh.sequence_ = sequence;

  const ACE_Time_Value now = ACE_OS::gettimeofday();
  dsh.source_timestamp_sec_ = static_cast<ACE_INT32>(now.sec());
  dsh.source_timestamp_nanosec_ = now.usec() * 1000;
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

static bool
decode_parameter_list(const DCPS::ReceivedDataSample& sample,
                      DCPS::Serializer& ser,
                      const ACE_CDR::Octet& encap,
                      ParameterList& data)
{
  bool ok = true;
  if (ok && sample.header_.key_fields_only_ && encap < 2) {
    GUID_t guid;
    ok &= (ser >> guid);
    data.length(1);
    data[0].guid(guid);
    data[0]._d(PID_ENDPOINT_GUID);
  } else {
    ok &= (ser >> data);
  }
  return ok;
}

void
Sedp::Reader::data_received(const DCPS::ReceivedDataSample& sample)
{
  if (shutting_down_.value()) return;

  switch (sample.header_.message_id_) {
  case DCPS::SAMPLE_DATA:
  case DCPS::DISPOSE_INSTANCE:
  case DCPS::UNREGISTER_INSTANCE:
  case DCPS::DISPOSE_UNREGISTER_INSTANCE: {
    const DCPS::MessageId id =
      static_cast<DCPS::MessageId>(sample.header_.message_id_);

    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::Octet encap, dummy;
    ACE_CDR::UShort options;
    ok &= (ser >> ACE_InputCDR::to_octet(dummy))
      && (ser >> ACE_InputCDR::to_octet(encap))
      && (ser >> options);

    // Ignore the 'encap' byte order since we use sample.header_.byte_order_
    // to determine whether or not to swap bytes.

    if (sample.header_.publication_id_.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER) {
      ParameterList data;
      if (!decode_parameter_list(sample, ser, encap, data)) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }

      ACE_Auto_Ptr<DiscoveredWriterData> wdata(new DiscoveredWriterData);
      if (ParameterListConverter::from_param_list(data, *wdata) < 0) {
        ACE_DEBUG((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to DiscoveredWriterData\n")));
        return;
      }
      sedp_.task_.enqueue(id, wdata.release());

    } else if (sample.header_.publication_id_.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER) {
      ParameterList data;
      if (!decode_parameter_list(sample, ser, encap, data)) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }

      ACE_Auto_Ptr<DiscoveredReaderData> rdata(new DiscoveredReaderData);
      if (ParameterListConverter::from_param_list(data, *rdata) < 0) {
        ACE_DEBUG((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to DiscoveredReaderData\n")));
        return;
      }
      if (rdata->readerProxy.expectsInlineQos) {
        set_inline_qos(rdata->readerProxy.allLocators);
      }
      sedp_.task_.enqueue(id, rdata.release());

    } else if (sample.header_.publication_id_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER) {
      ACE_Auto_Ptr<ParticipantMessageData> data(new ParticipantMessageData);
      if (!(ser >> *data)) {
        ACE_DEBUG((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }

      sedp_.task_.enqueue(id, data.release());

    }
  }
    break;

  default:
    break;
  }
}

void
Sedp::populate_discovered_writer_msg(
    DiscoveredWriterData& dwd,
    const RepoId& publication_id,
    const LocalPublication& pub)
{
  // Ignored on the wire dwd.ddsPublicationData.key
  // Ignored on the wire dwd.ddsPublicationData.participant_key
  OPENDDS_STRING topic_name = topic_names_[pub.topic_id_];
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
  // Ignore dwd.writerProxy.unicastLocatorList;
  // Ignore dwd.writerProxy.multicastLocatorList;
  dwd.writerProxy.allLocators = pub.trans_info_;
}

void
Sedp::populate_discovered_reader_msg(
    DiscoveredReaderData& drd,
    const RepoId& subscription_id,
    const LocalSubscription& sub)
{
  // Ignored on the wire drd.ddsSubscription.key
  // Ignored on the wire drd.ddsSubscription.participant_key
  OPENDDS_STRING topic_name = topic_names_[sub.topic_id_];
  drd.ddsSubscriptionData.topic_name = topic_name.c_str();
  TopicDetails& topic_details = topics_[topic_name];
  drd.ddsSubscriptionData.type_name = topic_details.data_type_.c_str();
  drd.ddsSubscriptionData.durability = sub.qos_.durability;
  drd.ddsSubscriptionData.deadline = sub.qos_.deadline;
  drd.ddsSubscriptionData.latency_budget = sub.qos_.latency_budget;
  drd.ddsSubscriptionData.liveliness = sub.qos_.liveliness;
  drd.ddsSubscriptionData.reliability = sub.qos_.reliability;
  drd.ddsSubscriptionData.ownership = sub.qos_.ownership;
  drd.ddsSubscriptionData.destination_order = sub.qos_.destination_order;
  drd.ddsSubscriptionData.user_data = sub.qos_.user_data;
  drd.ddsSubscriptionData.time_based_filter = sub.qos_.time_based_filter;
  drd.ddsSubscriptionData.presentation = sub.subscriber_qos_.presentation;
  drd.ddsSubscriptionData.partition = sub.subscriber_qos_.partition;
  drd.ddsSubscriptionData.topic_data = topic_details.qos_.topic_data;
  drd.ddsSubscriptionData.group_data = sub.subscriber_qos_.group_data;
  drd.readerProxy.remoteReaderGuid = subscription_id;
  drd.readerProxy.expectsInlineQos = false;  // We never expect inline qos
  // Ignore drd.readerProxy.unicastLocatorList;
  // Ignore drd.readerProxy.multicastLocatorList;
  drd.readerProxy.allLocators = sub.trans_info_;
  drd.contentFilterProperty.contentFilteredTopicName =
    OPENDDS_STRING(DCPS::GuidConverter(subscription_id)).c_str();
  drd.contentFilterProperty.relatedTopicName = topic_name.c_str();
  drd.contentFilterProperty.filterClassName = ""; // PLConverter adds default
  drd.contentFilterProperty.filterExpression = sub.filter_.c_str();
  drd.contentFilterProperty.expressionParameters = sub.params_;
  for (RepoIdSet::const_iterator writer = sub.remote_opendds_associations_.begin();
       writer != sub.remote_opendds_associations_.end();
       ++writer)
  {
    CORBA::ULong len = drd.readerProxy.associatedWriters.length();
    drd.readerProxy.associatedWriters.length(len + 1);
    drd.readerProxy.associatedWriters[len] = *writer;
  }
}

void
Sedp::write_durable_publication_data(const DCPS::RepoId& reader)
{
  LocalPublicationIter pub, end = local_publications_.end();
  for (pub = local_publications_.begin(); pub != end; ++pub) {
    write_publication_data(pub->first, pub->second, reader);
  }
  publications_writer_.end_historic_samples(reader);
}

void
Sedp::write_durable_subscription_data(const DCPS::RepoId& reader)
{
  LocalSubscriptionIter sub, end = local_subscriptions_.end();
  for (sub = local_subscriptions_.begin(); sub != end; ++sub) {
    write_subscription_data(sub->first, sub->second, reader);
  }
  subscriptions_writer_.end_historic_samples(reader);
}

void
Sedp::write_durable_participant_message_data(const DCPS::RepoId& reader)
{
  LocalParticipantMessageIter part, end = local_participant_messages_.end();
  for (part = local_participant_messages_.begin(); part != end; ++part) {
    write_participant_message_data(part->first, part->second, reader);
  }
  participant_message_writer_.end_historic_samples(reader);
}

DDS::ReturnCode_t
Sedp::write_publication_data(
    const RepoId& rid,
    LocalPublication& lp,
    const DCPS::RepoId& reader)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {
    DiscoveredWriterData dwd;
    ParameterList plist;
    populate_discovered_writer_msg(dwd, rid, lp);
    // Convert to parameter list
    if (ParameterListConverter::to_param_list(dwd, plist)) {
      ACE_DEBUG((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data - ")
                 ACE_TEXT("Failed to convert DiscoveredWriterData ")
                 ACE_TEXT(" to ParameterList\n")));
      result = DDS::RETCODE_ERROR;
    }
    if (DDS::RETCODE_OK == result) {
      result = publications_writer_.write_sample(plist, reader, lp.sequence_);
    }
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_publication_data - ")
                        ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}

DDS::ReturnCode_t
Sedp::write_subscription_data(
    const RepoId& rid,
    LocalSubscription& ls,
    const DCPS::RepoId& reader)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {
    DiscoveredReaderData drd;
    ParameterList plist;
    populate_discovered_reader_msg(drd, rid, ls);
    // Convert to parameter list
    if (ParameterListConverter::to_param_list(drd, plist)) {
      ACE_DEBUG((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::write_subscription_data - ")
                 ACE_TEXT("Failed to convert DiscoveredReaderData ")
                 ACE_TEXT("to ParameterList\n")));
      result = DDS::RETCODE_ERROR;
    }
    if (DDS::RETCODE_OK == result) {
      result = subscriptions_writer_.write_sample(plist, reader, ls.sequence_);
    }
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_subscription_data - ")
                        ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}

DDS::ReturnCode_t
Sedp::write_participant_message_data(
    const RepoId& rid,
    LocalParticipantMessage& pm,
    const DCPS::RepoId& reader)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {
    ParticipantMessageData pmd;
    pmd.participantGuid = rid;
    result = participant_message_writer_.write_sample(pmd, reader, pm.sequence_);
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_participant_message_data - ")
               ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}

void
Sedp::set_inline_qos(DCPS::TransportLocatorSeq& locators)
{
  const OPENDDS_STRING rtps_udp = "rtps_udp";
  for (CORBA::ULong i = 0; i < locators.length(); ++i) {
    if (locators[i].transport_type.in() == rtps_udp) {
      const CORBA::ULong len = locators[i].data.length();
      locators[i].data.length(len + 1);
      locators[i].data[len] = CORBA::Octet(1);
    }
  }
}

bool
Sedp::is_opendds(const GUID_t& endpoint)
{
  return !memcmp(endpoint.guidPrefix, DCPS::VENDORID_OCI,
                 sizeof(DCPS::VENDORID_OCI));
}

void
Sedp::acknowledge()
{
  task_.acknowledge();
}

void
Sedp::Task::enqueue(const SPDPdiscoveredParticipantData* pdata)
{
  if (spdp_->shutting_down()) { return; }
  putq(new Msg(Msg::MSG_PARTICIPANT, DCPS::SAMPLE_DATA, pdata));
}

void
Sedp::Task::enqueue(DCPS::MessageId id, const DiscoveredWriterData* wdata)
{
  if (spdp_->shutting_down()) { return; }
  putq(new Msg(Msg::MSG_WRITER, id, wdata));
}

void
Sedp::Task::enqueue(DCPS::MessageId id, const DiscoveredReaderData* rdata)
{
  if (spdp_->shutting_down()) { return; }
  putq(new Msg(Msg::MSG_READER, id, rdata));
}

void
Sedp::Task::enqueue(DCPS::MessageId id, const ParticipantMessageData* data)
{
  if (spdp_->shutting_down()) { return; }
  putq(new Msg(Msg::MSG_PARTICIPANT_DATA, id, data));
}

void
Sedp::Task::enqueue(Msg::MsgType which_bit, const DDS::InstanceHandle_t* bit_ih)
{
  if (spdp_->shutting_down()) { return; }
  putq(new Msg(which_bit, DCPS::DISPOSE_INSTANCE, bit_ih));
}

int
Sedp::Task::svc()
{
  for (Msg* msg = 0; getq(msg) != -1; /*no increment*/) {
    if (DCPS::DCPS_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::Task::svc "
        "got message from queue type %d\n", msg->type_));
    }
    ACE_Auto_Basic_Ptr<Msg> delete_the_msg(msg);
    switch (msg->type_) {
    case Msg::MSG_PARTICIPANT:
      svc_i(static_cast<const SPDPdiscoveredParticipantData*>(msg->payload_));
      break;
    case Msg::MSG_WRITER:
      svc_i(msg->id_, static_cast<const DiscoveredWriterData*>(msg->payload_));
      break;
    case Msg::MSG_READER:
      svc_i(msg->id_, static_cast<const DiscoveredReaderData*>(msg->payload_));
      break;
    case Msg::MSG_PARTICIPANT_DATA:
      svc_i(msg->id_, static_cast<const ParticipantMessageData*>(msg->payload_));
      break;
    case Msg::MSG_REMOVE_FROM_PUB_BIT:
    case Msg::MSG_REMOVE_FROM_SUB_BIT:
      svc_i(msg->type_,
            static_cast<const DDS::InstanceHandle_t*>(msg->payload_));
      break;
    case Msg::MSG_FINI_BIT:
      // acknowledge that fini_bit has been called (this just ensures that
      // this task is not in the act of using one of BIT Subscriber's Data
      // Readers while it is being deleted
      spdp_->wait_for_acks().ack();
      break;
    case Msg::MSG_STOP:
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::Task::svc - ")
                            ACE_TEXT("received MSG_STOP. Task exiting\n")));
      }
      return 0;
    }
    if (DCPS::DCPS_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::Task::svc done with message\n"));
    }
  }
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::Task::svc - ")
                        ACE_TEXT("Task exiting.\n")));
  }
  return 0;
}

Sedp::Task::~Task()
{
  shutdown();
}

WaitForAcks::WaitForAcks()
: cond_(lock_)
, acks_(0)
{
}

void
WaitForAcks::ack()
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    ++acks_;
  }
  cond_.signal();
}

void
WaitForAcks::wait_for_acks(unsigned int num_acks)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  while (num_acks > acks_) {
    cond_.wait();
  }
}

void
WaitForAcks::reset()
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  acks_ = 0;
  // no need to signal, going back to zero won't ever
  // cause wait_for_acks() to exit it's loop
}

}
}
#endif // DDS_HAS_MINIMUM_BIT
