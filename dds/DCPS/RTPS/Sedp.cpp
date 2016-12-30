/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Sedp.h"
#include "Spdp.h"
#include "MessageTypes.h"
#include "RtpsDiscovery.h"
#include "RtpsCoreTypeSupportImpl.h"
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
#include "dds/DCPS/transport/framework/NetworkAddress.h"

#include <ace/Reverse_Lock_T.h>
#include <ace/Auto_Ptr.h>

#include <cstring>

namespace {
bool qosChanged(DDS::PublicationBuiltinTopicData& dest,
                const DDS::PublicationBuiltinTopicData& src)
{
#ifndef OPENDDS_SAFETY_PROFILE
  using OpenDDS::DCPS::operator!=;
#endif
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

bool qosChanged(DDS::SubscriptionBuiltinTopicData& dest,
                const DDS::SubscriptionBuiltinTopicData& src)
{
#ifndef OPENDDS_SAFETY_PROFILE
  using OpenDDS::DCPS::operator!=;
#endif
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

bool paramsChanged(OpenDDS::DCPS::ContentFilterProperty_t& dest,
                   const OpenDDS::DCPS::ContentFilterProperty_t& src)
{
  if (dest.expressionParameters.length() != src.expressionParameters.length()) {
    dest.expressionParameters = src.expressionParameters;
    return true;
  }
  for (CORBA::ULong i = 0; i < src.expressionParameters.length(); ++i) {
    if (0 != std::strcmp(dest.expressionParameters[i],
                         src.expressionParameters[i])) {
      dest.expressionParameters = src.expressionParameters;
      return true;
    }
  }
  return false;
}

}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {
using DCPS::RepoId;
using DCPS::make_rch;

const bool Sedp::host_is_bigendian_(!ACE_CDR_BYTE_ORDER);

Sedp::Sedp(const RepoId& participant_id, Spdp& owner, ACE_Thread_Mutex& lock)
  : OpenDDS::DCPS::EndpointManager<SPDPdiscoveredParticipantData>(participant_id, lock)
  , spdp_(owner)
  , publications_writer_(make_id(participant_id,
                                 ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER),
                         *this)
  , subscriptions_writer_(make_id(participant_id,
                                  ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER),
                          *this)
  , participant_message_writer_(make_id(participant_id,
                                        ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER),
                        *this)
  , publications_reader_(make_rch<Reader>(make_id(participant_id,ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER),
                                         ref(*this)))
  , subscriptions_reader_(make_rch<Reader>(make_id(participant_id,ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER),
                                          ref(*this)))
  , participant_message_reader_(make_rch<Reader>(make_id(participant_id,ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER),
                                                ref(*this)))
  , task_(this)
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

  const OPENDDS_STRING sedp_addr = disco.sedp_local_address();
  if (!sedp_addr.empty()) {
    rtps_inst->local_address_config_str_ = sedp_addr;
    rtps_inst->local_address_.set(sedp_addr.c_str());
  }

  // Create a config
  OPENDDS_STRING config_name = DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                            OPENDDS_STRING("_SEDP_TransportCfg_") + key +
                            domainStr;
  DCPS::TransportConfig_rch transport_cfg =
    TheTransportRegistry->create_config(config_name.c_str());
  transport_cfg->instances_.push_back(transport_inst_);

  // Configure and enable each reader/writer
  rtps_inst->opendds_discovery_default_listener_ = publications_reader_;
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

void
Sedp::unicast_locators(OpenDDS::DCPS::LocatorSeq& locators) const
{
  DCPS::RtpsUdpInst_rch rtps_inst =
    DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  using namespace OpenDDS::RTPS;

  CORBA::ULong idx = 0;

  // multicast first so it's preferred by remote peers
  if (rtps_inst->use_multicast_ && rtps_inst->multicast_group_address_ != ACE_INET_Addr()) {
    idx = locators.length();
    locators.length(idx + 1);
    locators[idx].kind = address_to_kind(rtps_inst->multicast_group_address_);
    locators[idx].port = rtps_inst->multicast_group_address_.get_port_number();
    RTPS::address_to_bytes(locators[idx].address,
      rtps_inst->multicast_group_address_);
  }

  //if local_address_string is empty, or only the port has been set
  //need to get interface addresses to populate into the locator
  if (rtps_inst->local_address_config_str_.empty() ||
      rtps_inst->local_address_config_str_.rfind(':') == 0) {
    typedef OPENDDS_VECTOR(ACE_INET_Addr) AddrVector;
    AddrVector addrs;
    if (TheServiceParticipant->default_address ().empty ()) {
      OpenDDS::DCPS::get_interface_addrs(addrs);
    } else {
      addrs.push_back (ACE_INET_Addr (static_cast<u_short> (0), TheServiceParticipant->default_address ().c_str ()));
    }
    for (AddrVector::iterator adr_it = addrs.begin(); adr_it != addrs.end(); ++adr_it) {
      idx = locators.length();
      locators.length(idx + 1);
      locators[idx].kind = address_to_kind(*adr_it);
      locators[idx].port = rtps_inst->local_address_.get_port_number();
      RTPS::address_to_bytes(locators[idx].address,
        *adr_it);
    }
  } else {
    idx = locators.length();
    locators.length(idx + 1);
    locators[idx].kind = address_to_kind(rtps_inst->local_address_);
    locators[idx].port = rtps_inst->local_address_.get_port_number();
    RTPS::address_to_bytes(locators[idx].address,
      rtps_inst->local_address_);
  }
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
bool
Sedp::map_ipv4_to_ipv6() const
{
  bool map = false;
  if (local_address().get_type() != AF_INET) {
    map = true;
  }
  return map;
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
create_association_data_proto(DCPS::AssociationData& proto,
                              const SPDPdiscoveredParticipantData& pdata) {
  proto.publication_transport_priority_ = 0;
  proto.remote_reliable_ = true;
  proto.remote_durable_ = true;
  std::memcpy(proto.remote_id_.guidPrefix, pdata.participantProxy.guidPrefix,
              sizeof(GuidPrefix_t));

  const OpenDDS::DCPS::LocatorSeq& mll =
    pdata.participantProxy.metatrafficMulticastLocatorList;
  const OpenDDS::DCPS::LocatorSeq& ull =
    pdata.participantProxy.metatrafficUnicastLocatorList;
  const CORBA::ULong locator_count = mll.length() + ull.length();

  ACE_Message_Block mb_locator(4 + locator_count * sizeof(OpenDDS::DCPS::Locator_t) + 1);
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
  message_block_to_sequence (mb_locator, proto.remote_data_[0].data);
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

  for (DCPS::RepoIdSet::iterator it = sedp_->defer_match_endpoints_.begin();
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
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator ti =
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
    OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
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
Sedp::remove_from_bit_i(const DiscoveredPublication& pub)
{
#ifndef DDS_HAS_MINIMUM_BIT
  task_.enqueue(Msg::MSG_REMOVE_FROM_PUB_BIT, pub.bit_ih_);
#else
  ACE_UNUSED_ARG(pub);
#endif /* DDS_HAS_MINIMUM_BIT */
}

void
Sedp::remove_from_bit_i(const DiscoveredSubscription& sub)
{
#ifndef DDS_HAS_MINIMUM_BIT
  task_.enqueue(Msg::MSG_REMOVE_FROM_SUB_BIT, sub.bit_ih_);
#else
  ACE_UNUSED_ARG(sub);
#endif /* DDS_HAS_MINIMUM_BIT */
}

void
Sedp::Task::svc_i(Msg::MsgType which_bit, const DDS::InstanceHandle_t bit_ih)
{
#ifndef DDS_HAS_MINIMUM_BIT
  switch (which_bit) {
  case Msg::MSG_REMOVE_FROM_PUB_BIT: {
    OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = sedp_->pub_bit();
    // bit may be null if the DomainParticipant is shutting down
    if (bit && bit_ih != DDS::HANDLE_NIL) {
      bit->set_instance_state(bit_ih,
                              DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
    }
    break;
  }
  case Msg::MSG_REMOVE_FROM_SUB_BIT: {
    OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sedp_->sub_bit();
    // bit may be null if the DomainParticipant is shutting down
    if (bit && bit_ih != DDS::HANDLE_NIL) {
      bit->set_instance_state(bit_ih,
                              DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
    }
    break;
  }
  default:
    break;
  }
#else
  ACE_UNUSED_ARG(which_bit);
  ACE_UNUSED_ARG(bit_ih);
#endif /* DDS_HAS_MINIMUM_BIT */
}

#ifndef DDS_HAS_MINIMUM_BIT
OpenDDS::DCPS::TopicBuiltinTopicDataDataReaderImpl*
Sedp::topic_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_TOPIC_TOPIC);
  return dynamic_cast<OpenDDS::DCPS::TopicBuiltinTopicDataDataReaderImpl*>(d.in());
}

OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl*
Sedp::pub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_PUBLICATION_TOPIC);
  return dynamic_cast<OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl*>(d.in());
}

OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl*
Sedp::sub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  return dynamic_cast<OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl*>(d.in());
}
#endif /* DDS_HAS_MINIMUM_BIT */

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
  TopicDetails& topic = topics_[name];
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
Sedp::inconsistent_topic(const DCPS::RepoIdSet& eps) const
{
  using DCPS::RepoIdSet;
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

DDS::ReturnCode_t
Sedp::remove_publication_i(const RepoId& publicationId)
{
  return publications_writer_.write_unregister_dispose(publicationId);
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
    OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
          topics_.find(topic_name);
    if (top_it != topics_.end()) {
      match_endpoints(publicationId, top_it->second);
    }
    return true;
  }
  return false;
}

DDS::ReturnCode_t
Sedp::remove_subscription_i(const RepoId& subscriptionId)
{
  return subscriptions_writer_.write_unregister_dispose(subscriptionId);
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
    OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
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
    sb.filterProperties.expressionParameters = params;

    if (DDS::RETCODE_OK != write_subscription_data(subId, sb)) {
      return false;
    }

    // Let any associated local publications know about the change
    for (DCPS::RepoIdSet::iterator i = iter->second.matched_endpoints_.begin();
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
                  const OpenDDS::DCPS::DiscoveredWriterData* pwdata)
{
  ACE_Auto_Basic_Ptr<const OpenDDS::DCPS::DiscoveredWriterData> delete_the_data(pwdata);
  sedp_->data_received(message_id, *pwdata);
}

void
Sedp::data_received(DCPS::MessageId message_id,
                    const OpenDDS::DCPS::DiscoveredWriterData& wdata)
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
    OpenDDS::DCPS::DiscoveredWriterData wdata_copy;

    if (iter == discovered_publications_.end()) { // add new
      // Must unlock when calling into pub_bit() as it may call back into us
      ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

      { // Reduce scope of pub and td
        DiscoveredPublication& pub =
            discovered_publications_[guid] = DiscoveredPublication(wdata);

        topic_name = get_topic_name(pub);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
          topics_.find(topic_name);
        if (top_it == topics_.end()) {
          top_it =
            topics_.insert(std::make_pair(topic_name, TopicDetails())).first;
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

        TopicDetails& td = top_it->second;
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
#ifndef DDS_HAS_MINIMUM_BIT
      {
        // Release lock for call into pub_bit
        ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
        OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          instance_handle =
            bit->store_synthetic_data(wdata_copy.ddsPublicationData,
                                      DDS::NEW_VIEW_STATE);
        }
      }
#endif /* DDS_HAS_MINIMUM_BIT */

      if (spdp_.shutting_down()) { return; }
      // Publication may have been removed while lock released
      iter = discovered_publications_.find(guid);
      if (iter != discovered_publications_.end()) {
        iter->second.bit_ih_ = instance_handle;
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
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
#ifndef DDS_HAS_MINIMUM_BIT
      OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
      if (bit) { // bit may be null if the DomainParticipant is shutting down
        bit->store_synthetic_data(iter->second.writer_data_.ddsPublicationData,
                                  DDS::NOT_NEW_VIEW_STATE);
      }
#endif /* DDS_HAS_MINIMUM_BIT */

      // Match/unmatch local subscription(s)
      topic_name = get_topic_name(iter->second);
      OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
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
      OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
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
                  const OpenDDS::DCPS::DiscoveredReaderData* prdata)
{
  ACE_Auto_Basic_Ptr<const OpenDDS::DCPS::DiscoveredReaderData> delete_the_data(prdata);
  sedp_->data_received(message_id, *prdata);
}

void
Sedp::data_received(DCPS::MessageId message_id,
                    const OpenDDS::DCPS::DiscoveredReaderData& rdata)
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
    OpenDDS::DCPS::DiscoveredReaderData rdata_copy;

    if (iter == discovered_subscriptions_.end()) { // add new
      { // Reduce scope of sub and td
        DiscoveredSubscription& sub =
          discovered_subscriptions_[guid] = DiscoveredSubscription(rdata);

        topic_name = get_topic_name(sub);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
          topics_.find(topic_name);
        if (top_it == topics_.end()) {
          top_it =
            topics_.insert(std::make_pair(topic_name, TopicDetails())).first;
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

        TopicDetails& td = top_it->second;
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
#ifndef DDS_HAS_MINIMUM_BIT
      {
        // Release lock for call into sub_bit
        ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
        OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          instance_handle =
            bit->store_synthetic_data(rdata_copy.ddsSubscriptionData,
                                      DDS::NEW_VIEW_STATE);
        }
      }
#endif /* DDS_HAS_MINIMUM_BIT */

      if (spdp_.shutting_down()) { return; }
      // Subscription may have been removed while lock released
      iter = discovered_subscriptions_.find(guid);
      if (iter != discovered_subscriptions_.end()) {
        iter->second.bit_ih_ = instance_handle;
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
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
#ifndef DDS_HAS_MINIMUM_BIT
        OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          bit->store_synthetic_data(
                iter->second.reader_data_.ddsSubscriptionData,
                DDS::NOT_NEW_VIEW_STATE);
        }
#endif /* DDS_HAS_MINIMUM_BIT */

        // Match/unmatch local publication(s)
        topic_name = get_topic_name(iter->second);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
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
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
            topics_.find(topic_name);
        using DCPS::RepoIdSet;
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
      OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
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
  prefix.entityId = EntityId_t(); // Clear the entityId so lower bound will work.

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)) {
    return;
  }

  if (!spdp_.has_discovered_participant (guid_participant)) {
    return;
  }

  for (LocalSubscriptionMap::const_iterator sub_pos = local_subscriptions_.begin(),
         sub_limit = local_subscriptions_.end();
       sub_pos != sub_limit; ++sub_pos) {
    const DCPS::RepoIdSet::const_iterator pos =
      sub_pos->second.matched_endpoints_.lower_bound(prefix);
    if (pos != sub_pos->second.matched_endpoints_.end() &&
        OpenDDS::DCPS::GuidPrefixEqual()(pos->guidPrefix, prefix.guidPrefix)) {
      sub_pos->second.subscription_->signal_liveliness(guid_participant);
    }
  }
}

// helper for match(), below

void
Sedp::association_complete(const RepoId& localId,
                           const RepoId& remoteId)
{
  // If the remote endpoint is an opendds endpoint
  if (is_opendds(remoteId)) {
    LocalSubscriptionIter sub = local_subscriptions_.find(localId);
    // If the local endpoint is a reader
    if (sub != local_subscriptions_.end()) {
      std::pair<DCPS::RepoIdSet::iterator, bool> result =
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
      new DCPS::DataSampleElement(repo_id_, this, DCPS::PublicationInstance_rch(), &alloc_, 0);
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
      new DCPS::DataSampleElement(repo_id_, this, DCPS::PublicationInstance_rch(), &alloc_, 0);
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

      ACE_Auto_Ptr<OpenDDS::DCPS::DiscoveredWriterData> wdata(new OpenDDS::DCPS::DiscoveredWriterData);
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

      ACE_Auto_Ptr<OpenDDS::DCPS::DiscoveredReaderData> rdata(new OpenDDS::DCPS::DiscoveredReaderData);
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

    } else if (sample.header_.publication_id_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER
               && !sample.header_.key_fields_only_) {
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
    OpenDDS::DCPS::DiscoveredWriterData& dwd,
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
    OpenDDS::DCPS::DiscoveredReaderData& drd,
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
  drd.contentFilterProperty.filterExpression = sub.filterProperties.filterExpression;
  drd.contentFilterProperty.expressionParameters = sub.filterProperties.expressionParameters;
  for (DCPS::RepoIdSet::const_iterator writer =
        sub.remote_opendds_associations_.begin();
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
    OpenDDS::DCPS::DiscoveredWriterData dwd;
    ParameterList plist;
    populate_discovered_writer_msg(dwd, rid, lp);

    // Convert to parameter list
    if (ParameterListConverter::to_param_list(dwd, plist, map_ipv4_to_ipv6())) {
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
    OpenDDS::DCPS::DiscoveredReaderData drd;
    ParameterList plist;
    populate_discovered_reader_msg(drd, rid, ls);

    // Convert to parameter list
    if (ParameterListConverter::to_param_list(drd, plist, map_ipv4_to_ipv6())) {
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
Sedp::Task::enqueue(DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredWriterData* wdata)
{
  if (spdp_->shutting_down()) { return; }
  putq(new Msg(Msg::MSG_WRITER, id, wdata));
}

void
Sedp::Task::enqueue(DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredReaderData* rdata)
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
Sedp::Task::enqueue(Msg::MsgType which_bit, const DDS::InstanceHandle_t bit_ih)
{
#ifndef DDS_HAS_MINIMUM_BIT
  if (spdp_->shutting_down()) { return; }
  putq(new Msg(which_bit, DCPS::DISPOSE_INSTANCE, bit_ih));
#else
  ACE_UNUSED_ARG(which_bit);
  ACE_UNUSED_ARG(bit_ih);
#endif /* DDS_HAS_MINIMUM_BIT */
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
      svc_i(msg->dpdata_);
      break;
    case Msg::MSG_WRITER:
      svc_i(msg->id_, msg->wdata_);
      break;
    case Msg::MSG_READER:
      svc_i(msg->id_, msg->rdata_);
      break;
    case Msg::MSG_PARTICIPANT_DATA:
      svc_i(msg->id_, msg->pmdata_);
      break;
    case Msg::MSG_REMOVE_FROM_PUB_BIT:
    case Msg::MSG_REMOVE_FROM_SUB_BIT:
      svc_i(msg->type_, msg->ih_);
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

bool
Sedp::shutting_down() const
{
  return spdp_.shutting_down();
}

void
Sedp::populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& rTls,
                                          DiscoveredSubscriptionIter& dsi,
                                          const DCPS::RepoId& reader)
{
  OpenDDS::DCPS::LocatorSeq locs;
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
      message_block_to_sequence (mb_locator, tl.data);
      rTls->length(1);
      (*rTls)[0] = tl;
    } else {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) Sedp::match - ")
                 ACE_TEXT("remote reader found with no locators ")
                 ACE_TEXT("and no default locators\n")));
    }
  }
}

void
Sedp::populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& wTls,
                                          DiscoveredPublicationIter& /*dpi*/,
                                          const DCPS::RepoId& writer)
{
  OpenDDS::DCPS::LocatorSeq locs;
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
      message_block_to_sequence (mb_locator, tl.data);
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

bool
Sedp::defer_writer(const RepoId& writer,
                   const RepoId& writer_participant)
{
  if (!associated_participants_.count(writer_participant)) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match - ")
                 ACE_TEXT("remote writer deferred\n")));
    }
    defer_match_endpoints_.insert(writer);
    return true;
  }
  return false;
}

bool
Sedp::defer_reader(const RepoId& reader,
                   const RepoId& reader_participant)
{
  if (!associated_participants_.count(reader_participant)) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match - ")
                 ACE_TEXT("remote reader deferred\n")));
    }
    defer_match_endpoints_.insert(reader);
    return true;
  }
  return false;
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL
