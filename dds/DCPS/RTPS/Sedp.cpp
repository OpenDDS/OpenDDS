/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Sedp.h"

#include "MessageTypes.h"
#include "ParameterListConverter.h"
#include "RtpsDiscovery.h"
#include "Spdp.h"
#ifdef OPENDDS_SECURITY
#  include "SecurityHelpers.h"
#endif

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/AssociationData.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/DataSampleHeader.h>
#include <dds/DCPS/SendStateDataSampleList.h>
#include <dds/DCPS/DataReaderCallbacks.h>
#include <dds/DCPS/DataWriterCallbacks.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/Logging.h>
#include <dds/DCPS/RecorderImpl.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/DcpsUpcalls.h>
#include <dds/DCPS/Util.h>
#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/transport/framework/ReceivedDataSample.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst_rch.h>
#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/TypeAssignability.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/framework/HandleRegistry.h>
#endif

#include <dds/DdsDcpsGuidTypeSupportImpl.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DdsSecurityCoreTypeSupportImpl.h>
#endif

#include <cstring>

namespace {
bool checkAndAssignQos(DDS::PublicationBuiltinTopicData& dest,
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

bool checkAndAssignQos(DDS::SubscriptionBuiltinTopicData& dest,
                       const DDS::SubscriptionBuiltinTopicData& src)
{
#ifndef OPENDDS_SAFETY_PROFILE
  using OpenDDS::DCPS::operator!=;
#endif
  bool changed = false;

  // check each Changeable QoS policy value in Subscription BIT Data

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

bool checkAndAssignParams(OpenDDS::DCPS::ContentFilterProperty_t& dest,
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

#ifndef OPENDDS_SAFETY_PROFILE
bool operator==(const OpenDDS::DCPS::Locator_t& x,
                const OpenDDS::DCPS::Locator_t& y)
{
  return x.kind == y.kind && x.port == y.port && std::memcmp(x.address, y.address, sizeof(x.address)) == 0;
}

bool operator==(const OpenDDS::DCPS::TransportLocator& x,
                const OpenDDS::DCPS::TransportLocator& y)
{
  return x.transport_type == y.transport_type && x.data == y.data;
}
#endif

template<typename T>
bool sequence_equal(const T& x,
                    const T& y)
{
  if (x.length() != y.length()) {
    return false;
  }

  for (unsigned int idx = 0; idx != x.length(); ++idx) {
    if (!(x[idx] == y[idx])) {
      return false;
    }
  }

  return true;
}

bool operator==(const OpenDDS::DCPS::LocatorSeq& x,
                const OpenDDS::DCPS::LocatorSeq& y)
{
  return sequence_equal(x, y);
}

bool operator==(const OpenDDS::DCPS::TransportLocatorSeq& x,
                const OpenDDS::DCPS::TransportLocatorSeq& y)
{
  return sequence_equal(x, y);
}

bool locatorsChanged(const OpenDDS::RTPS::ParticipantProxy_t& x,
                     const OpenDDS::RTPS::ParticipantProxy_t& y)
{
  return !(x.metatrafficUnicastLocatorList == y.metatrafficUnicastLocatorList &&
           x.metatrafficMulticastLocatorList == y.metatrafficMulticastLocatorList &&
           x.defaultMulticastLocatorList == y.defaultMulticastLocatorList &&
           x.defaultUnicastLocatorList == y.defaultUnicastLocatorList);
}

bool checkAndAssignLocators(OpenDDS::DCPS::WriterProxy_t& x,
                            const OpenDDS::DCPS::WriterProxy_t& y)
{
  if (!(x.allLocators == y.allLocators)) {
    x.allLocators = y.allLocators;
    return true;
  }

  return false;
}

bool checkAndAssignLocators(OpenDDS::DCPS::ReaderProxy_t& x,
                            const OpenDDS::DCPS::ReaderProxy_t& y)
{
  if (!(x.allLocators == y.allLocators)) {
    x.allLocators = y.allLocators;
    return true;
  }

  return false;
}

#ifdef OPENDDS_SECURITY
bool is_stateless(const OpenDDS::DCPS::GUID_t& guid)
{
  return guid.entityId == OpenDDS::RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER ||
    guid.entityId == OpenDDS::RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
}

bool is_volatile(const OpenDDS::DCPS::GUID_t& guid)
{
  return guid.entityId == OpenDDS::RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER ||
    guid.entityId == OpenDDS::RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
}

bool is_stateless_or_volatile(const OpenDDS::DCPS::GUID_t& guid)
{
  return is_stateless(guid) || is_volatile(guid);
}
#endif

}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {
using DCPS::GUID_t;
using DCPS::RepoId;
using DCPS::make_rch;
using DCPS::TimeDuration;
using DCPS::SystemTimePoint;
using DCPS::Serializer;
using DCPS::Encoding;
using DCPS::TopicDetails;
using DCPS::TopicDetailsMap;

namespace {
  const Encoding sedp_encoding(Encoding::KIND_XCDR1, DCPS::ENDIAN_LITTLE);
  const Encoding type_lookup_encoding(Encoding::KIND_XCDR2, DCPS::ENDIAN_NATIVE);
}

Sedp::Sedp(const RepoId& participant_id, Spdp& owner, ACE_Thread_Mutex& lock)
  : spdp_(owner)
  , lock_(lock)
  , participant_id_(participant_id)
  , publication_counter_(0)
  , subscription_counter_(0)
  , topic_counter_(0)
  , max_type_lookup_service_reply_period_(0)
  , type_lookup_service_sequence_number_(0)
  , use_xtypes_(true)
  , use_xtypes_complete_(false)
#ifdef OPENDDS_SECURITY
  , permissions_handle_(DDS::HANDLE_NIL)
  , crypto_handle_(DDS::HANDLE_NIL)
#endif
  , publications_writer_(make_rch<DiscoveryWriter>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER), ref(*this)))
#ifdef OPENDDS_SECURITY
  , publications_secure_writer_(make_rch<DiscoveryWriter>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER), ref(*this)))
#endif
  , subscriptions_writer_(make_rch<DiscoveryWriter>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER), ref(*this)))
#ifdef OPENDDS_SECURITY
  , subscriptions_secure_writer_(make_rch<DiscoveryWriter>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER), ref(*this)))
#endif
  , participant_message_writer_(make_rch<LivelinessWriter>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER), ref(*this)))
#ifdef OPENDDS_SECURITY
  , participant_message_secure_writer_(make_rch<LivelinessWriter>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER), ref(*this)))
  , participant_stateless_message_writer_(make_rch<SecurityWriter>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER), ref(*this)))
  , dcps_participant_secure_writer_(make_rch<DiscoveryWriter>(
      make_id(participant_id, ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER), ref(*this), 2))
  , participant_volatile_message_secure_writer_(make_rch<SecurityWriter>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER), ref(*this)))
#endif
  , type_lookup_request_writer_(make_rch<TypeLookupRequestWriter>(
      make_id(participant_id, ENTITYID_TL_SVC_REQ_WRITER), ref(*this)))
  , type_lookup_reply_writer_(make_rch<TypeLookupReplyWriter>(
      make_id(participant_id, ENTITYID_TL_SVC_REPLY_WRITER), ref(*this)))
#ifdef OPENDDS_SECURITY
  , type_lookup_request_secure_writer_(make_rch<TypeLookupRequestWriter>(
      make_id(participant_id, ENTITYID_TL_SVC_REQ_WRITER_SECURE), ref(*this)))
  , type_lookup_reply_secure_writer_(make_rch<TypeLookupReplyWriter>(
      make_id(participant_id, ENTITYID_TL_SVC_REPLY_WRITER_SECURE), ref(*this)))
#endif
  , publications_reader_(make_rch<DiscoveryReader>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER), ref(*this)))
#ifdef OPENDDS_SECURITY
  , publications_secure_reader_(make_rch<DiscoveryReader>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER), ref(*this)))
#endif
  , subscriptions_reader_(make_rch<DiscoveryReader>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER), ref(*this)))
#ifdef OPENDDS_SECURITY
  , subscriptions_secure_reader_(make_rch<DiscoveryReader>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER), ref(*this)))
#endif
  , participant_message_reader_(make_rch<LivelinessReader>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER), ref(*this)))
#ifdef OPENDDS_SECURITY
  , participant_message_secure_reader_(make_rch<LivelinessReader>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER), ref(*this)))
  , participant_stateless_message_reader_(make_rch<SecurityReader>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER), ref(*this)))
  , participant_volatile_message_secure_reader_(make_rch<SecurityReader>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER), ref(*this)))
  , dcps_participant_secure_reader_(make_rch<DiscoveryReader>(
      make_id(participant_id, ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER), ref(*this)))
#endif
  , type_lookup_request_reader_(make_rch<TypeLookupRequestReader>(
      make_id(participant_id, ENTITYID_TL_SVC_REQ_READER), ref(*this)))
  , type_lookup_reply_reader_(make_rch<TypeLookupReplyReader>(
      make_id(participant_id, ENTITYID_TL_SVC_REPLY_READER), ref(*this)))
#ifdef OPENDDS_SECURITY
  , type_lookup_request_secure_reader_(make_rch<TypeLookupRequestReader>(
      make_id(participant_id, ENTITYID_TL_SVC_REQ_READER_SECURE), ref(*this)))
  , type_lookup_reply_secure_reader_(make_rch<TypeLookupReplyReader>(
      make_id(participant_id, ENTITYID_TL_SVC_REPLY_READER_SECURE), ref(*this)))
  , ice_agent_(ICE::Agent::instance())
  , publication_agent_info_listener_(DCPS::make_rch<PublicationAgentInfoListener>(ref(*this)))
  , subscription_agent_info_listener_(DCPS::make_rch<SubscriptionAgentInfoListener>(ref(*this)))
#endif // OPENDDS_SECURITY
{}

DDS::ReturnCode_t
Sedp::init(const RepoId& guid,
           const RtpsDiscovery& disco,
           DDS::DomainId_t domainId,
           XTypes::TypeLookupService_rch tls)
{
  type_lookup_service_ = tls;

  const OPENDDS_STRING domainStr = DCPS::to_dds_string(domainId);
  const OPENDDS_STRING key = DCPS::GuidConverter(guid).uniqueParticipantId();

  // configure one transport
  transport_inst_ = TheTransportRegistry->create_inst(
                       DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                       OPENDDS_STRING("_SEDPTransportInst_") + key + domainStr,
                       "rtps_udp");

  // Be careful to not call any function that causes the transport be
  // to created before the configuration is complete.

  // Use a static cast to avoid dependency on the RtpsUdp library
  DCPS::RtpsUdpInst_rch rtps_inst =
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  rtps_inst->max_message_size_ = disco.config()->sedp_max_message_size();
  rtps_inst->heartbeat_period_ = disco.config()->sedp_heartbeat_period();
  rtps_inst->nak_response_delay_ = disco.config()->sedp_nak_response_delay();
  rtps_inst->responsive_mode_ = disco.config()->sedp_responsive_mode();
  rtps_inst->send_delay_ = disco.config()->sedp_send_delay();
  rtps_inst->send_buffer_size_ = disco.config()->send_buffer_size();
  rtps_inst->rcv_buffer_size_ = disco.config()->recv_buffer_size();
  rtps_inst->receive_preallocated_message_blocks_ = disco.config()->sedp_receive_preallocated_message_blocks();
  rtps_inst->receive_preallocated_data_blocks_ = disco.config()->sedp_receive_preallocated_data_blocks();

  if (disco.sedp_multicast()) {
    // Bind to a specific multicast group
    const u_short mc_port = disco.pb() + disco.dg() * domainId + disco.dx();

    ACE_INET_Addr mc_addr = disco.default_multicast_group();
    mc_addr.set_port_number(mc_port);
    rtps_inst->multicast_group_address_ = mc_addr;

    rtps_inst->ttl_ = disco.ttl();
    rtps_inst->multicast_interface_ = disco.multicast_interface();

  } else {
    rtps_inst->use_multicast_ = false;
  }

  rtps_inst->local_address_ = disco.config()->sedp_local_address();
  rtps_inst->advertised_address_ = disco.config()->sedp_advertised_address();
#ifdef ACE_HAS_IPV6
  rtps_inst->ipv6_local_address_ = disco.config()->ipv6_sedp_local_address();
  rtps_inst->ipv6_advertised_address_ = disco.config()->ipv6_sedp_advertised_address();
#endif

  if (!disco.config()->sedp_fragment_reassembly_timeout().is_zero()) {
    rtps_inst->fragment_reassembly_timeout_ = disco.config()->sedp_fragment_reassembly_timeout();
  }

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, rtps_inst->config_lock_, DDS::RETCODE_PRECONDITION_NOT_MET);
    rtps_inst->rtps_relay_address_ = disco.config()->sedp_rtps_relay_address();
    rtps_inst->use_rtps_relay_ = disco.config()->use_rtps_relay();
    rtps_inst->rtps_relay_only_ = disco.config()->rtps_relay_only();
    rtps_inst->stun_server_address_ = disco.config()->sedp_stun_server_address();
    rtps_inst->use_ice_ = disco.config()->use_ice();
  }

  // Create a config
  OPENDDS_STRING config_name = DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                            OPENDDS_STRING("_SEDP_TransportCfg_") + key +
                            domainStr;
  transport_cfg_ = TheTransportRegistry->create_config(config_name.c_str());
  transport_cfg_->instances_.push_back(transport_inst_);
  transport_cfg_->passive_connect_duration_ =
    static_cast<unsigned long>(disco.config()->sedp_passive_connect_duration().value().get_msec());

  rtps_inst->opendds_discovery_default_listener_ = publications_reader_;
  rtps_inst->opendds_discovery_guid_ = guid;

  const_cast<bool&>(use_xtypes_) = disco.use_xtypes();
  const_cast<bool&>(use_xtypes_complete_) = disco.use_xtypes_complete();

  reactor_task_ = transport_inst_->reactor_task();
  // One should assume that the transport is configured after this
  // point.  Changes to transport_inst_ or rtps_inst after this line
  // may not be reflected.
  ACE_Reactor* reactor = reactor_task_->get_reactor();
  job_queue_ = DCPS::make_rch<DCPS::JobQueue>(reactor);
  type_lookup_init(reactor_task_->interceptor());

  // Configure and enable each reader/writer
  const bool reliable = true;
  const bool durable = true;
  const bool nondurable = false;

#ifdef OPENDDS_SECURITY
  const bool besteffort = false;
#endif

  const BuiltinEndpointSet_t bep = spdp_.available_builtin_endpoints();
#ifdef OPENDDS_SECURITY
  const DDS::Security::ExtendedBuiltinEndpointSet_t xbep =
    spdp_.available_extended_builtin_endpoints();
#endif

  if (bep & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    publications_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  publications_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);

#ifdef OPENDDS_SECURITY
  publications_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
  publications_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
  if (bep & DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    publications_secure_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  publications_secure_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);
#endif

  if (bep & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    subscriptions_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  subscriptions_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);

#ifdef OPENDDS_SECURITY
  subscriptions_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
  subscriptions_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
  if (bep & DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    subscriptions_secure_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  subscriptions_secure_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);
#endif

  if (bep & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
    participant_message_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  participant_message_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);

#ifdef OPENDDS_SECURITY
  participant_message_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
  participant_message_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
  if (bep & DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
    participant_message_secure_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  participant_message_secure_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);

  participant_stateless_message_writer_->enable_transport_using_config(besteffort, nondurable, transport_cfg_);
  participant_stateless_message_reader_->enable_transport_using_config(besteffort, nondurable, transport_cfg_);

  participant_volatile_message_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
  participant_volatile_message_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
  participant_volatile_message_secure_writer_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  participant_volatile_message_secure_reader_->enable_transport_using_config(reliable, nondurable, transport_cfg_);

  dcps_participant_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
  dcps_participant_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
  dcps_participant_secure_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  dcps_participant_secure_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);
#endif

  if (bep & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_WRITER) {
    type_lookup_request_writer_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  }
  if (bep & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_READER) {
    type_lookup_request_reader_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  }

  if (bep & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_WRITER) {
    type_lookup_reply_writer_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  }
  if (bep & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER) {
    type_lookup_reply_reader_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  }

#ifdef OPENDDS_SECURITY
  if (xbep & DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE) {
    type_lookup_request_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
    type_lookup_request_secure_writer_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  }
  if (xbep & DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE) {
    type_lookup_request_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
    type_lookup_request_secure_reader_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  }

  if (xbep & DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE) {
    type_lookup_reply_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
    type_lookup_reply_secure_writer_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  }
  if (xbep & DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE) {
    type_lookup_reply_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
    type_lookup_reply_secure_reader_->enable_transport_using_config(reliable, nondurable, transport_cfg_);
  }
#endif

  max_type_lookup_service_reply_period_ = disco.config()->max_type_lookup_service_reply_period();

  return DDS::RETCODE_OK;
}

#ifdef OPENDDS_SECURITY
DDS::ReturnCode_t Sedp::init_security(DDS::Security::IdentityHandle /* id_handle */,
                                      DDS::Security::PermissionsHandle perm_handle,
                                      DDS::Security::ParticipantCryptoHandle crypto_handle)
{
  using namespace OpenDDS::Security;
  using namespace DDS::Security;

  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();
  CryptoKeyExchange_var key_exchange = spdp_.get_security_config()->get_crypto_key_exchange();
  AccessControl_var acl = spdp_.get_security_config()->get_access_control();
  Authentication_var auth = spdp_.get_security_config()->get_authentication();
  HandleRegistry_rch handle_registry = spdp_.get_security_config()->get_handle_registry(participant_id_);

  set_permissions_handle(perm_handle);
  set_access_control(acl);
  set_crypto_key_factory(key_factory);
  set_crypto_key_exchange(key_exchange);
  set_handle_registry(handle_registry);
  crypto_handle_ = crypto_handle;

  // TODO: Handle all exceptions below once error-codes have been defined, etc.
  SecurityException ex = {"", 0, 0};

  bool ok = acl->get_participant_sec_attributes(perm_handle, participant_sec_attr_, ex);
  if (ok) {

    const EndpointSecurityAttributes default_sec_attr = get_handle_registry()->default_endpoint_security_attributes();

    NativeCryptoHandle h = DDS::HANDLE_NIL;

    const DDS::PartitionQosPolicy& default_part_qos = TheServiceParticipant->initial_PartitionQosPolicy();
    const DDS::Security::DataTagQosPolicy default_data_tag_qos; // default is empty sequence

    // Volatile-Message-Secure
    {
      PropertySeq writer_props(1), reader_props(1);
      writer_props.length(1);
      writer_props[0].name = "dds.sec.builtin_endpoint_name";
      writer_props[0].value = "BuiltinParticipantVolatileMessageSecureWriter";

      reader_props.length(1);
      reader_props[0].name = "dds.sec.builtin_endpoint_name";
      reader_props[0].value = "BuiltinParticipantVolatileMessageSecureReader";

      EndpointSecurityAttributes dw_sec_attr(default_sec_attr);

      ok = acl->get_datawriter_sec_attributes(perm_handle, "DCPSParticipantVolatileMessageSecure",
                                              default_part_qos, default_data_tag_qos, dw_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datawriter_sec_attributes for topic 'DCPSParticipantVolatileMessageSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datawriter(crypto_handle, writer_props, dw_sec_attr, ex);
      participant_volatile_message_secure_writer_->set_crypto_handles(crypto_handle, h);
      const RepoId pvms_writer = participant_volatile_message_secure_writer_->get_repo_id();
      get_handle_registry()->insert_local_datawriter_crypto_handle(pvms_writer, h, dw_sec_attr);

      EndpointSecurityAttributes dr_sec_attr(default_sec_attr);
      ok = acl->get_datareader_sec_attributes(perm_handle, "DCPSParticipantVolatileMessageSecure",
                                              default_part_qos, default_data_tag_qos, dr_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datareader_sec_attributes for topic 'DCPSParticipantVolatileMessageSecure'.")
          ACE_TEXT(" Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datareader(crypto_handle, reader_props, dr_sec_attr, ex);
      participant_volatile_message_secure_reader_->set_crypto_handles(crypto_handle, h);
      const RepoId pvms_reader = participant_volatile_message_secure_reader_->get_repo_id();
      get_handle_registry()->insert_local_datareader_crypto_handle(pvms_reader, h, dr_sec_attr);
    }

    // DCPS-Participant-Message-Secure
    {
      PropertySeq reader_props, writer_props;

      EndpointSecurityAttributes dw_sec_attr(default_sec_attr);
      ok = acl->get_datawriter_sec_attributes(perm_handle, "DCPSParticipantMessageSecure",
                                              default_part_qos, default_data_tag_qos, dw_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datawriter_sec_attributes for topic 'DCPSParticipantMessageSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datawriter(crypto_handle, writer_props, dw_sec_attr, ex);
      participant_message_secure_writer_->set_crypto_handles(crypto_handle, h);
      const RepoId pms_writer = participant_message_secure_writer_->get_repo_id();
      get_handle_registry()->insert_local_datawriter_crypto_handle(pms_writer, h, dw_sec_attr);

      EndpointSecurityAttributes dr_sec_attr(default_sec_attr);
      ok = acl->get_datareader_sec_attributes(perm_handle, "DCPSParticipantMessageSecure",
                                              default_part_qos, default_data_tag_qos, dr_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datareader_sec_attributes for topic 'DCPSParticipantMessageSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datareader(crypto_handle, reader_props, dr_sec_attr, ex);
      participant_message_secure_reader_->set_crypto_handles(crypto_handle, h);
      const RepoId pms_reader = participant_message_secure_reader_->get_repo_id();
      get_handle_registry()->insert_local_datareader_crypto_handle(pms_reader, h, dr_sec_attr);
    }

    // DCPS-Publications-Secure
    {
      PropertySeq reader_props, writer_props;

      EndpointSecurityAttributes dw_sec_attr(default_sec_attr);
      ok = acl->get_datawriter_sec_attributes(perm_handle, "DCPSPublicationsSecure",
                                              default_part_qos, default_data_tag_qos, dw_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datawriter_sec_attributes for topic 'DCPSPublicationsSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datawriter(crypto_handle, writer_props, dw_sec_attr, ex);
      publications_secure_writer_->set_crypto_handles(crypto_handle, h);
      const RepoId ps_writer = publications_secure_writer_->get_repo_id();
      get_handle_registry()->insert_local_datawriter_crypto_handle(ps_writer, h, dw_sec_attr);

      EndpointSecurityAttributes dr_sec_attr(default_sec_attr);
      ok = acl->get_datareader_sec_attributes(perm_handle, "DCPSPublicationsSecure",
                                              default_part_qos, default_data_tag_qos, dr_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datareader_sec_attributes for topic 'DCPSPublicationsSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datareader(crypto_handle, reader_props, dr_sec_attr, ex);
      publications_secure_reader_->set_crypto_handles(crypto_handle, h);
      const RepoId ps_reader = publications_secure_reader_->get_repo_id();
      get_handle_registry()->insert_local_datareader_crypto_handle(ps_reader, h, dr_sec_attr);
    }

    // DCPS-Subscriptions-Secure
    {
      PropertySeq reader_props, writer_props;

      EndpointSecurityAttributes dw_sec_attr(default_sec_attr);
      ok = acl->get_datawriter_sec_attributes(perm_handle, "DCPSSubscriptionsSecure",
                                              default_part_qos, default_data_tag_qos, dw_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datawriter_sec_attributes for topic 'DCPSSubscriptionsSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datawriter(crypto_handle, writer_props, dw_sec_attr, ex);
      subscriptions_secure_writer_->set_crypto_handles(crypto_handle, h);
      const RepoId ss_writer = subscriptions_secure_writer_->get_repo_id();
      get_handle_registry()->insert_local_datawriter_crypto_handle(ss_writer, h, dw_sec_attr);

      EndpointSecurityAttributes dr_sec_attr(default_sec_attr);
      ok = acl->get_datareader_sec_attributes(perm_handle, "DCPSSubscriptionsSecure",
                                              default_part_qos, default_data_tag_qos, dr_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datareader_sec_attributes for topic 'DCPSSubscriptionsSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datareader(crypto_handle, reader_props, dr_sec_attr, ex);
      subscriptions_secure_reader_->set_crypto_handles(crypto_handle, h);
      const RepoId ss_reader = subscriptions_secure_reader_->get_repo_id();
      get_handle_registry()->insert_local_datareader_crypto_handle(ss_reader, h, dr_sec_attr);
    }

    // DCPS-Participants-Secure
    {
      PropertySeq reader_props, writer_props;

      EndpointSecurityAttributes dw_sec_attr(default_sec_attr);
      ok = acl->get_datawriter_sec_attributes(perm_handle, "DCPSParticipantSecure",
                                              default_part_qos, default_data_tag_qos, dw_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datawriter_sec_attributes for topic 'DCPSParticipantSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datawriter(crypto_handle, writer_props, dw_sec_attr, ex);
      dcps_participant_secure_writer_->set_crypto_handles(crypto_handle, h);
      const RepoId dps_writer = dcps_participant_secure_writer_->get_repo_id();
      get_handle_registry()->insert_local_datawriter_crypto_handle(dps_writer, h, dw_sec_attr);

      EndpointSecurityAttributes dr_sec_attr(default_sec_attr);
      ok = acl->get_datareader_sec_attributes(perm_handle, "DCPSParticipantSecure",
                                              default_part_qos, default_data_tag_qos, dr_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datareader_sec_attributes for topic 'DCPSParticipantSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datareader(crypto_handle, reader_props, dr_sec_attr, ex);
      dcps_participant_secure_reader_->set_crypto_handles(crypto_handle, h);
      const RepoId dps_reader = dcps_participant_secure_reader_->get_repo_id();
      get_handle_registry()->insert_local_datareader_crypto_handle(dps_reader, h, dr_sec_attr);
    }

    // Type Lookup Service Request
    {
      PropertySeq reader_props, writer_props;

      EndpointSecurityAttributes dw_sec_attr(default_sec_attr);
      ok = acl->get_datawriter_sec_attributes(perm_handle, "TypeLookupServiceRequestSecure",
                                              default_part_qos, default_data_tag_qos, dw_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datawriter_sec_attributes for topic 'TypeLookupServiceRequestSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datawriter(crypto_handle, writer_props, dw_sec_attr, ex);
      type_lookup_request_secure_writer_->set_crypto_handles(crypto_handle, h);
      const RepoId ps_writer = type_lookup_request_secure_writer_->get_repo_id();
      get_handle_registry()->insert_local_datawriter_crypto_handle(ps_writer, h, dw_sec_attr);

      EndpointSecurityAttributes dr_sec_attr(default_sec_attr);
      ok = acl->get_datareader_sec_attributes(perm_handle, "TypeLookupServiceRequestSecure",
                                              default_part_qos, default_data_tag_qos, dr_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datareader_sec_attributes for topic 'TypeLookupServiceRequestSecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datareader(crypto_handle, reader_props, dr_sec_attr, ex);
      type_lookup_request_secure_reader_->set_crypto_handles(crypto_handle, h);
      const RepoId ps_reader = type_lookup_request_secure_reader_->get_repo_id();
      get_handle_registry()->insert_local_datareader_crypto_handle(ps_reader, h, dr_sec_attr);
    }

    // Type Lookup Service Reply
    {
      PropertySeq reader_props, writer_props;

      EndpointSecurityAttributes dw_sec_attr(default_sec_attr);
      ok = acl->get_datawriter_sec_attributes(perm_handle, "TypeLookupServiceReplySecure",
                                              default_part_qos, default_data_tag_qos, dw_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datawriter_sec_attributes for topic 'TypeLookupServiceReplySecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datawriter(crypto_handle, writer_props, dw_sec_attr, ex);
      type_lookup_reply_secure_writer_->set_crypto_handles(crypto_handle, h);
      const RepoId ps_writer = type_lookup_reply_secure_writer_->get_repo_id();
      get_handle_registry()->insert_local_datawriter_crypto_handle(ps_writer, h, dw_sec_attr);

      EndpointSecurityAttributes dr_sec_attr(default_sec_attr);
      ok = acl->get_datareader_sec_attributes(perm_handle, "TypeLookupServiceReplySecure",
                                              default_part_qos, default_data_tag_qos, dr_sec_attr, ex);
      if (!ok) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
          ACE_TEXT("Failure calling get_datareader_sec_attributes for topic 'TypeLookupServiceReplySecure'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
        result = DDS::RETCODE_ERROR;
      }

      h = key_factory->register_local_datareader(crypto_handle, reader_props, dr_sec_attr, ex);
      type_lookup_reply_secure_reader_->set_crypto_handles(crypto_handle, h);
      const RepoId ps_reader = type_lookup_reply_secure_reader_->get_repo_id();
      get_handle_registry()->insert_local_datareader_crypto_handle(ps_reader, h, dr_sec_attr);
    }

  } else {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::init_security() - ")
      ACE_TEXT("Failure calling get_participant_sec_attributes. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"), ex.code, ex.minor_code, ex.message.in()));
    result = DDS::RETCODE_ERROR;
  }
  return result;
}
#endif

Sedp::~Sedp()
{
  type_lookup_fini();

#ifdef OPENDDS_SECURITY
  using namespace OpenDDS::Security;
  using namespace DDS::Security;

  SecurityException ex = {"", 0, 0};

  cleanup_secure_writer(participant_volatile_message_secure_writer_->get_repo_id());
  cleanup_secure_reader(participant_volatile_message_secure_reader_->get_repo_id());
  cleanup_secure_writer(participant_message_secure_writer_->get_repo_id());
  cleanup_secure_reader(participant_message_secure_reader_->get_repo_id());
  cleanup_secure_writer(publications_secure_writer_->get_repo_id());
  cleanup_secure_reader(publications_secure_reader_->get_repo_id());
  cleanup_secure_writer(subscriptions_secure_writer_->get_repo_id());
  cleanup_secure_reader(subscriptions_secure_reader_->get_repo_id());
  cleanup_secure_writer(dcps_participant_secure_writer_->get_repo_id());
  cleanup_secure_reader(dcps_participant_secure_reader_->get_repo_id());
  if (spdp_.get_security_config()) {
    spdp_.get_security_config()->erase_handle_registry(participant_id_);
  }
#endif

  job_queue_.reset();
  reactor_task_.reset();
  DCPS::RtpsUdpInst_rch rtps_inst =
    DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  rtps_inst->opendds_discovery_default_listener_.reset();
  TheTransportRegistry->remove_config(transport_cfg_);
  TheTransportRegistry->remove_inst(transport_inst_);
}

DCPS::LocatorSeq
Sedp::unicast_locators() const
{
  DCPS::TransportLocator trans_info;
  transport_inst_->populate_locator(trans_info, DCPS::CONNINFO_UNICAST);
  return transport_locator_to_locator_seq(trans_info);
}

DCPS::LocatorSeq
Sedp::multicast_locators() const
{
  DCPS::TransportLocator trans_info;
  transport_inst_->populate_locator(trans_info, DCPS::CONNINFO_MULTICAST);
  return transport_locator_to_locator_seq(trans_info);
}

const DCPS::NetworkAddress&
Sedp::local_address() const
{
  DCPS::RtpsUdpInst_rch rtps_inst =
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  return rtps_inst->local_address_;
}

#ifdef ACE_HAS_IPV6
const DCPS::NetworkAddress&
Sedp::ipv6_local_address() const
{
  DCPS::RtpsUdpInst_rch rtps_inst =
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  return rtps_inst->ipv6_local_address_;
}
#endif

const DCPS::NetworkAddress&
Sedp::multicast_group() const
{
  DCPS::RtpsUdpInst_rch rtps_inst =
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  return rtps_inst->multicast_group_address_;
}

void
Sedp::assign_bit_key(DiscoveredPublication& pub)
{
  const DDS::BuiltinTopicKey_t key = repo_id_to_bit_key(pub.writer_data_.writerProxy.remoteWriterGuid);
  pub.writer_data_.ddsPublicationData.key = key;
  pub.writer_data_.ddsPublicationData.participant_key = repo_id_to_bit_key(make_id(pub.writer_data_.writerProxy.remoteWriterGuid, ENTITYID_PARTICIPANT));
}

void
Sedp::assign_bit_key(DiscoveredSubscription& sub)
{
  const DDS::BuiltinTopicKey_t key = repo_id_to_bit_key(sub.reader_data_.readerProxy.remoteReaderGuid);
  sub.reader_data_.ddsSubscriptionData.key = key;
  sub.reader_data_.ddsSubscriptionData.participant_key = repo_id_to_bit_key(make_id(sub.reader_data_.readerProxy.remoteReaderGuid, ENTITYID_PARTICIPANT));
}

void
populate_locators(DCPS::TransportLocatorSeq& remote_data,
                  const ParticipantData_t& pdata)
{
  const DCPS::LocatorSeq& mll =
    pdata.participantProxy.metatrafficMulticastLocatorList;
  const DCPS::LocatorSeq& ull =
    pdata.participantProxy.metatrafficUnicastLocatorList;
  const CORBA::ULong locator_count = mll.length() + ull.length();

  const Encoding& encoding = get_locators_encoding();
  ACE_Message_Block mb_locator(
    DCPS::uint32_cdr_size +
    (locator_count * serialized_size(encoding, DCPS::Locator_t())) + DCPS::boolean_cdr_size);
  Serializer ser_loc(&mb_locator, encoding);
  ser_loc << locator_count;

  for (CORBA::ULong i = 0; i < mll.length(); ++i) {
    ser_loc << mll[i];
  }
  for (CORBA::ULong i = 0; i < ull.length(); ++i) {
    ser_loc << ull[i];
  }
  ser_loc << ACE_OutputCDR::from_boolean(false); // requires_inline_qos

  remote_data.length(1);
  remote_data[0].transport_type = "rtps_udp";
  message_block_to_sequence (mb_locator, remote_data[0].data);
}

void
create_association_data_proto(DCPS::AssociationData& proto,
                              const ParticipantData_t& pdata) {
  proto.publication_transport_priority_ = 0;
  proto.remote_reliable_ = true;
  proto.remote_durable_ = true;
  DCPS::assign(proto.remote_id_.guidPrefix, pdata.participantProxy.guidPrefix);
  proto.participant_discovered_at_ = pdata.discoveredAt;
  proto.remote_transport_context_ = pdata.participantProxy.opendds_participant_flags.bits;
  populate_locators(proto.remote_data_, pdata);
}

#ifdef OPENDDS_SECURITY
void Sedp::generate_remote_matched_crypto_handle(const BuiltinAssociationRecord& record)
{
  if (DCPS::GuidConverter(record.remote_id()).isWriter()) {
    generate_remote_matched_writer_crypto_handle(record.remote_id(), record.local_id());
  } else {
    generate_remote_matched_reader_crypto_handle(record.remote_id(), record.local_id(), false);
  }
}
#endif

bool Sedp::ready(const DiscoveredParticipant& participant,
                 const GUID_t& local_id,
                 const GUID_t& remote_id,
                 bool local_tokens_sent) const
{
#ifndef OPENDDS_SECURITY
  ACE_UNUSED_ARG(participant);
  ACE_UNUSED_ARG(local_tokens_sent);
#endif

  return remote_knows_about_local_i(local_id, remote_id)
#ifdef OPENDDS_SECURITY
    && remote_is_authenticated_i(local_id, remote_id, participant)
    && local_has_remote_participant_token_i(local_id, remote_id)
    && remote_has_local_participant_token_i(local_id, remote_id, participant)
    && local_has_remote_endpoint_token_i(local_id, remote_id)
    && remote_has_local_endpoint_token_i(local_id, local_tokens_sent, remote_id)
#endif
    ;
}

void
Sedp::associate(DiscoveredParticipant& participant
#ifdef OPENDDS_SECURITY
                , const DDS::Security::ParticipantSecurityAttributes& participant_sec_attr
#endif
                )
{
  const BuiltinEndpointSet_t local_available = spdp_.available_builtin_endpoints();
  const BuiltinEndpointSet_t remote_available = participant.pdata_.participantProxy.availableBuiltinEndpoints;
  const BuiltinEndpointQos_t& beq = participant.pdata_.participantProxy.builtinEndpointQos;

  // See RTPS v2.1 section 8.5.5.1
  if ((local_available & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) &&
      (remote_available & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER)) {
    BuiltinAssociationRecord record(publications_reader_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER),
                                    AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE);
    participant.builtin_pending_records_.push_back(record);
  }
  if ((local_available & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) &&
      (remote_available & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER)) {
    BuiltinAssociationRecord record(subscriptions_reader_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER),
                                    AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE);
    participant.builtin_pending_records_.push_back(record);
  }
  if ((local_available & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) &&
      (remote_available & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER)) {
    BuiltinAssociationRecord record(participant_message_reader_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER),
                                    AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE);
    participant.builtin_pending_records_.push_back(record);
  }

  if ((local_available & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_READER) &&
      (remote_available & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_WRITER)) {
    BuiltinAssociationRecord record(type_lookup_request_reader_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_TL_SVC_REQ_WRITER),
                                    AC_REMOTE_RELIABLE);
    participant.builtin_pending_records_.push_back(record);
  }
  if ((local_available & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_WRITER) &&
      (remote_available & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_READER)) {
    BuiltinAssociationRecord record(type_lookup_request_writer_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_TL_SVC_REQ_READER),
                                    AC_REMOTE_RELIABLE);
    participant.builtin_pending_records_.push_back(record);
  }
  if ((local_available & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER) &&
      (remote_available & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_WRITER)) {
    BuiltinAssociationRecord record(type_lookup_reply_reader_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_TL_SVC_REPLY_WRITER),
                                    AC_REMOTE_RELIABLE);
    participant.builtin_pending_records_.push_back(record);
  }
  if ((local_available & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_WRITER) &&
      (remote_available & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER)) {
    BuiltinAssociationRecord record(type_lookup_reply_writer_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_TL_SVC_REPLY_READER),
                                    AC_REMOTE_RELIABLE);
    participant.builtin_pending_records_.push_back(record);
  }

  if ((local_available & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) &&
      (remote_available & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR)) {
    BuiltinAssociationRecord record(publications_writer_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER),
                                    AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE);
    participant.builtin_pending_records_.push_back(record);
  }
  if ((local_available & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) &&
      (remote_available & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR)) {
    BuiltinAssociationRecord record(subscriptions_writer_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER),
                                    AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE);
    participant.builtin_pending_records_.push_back(record);
  }
  if ((local_available & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) &&
      (remote_available & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER)) {
    BuiltinAssociationRecord record(participant_message_writer_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER),
                                    ((beq & BEST_EFFORT_PARTICIPANT_MESSAGE_DATA_READER) ? AC_EMPTY : AC_REMOTE_RELIABLE) | AC_REMOTE_DURABLE);
    participant.builtin_pending_records_.push_back(record);
  }

  //FUTURE: if/when topic propagation is supported, add it here

#ifdef OPENDDS_SECURITY
  if (spdp_.is_security_enabled()) {
    using namespace DDS::Security;

    const ExtendedBuiltinEndpointSet_t local_available_extended = spdp_.available_extended_builtin_endpoints();
    const ExtendedBuiltinEndpointSet_t remote_available_extended = participant.pdata_.participantProxy.availableExtendedBuiltinEndpoints;

    if ((local_available & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER) &&
        (remote_available & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER)) {
      BuiltinAssociationRecord record(participant_stateless_message_reader_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER),
                                      AC_EMPTY);
      participant.builtin_pending_records_.push_back(record);
    }

    if ((local_available & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) &&
        (remote_available & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER)) {
      BuiltinAssociationRecord record(participant_stateless_message_writer_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER),
                                      AC_EMPTY);
      participant.builtin_pending_records_.push_back(record);
    }

    if ((local_available & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) &&
        (remote_available & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER)) {
      BuiltinAssociationRecord record(participant_volatile_message_secure_reader_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER),
                                      AC_REMOTE_RELIABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE);
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) &&
        (remote_available & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER)) {
      BuiltinAssociationRecord record(participant_volatile_message_secure_writer_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER),
                                      AC_REMOTE_RELIABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE);
      participant.builtin_pending_records_.push_back(record);
    }

    if ((local_available & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) &&
        (remote_available & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER)) {
      BuiltinAssociationRecord record(participant_message_secure_reader_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER),
                                      AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_liveliness_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) &&
        (remote_available & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER)) {
      BuiltinAssociationRecord record(dcps_participant_secure_reader_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER),
                                      AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) &&
        (remote_available & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER)) {
      BuiltinAssociationRecord record(publications_secure_reader_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER),
                                      AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) &&
        (remote_available & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER)) {
      BuiltinAssociationRecord record(subscriptions_secure_reader_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER),
                                      AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }

    if ((local_available_extended & TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE) &&
        (remote_available_extended & TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE)) {
      BuiltinAssociationRecord record(type_lookup_request_secure_reader_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_TL_SVC_REQ_WRITER_SECURE),
                                      AC_REMOTE_RELIABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available_extended & TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE) &&
        (remote_available_extended & TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE)) {
      BuiltinAssociationRecord record(type_lookup_request_secure_writer_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_TL_SVC_REQ_READER_SECURE),
                                      AC_REMOTE_RELIABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available_extended & TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE) &&
        (remote_available_extended & TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE)) {
      BuiltinAssociationRecord record(type_lookup_reply_secure_reader_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_TL_SVC_REPLY_WRITER_SECURE),
                                      AC_REMOTE_RELIABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available_extended & TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE) &&
        (remote_available_extended & TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE)) {
      BuiltinAssociationRecord record(type_lookup_reply_secure_writer_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_TL_SVC_REPLY_READER_SECURE),
                                      AC_REMOTE_RELIABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }

    if ((local_available & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) &&
        (remote_available & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER)) {
      BuiltinAssociationRecord record(participant_message_secure_writer_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER),
                                      ((beq & BEST_EFFORT_PARTICIPANT_MESSAGE_DATA_READER) ? AC_EMPTY : AC_REMOTE_RELIABLE) | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_liveliness_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) &&
        (remote_available & SPDP_BUILTIN_PARTICIPANT_SECURE_READER)) {
      BuiltinAssociationRecord record(dcps_participant_secure_writer_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER),
                                      AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) &&
        (remote_available & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER)) {
      BuiltinAssociationRecord record(publications_secure_writer_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER),
                                      AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
    if ((local_available & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) &&
        (remote_available & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER)) {
      BuiltinAssociationRecord record(subscriptions_secure_writer_,
                                      make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER),
                                      AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | (participant_sec_attr.is_discovery_protected ? AC_SEND_LOCAL_TOKEN : AC_EMPTY));
      participant.builtin_pending_records_.push_back(record);
    }
  }
#endif

  if (spdp_.shutting_down()) { return; }

  associated_participants_.insert(make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_PARTICIPANT));

  process_association_records_i(participant);
}

void Sedp::process_association_records_i(DiscoveredParticipant& participant)
{
  for (DiscoveredParticipant::BuiltinAssociationRecords::iterator pos = participant.builtin_pending_records_.begin(),
         limit = participant.builtin_pending_records_.end(); pos != limit;) {
    const BuiltinAssociationRecord& record = *pos;
    if (ready(participant, record.local_id(), record.remote_id(), record.local_tokens_sent())) {
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::process_association_records_i associating built-ins local %C remote %C pending %B done %B\n"),
                   LogGuid(record.local_id()).c_str(), LogGuid(record.remote_id()).c_str(), participant.builtin_pending_records_.size(), participant.builtin_associated_records_.size()));
      }

      DCPS::AssociationData association_data;
      create_association_data_proto(association_data, participant.pdata_);
      populate_origination_locator(record.remote_id(), association_data.discovery_locator_);
      association_data.remote_id_ = record.remote_id();
      association_data.remote_reliable_ = record.remote_reliable();
      association_data.remote_durable_ = record.remote_durable();
      record.transport_client_->associate(association_data, DCPS::GuidConverter(association_data.remote_id_).isReader());

      participant.builtin_associated_records_.push_back(record);
      participant.builtin_pending_records_.erase(pos++);

    } else {
      if (DCPS::DCPS_debug_level > 6) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::process_association_records_i builtin not ready local %C remote %C pending %B done %B\n"),
                   LogGuid(record.local_id()).c_str(), LogGuid(record.remote_id()).c_str(), participant.builtin_pending_records_.size(), participant.builtin_associated_records_.size()));
      }

      ++pos;
    }
  }

  for (DiscoveredParticipant::WriterAssociationRecords::iterator pos = participant.writer_pending_records_.begin(),
         limit = participant.writer_pending_records_.end(); pos != limit;) {
    const WriterAssociationRecord& record = **pos;
    // The local tokens have already been sent.
    if (ready(participant, record.writer_id(), record.reader_id(), true)) {
      job_queue_->enqueue(DCPS::make_rch<WriterAddAssociation>(*pos));

      participant.writer_associated_records_.push_back(*pos);
      participant.writer_pending_records_.erase(pos++);
    } else {
      if (DCPS::DCPS_debug_level > 6) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::process_association_records_i writer not ready local %C remote %C pending %B done %B\n"),
                   LogGuid(record.writer_id()).c_str(), LogGuid(record.reader_id()).c_str(), participant.writer_pending_records_.size(), participant.writer_associated_records_.size()));
      }
      ++pos;
    }
  }

  for (DiscoveredParticipant::ReaderAssociationRecords::iterator pos = participant.reader_pending_records_.begin(),
         limit = participant.reader_pending_records_.end(); pos != limit;) {
    const ReaderAssociationRecord& record = **pos;
    // The local tokens have already been sent.
    if (ready(participant, record.reader_id(), record.writer_id(), true)) {
      job_queue_->enqueue(DCPS::make_rch<ReaderAddAssociation>(*pos));

      participant.reader_associated_records_.push_back(*pos);
      participant.reader_pending_records_.erase(pos++);
    } else {
      if (DCPS::DCPS_debug_level > 6) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::process_association_records_i reader not ready local %C remote %C pending %B done %B\n"),
                   LogGuid(record.reader_id()).c_str(), LogGuid(record.writer_id()).c_str(), participant.reader_pending_records_.size(), participant.reader_associated_records_.size()));
      }
      ++pos;
    }
  }
}

#ifdef OPENDDS_SECURITY
void Sedp::generate_remote_matched_crypto_handles(DiscoveredParticipant& participant)
{
  for (DiscoveredParticipant::BuiltinAssociationRecords::iterator pos = participant.builtin_pending_records_.begin(),
         limit = participant.builtin_pending_records_.end(); pos != limit; ++pos) {
    const BuiltinAssociationRecord& record = *pos;
    if (record.generate_remote_matched_crypto_handle()) {
      generate_remote_matched_crypto_handle(record);
    }
  }
}

void Sedp::disassociate_volatile(DiscoveredParticipant& participant)
{
  const GUID_t local_writer = make_id(participant_id_, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER);
  const GUID_t local_reader = make_id(participant_id_, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER);
  const GUID_t remote_writer = make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER);
  const GUID_t remote_reader = make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER);

  for (DiscoveredParticipant::BuiltinAssociationRecords::iterator pos = participant.builtin_pending_records_.begin(),
         limit = participant.builtin_pending_records_.end(); pos != limit;) {
    const BuiltinAssociationRecord& record = *pos;
    if (record.local_id() == local_writer && record.remote_id() == remote_reader) {
      participant.builtin_pending_records_.erase(pos++);
    } else if (record.local_id() == local_reader && record.remote_id() == remote_writer) {
      participant.builtin_pending_records_.erase(pos++);
    } else {
      ++pos;
    }
  }

  for (DiscoveredParticipant::BuiltinAssociationRecords::iterator pos = participant.builtin_associated_records_.begin(),
         limit = participant.builtin_associated_records_.end(); pos != limit;) {
    const BuiltinAssociationRecord& record = *pos;
    if (record.local_id() == local_writer && record.remote_id() == remote_reader) {
      record.transport_client_->disassociate(record.remote_id());
      participant.builtin_associated_records_.erase(pos++);
    } else if (record.local_id() == local_reader && record.remote_id() == remote_writer) {
      record.transport_client_->disassociate(record.remote_id());
      participant.builtin_associated_records_.erase(pos++);
    } else {
      ++pos;
    }
  }
}

void Sedp::cleanup_volatile_crypto(const DCPS::RepoId& remote)
{
  remove_remote_crypto_handle(remote, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER);
  remove_remote_crypto_handle(remote, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER);
}

void Sedp::associate_volatile(DiscoveredParticipant& participant)
{
  using namespace DDS::Security;

  const BuiltinEndpointSet_t local_available = spdp_.available_builtin_endpoints();
  const BuiltinEndpointSet_t remote_available = participant.pdata_.participantProxy.availableBuiltinEndpoints;

  if ((local_available & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) &&
      (remote_available & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER)) {
    BuiltinAssociationRecord record(participant_volatile_message_secure_reader_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER),
                                    AC_REMOTE_RELIABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE);
    participant.builtin_pending_records_.push_back(record);
  }
  if ((local_available & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) &&
      (remote_available & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER)) {
    BuiltinAssociationRecord record(participant_volatile_message_secure_writer_,
                                    make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER),
                                    AC_REMOTE_RELIABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE);
    participant.builtin_pending_records_.push_back(record);
  }
}
#endif // OPENDDS_SECURITY

#ifdef OPENDDS_SECURITY

void disassociate_helper_extended(DDS::Security::ExtendedBuiltinEndpointSet_t& extended_associated_endpoints,
                                  const CORBA::ULong flags, const DCPS::RepoId& id, const EntityId_t& ent,
                                  DCPS::TransportClient& client)
{
  if (extended_associated_endpoints & flags) {
    client.disassociate(make_id(id, ent));
    extended_associated_endpoints &= ~flags;
  }
}

void Sedp::remove_remote_crypto_handle(const RepoId& participant, const EntityId_t& entity)
{
  using namespace DDS::Security;

  const RepoId remote = make_id(participant, entity);
  SecurityException se = {"", 0, 0};
  CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();

  const DCPS::GuidConverter traits(remote);
  if (traits.isReader()) {
    const DDS::Security::DatareaderCryptoHandle drch =
      get_handle_registry()->get_remote_datareader_crypto_handle(remote);
    if (drch == DDS::HANDLE_NIL) {
      return;
    }
    if (!key_factory->unregister_datareader(drch, se)) {
      if (DCPS::security_debug.cleanup_error) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) {cleanup_error} Sedp::remove_remote_crypto_handle() - ")
                   ACE_TEXT("Failure calling unregister_datareader() (ch %d). Security Exception[%d.%d]: %C\n"),
                   drch, se.code, se.minor_code, se.message.in()));
      }
    }
    get_handle_registry()->erase_remote_datareader_crypto_handle(remote);
  } else if (traits.isWriter()) {
    const DDS::Security::DatawriterCryptoHandle dwch =
      get_handle_registry()->get_remote_datawriter_crypto_handle(remote);
    if (dwch == DDS::HANDLE_NIL) {
      return;
    }
    if (!key_factory->unregister_datawriter(dwch, se)) {
      if (DCPS::security_debug.cleanup_error) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) {cleanup_error} Sedp::remove_remote_crypto_handle() - ")
                   ACE_TEXT("Failure calling unregister_datawriter() (ch %d). Security Exception[%d.%d]: %C\n"),
                   dwch, se.code, se.minor_code, se.message.in()));
      }
    }
    get_handle_registry()->erase_remote_datawriter_crypto_handle(remote);
  }
}

void
Sedp::create_and_send_datareader_crypto_tokens(
  const DDS::Security::DatareaderCryptoHandle& drch, const DCPS::RepoId& local_reader,
  const DDS::Security::DatawriterCryptoHandle& dwch, const DCPS::RepoId& remote_writer)
{
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("Sedp::create_and_send_datareader_crypto_tokens() - ")
               ACE_TEXT("sending tokens for local reader %C (ch %d) to remote writer %C (ch %d)\n"),
               DCPS::LogGuid(local_reader).c_str(), drch,
               DCPS::LogGuid(remote_writer).c_str(), dwch));
  }

  DDS::Security::DatareaderCryptoTokenSeq drcts;
  create_datareader_crypto_tokens(drch, dwch, drcts);

  send_datareader_crypto_tokens(local_reader, remote_writer, drcts);
}

void
Sedp::create_and_send_datawriter_crypto_tokens(
  const DDS::Security::DatawriterCryptoHandle& dwch, const DCPS::RepoId& local_writer,
  const DDS::Security::DatareaderCryptoHandle& drch, const DCPS::RepoId& remote_reader)
{
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("Sedp::create_and_send_datawriter_crypto_tokens() - ")
               ACE_TEXT("sending tokens for local writer %C (ch %d) to remote reader %C (ch %d)\n"),
               DCPS::LogGuid(local_writer).c_str(), dwch,
               DCPS::LogGuid(remote_reader).c_str(), drch));
  }

  DDS::Security::DatawriterCryptoTokenSeq dwcts;
  create_datawriter_crypto_tokens(dwch, drch, dwcts);

  send_datawriter_crypto_tokens(local_writer, remote_reader, dwcts);
}

void
Sedp::send_builtin_crypto_tokens(const DCPS::RepoId& dst, const DCPS::RepoId& src)
{
  if (DCPS::GuidConverter(src).isReader()) {
    create_and_send_datareader_crypto_tokens(get_handle_registry()->get_local_datareader_crypto_handle(src), src,
                                             get_handle_registry()->get_remote_datawriter_crypto_handle(dst), dst);
  } else {
    create_and_send_datawriter_crypto_tokens(get_handle_registry()->get_local_datawriter_crypto_handle(src), src,
                                             get_handle_registry()->get_remote_datareader_crypto_handle(dst), dst);
  }
}

void
Sedp::send_builtin_crypto_tokens(const DCPS::RepoId& remoteId)
{
  const GUID_t remote_part = make_id(remoteId, ENTITYID_PARTICIPANT);
  Spdp::DiscoveredParticipantIter iter = spdp_.participants_.find(remote_part);
  if (iter == spdp_.participants_.end()) {
    if (DCPS::security_debug.bookkeeping) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) {bookkeeping} Sedp::send_builtin_crypto_tokens - could not find participant %C\n"), LogGuid(remote_part).c_str()));
    }
    return;
  }

  for (DiscoveredParticipant::BuiltinAssociationRecords::iterator pos = iter->second.builtin_pending_records_.begin(),
         limit = iter->second.builtin_pending_records_.end(); pos != limit; ++pos) {
    BuiltinAssociationRecord& record = *pos;
    if (record.send_local_token()) {
      send_builtin_crypto_tokens(record.remote_id(), record.local_id());
      record.local_tokens_sent(true);
    }
  }

  for (DiscoveredParticipant::BuiltinAssociationRecords::iterator pos = iter->second.builtin_associated_records_.begin(),
         limit = iter->second.builtin_associated_records_.end(); pos != limit; ++pos) {
    BuiltinAssociationRecord& record = *pos;
    if (record.send_local_token()) {
      send_builtin_crypto_tokens(record.remote_id(), record.local_id());
      record.local_tokens_sent(true);
    }
  }
}
#endif

bool
Sedp::disassociate(DiscoveredParticipant& participant)
{
  const RepoId part = make_id(participant.pdata_.participantProxy.guidPrefix, ENTITYID_PARTICIPANT);

  associated_participants_.erase(part);

#ifdef OPENDDS_SECURITY
  if (spdp_.is_security_enabled()) {
    static const EntityId_t secure_entities[] = {
      ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER,
      ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER,
      ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER,
      ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER,
      ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER,
      ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER,
      ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER,
      ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER,
      ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER,
      ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER,
      ENTITYID_TL_SVC_REQ_WRITER_SECURE,
      ENTITYID_TL_SVC_REQ_READER_SECURE,
      ENTITYID_TL_SVC_REPLY_WRITER_SECURE,
      ENTITYID_TL_SVC_REPLY_READER_SECURE
    };
    for (size_t i = 0; i < DCPS::array_count(secure_entities); ++i) {
      remove_remote_crypto_handle(part, secure_entities[i]);
    }

    const DDS::Security::CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();
    DDS::Security::SecurityException se;

    typedef Security::HandleRegistry::DatareaderCryptoHandleList DatareaderCryptoHandleList;
    typedef Security::HandleRegistry::DatawriterCryptoHandleList DatawriterCryptoHandleList;

    const RepoId key = make_unknown_guid(part);
    const DatareaderCryptoHandleList drlist = get_handle_registry()->get_all_remote_datareaders(key);
    for (DatareaderCryptoHandleList::const_iterator pos = drlist.begin(), limit = drlist.end();
         pos != limit; ++pos) {
      if (!key_factory->unregister_datareader(pos->second, se)) {
        if (DCPS::security_debug.cleanup_error) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) {cleanup_error} Sedp::disassociate() - ")
                     ACE_TEXT("Failure calling unregister_datareader() (ch %d). Security Exception[%d.%d]: %C\n"),
                     pos->second, se.code, se.minor_code, se.message.in()));
        }
      }
      get_handle_registry()->erase_remote_datareader_crypto_handle(pos->first);
    }
    const DatawriterCryptoHandleList dwlist = get_handle_registry()->get_all_remote_datawriters(key);
    for (DatawriterCryptoHandleList::const_iterator pos = dwlist.begin(), limit = dwlist.end();
         pos != limit; ++pos) {
      if (!key_factory->unregister_datawriter(pos->second, se)) {
        if (DCPS::security_debug.cleanup_error) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) {cleanup_error} Sedp::disassociate() - ")
                     ACE_TEXT("Failure calling unregister_datawriter() (ch %d). Security Exception[%d.%d]: %C\n"),
                     pos->second, se.code, se.minor_code, se.message.in()));
        }
      }
      get_handle_registry()->erase_remote_datawriter_crypto_handle(pos->first);
    }
  }
#endif

  OPENDDS_VECTOR(DiscoveredPublication) pubs_to_remove_from_bit;
  OPENDDS_VECTOR(DiscoveredSubscription) subs_to_remove_from_bit;

  bool result = false;
  if (spdp_.has_discovered_participant(part)) {
    remove_entities_belonging_to(discovered_publications_, part, false, pubs_to_remove_from_bit);
    remove_entities_belonging_to(discovered_subscriptions_, part, true, subs_to_remove_from_bit);
    result = true;
  }

  for (OPENDDS_VECTOR(DiscoveredPublication)::iterator it = pubs_to_remove_from_bit.begin(); it != pubs_to_remove_from_bit.end(); ++it) {
    remove_from_bit_i(*it);
  }

  for (OPENDDS_VECTOR(DiscoveredSubscription)::iterator it = subs_to_remove_from_bit.begin(); it != subs_to_remove_from_bit.end(); ++it) {
    remove_from_bit_i(*it);
  }

  participant.builtin_pending_records_.clear();

  for (DiscoveredParticipant::BuiltinAssociationRecords::const_iterator pos = participant.builtin_associated_records_.begin(), limit = participant.builtin_associated_records_.end(); pos != limit; ++pos) {
    const BuiltinAssociationRecord& record = *pos;
    record.transport_client_->disassociate(record.remote_id());
  }

  participant.builtin_associated_records_.clear();

  //FUTURE: if/when topic propagation is supported, add it here

  return result;
}

void
Sedp::replay_durable_data_for(const DCPS::RepoId& remote_sub_id)
{
  DCPS::GuidConverter conv(remote_sub_id);
  ACE_DEBUG((LM_DEBUG, "Sedp::replay_durable_data_for %C\n", OPENDDS_STRING(conv).c_str()));
  if (remote_sub_id.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER) {
    write_durable_publication_data(remote_sub_id, false);
  } else if (remote_sub_id.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER) {
    write_durable_subscription_data(remote_sub_id, false);
  } else if (remote_sub_id.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER) {
    write_durable_participant_message_data(remote_sub_id);
#ifdef OPENDDS_SECURITY
  } else if (remote_sub_id.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    write_durable_publication_data(remote_sub_id, true);
  } else if (remote_sub_id.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    write_durable_subscription_data(remote_sub_id, true);
  } else if (remote_sub_id.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    write_durable_participant_message_data_secure(remote_sub_id);
  } else if (remote_sub_id.entityId == ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER) {
    write_durable_dcps_participant_secure(remote_sub_id);
#endif
  }
}

void
Sedp::update_locators(const ParticipantData_t& pdata)
{
  DCPS::TransportLocatorSeq remote_data;
  populate_locators(remote_data, pdata);

  DCPS::RepoId remote_id = make_id(pdata.participantProxy.guidPrefix, ENTITYID_PARTICIPANT);

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::update_locators updating locators for %C\n"), DCPS::LogGuid(remote_id).c_str()));
  }

  const BuiltinEndpointSet_t& avail =
    pdata.participantProxy.availableBuiltinEndpoints;

  if (avail & DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER) {
    remote_id.entityId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR) {
    remote_id.entityId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_WRITER) {
    remote_id.entityId = ENTITYID_TL_SVC_REQ_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_READER) {
    remote_id.entityId = ENTITYID_TL_SVC_REQ_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_WRITER) {
    remote_id.entityId = ENTITYID_TL_SVC_REPLY_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER) {
    remote_id.entityId = ENTITYID_TL_SVC_REPLY_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }

#ifdef OPENDDS_SECURITY
  if (avail & DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (avail & DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_READER) {
    remote_id.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    transport_inst_->update_locators(remote_id, remote_data);
  }

  const DDS::Security::ExtendedBuiltinEndpointSet_t& extended_avail =
    pdata.participantProxy.availableExtendedBuiltinEndpoints;

  if (extended_avail & DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE) {
    remote_id.entityId = ENTITYID_TL_SVC_REQ_WRITER_SECURE;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (extended_avail & DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE) {
    remote_id.entityId = ENTITYID_TL_SVC_REQ_READER_SECURE;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (extended_avail & DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE) {
    remote_id.entityId = ENTITYID_TL_SVC_REPLY_WRITER_SECURE;
    transport_inst_->update_locators(remote_id, remote_data);
  }
  if (extended_avail & DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE) {
    remote_id.entityId = ENTITYID_TL_SVC_REPLY_READER_SECURE;
    transport_inst_->update_locators(remote_id, remote_data);
  }
#endif
}

template<typename Map>
void Sedp::remove_entities_belonging_to(
  Map& m, GUID_t participant, bool subscription, OPENDDS_VECTOR(typename Map::mapped_type)& to_remove_from_bit)
{
  participant.entityId = ENTITYID_UNKNOWN;
  for (typename Map::iterator i = m.lower_bound(participant);
       i != m.end() && 0 == std::memcmp(i->first.guidPrefix,
                                        participant.guidPrefix,
                                        sizeof(GuidPrefix_t));) {
    String topic_name = i->second.get_topic_name();
    DCPS::TopicDetailsMap::iterator top_it = topics_.find(topic_name);
    if (top_it != topics_.end()) {
      if (subscription) {
        top_it->second.remove_discovered_subscription(i->first);
      } else {
        top_it->second.remove_discovered_publication(i->first);
      }
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Sedp::remove_entities_belonging_to - ")
                   ACE_TEXT("calling match_endpoints remove\n")));
      }
      match_endpoints(i->first, top_it->second, true /*remove*/);
      if (spdp_.shutting_down()) { return; }
      if (top_it->second.is_dead()) {
        purge_dead_topic(topic_name);
      }
    }
    to_remove_from_bit.push_back(i->second);
    m.erase(i++);
  }
}

void
Sedp::remove_from_bit_i(const DiscoveredPublication& pub)
{
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
  // bit may be null if the DomainParticipant is shutting down
  if (bit && pub.bit_ih_ != DDS::HANDLE_NIL) {
    bit->set_instance_state(pub.bit_ih_,
                            DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  }
#else
  ACE_UNUSED_ARG(pub);
#endif /* DDS_HAS_MINIMUM_BIT */
}

void
Sedp::remove_from_bit_i(const DiscoveredSubscription& sub)
{
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
  // bit may be null if the DomainParticipant is shutting down
  if (bit && sub.bit_ih_ != DDS::HANDLE_NIL) {
    bit->set_instance_state(sub.bit_ih_,
                            DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  }
#else
  ACE_UNUSED_ARG(sub);
#endif /* DDS_HAS_MINIMUM_BIT */
}

#ifndef DDS_HAS_MINIMUM_BIT
DCPS::TopicBuiltinTopicDataDataReaderImpl* Sedp::topic_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_TOPIC_TOPIC);
  return dynamic_cast<DCPS::TopicBuiltinTopicDataDataReaderImpl*>(d.in());
}

DCPS::PublicationBuiltinTopicDataDataReaderImpl* Sedp::pub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_PUBLICATION_TOPIC);
  return dynamic_cast<DCPS::PublicationBuiltinTopicDataDataReaderImpl*>(d.in());
}

DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* Sedp::sub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  return dynamic_cast<DCPS::SubscriptionBuiltinTopicDataDataReaderImpl*>(d.in());
}

#endif /* DDS_HAS_MINIMUM_BIT */

bool
Sedp::update_topic_qos(const RepoId& topicId, const DDS::TopicQos& qos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  OPENDDS_MAP_CMP(RepoId, OPENDDS_STRING, DCPS::GUID_tKeyLessThan)::iterator iter =
    topic_names_.find(topicId);
  if (iter == topic_names_.end()) {
    return false;
  }
  const OPENDDS_STRING& name = iter->second;
  TopicDetails& topic = topics_[name];
  using namespace DCPS;
  // If the TOPIC_DATA QoS changed our local endpoints must be resent
  // with new QoS
  if (qos.topic_data != topic.local_qos().topic_data) {
    topic.update(qos);
    // For each endpoint associated on this topic
    for (RepoIdSet::const_iterator topic_endpoints = topic.local_publications().begin();
         topic_endpoints != topic.local_publications().end(); ++topic_endpoints) {

      const RepoId& rid = *topic_endpoints;
      LocalPublicationIter lp = local_publications_.find(rid);
      OPENDDS_ASSERT(lp != local_publications_.end());
      write_publication_data(rid, lp->second);
    }
    for (RepoIdSet::const_iterator topic_endpoints = topic.local_subscriptions().begin();
         topic_endpoints != topic.local_subscriptions().end(); ++topic_endpoints) {

      const RepoId& rid = *topic_endpoints;
      LocalSubscriptionIter ls = local_subscriptions_.find(rid);
      OPENDDS_ASSERT(ls != local_subscriptions_.end());
      write_subscription_data(rid, ls->second);
    }
  }

  return true;
}

DDS::ReturnCode_t
Sedp::remove_publication_i(const RepoId& publicationId, LocalPublication& pub)
{
#ifdef OPENDDS_SECURITY
  DCPS::DataWriterCallbacks_rch pl = pub.publication_.lock();
  if (pl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = pl->get_ice_endpoint();
    if (endpoint) {
      ice_agent_->remove_local_agent_info_listener(endpoint, publicationId);
    }
  }

  if (is_security_enabled() && pub.security_attribs_.base.is_discovery_protected) {
    return publications_secure_writer_->write_unregister_dispose(publicationId);
  } else {
    return publications_writer_->write_unregister_dispose(publicationId);
  }
#else
  ACE_UNUSED_ARG(pub);
  return publications_writer_->write_unregister_dispose(publicationId);
#endif
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
    String topic_name = topic_names_[pb.topic_id_];
    TopicDetailsMap::iterator top_it = topics_.find(topic_name);
    if (top_it != topics_.end()) {
      match_endpoints(publicationId, top_it->second);
    }
    return true;
  }
  return false;
}

DDS::ReturnCode_t
Sedp::remove_subscription_i(const RepoId& subscriptionId,
                            LocalSubscription& sub)
{
#ifdef OPENDDS_SECURITY
  DCPS::DataReaderCallbacks_rch sl = sub.subscription_.lock();
  if (sl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = sl->get_ice_endpoint();
    if (endpoint) {
      ice_agent_->remove_local_agent_info_listener(endpoint, subscriptionId);
    }
  }

  if (is_security_enabled() && sub.security_attribs_.base.is_discovery_protected) {
    return subscriptions_secure_writer_->write_unregister_dispose(subscriptionId);
  } else {
    return subscriptions_writer_->write_unregister_dispose(subscriptionId);
  }
#else
  ACE_UNUSED_ARG(sub);
  return subscriptions_writer_->write_unregister_dispose(subscriptionId);
#endif
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
    String topic_name = topic_names_[sb.topic_id_];
    TopicDetailsMap::iterator top_it = topics_.find(topic_name);
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
        DCPS::DataWriterCallbacks_rch pl = lpi->second.publication_.lock();
        if (pl) {
          pl->update_subscription_params(subId, params);
        }
      }
    }

    return true;
  }
  return false;
}

void
Sedp::shutdown()
{
  publications_reader_->shutting_down();
  subscriptions_reader_->shutting_down();
  participant_message_reader_->shutting_down();
  type_lookup_request_reader_->shutting_down();
  type_lookup_reply_reader_->shutting_down();
#ifdef OPENDDS_SECURITY
  publications_secure_reader_->shutting_down();
  subscriptions_secure_reader_->shutting_down();
  participant_message_secure_reader_->shutting_down();
  participant_stateless_message_reader_->shutting_down();
  participant_volatile_message_secure_reader_->shutting_down();
  dcps_participant_secure_reader_->shutting_down();
  type_lookup_request_secure_reader_->shutting_down();
  type_lookup_reply_secure_reader_->shutting_down();
#endif
  publications_writer_->shutting_down();
  subscriptions_writer_->shutting_down();
  participant_message_writer_->shutting_down();
  type_lookup_request_writer_->shutting_down();
  type_lookup_reply_writer_->shutting_down();
#ifdef OPENDDS_SECURITY
  publications_secure_writer_->shutting_down();
  subscriptions_secure_writer_->shutting_down();
  participant_message_secure_writer_->shutting_down();
  participant_stateless_message_writer_->shutting_down();
  participant_volatile_message_secure_writer_->shutting_down();
  dcps_participant_secure_writer_->shutting_down();
  type_lookup_request_secure_writer_->shutting_down();
  type_lookup_reply_secure_writer_->shutting_down();
#endif
}

void Sedp::process_discovered_writer_data(DCPS::MessageId message_id,
                                          const DCPS::DiscoveredWriterData& wdata,
                                          const RepoId& guid,
                                          const XTypes::TypeInformation& type_info
#ifdef OPENDDS_SECURITY
                                          ,
                                          bool have_ice_agent_info,
                                          const ICE::AgentInfo& ice_agent_info,
                                          const DDS::Security::EndpointSecurityInfo* security_info
#endif
                                          )
{
  OPENDDS_STRING topic_name;

  const RepoId participant_id = make_part_guid(guid);

  // Find the publication - iterator valid only as long as we hold the lock
  DiscoveredPublicationIter iter = discovered_publications_.find(guid);

  if (message_id == DCPS::SAMPLE_DATA) {
    DCPS::DiscoveredWriterData wdata_copy;

#ifdef OPENDDS_SECURITY
    if (iter != discovered_publications_.end()) {
      DiscoveredPublication& dpub = iter->second;
      if (!dpub.have_ice_agent_info_ && have_ice_agent_info) {
        dpub.have_ice_agent_info_ = have_ice_agent_info;
        dpub.ice_agent_info_ = ice_agent_info;
        start_ice(guid, dpub);
      } else if (dpub.have_ice_agent_info_ && !have_ice_agent_info) {
        dpub.have_ice_agent_info_ = have_ice_agent_info;
        dpub.ice_agent_info_ = ice_agent_info;
        stop_ice(guid, dpub);
      } else if (dpub.ice_agent_info_ != ice_agent_info) {
        dpub.ice_agent_info_ = ice_agent_info;
        start_ice(guid, dpub);
      }
    }
#endif

    if (iter == discovered_publications_.end()) { // add new
      DiscoveredPublication prepub(wdata);
      prepub.participant_discovered_at_ = spdp_.get_participant_discovered_at(participant_id);
      prepub.transport_context_ = spdp_.get_participant_flags(participant_id);
      prepub.type_info_ = type_info;

#ifdef OPENDDS_SECURITY
      prepub.have_ice_agent_info_ = have_ice_agent_info;
      prepub.ice_agent_info_ = ice_agent_info;
#endif
      topic_name = prepub.get_topic_name();

#ifdef OPENDDS_SECURITY
      if (is_security_enabled()) {

        DDS::Security::SecurityException ex = {"", 0, 0};

        DDS::TopicBuiltinTopicData data;
        data.key = wdata.ddsPublicationData.key;
        data.name = wdata.ddsPublicationData.topic_name;
        data.type_name = wdata.ddsPublicationData.type_name;
        data.durability = wdata.ddsPublicationData.durability;
        data.durability_service = wdata.ddsPublicationData.durability_service;
        data.deadline = wdata.ddsPublicationData.deadline;
        data.latency_budget = wdata.ddsPublicationData.latency_budget;
        data.liveliness = wdata.ddsPublicationData.liveliness;
        data.reliability = wdata.ddsPublicationData.reliability;
        data.lifespan = wdata.ddsPublicationData.lifespan;
        data.destination_order = wdata.ddsPublicationData.destination_order;
        data.ownership = wdata.ddsPublicationData.ownership;
        data.topic_data = wdata.ddsPublicationData.topic_data;

        AuthState auth_state = spdp_.lookup_participant_auth_state(participant_id);
        if (auth_state == AUTH_STATE_AUTHENTICATED) {

          DDS::Security::PermissionsHandle remote_permissions = spdp_.lookup_participant_permissions(participant_id);

          if (participant_sec_attr_.is_access_protected &&
              !get_access_control()->check_remote_topic(remote_permissions, spdp_.get_domain_id(), data, ex))
            {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ")
                         ACE_TEXT("Sedp::process_discovered_writer_data - ")
                         ACE_TEXT("Unable to check remote topic '%C'. SecurityException[%d.%d]: %C\n"),
                         topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }

          DDS::Security::TopicSecurityAttributes topic_sec_attr;
          if (!get_access_control()->get_topic_sec_attributes(remote_permissions, topic_name.data(), topic_sec_attr, ex))
            {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ")
                         ACE_TEXT("Sedp::process_discovered_writer_data - ")
                         ACE_TEXT("Unable to get security attributes for remote topic '%C'. SecurityException[%d.%d]: %C\n"),
                         topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }

          DDS::Security::PublicationBuiltinTopicDataSecure pub_data_sec;
          pub_data_sec.base.base = wdata.ddsPublicationData;

          if (security_info != NULL) {
            pub_data_sec.base.security_info.endpoint_security_attributes =
              security_info->endpoint_security_attributes;
            pub_data_sec.base.security_info.plugin_endpoint_security_attributes =
              security_info->plugin_endpoint_security_attributes;
          }

          if (topic_sec_attr.is_write_protected &&
              !get_access_control()->check_remote_datawriter(remote_permissions, spdp_.get_domain_id(), pub_data_sec, ex))
            {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ")
                         ACE_TEXT("Sedp::process_discovered_writer_data - ")
                         ACE_TEXT("Unable to check remote datawriter '%C'. SecurityException[%d.%d]: %C\n"),
                         topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }
        } else if (auth_state != AUTH_STATE_UNAUTHENTICATED) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ")
                     ACE_TEXT("Sedp::process_discovered_writer_data - ")
                     ACE_TEXT("Unsupported remote participant authentication state for discovered datawriter '%C'. ")
                     ACE_TEXT("SecurityException[%d.%d]: %C\n"),
                     topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
          return;
        }
      }
#endif

      DiscoveredPublication& pub = discovered_publications_[guid] = prepub;

      // Create a topic if necessary.
      TopicDetailsMap::iterator top_it = topics_.find(topic_name);
      if (top_it == topics_.end()) {
        top_it = topics_.insert(std::make_pair(topic_name, TopicDetails())).first;
        DCPS::RepoId topic_id = make_topic_guid();
        top_it->second.init(topic_name, topic_id);
        topic_names_[topic_id] = topic_name;
      }

      TopicDetails& td = top_it->second;

      // Upsert the remote topic.
      td.add_discovered_publication(guid);

      assign_bit_key(pub);
      wdata_copy = pub.writer_data_;

#ifndef DDS_HAS_MINIMUM_BIT
      DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
      if (bit) { // bit may be null if the DomainParticipant is shutting down
        pub.bit_ih_ =
          bit->store_synthetic_data(wdata_copy.ddsPublicationData,
                                    DDS::NEW_VIEW_STATE);
      }
      if (spdp_.shutting_down()) { return; }
#endif /* DDS_HAS_MINIMUM_BIT */

      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::process_discovered_writer_data - ")
                   ACE_TEXT("calling match_endpoints new\n")));
      }
      if (DCPS::transport_debug.log_progress) {
        DCPS::log_progress("discovered writer data new", participant_id_, participant_id, spdp_.get_participant_discovered_at(participant_id), guid);
      }
      match_endpoints(guid, top_it->second);
    } else {
      if (checkAndAssignQos(iter->second.writer_data_.ddsPublicationData,
                            wdata.ddsPublicationData)) { // update existing

#ifndef DDS_HAS_MINIMUM_BIT
        DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          wdata_copy = iter->second.writer_data_;
          bit->store_synthetic_data(wdata_copy.ddsPublicationData,
                                    DDS::NOT_NEW_VIEW_STATE);
        }
        if (spdp_.shutting_down()) { return; }
#endif /* DDS_HAS_MINIMUM_BIT */

        // Match/unmatch local subscription(s)
        topic_name = iter->second.get_topic_name();
        TopicDetailsMap::iterator top_it = topics_.find(topic_name);
        if (top_it != topics_.end()) {
          if (DCPS::DCPS_debug_level > 3) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::process_discovered_writer_data - ")
                       ACE_TEXT("calling match_endpoints update\n")));
          }
          if (DCPS::transport_debug.log_progress) {
            log_progress("discovered writer data update", participant_id_, participant_id,
                         spdp_.get_participant_discovered_at(participant_id), guid);
          }
          match_endpoints(guid, top_it->second);
          iter = discovered_publications_.find(guid);
          if (iter == discovered_publications_.end()) {
            return;
          }
        }
      }

      if (checkAndAssignLocators(iter->second.writer_data_.writerProxy, wdata.writerProxy)) {
        topic_name = iter->second.get_topic_name();
        TopicDetailsMap::const_iterator top_it = topics_.find(topic_name);
        using DCPS::RepoIdSet;
        const RepoIdSet& assoc =
          (top_it == topics_.end()) ? RepoIdSet() : top_it->second.local_subscriptions();
        for (RepoIdSet::const_iterator i = assoc.begin(); i != assoc.end(); ++i) {
          LocalSubscriptionIter lsi = local_subscriptions_.find(*i);
          if (lsi != local_subscriptions_.end()) {
            DCPS::DataReaderCallbacks_rch sl = lsi->second.subscription_.lock();
            if (sl) {
              sl->update_locators(guid, wdata.writerProxy.allLocators);
            }
          }
        }
      }
    }

  } else if (message_id == DCPS::UNREGISTER_INSTANCE ||
             message_id == DCPS::DISPOSE_INSTANCE ||
             message_id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
    if (iter != discovered_publications_.end()) {
      // Unmatch local subscription(s)
      topic_name = iter->second.get_topic_name();
      TopicDetailsMap::iterator top_it = topics_.find(topic_name);
      if (top_it != topics_.end()) {
        top_it->second.remove_discovered_publication(guid);
        match_endpoints(guid, top_it->second, true /*remove*/);
        if (spdp_.shutting_down()) { return; }
        if (top_it->second.is_dead()) {
          purge_dead_topic(topic_name);
        }
      }
      DiscoveredPublication p = iter->second;
      discovered_publications_.erase(iter);
      remove_from_bit(p);
      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::process_discovered_writer_data - ")
                   ACE_TEXT("calling match_endpoints disp/unreg\n")));
      }
      if (DCPS::transport_debug.log_progress) {
        log_progress("discovered writer data disposed", participant_id_, participant_id,
                     spdp_.get_participant_discovered_at(participant_id), guid);
      }
    }
  }
}

void
Sedp::data_received(DCPS::MessageId message_id,
                    const DiscoveredPublication& dpub)
{
  if (!spdp_.initialized() || spdp_.shutting_down()) { return; }

  const DCPS::DiscoveredWriterData& wdata = dpub.writer_data_;
  const RepoId& guid = wdata.writerProxy.remoteWriterGuid;
  const RepoId guid_participant = make_part_guid(guid);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(wdata.ddsPublicationData.topic_name)) {
    return;
  }

#ifdef OPENDDS_SECURITY
  if (message_id == DCPS::SAMPLE_DATA && should_drop_message(wdata.ddsPublicationData.topic_name)) {
    return;
  }
#endif

  if (!spdp_.has_discovered_participant(guid_participant)) {
    return;
  }

  process_discovered_writer_data(message_id, wdata, guid, dpub.type_info_
#ifdef OPENDDS_SECURITY
                                 , dpub.have_ice_agent_info_, dpub.ice_agent_info_
#endif
                                 );
}

#ifdef OPENDDS_SECURITY
void Sedp::data_received(DCPS::MessageId message_id,
                         const DiscoveredPublication_SecurityWrapper& wrapper)
{
  if (!spdp_.initialized() || spdp_.shutting_down()) { return; }

  const RepoId& guid = wrapper.data.writerProxy.remoteWriterGuid;
  const RepoId guid_participant = make_part_guid(guid);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(wrapper.data.ddsPublicationData.topic_name)) {
    return;
  }

  process_discovered_writer_data(message_id, wrapper.data, guid, wrapper.type_info,
                                 wrapper.have_ice_agent_info, wrapper.ice_agent_info,
                                 &wrapper.security_info);
}
#endif

void Sedp::process_discovered_reader_data(DCPS::MessageId message_id,
                                          const DCPS::DiscoveredReaderData& rdata,
                                          const RepoId& guid,
                                          const XTypes::TypeInformation& type_info
#ifdef OPENDDS_SECURITY
                                          ,
                                          bool have_ice_agent_info,
                                          const ICE::AgentInfo& ice_agent_info,
                                          const DDS::Security::EndpointSecurityInfo* security_info
#endif
                                          )
{
  OPENDDS_STRING topic_name;

  RepoId participant_id = guid;
  participant_id.entityId = ENTITYID_PARTICIPANT;

  // Find the subscripion - iterator valid only as long as we hold the lock
  DiscoveredSubscriptionIter iter = discovered_subscriptions_.find(guid);

  if (message_id == DCPS::SAMPLE_DATA) {
    DCPS::DiscoveredReaderData rdata_copy;

#ifdef OPENDDS_SECURITY
    if (iter != discovered_subscriptions_.end()) {
      DiscoveredSubscription& dsub = iter->second;
      if (!dsub.have_ice_agent_info_ && have_ice_agent_info) {
        dsub.have_ice_agent_info_ = have_ice_agent_info;
        dsub.ice_agent_info_ = ice_agent_info;
        start_ice(guid, dsub);
      } else if (dsub.have_ice_agent_info_ && !have_ice_agent_info) {
        dsub.have_ice_agent_info_ = have_ice_agent_info;
        dsub.ice_agent_info_ = ice_agent_info;
        stop_ice(guid, dsub);
      } else if (dsub.ice_agent_info_ != ice_agent_info) {
        dsub.ice_agent_info_ = ice_agent_info;
        start_ice(guid, dsub);
      }
    }
#endif

    if (iter == discovered_subscriptions_.end()) { // add new
      DiscoveredSubscription presub(rdata);
      presub.participant_discovered_at_ = spdp_.get_participant_discovered_at(participant_id);
      presub.transport_context_ = spdp_.get_participant_flags(participant_id);
      presub.type_info_ = type_info;
#ifdef OPENDDS_SECURITY
      presub.have_ice_agent_info_ = have_ice_agent_info;
      presub.ice_agent_info_ = ice_agent_info;
#endif

      topic_name = presub.get_topic_name();

#ifdef OPENDDS_SECURITY
      if (is_security_enabled()) {

        DDS::Security::SecurityException ex = {"", 0, 0};

        DDS::TopicBuiltinTopicData data;
        data.key = rdata.ddsSubscriptionData.key;
        data.name = rdata.ddsSubscriptionData.topic_name;
        data.type_name = rdata.ddsSubscriptionData.type_name;
        data.durability = rdata.ddsSubscriptionData.durability;
        data.deadline = rdata.ddsSubscriptionData.deadline;
        data.latency_budget = rdata.ddsSubscriptionData.latency_budget;
        data.liveliness = rdata.ddsSubscriptionData.liveliness;
        data.reliability = rdata.ddsSubscriptionData.reliability;
        data.destination_order = rdata.ddsSubscriptionData.destination_order;
        data.ownership = rdata.ddsSubscriptionData.ownership;
        data.topic_data = rdata.ddsSubscriptionData.topic_data;

        AuthState auth_state = spdp_.lookup_participant_auth_state(participant_id);
        if (auth_state == AUTH_STATE_AUTHENTICATED) {

          DDS::Security::PermissionsHandle remote_permissions = spdp_.lookup_participant_permissions(participant_id);

          if (participant_sec_attr_.is_access_protected &&
              !get_access_control()->check_remote_topic(remote_permissions, spdp_.get_domain_id(), data, ex))
            {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ")
                         ACE_TEXT("Sedp::process_discovered_reader_data - ")
                         ACE_TEXT("Unable to check remote topic '%C'. SecurityException[%d.%d]: %C\n"),
                         topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }

          DDS::Security::TopicSecurityAttributes topic_sec_attr;
          if (!get_access_control()->get_topic_sec_attributes(remote_permissions, topic_name.data(), topic_sec_attr, ex))
            {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ")
                         ACE_TEXT("Sedp::process_discovered_reader_data - ")
                         ACE_TEXT("Unable to get security attributes for remote topic '%C'. SecurityException[%d.%d]: %C\n"),
                         topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }

          DDS::Security::SubscriptionBuiltinTopicDataSecure sub_data_sec;
          sub_data_sec.base.base = rdata.ddsSubscriptionData;

          if (security_info != NULL) {
            sub_data_sec.base.security_info.endpoint_security_attributes =
              security_info->endpoint_security_attributes;
            sub_data_sec.base.security_info.plugin_endpoint_security_attributes =
              security_info->plugin_endpoint_security_attributes;
          }

          bool relay_only = false;
          if (topic_sec_attr.is_read_protected &&
              !get_access_control()->check_remote_datareader(
                                                             remote_permissions, spdp_.get_domain_id(), sub_data_sec, relay_only, ex))
            {
              ACE_ERROR((LM_WARNING,
                         ACE_TEXT("(%P|%t) WARNING: ")
                         ACE_TEXT("Sedp::process_discovered_reader_data - ")
                         ACE_TEXT("Unable to check remote datareader '%C'. SecurityException[%d.%d]: %C\n"),
                         topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }

          if (relay_only) {
            relay_only_readers_.insert(guid);
          } else {
            relay_only_readers_.erase(guid);
          }
        } else if (auth_state != AUTH_STATE_UNAUTHENTICATED) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) WARNING: ")
                     ACE_TEXT("Sedp::process_discovered_reader_data - ")
                     ACE_TEXT("Unsupported remote participant authentication state for discovered datareader '%C'. ")
                     ACE_TEXT("SecurityException[%d.%d]: %C\n"),
                     topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
          return;
        }
      }
#endif

      DiscoveredSubscription& sub = discovered_subscriptions_[guid] = presub;

      // Create a topic if necessary.
      TopicDetailsMap::iterator top_it = topics_.find(topic_name);
      if (top_it == topics_.end()) {
        top_it = topics_.insert(std::make_pair(topic_name, TopicDetails())).first;
        DCPS::RepoId topic_id = make_topic_guid();
        top_it->second.init(topic_name, topic_id);
        topic_names_[topic_id] = topic_name;
      }

      TopicDetails& td = top_it->second;

      // Upsert the remote topic.
      td.add_discovered_subscription(guid);

      assign_bit_key(sub);
      rdata_copy = sub.reader_data_;

#ifndef DDS_HAS_MINIMUM_BIT
      DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
      if (bit) { // bit may be null if the DomainParticipant is shutting down
        sub.bit_ih_ =
          bit->store_synthetic_data(rdata_copy.ddsSubscriptionData,
                                    DDS::NEW_VIEW_STATE);
      }
      if (spdp_.shutting_down()) { return; }
#endif /* DDS_HAS_MINIMUM_BIT */

      if (DCPS::DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::process_discovered_reader_data - ")
                   ACE_TEXT("calling match_endpoints new\n")));
      }
      if (DCPS::transport_debug.log_progress) {
        DCPS::log_progress("discovered reader data new", participant_id_, participant_id, spdp_.get_participant_discovered_at(participant_id), guid);
      }
      match_endpoints(guid, top_it->second);
    } else { // update existing
      if (checkAndAssignQos(iter->second.reader_data_.ddsSubscriptionData,
                            rdata.ddsSubscriptionData)) {
#ifndef DDS_HAS_MINIMUM_BIT
        DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          rdata_copy = iter->second.reader_data_;
          bit->store_synthetic_data(rdata_copy.ddsSubscriptionData,
                                    DDS::NOT_NEW_VIEW_STATE);
        }
        if (spdp_.shutting_down()) { return; }
#endif /* DDS_HAS_MINIMUM_BIT */

        // Match/unmatch local publication(s)
        topic_name = iter->second.get_topic_name();
        TopicDetailsMap::iterator top_it = topics_.find(topic_name);
        if (top_it != topics_.end()) {
          if (DCPS::DCPS_debug_level > 3) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::process_discovered_reader_data - ")
                       ACE_TEXT("calling match_endpoints update\n")));
          }
          if (DCPS::transport_debug.log_progress) {
            log_progress("discovered reader data update", participant_id_, participant_id,
                         spdp_.get_participant_discovered_at(participant_id), guid);
          }
          match_endpoints(guid, top_it->second);
          iter = discovered_subscriptions_.find(guid);
          if (iter == discovered_subscriptions_.end()) {
            return;
          }
        }
      }

      if (checkAndAssignParams(iter->second.reader_data_.contentFilterProperty,
                               rdata.contentFilterProperty)) {
        // Let any associated local publications know about the change
        topic_name = iter->second.get_topic_name();
        TopicDetailsMap::iterator top_it = topics_.find(topic_name);
        using DCPS::RepoIdSet;
        const RepoIdSet& assoc =
          (top_it == topics_.end()) ? RepoIdSet() : top_it->second.local_publications();
        for (RepoIdSet::const_iterator i = assoc.begin(); i != assoc.end(); ++i) {
          const LocalPublicationIter lpi = local_publications_.find(*i);
          if (lpi != local_publications_.end()) {
            DCPS::DataWriterCallbacks_rch pl = lpi->second.publication_.lock();
            if (pl) {
              pl->update_subscription_params(guid, rdata.contentFilterProperty.expressionParameters);
            }
          }
        }
      }

      if (checkAndAssignLocators(iter->second.reader_data_.readerProxy, rdata.readerProxy)) {
        topic_name = iter->second.get_topic_name();
        TopicDetailsMap::const_iterator top_it = topics_.find(topic_name);
        using DCPS::RepoIdSet;
        const RepoIdSet& assoc =
          (top_it == topics_.end()) ? RepoIdSet() : top_it->second.local_publications();
        for (RepoIdSet::const_iterator i = assoc.begin(); i != assoc.end(); ++i) {
          LocalPublicationIter lpi = local_publications_.find(*i);
          OPENDDS_ASSERT(lpi != local_publications_.end());
          if (lpi != local_publications_.end()) {
            DCPS::DataWriterCallbacks_rch pl = lpi->second.publication_.lock();
            if (pl) {
              pl->update_locators(guid, rdata.readerProxy.allLocators);
            }
          }
        }
      }
    }

    if (is_expectant_opendds(guid)) {
      // For each associated opendds writer to this reader
      CORBA::ULong len = rdata.readerProxy.associatedWriters.length();
      for (CORBA::ULong writerIndex = 0; writerIndex < len; ++writerIndex) {
        GUID_t writerGuid = rdata.readerProxy.associatedWriters[writerIndex];

        // If the associated writer is in this participant
        LocalPublicationIter lp = local_publications_.find(writerGuid);
        if (lp != local_publications_.end()) {
          // If the local writer is not fully associated with the reader
          lp->second.remote_expectant_opendds_associations_.insert(guid);
        }
      }
    }

  } else if (message_id == DCPS::UNREGISTER_INSTANCE ||
             message_id == DCPS::DISPOSE_INSTANCE ||
             message_id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
    if (iter != discovered_subscriptions_.end()) {
      // Unmatch local publication(s)
      topic_name = iter->second.get_topic_name();
      TopicDetailsMap::iterator top_it = topics_.find(topic_name);
      if (top_it != topics_.end()) {
        top_it->second.remove_discovered_subscription(guid);
        if (DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::process_discovered_reader_data - ")
                     ACE_TEXT("calling match_endpoints disp/unreg\n")));
        }
        if (DCPS::transport_debug.log_progress) {
          log_progress("discovered reader data disposed", participant_id_, participant_id,
                       spdp_.get_participant_discovered_at(participant_id), guid);
        }
        match_endpoints(guid, top_it->second, true /*remove*/);
        if (top_it->second.is_dead()) {
          purge_dead_topic(topic_name);
        }
        if (spdp_.shutting_down()) { return; }
      }
      DiscoveredSubscription s = iter->second;
      discovered_subscriptions_.erase(iter);
      remove_from_bit(s);
    }
  }
}

void
Sedp::data_received(DCPS::MessageId message_id,
                    const DiscoveredSubscription& dsub)
{
  if (!spdp_.initialized() || spdp_.shutting_down()) { return; }

  const DCPS::DiscoveredReaderData& rdata = dsub.reader_data_;
  const GUID_t& guid = rdata.readerProxy.remoteReaderGuid;
  const GUID_t part_guid = make_part_guid(guid);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid) || ignoring(part_guid)
      || ignoring(rdata.ddsSubscriptionData.topic_name)) {
    return;
  }

#ifdef OPENDDS_SECURITY
  if (message_id == DCPS::SAMPLE_DATA && should_drop_message(rdata.ddsSubscriptionData.topic_name)) {
    return;
  }
#endif

  if (!spdp_.has_discovered_participant(part_guid)) {
    return;
  }

  process_discovered_reader_data(message_id, rdata, guid, dsub.type_info_
#ifdef OPENDDS_SECURITY
                                 , dsub.have_ice_agent_info_, dsub.ice_agent_info_
#endif
                                 );
}

#ifdef OPENDDS_SECURITY
void Sedp::data_received(DCPS::MessageId message_id,
                         const DiscoveredSubscription_SecurityWrapper& wrapper)
{
  if (!spdp_.initialized() || spdp_.shutting_down()) { return; }

  const RepoId& guid = wrapper.data.readerProxy.remoteReaderGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(wrapper.data.ddsSubscriptionData.topic_name)) {
    return;
  }

  process_discovered_reader_data(message_id, wrapper.data, guid, wrapper.type_info, wrapper.have_ice_agent_info, wrapper.ice_agent_info, &wrapper.security_info);
}
#endif

void
Sedp::notify_liveliness(const ParticipantMessageData& pmd)
{
  const RepoId& guid = pmd.participantGuid;
  const RepoId guid_participant = make_part_guid(guid);
  // Clear the entityId so lower bound will work.
  const RepoId prefix = make_unknown_guid(pmd.participantGuid);
  const bool is_automatic = PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE != pmd.participantGuid.entityId;
  if (DCPS::DCPS_debug_level >= 8) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Sedp::notify_liveliness: Notifying Liveliness, %C\n"),
               DCPS::LogGuid(guid).c_str()));
  }

  for (LocalSubscriptionMap::const_iterator sub_pos = local_subscriptions_.begin(),
         sub_limit = local_subscriptions_.end();
       sub_pos != sub_limit; ++sub_pos) {
    const DCPS::RepoIdSet::const_iterator pos =
      sub_pos->second.matched_endpoints_.lower_bound(prefix);
    if (pos != sub_pos->second.matched_endpoints_.end() &&
        DCPS::equal_guid_prefixes(*pos, prefix)) {
      DCPS::DataReaderCallbacks_rch sl = sub_pos->second.subscription_.lock();
      if (sl) {
        if (sub_pos->second.qos_.liveliness.kind == ::DDS::AUTOMATIC_LIVELINESS_QOS && is_automatic) {
          sl->signal_liveliness(guid_participant);
        } else if ((sub_pos->second.qos_.liveliness.kind == ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS
                   || sub_pos->second.qos_.liveliness.kind == ::DDS::AUTOMATIC_LIVELINESS_QOS) && !is_automatic) {
          sl->signal_liveliness(guid_participant);
        }
      }
    }
  }
}

void
Sedp::data_received(DCPS::MessageId /*message_id*/,
                    const ParticipantMessageData& data)
{
  if (!spdp_.initialized() || spdp_.shutting_down()) {
    return;
  }

  const GUID_t& guid = data.participantGuid;
  const GUID_t guid_participant = make_part_guid(guid);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid) || ignoring(guid_participant)) {
    return;
  }

  if (!spdp_.has_discovered_participant(guid_participant)) {
    return;
  }
  notify_liveliness(data);
}

#ifdef OPENDDS_SECURITY
void
Sedp::received_participant_message_data_secure(DCPS::MessageId /*message_id*/,
  const ParticipantMessageData& data)
{
  if (spdp_.shutting_down()) {
    return;
  }

  const GUID_t& guid = data.participantGuid;
  const GUID_t guid_participant = make_part_guid(guid);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid) || ignoring(guid_participant)) {
    return;
  }

  if (!spdp_.has_discovered_participant(guid_participant)) {
    return;
  }

  notify_liveliness(data);
}

bool Sedp::should_drop_stateless_message(const DDS::Security::ParticipantGenericMessage& msg)
{
  using DCPS::GUID_UNKNOWN;

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, true);

  const GUID_t& src_endpoint = msg.source_endpoint_guid;
  const GUID_t& dst_endpoint = msg.destination_endpoint_guid;
  const GUID_t& this_endpoint = participant_stateless_message_reader_->get_repo_id();
  const GUID_t& dst_participant = msg.destination_participant_guid;
  const GUID_t& this_participant = participant_id_;

  if (ignoring(src_endpoint)) {
    return true;
  }

  if (dst_participant != GUID_UNKNOWN && dst_participant != this_participant) {
    return true;
  }

  if (dst_endpoint != GUID_UNKNOWN && dst_endpoint != this_endpoint) {
    return true;
  }

  return false;
}

bool Sedp::should_drop_volatile_message(const DDS::Security::ParticipantGenericMessage& msg)
{
  using DCPS::GUID_UNKNOWN;

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, true);

  const GUID_t src_endpoint = msg.source_endpoint_guid;
  const GUID_t dst_participant = msg.destination_participant_guid;
  const GUID_t this_participant = participant_id_;

  if (ignoring(src_endpoint)) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Sedp::should_drop_volatile_message() - ")
                 ACE_TEXT("ignoring %C -> %C local %C\n"),
                 DCPS::LogGuid(src_endpoint).c_str(),
                 DCPS::LogGuid(dst_participant).c_str(),
                 DCPS::LogGuid(this_participant).c_str()));
    }
    return true;
  }

  if (!msg.message_data.length()) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Sedp::should_drop_volatile_message() - ")
                 ACE_TEXT("no data %C -> %C local %C\n"),
                 DCPS::LogGuid(src_endpoint).c_str(),
                 DCPS::LogGuid(dst_participant).c_str(),
                 DCPS::LogGuid(this_participant).c_str()));
    }
    return true;
  }

  if (dst_participant != GUID_UNKNOWN && dst_participant != this_participant) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Sedp::should_drop_volatile_message() - ")
                 ACE_TEXT("not for us %C -> %C local %C\n"),
                 DCPS::LogGuid(src_endpoint).c_str(),
                 DCPS::LogGuid(dst_participant).c_str(),
                 DCPS::LogGuid(this_participant).c_str()));
    }
    return true;
  }

  return false;
}

bool Sedp::should_drop_message(const char* unsecure_topic_name)
{
  if (is_security_enabled()) {
    DDS::Security::TopicSecurityAttributes attribs;
    DDS::Security::SecurityException ex = {"", 0, 0};

    bool ok = get_access_control()->get_topic_sec_attributes(
      get_permissions_handle(),
      unsecure_topic_name,
      attribs,
      ex);

    if (!ok || attribs.is_discovery_protected) {
      return true;
    }
  }

  return false;
}

void
Sedp::received_stateless_message(DCPS::MessageId /*message_id*/,
                                 const DDS::Security::ParticipantStatelessMessage& msg)
{
  if (spdp_.shutting_down()) {
    return;
  }

  if (should_drop_stateless_message(msg)) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Sedp::received_stateless_message() - ")
                 ACE_TEXT("dropping\n")));
    }
    return;
  }

  if (0 == std::strcmp(msg.message_class_id,
                       DDS::Security::GMCLASSID_SECURITY_AUTH_REQUEST)) {
    spdp_.handle_auth_request(msg);
  } else if (0 == std::strcmp(msg.message_class_id,
                              DDS::Security::GMCLASSID_SECURITY_AUTH_HANDSHAKE)) {
    spdp_.handle_handshake_message(msg);
  } else {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Sedp::received_stateless_message() - ")
                 ACE_TEXT("Unknown message class id\n")));
    }
  }
}

void
Sedp::received_volatile_message_secure(DCPS::MessageId /* message_id */,
                                       const DDS::Security::ParticipantVolatileMessageSecure& msg)
{
  if (spdp_.shutting_down()) {
    return;
  }

  if (should_drop_volatile_message(msg)) {
    return;
  }

  if (0 == std::strcmp(msg.message_class_id,
                       DDS::Security::GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS)) {
    if (!spdp_.handle_participant_crypto_tokens(msg)) {
      ACE_DEBUG((LM_DEBUG, "Sedp::received_volatile_message_secure handle_participant_crypto_tokens failed\n"));
      return;
    }
  } else if (0 == std::strcmp(msg.message_class_id,
                              DDS::Security::GMCLASSID_SECURITY_DATAWRITER_CRYPTO_TOKENS)) {
    if (!handle_datawriter_crypto_tokens(msg)) {
      ACE_DEBUG((LM_DEBUG, "Sedp::received_volatile_message_secure handle_datawriter_crypto_tokens failed\n"));
      return;
    }
  } else if (0 == std::strcmp(msg.message_class_id,
                              DDS::Security::GMCLASSID_SECURITY_DATAREADER_CRYPTO_TOKENS)) {
    if (!handle_datareader_crypto_tokens(msg)) {
      ACE_DEBUG((LM_DEBUG, "Sedp::received_volatile_message_secure handle_datareader_crypto_tokens failed\n"));
      return;
    }
  } else {
    return;
  }
}
#endif

bool
Sedp::is_expectant_opendds(const GUID_t& endpoint) const
{
  GUID_t participant = endpoint;
  participant.entityId = DCPS::ENTITYID_PARTICIPANT;
  return spdp_.is_expectant_opendds(participant);
}

void
Sedp::association_complete_i(const RepoId& localId,
                             const RepoId& remoteId)
{
  if (DCPS::DCPS_debug_level) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DEBUG: Sedp::association_complete_i local %C remote %C\n"),
               DCPS::LogGuid(localId).c_str(),
               DCPS::LogGuid(remoteId).c_str()));
  }

  // If the remote endpoint is an opendds endpoint that expects associated datawriter announcements
  if (is_expectant_opendds(remoteId)) {
    LocalSubscriptionIter sub = local_subscriptions_.find(localId);
    // If the local endpoint is a reader
    if (sub != local_subscriptions_.end()) {
      std::pair<DCPS::RepoIdSet::iterator, bool> result =
          sub->second.remote_expectant_opendds_associations_.insert(remoteId);
      // If this is a new association for the local reader
      if (result.second) {
        // Tell other participants
        write_subscription_data(localId, sub->second);
      }
    }
  }

#ifdef OPENDDS_SECURITY
  if (remoteId.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    write_durable_publication_data(remoteId, true);
  } else if (remoteId.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    write_durable_subscription_data(remoteId, true);
  } else if (remoteId.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    write_durable_participant_message_data_secure(remoteId);
  } else if (remoteId.entityId == ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER) {
    write_durable_dcps_participant_secure(remoteId);
  } else if (remoteId.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER) {
    spdp_.send_participant_crypto_tokens(remoteId);
    send_builtin_crypto_tokens(remoteId);
    resend_user_crypto_tokens(remoteId);
  } else if (remoteId.entityId == ENTITYID_TL_SVC_REQ_READER_SECURE) {
    type_lookup_request_secure_writer_->send_deferred_samples(remoteId);
  } else if (remoteId.entityId == ENTITYID_TL_SVC_REPLY_READER_SECURE) {
    type_lookup_reply_secure_writer_->send_deferred_samples(remoteId);
  } else
#endif
  if (remoteId.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER) {
    write_durable_publication_data(remoteId, false);
  } else if (remoteId.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER) {
    write_durable_subscription_data(remoteId, false);
  } else if (remoteId.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER) {
    write_durable_participant_message_data(remoteId);
  } else if (remoteId.entityId == ENTITYID_TL_SVC_REQ_READER) {
    type_lookup_request_writer_->send_deferred_samples(remoteId);
  } else if (remoteId.entityId == ENTITYID_TL_SVC_REPLY_READER) {
    type_lookup_reply_writer_->send_deferred_samples(remoteId);
  }
}

void Sedp::data_acked_i(const DCPS::RepoId& local_id,
                        const DCPS::RepoId& remote_id)
{
  if (local_id.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER ||
      local_id.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER
#ifdef OPENDDS_SECURITY
      || local_id.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER
      || local_id.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER
      || local_id.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER
#endif
      ) {
    const GUID_t remote_part = make_id(remote_id, ENTITYID_PARTICIPANT);
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    Spdp::DiscoveredParticipantIter iter = spdp_.participants_.find(remote_part);
    if (iter != spdp_.participants_.end()) {
      process_association_records_i(iter->second);
    }
  }
}

void Sedp::signal_liveliness(DDS::LivelinessQosPolicyKind kind)
{

#ifdef OPENDDS_SECURITY
  DDS::Security::SecurityException se = {"", 0, 0};
  DDS::Security::TopicSecurityAttributes attribs;

  if (is_security_enabled()) {
    // TODO: Pending issue DDSSEC12-28 Topic security attributes
    // may get changed to a different set of security attributes.
    bool ok = get_access_control()->get_topic_sec_attributes(
      get_permissions_handle(), "DCPSParticipantMessageSecure", attribs, se);

    if (ok) {

      if (attribs.is_liveliness_protected) {
        signal_liveliness_secure(kind);

      } else {
        signal_liveliness_unsecure(kind);
      }

    } else {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::signal_liveliness() - ")
        ACE_TEXT("Failure calling get_topic_sec_attributes(). Security Exception[%d.%d]: %C\n"),
          se.code, se.minor_code, se.message.in()));
    }

  } else {
#endif

    signal_liveliness_unsecure(kind);

#ifdef OPENDDS_SECURITY
  }
#endif
}

void
Sedp::signal_liveliness_unsecure(DDS::LivelinessQosPolicyKind kind)
{
  if (!(spdp_.available_builtin_endpoints() & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER)) {
    return;
  }

  switch (kind) {
  case DDS::AUTOMATIC_LIVELINESS_QOS: {
    const GUID_t guid = make_id(participant_id_,
      PARTICIPANT_MESSAGE_DATA_KIND_AUTOMATIC_LIVELINESS_UPDATE);
    write_participant_message_data(guid, local_participant_messages_[guid], GUID_UNKNOWN);
    break;
  }

  case DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS: {
    const GUID_t guid = make_id(participant_id_,
      PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE);
    write_participant_message_data(guid, local_participant_messages_[guid], GUID_UNKNOWN);
    break;
  }

  case DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS:
    // Do nothing.
    break;
  }
}


bool Sedp::send_type_lookup_request(const XTypes::TypeIdentifierSeq& type_ids,
                                    const DCPS::RepoId& reader,
                                    bool is_discovery_protected,
                                    bool send_get_types,
                                    const SequenceNumber& seq_num)
{
  TypeLookupRequestWriter_rch writer = type_lookup_request_writer_;
  DCPS::RepoId remote_reader = make_id(reader, ENTITYID_TL_SVC_REQ_READER);
#ifdef OPENDDS_SECURITY
  if (is_security_enabled() && is_discovery_protected) {
    writer = type_lookup_request_secure_writer_;
    remote_reader = make_id(reader, ENTITYID_TL_SVC_REQ_READER_SECURE);
  }
#else
  ACE_UNUSED_ARG(is_discovery_protected);
#endif

  return writer->send_type_lookup_request(
    type_ids, remote_reader, seq_num,
    send_get_types ?
      XTypes::TypeLookup_getTypes_HashId : XTypes::TypeLookup_getDependencies_HashId);
}

#ifdef OPENDDS_SECURITY
void
Sedp::signal_liveliness_secure(DDS::LivelinessQosPolicyKind kind)
{
  if (!(spdp_.available_builtin_endpoints() & DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER)) {
    return;
  }

  switch (kind) {
  case DDS::AUTOMATIC_LIVELINESS_QOS: {
    const GUID_t guid = make_id(participant_id_,
      PARTICIPANT_MESSAGE_DATA_KIND_AUTOMATIC_LIVELINESS_UPDATE);
    write_participant_message_data_secure(guid, local_participant_messages_secure_[guid], GUID_UNKNOWN);
    break;
  }

  case DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS: {
    const GUID_t guid = make_id(participant_id_,
      PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE);
    write_participant_message_data_secure(guid, local_participant_messages_secure_[guid], GUID_UNKNOWN);
    break;
  }

  case DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS:
    // Do nothing.
    break;
  }
}
#endif

DCPS::WeakRcHandle<ICE::Endpoint> Sedp::get_ice_endpoint() {
  return transport_inst_->get_ice_endpoint();
}

Sedp::Endpoint::~Endpoint()
{
  remove_all_msgs();
  transport_stop();
}

DDS::Subscriber_var
Sedp::Endpoint::get_builtin_subscriber() const
{
  return sedp_.spdp_.bit_subscriber();
}

EntityId_t Sedp::Endpoint::counterpart_entity_id() const
{
  EntityId_t rv = repo_id_.entityId;
  switch (rv.entityKind) {
  case DCPS::ENTITYKIND_BUILTIN_WRITER_WITH_KEY:
    rv.entityKind = DCPS::ENTITYKIND_BUILTIN_READER_WITH_KEY;
    break;

  case DCPS::ENTITYKIND_BUILTIN_WRITER_NO_KEY:
    rv.entityKind = DCPS::ENTITYKIND_BUILTIN_READER_NO_KEY;
    break;

  case DCPS::ENTITYKIND_BUILTIN_READER_NO_KEY:
    rv.entityKind = DCPS::ENTITYKIND_BUILTIN_WRITER_NO_KEY;
    break;

  case DCPS::ENTITYKIND_BUILTIN_READER_WITH_KEY:
    rv.entityKind = DCPS::ENTITYKIND_BUILTIN_WRITER_WITH_KEY;
    break;

  default:
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Sedp::Endpoint::counterpart_entity_id: "
        "Unexpected entityKind: %u\n", rv.entityKind));
    }
    rv.entityKind = DCPS::ENTITYKIND_BUILTIN_UNKNOWN;
  }
  return rv;
}

GUID_t Sedp::Endpoint::make_counterpart_guid(const GUID_t& remote_part) const
{
  return make_id(remote_part, counterpart_entity_id());
}

bool Sedp::Endpoint::associated_with_counterpart(const GUID_t& remote_part) const
{
  return associated_with(make_counterpart_guid(remote_part));
}

bool Sedp::Endpoint::associated_with_counterpart_if_not_pending(
  const DCPS::GUID_t& remote_part) const
{
  const GUID_t counterpart = make_counterpart_guid(remote_part);
  return associated_with(counterpart) || !pending_association_with(counterpart);
}

//---------------------------------------------------------------
Sedp::Writer::Writer(const RepoId& pub_id, Sedp& sedp, ACE_INT64 seq_init)
  : Endpoint(pub_id, sedp), seq_(seq_init)
{
}

Sedp::Writer::~Writer()
{
}

bool
Sedp::Writer::assoc(const DCPS::AssociationData& subscription)
{
  return associate(subscription, true);
}

void Sedp::Writer::transport_assoc_done(int flags, const RepoId& remote)
{
  if (!(flags & ASSOC_OK)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) Sedp::Writer::transport_assoc_done: ")
               ACE_TEXT("ERROR: transport layer failed to associate %C\n"),
               DCPS::LogGuid(remote).c_str()));
    return;
  }

  if (shutting_down_ == true) {
    return;
  }

  sedp_.association_complete_i(repo_id_, remote);
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
Sedp::Writer::data_acked(const GUID_t& remote)
{
  sedp_.data_acked_i(get_repo_id(), remote);
}

void
Sedp::Writer::control_delivered(const DCPS::Message_Block_Ptr& /* sample */)
{
}

void
Sedp::Writer::control_dropped(const DCPS::Message_Block_Ptr& /* sample */, bool)
{
}

void
Sedp::Writer::replay_durable_data_for(const DCPS::RepoId& remote_sub_id)
{
  // Ideally, we would have the data cached and ready for replay but we do not.
  sedp_.replay_durable_data_for(remote_sub_id);
}

void Sedp::Writer::send_sample(DCPS::Message_Block_Ptr payload,
                               size_t size,
                               const RepoId& reader,
                               DCPS::SequenceNumber& sequence,
                               bool historic)
{
  if (DCPS::DCPS_debug_level >= 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::Writer::send_sample from %C to %C\n",
      DCPS::LogGuid(repo_id_).c_str(),
      DCPS::LogGuid(reader).c_str()));
  }
  DCPS::DataSampleElement* el = new DCPS::DataSampleElement(repo_id_, this, DCPS::PublicationInstance_rch());
  set_header_fields(el->get_header(), size, reader, sequence, historic);

  el->set_sample(DCPS::move(payload));
  *el->get_sample() << el->get_header();

  if (reader != GUID_UNKNOWN) {
    el->set_sub_id(0, reader);
    el->set_num_subs(1);

    if (deferrable() && !associated_with_counterpart(reader)) {
      if (DCPS::DCPS_debug_level >= 8) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::Writer::send_sample: "
          "counterpart isn't associated, deferring\n"));
      }
      DeferredSamples::iterator samples_for_reader = deferred_samples_.insert(
        std::make_pair(reader, PerReaderDeferredSamples())).first;
      samples_for_reader->second.insert(std::make_pair(sequence, el));
      return;
    }
  }

  send_sample_i(el);
}

void Sedp::Writer::send_deferred_samples(const GUID_t& reader)
{
  DeferredSamples::iterator samples_for_reader = deferred_samples_.find(reader);
  if (samples_for_reader != deferred_samples_.end()) {
    if (DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::Writer::send_deferred_samples to %C\n",
        DCPS::LogGuid(reader).c_str()));
    }
    for (PerReaderDeferredSamples::iterator i = samples_for_reader->second.begin();
        i != samples_for_reader->second.end(); ++i) {
      send_sample_i(i->second);
    }
    deferred_samples_.erase(samples_for_reader);
  }
}

void Sedp::Writer::send_sample_i(DCPS::DataSampleElement* el)
{
  DCPS::SendStateDataSampleList list;
  list.enqueue_tail(el);

  send(list);
}

DDS::ReturnCode_t
Sedp::Writer::write_parameter_list(const ParameterList& plist,
                                   const RepoId& reader,
                                   DCPS::SequenceNumber& sequence)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  // Determine message length
  size_t size = 0;
  DCPS::primitive_serialized_size_ulong(sedp_encoding, size);
  DCPS::serialized_size(sedp_encoding, size, plist);

  // Build and send RTPS message
  DCPS::Message_Block_Ptr payload(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size)));

  if (!payload) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::Writer::write_parameter_list")
               ACE_TEXT(" - Failed to allocate message block message\n")));
    return DDS::RETCODE_ERROR;
  }

  Serializer serializer(payload->cont(), sedp_encoding);
  DCPS::EncapsulationHeader encap;
  if (encap.from_encoding(sedp_encoding, DCPS::MUTABLE) &&
      serializer << encap && serializer << plist) {
    send_sample(move(payload), size, reader, sequence, reader != GUID_UNKNOWN);
  } else {
    result = DDS::RETCODE_ERROR;
  }

  return result;
}

DDS::ReturnCode_t
Sedp::LivelinessWriter::write_participant_message(const ParticipantMessageData& pmd,
                                                  const RepoId& reader,
                                                  DCPS::SequenceNumber& sequence)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  // Determine message length
  size_t size = 0;
  DCPS::primitive_serialized_size_ulong(sedp_encoding, size);
  DCPS::serialized_size(sedp_encoding, size, pmd);

  // Build and send RTPS message
  DCPS::Message_Block_Ptr payload(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size)));
  Serializer serializer(payload->cont(), sedp_encoding);
  DCPS::EncapsulationHeader encap;
  if (encap.from_encoding(sedp_encoding, DCPS::FINAL) &&
      serializer << encap && serializer << pmd) {
    send_sample(move(payload), size, reader, sequence, reader != GUID_UNKNOWN);
  } else {
    result = DDS::RETCODE_ERROR;
  }

  return result;
}

#ifdef OPENDDS_SECURITY
DDS::ReturnCode_t
Sedp::SecurityWriter::write_stateless_message(const DDS::Security::ParticipantStatelessMessage& msg,
                                              const RepoId& reader,
                                              DCPS::SequenceNumber& sequence)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  size_t size = 0;
  DCPS::primitive_serialized_size_ulong(sedp_encoding, size);
  DCPS::serialized_size(sedp_encoding, size, msg);

  DCPS::Message_Block_Ptr payload(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size)));
  Serializer serializer(payload->cont(), sedp_encoding);
  DCPS::EncapsulationHeader encap;
  if (encap.from_encoding(sedp_encoding, DCPS::FINAL) &&
      serializer << encap && serializer << msg) {
    send_sample(move(payload), size, reader, sequence);
  } else {
    result = DDS::RETCODE_ERROR;
  }

  return result;
}

DDS::ReturnCode_t
Sedp::SecurityWriter::write_volatile_message_secure(const DDS::Security::ParticipantVolatileMessageSecure& msg,
                                                    const RepoId& reader,
                                                    DCPS::SequenceNumber& sequence)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  size_t size = 0;
  DCPS::primitive_serialized_size_ulong(sedp_encoding, size);
  DCPS::serialized_size(sedp_encoding, size, msg);

  DCPS::Message_Block_Ptr payload(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size)));
  Serializer serializer(payload->cont(), sedp_encoding);
  DCPS::EncapsulationHeader encap;
  if (encap.from_encoding(sedp_encoding, DCPS::FINAL) &&
      serializer << encap && serializer << msg) {
    send_sample(move(payload), size, reader, sequence);
  } else {
    result = DDS::RETCODE_ERROR;
  }

  return result;
}

DDS::ReturnCode_t
Sedp::DiscoveryWriter::write_dcps_participant_secure(const Security::SPDPdiscoveredParticipantData& msg,
                                                     const RepoId& reader, DCPS::SequenceNumber& sequence)
{
  ParameterList plist;

  if (!ParameterListConverter::to_param_list(msg, plist)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::DiscoveryWriter::write_dcps_participant_secure - ")
               ACE_TEXT("Failed to convert SPDPdiscoveredParticipantData ")
               ACE_TEXT("to ParameterList\n")));

    return DDS::RETCODE_ERROR;
  }

  ICE::AgentInfoMap ai_map;
  DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = get_ice_endpoint();
  if (sedp_endpoint) {
    ai_map[SEDP_AGENT_INFO_KEY] = sedp_.ice_agent_->get_local_agent_info(sedp_endpoint);
  }
  DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint = sedp_.spdp_.get_ice_endpoint_if_added();
  if (spdp_endpoint) {
    ai_map[SPDP_AGENT_INFO_KEY] = sedp_.ice_agent_->get_local_agent_info(spdp_endpoint);
  }
  if (!ParameterListConverter::to_param_list(ai_map, plist)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("Sedp::DiscoveryWriter::write_dcps_participant_secure - ")
               ACE_TEXT("failed to convert from ICE::AgentInfo ")
               ACE_TEXT("to ParameterList\n")));
    return DDS::RETCODE_ERROR;
  }

  return write_parameter_list(plist, reader, sequence);
}
#endif

DDS::ReturnCode_t
Sedp::DiscoveryWriter::write_unregister_dispose(const RepoId& rid, CORBA::UShort pid)
{
  // Build param list for message
  Parameter param;
  param.guid(rid);
  param._d(pid);
  ParameterList plist;
  plist.length(1);
  plist[0] = param;

  // Determine message length
  size_t size = 0;
  DCPS::primitive_serialized_size_ulong(sedp_encoding, size);
  DCPS::serialized_size(sedp_encoding, size, plist);

  DCPS::Message_Block_Ptr payload(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size)));
  if (!payload) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::Writer::write_unregister_dispose")
               ACE_TEXT(" - Failed to allocate message block message\n")));
    return DDS::RETCODE_ERROR;
  }

  Serializer serializer(payload->cont(), sedp_encoding);
  DCPS::EncapsulationHeader encap;
  if (encap.from_encoding(sedp_encoding, DCPS::MUTABLE) &&
      serializer << encap && serializer << plist) {
    // Send
    write_control_msg(move(payload), size, DCPS::DISPOSE_UNREGISTER_INSTANCE);
    return DDS::RETCODE_OK;
  } else {
    // Error
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::Writer::write_unregister_dispose")
               ACE_TEXT(" - Failed to serialize RTPS control message\n")));
    return DDS::RETCODE_ERROR;
  }
}

void
Sedp::Writer::end_historic_samples(const RepoId& reader)
{
  DCPS::Message_Block_Ptr mb(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(
        reinterpret_cast<const char*>(&reader),
        sizeof(reader))));
  if (mb.get()) {
    mb->cont()->wr_ptr(sizeof(reader));
    // 'mb' would contain the DSHeader, but we skip it. mb.cont() has the data
    write_control_msg(move(mb), sizeof(reader), DCPS::END_HISTORIC_SAMPLES,
                      DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN());
  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::Writer::end_historic_samples")
               ACE_TEXT(" - Failed to allocate message block message\n")));
  }
}

void
Sedp::Writer::request_ack(const RepoId& reader)
{
  DCPS::Message_Block_Ptr mb(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(
        reinterpret_cast<const char*>(&reader),
        sizeof(reader))));
  if (mb.get()) {
    mb->cont()->wr_ptr(sizeof(reader));
    // 'mb' would contain the DSHeader, but we skip it. mb.cont() has the data
    write_control_msg(move(mb), sizeof(reader), DCPS::REQUEST_ACK,
                      DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN());
  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::Writer::request_ack")
               ACE_TEXT(" - Failed to allocate message block message\n")));
  }
}

void
Sedp::Writer::write_control_msg(DCPS::Message_Block_Ptr payload,
                                size_t size,
                                DCPS::MessageId id,
                                DCPS::SequenceNumber seq)
{
  DCPS::DataSampleHeader header;
  Writer::set_header_fields(header, size, GUID_UNKNOWN, seq, false, id);
  // no need to serialize header since rtps_udp transport ignores it
  send_control(header, DCPS::move(payload));
}

void
Sedp::Writer::set_header_fields(DCPS::DataSampleHeader& dsh,
                                size_t size,
                                const RepoId& reader,
                                DCPS::SequenceNumber& sequence,
                                bool historic_sample,
                                DCPS::MessageId id)
{
  dsh.message_id_ = id;
  dsh.byte_order_ = ACE_CDR_BYTE_ORDER;
  dsh.message_length_ = static_cast<ACE_UINT32>(size);
  dsh.publication_id_ = repo_id_;

  if (id != DCPS::END_HISTORIC_SAMPLES && id != DCPS::REQUEST_ACK &&
      (reader == GUID_UNKNOWN ||
       sequence == DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN())) {
    sequence = seq_++;
  }

  if (historic_sample && reader != GUID_UNKNOWN) {
    // retransmit with same seq# for durability
    dsh.historic_sample_ = true;
  }

  dsh.sequence_ = sequence;

  const SystemTimePoint now = SystemTimePoint::now();
  dsh.source_timestamp_sec_ = static_cast<ACE_INT32>(now.value().sec());
  dsh.source_timestamp_nanosec_ = now.value().usec() * 1000;
}

Sedp::SecurityWriter::~SecurityWriter()
{
}

Sedp::DiscoveryWriter::~DiscoveryWriter()
{
}

Sedp::LivelinessWriter::~LivelinessWriter()
{
}

Sedp::TypeLookupRequestWriter::~TypeLookupRequestWriter()
{
}

Sedp::TypeLookupReplyWriter::~TypeLookupReplyWriter()
{
}

Sedp::TypeLookupRequestReader::~TypeLookupRequestReader()
{
}

Sedp::TypeLookupReplyReader::~TypeLookupReplyReader()
{
}

bool Sedp::TypeLookupRequestWriter::send_type_lookup_request(
  const XTypes::TypeIdentifierSeq& type_ids,
  const DCPS::RepoId& reader,
  const DCPS::SequenceNumber& rpc_sequence,
  CORBA::ULong tl_kind)
{
  if (DCPS::DCPS_debug_level >= 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupRequestWriter::send_type_lookup_request: "
      "to %C seq: %q\n", DCPS::LogGuid(reader).c_str(), rpc_sequence.getValue()));
  }
  if (DCPS::transport_debug.log_progress) {
    log_progress("send type lookup request", get_repo_id(), reader, sedp_.spdp_.get_participant_discovered_at(reader));
  }

  if (tl_kind != XTypes::TypeLookup_getTypes_HashId &&
      tl_kind != XTypes::TypeLookup_getDependencies_HashId) {
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "Sedp::TypeLookupRequestWriter::send_type_lookup_request: tl_kind is %u\n",
        tl_kind));
    }
    return false;
  }

  XTypes::TypeLookup_Request type_lookup_request;
  type_lookup_request.header.requestId.writer_guid = get_repo_id();
  type_lookup_request.header.requestId.sequence_number = to_rtps_seqnum(rpc_sequence);
  type_lookup_request.header.instanceName = get_instance_name(reader).c_str();
  type_lookup_request.data.kind = tl_kind;

  if (tl_kind == XTypes::TypeLookup_getTypes_HashId) {
    type_lookup_request.data.getTypes.type_ids = type_ids;
  } else {
    type_lookup_request.data.getTypeDependencies.type_ids = type_ids;
    sedp_.type_lookup_reply_reader_->get_continuation_point(reader.guidPrefix, type_ids[0],
      type_lookup_request.data.getTypeDependencies.continuation_point);
  }

  // Determine message length
  const size_t size = DCPS::EncapsulationHeader::serialized_size +
    DCPS::serialized_size(type_lookup_encoding, type_lookup_request);

  // Build and send type lookup message
  DCPS::Message_Block_Ptr payload(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size)));
  Serializer serializer(payload->cont(), type_lookup_encoding);
  DCPS::EncapsulationHeader encap;
  bool success = true;
  if (encap.from_encoding(serializer.encoding(), DCPS::FINAL) &&
      serializer << encap && serializer << type_lookup_request) {
    DCPS::SequenceNumber sn(seq_++);
    send_sample(move(payload), size, reader, sn);
  } else {
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "Sedp::TypeLookupRequestWriter::send_type_lookup_request: serialization failed\n"));
    }
    success = false;
  }

  return success;
}

bool Sedp::TypeLookupReplyWriter::send_type_lookup_reply(
  XTypes::TypeLookup_Reply& type_lookup_reply,
  const DCPS::RepoId& reader)
{
  if (DCPS::DCPS_debug_level >= 8) {
    const DDS::SampleIdentity id = type_lookup_reply.header.relatedRequestId;
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupReplyWriter::send_type_lookup_reply: "
      "to %C seq: %q\n", DCPS::LogGuid(reader).c_str(),
      to_opendds_seqnum(id.sequence_number).getValue()));
  }

  type_lookup_reply.header.remoteEx = DDS::RPC::REMOTE_EX_OK;

  // Determine message length
  const size_t size = DCPS::EncapsulationHeader::serialized_size +
    DCPS::serialized_size(type_lookup_encoding, type_lookup_reply);

  // Build and send type lookup message
  DCPS::Message_Block_Ptr payload(
    new ACE_Message_Block(
      DCPS::DataSampleHeader::get_max_serialized_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size)));
  Serializer serializer(payload->cont(), type_lookup_encoding);
  DCPS::EncapsulationHeader encap;
  bool success = true;
  if (encap.from_encoding(serializer.encoding(), DCPS::FINAL) &&
      serializer << encap && serializer << type_lookup_reply) {
    DCPS::SequenceNumber sn(seq_++);
    send_sample(move(payload), size, reader, sn);
  } else {
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "Sedp::TypeLookupReplyWriter::send_type_lookup_reply: serialization failed\n"));
    }
    success = false;
  }

  return success;
}

bool Sedp::TypeLookupRequestReader::process_type_lookup_request(
  Serializer& ser, XTypes::TypeLookup_Reply& type_lookup_reply)
{
  XTypes::TypeLookup_Request type_lookup_request;
  if (!(ser >> type_lookup_request)) {
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupRequestReader::process_type_lookup_request - ")
                 ACE_TEXT("failed to deserialize type lookup request\n")));
    }
    return false;
  }

  if (DCPS::DCPS_debug_level >= 8) {
    const DDS::SampleIdentity& request_id = type_lookup_request.header.requestId;
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupReplyWriter::process_type_lookup_request: "
      "from %C seq: %q\n",
      DCPS::LogGuid(request_id.writer_guid).c_str(),
      to_opendds_seqnum(request_id.sequence_number).getValue()));
  }

  if (OPENDDS_STRING(type_lookup_request.header.instanceName) != instance_name_) {
    return true;
  }

  switch (type_lookup_request.data.kind) {
  case XTypes::TypeLookup_getTypes_HashId:
    return process_get_types_request(type_lookup_request, type_lookup_reply);
  case XTypes::TypeLookup_getDependencies_HashId:
    return process_get_dependencies_request(type_lookup_request, type_lookup_reply);
  default:
    return false;
  }
}

bool Sedp::TypeLookupRequestReader::process_get_types_request(
  const XTypes::TypeLookup_Request& type_lookup_request,
  XTypes::TypeLookup_Reply& type_lookup_reply)
{
  sedp_.type_lookup_service_->get_type_objects(type_lookup_request.data.getTypes.type_ids,
    type_lookup_reply._cxx_return.getType.result.types);
  type_lookup_reply._cxx_return.getType.result.complete_to_minimal.length(0);
  if (type_lookup_reply._cxx_return.getType.result.types.length() > 0) {
    type_lookup_reply._cxx_return.getType.return_code = DDS::RETCODE_OK;
    type_lookup_reply._cxx_return.kind = XTypes::TypeLookup_getTypes_HashId;
    type_lookup_reply.header.relatedRequestId = type_lookup_request.header.requestId;
    return true;
  }
  if (DCPS::DCPS_debug_level) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Sedp::TypeLookupRequestReader::process_get_types_request: "
      "No types received\n"));
  }
  return false;
}

void Sedp::TypeLookupRequestReader::gen_continuation_point(XTypes::OctetSeq32& cont_point) const
{
  // We are sending all dependencies of requested types in 1 reply, and thus
  // the continuation_point will be "absent" by setting its length to zero.
  cont_point.length(0);
}

bool Sedp::TypeLookupRequestReader::process_get_dependencies_request(
  const XTypes::TypeLookup_Request& request,
  XTypes::TypeLookup_Reply& reply)
{
  // Send all dependencies (may be empty) of the requested types
  sedp_.type_lookup_service_->get_type_dependencies(request.data.getTypeDependencies.type_ids,
    reply._cxx_return.getTypeDependencies.result.dependent_typeids);
  reply._cxx_return.kind = XTypes::TypeLookup_getDependencies_HashId;
  reply._cxx_return.getTypeDependencies.return_code = DDS::RETCODE_OK;
  gen_continuation_point(reply._cxx_return.getTypeDependencies.result.continuation_point);
  reply.header.relatedRequestId = request.header.requestId;
  return true;
}

void Sedp::TypeLookupReplyReader::get_continuation_point(const GuidPrefix_t& guid_prefix,
                                                         const XTypes::TypeIdentifier& remote_ti,
                                                         XTypes::OctetSeq32& cont_point) const
{
  const GuidPrefixWrapper guid_pref_wrap(guid_prefix);
  const DependenciesMap::const_iterator it = dependencies_.find(guid_pref_wrap);
  if (it == dependencies_.end() || it->second.find(remote_ti) == it->second.end()) {
    cont_point.length(0);
  } else {
    cont_point = it->second.find(remote_ti)->second.first;
  }
}

void Sedp::TypeLookupReplyReader::cleanup(const DCPS::GuidPrefix_t& guid_prefix,
                                          const XTypes::TypeIdentifier& type_id)
{
  const DependenciesMap::iterator it = dependencies_.find(guid_prefix);
  if (it != dependencies_.end() && it->second.find(type_id) != it->second.end()) {
    it->second.erase(type_id);
  }

  if (it != dependencies_.end() && it->second.empty()) {
    dependencies_.erase(it);
  }
}

bool Sedp::TypeLookupReplyReader::process_type_lookup_reply(
  const DCPS::ReceivedDataSample& sample, Serializer& ser, bool is_discovery_protected)
{
  XTypes::TypeLookup_Reply type_lookup_reply;
  if (!(ser >> type_lookup_reply)) {
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupReplyReader::process_type_lookup_reply - ")
        ACE_TEXT("failed to deserialize type lookup reply\n")));
    }
    return false;
  }

  const DDS::SampleIdentity& request_id = type_lookup_reply.header.relatedRequestId;
  const DCPS::SequenceNumber seq_num = to_opendds_seqnum(request_id.sequence_number);
  if (DCPS::DCPS_debug_level >= 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupReplyReader::process_type_lookup_reply - "
      "from %C seq %q\n",
      DCPS::LogGuid(request_id.writer_guid).c_str(),
      seq_num.getValue()));
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, sedp_.lock_, false);
  if (DCPS::transport_debug.log_progress) {
    log_progress("receive type lookup reply", get_repo_id(), sample.header_.publication_id_, sedp_.spdp_.get_participant_discovered_at(sample.header_.publication_id_));
  }
  const OrigSeqNumberMap::const_iterator seq_num_it = sedp_.orig_seq_numbers_.find(seq_num);
  if (seq_num_it == sedp_.orig_seq_numbers_.end()) {
    ACE_DEBUG((LM_WARNING,
      ACE_TEXT("(%P|%t) WARNING: Sedp::TypeLookupReplyReader::process_type_lookup_reply - ")
      ACE_TEXT("could not find request corresponding to the reply from %C seq %q\n"),
      DCPS::LogGuid(request_id.writer_guid).c_str(), seq_num.getValue()));
    return false;
  }

  bool success;
  const ACE_CDR::ULong kind = static_cast<ACE_CDR::ULong>(type_lookup_reply._cxx_return.kind);
  switch (kind) {
  case XTypes::TypeLookup_getTypes_HashId:
    success = process_get_types_reply(type_lookup_reply);
    break;
  case XTypes::TypeLookup_getDependencies_HashId:
    success = process_get_dependencies_reply(sample, type_lookup_reply, seq_num, is_discovery_protected);
    break;
  default:
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "Sedp::TypeLookupReplyReader::process_type_lookup_reply - "
        "reply kind is %u\n", kind));
    }
    return false;
  }

  if (kind == XTypes::TypeLookup_getTypes_HashId) {
    if (DCPS::DCPS_debug_level > 8) {
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) Sedp::TypeLookupReplyReader::process_type_lookup_reply - "
                 "got the reply for the final request in the sequence\n"));
    }
    const DCPS::SequenceNumber key_seq_num = seq_num_it->second.seq_number;

    // Cleanup data
    cleanup(sample.header_.publication_id_.guidPrefix, seq_num_it->second.type_id);
    sedp_.orig_seq_numbers_.erase(seq_num);

    if (success) {
      MatchingDataIter it;
      for (it = sedp_.matching_data_buffer_.begin(); it != sedp_.matching_data_buffer_.end(); ++it) {
        const DCPS::SequenceNumber& seqnum_minimal = it->second.rpc_seqnum_minimal;
        const DCPS::SequenceNumber& seqnum_complete = it->second.rpc_seqnum_complete;
        if (seqnum_minimal == key_seq_num || seqnum_complete == key_seq_num) {
          if (seqnum_minimal == key_seq_num) {
            it->second.got_minimal = true;
          } else {
            it->second.got_complete = true;
          }

          if (it->second.got_minimal && it->second.got_complete) {
            // All remote type objects are obtained, continue the matching process
            const RepoId writer = it->first.writer_;
            const RepoId reader = it->first.reader_;
            sedp_.matching_data_buffer_.erase(it);
            sedp_.match_continue(writer, reader);
            return true;
          }
          break;
        }
      }

      if (it == sedp_.matching_data_buffer_.end()) {
        if (DCPS::DCPS_debug_level) {
          ACE_ERROR((LM_ERROR, "(%P|%t) Sedp::TypeLookupReplyReader::process_type_lookup_reply - "
                     " RPC sequence number %q: No data found in matching data buffer\n",
                     key_seq_num.getValue()));
        }
      }
    }
  }

  return success;
}

bool Sedp::TypeLookupReplyReader::process_get_types_reply(const XTypes::TypeLookup_Reply& reply)
{
  if (DCPS::DCPS_debug_level > 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupReplyReader::process_get_types_reply\n"));
  }

  if (reply._cxx_return.getType.return_code != DDS::RETCODE_OK) {
    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupReplyReader::process_get_types_reply - "
                 "received reply with return code %C\n",
                 DCPS::retcode_to_string(reply._cxx_return.getType.return_code)));
    }
    return false;
  }

  if (reply._cxx_return.getType.result.types.length() == 0) {
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Sedp::TypeLookupReplyReader::process_get_types_reply - "
                 "received reply with no data\n"));
    }
    return false;
  }

  sedp_.type_lookup_service_->add_type_objects_to_cache(reply._cxx_return.getType.result.types);

  if (reply._cxx_return.getType.result.complete_to_minimal.length() != 0) {
    if (DCPS::DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupReplyReader::process_get_types_reply - "
                 "received reply with non-empty complete to minimal map\n"));
    }
    sedp_.type_lookup_service_->update_type_identifier_map(reply._cxx_return.getType.result.complete_to_minimal);
  }

  return true;
}

bool Sedp::TypeLookupReplyReader::process_get_dependencies_reply(
  const DCPS::ReceivedDataSample& sample, const XTypes::TypeLookup_Reply& reply,
  const DCPS::SequenceNumber& seq_num, bool is_discovery_protected)
{
  if (DCPS::DCPS_debug_level > 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupReplyReader::process_get_dependencies_reply - "
      "seq %q\n", seq_num.getValue()));
  }

  if (reply._cxx_return.getTypeDependencies.result.continuation_point.length() != 0 &&
      reply._cxx_return.getTypeDependencies.result.dependent_typeids.length() == 0) {
    if (DCPS::DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "Sedp::TypeLookupReplyReader::process_get_dependencies_reply - "
        "received reply with no data\n"));
    }
    return false;
  }

  const XTypes::TypeLookup_getTypeDependencies_Out& data = reply._cxx_return.getTypeDependencies.result;
  const DCPS::RepoId remote_id = sample.header_.publication_id_;

  const XTypes::TypeIdentifier remote_ti = sedp_.orig_seq_numbers_[seq_num].type_id;
  const GuidPrefixWrapper guid_pref(remote_id.guidPrefix);
  XTypes::TypeIdentifierSeq& deps = dependencies_[guid_pref][remote_ti].second;
  for (unsigned i = 0; i < data.dependent_typeids.length(); ++i) {
    const XTypes::TypeIdentifier& ti = data.dependent_typeids[i].type_id;
    // Optimization - only store TypeIdentifiers for which TypeObjects haven't
    // already been in the type objects cache yet
    if (!sedp_.type_lookup_service_->type_object_in_cache(ti)) {
      deps.append(ti);
    }
  }
  dependencies_[guid_pref][remote_ti].first = data.continuation_point;

  // Update internal data
  sedp_.orig_seq_numbers_.insert(std::make_pair(++sedp_.type_lookup_service_sequence_number_,
                                                sedp_.orig_seq_numbers_[seq_num]));
  sedp_.orig_seq_numbers_.erase(seq_num);

  if (data.continuation_point.length() == 0) { // Get all type objects
    deps.append(remote_ti);
    if (!sedp_.send_type_lookup_request(deps, remote_id, is_discovery_protected, true, sedp_.type_lookup_service_sequence_number_)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupReplyReader::process_get_dependencies_reply - ")
        ACE_TEXT("failed to send getTypes request\n")));
      return false;
    }
  } else { // Get more dependencies
    XTypes::TypeIdentifierSeq type_ids;
    type_ids.append(remote_ti);
    if (!sedp_.send_type_lookup_request(type_ids, remote_id, is_discovery_protected, false, sedp_.type_lookup_service_sequence_number_)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupReplyReader::process_get_dependencies_reply - ")
        ACE_TEXT("failed to send getTypeDependencies request\n")));
      return false;
    }
  }

  return true;
}

void Sedp::cleanup_type_lookup_data(const DCPS::GuidPrefix_t& guid_prefix,
                                    const XTypes::TypeIdentifier& ti,
                                    bool secure)
{
#ifdef OPENDDS_SECURITY
  if (secure) {
    type_lookup_reply_secure_reader_->cleanup(guid_prefix, ti);
  } else {
    type_lookup_reply_reader_->cleanup(guid_prefix, ti);
  }
#else
  ACE_UNUSED_ARG(secure);
  type_lookup_reply_reader_->cleanup(guid_prefix, ti);
#endif
}

Sedp::Reader::~Reader()
{
}

Sedp::DiscoveryReader::~DiscoveryReader()
{
}

Sedp::LivelinessReader::~LivelinessReader()
{
}

Sedp::SecurityReader::~SecurityReader()
{
}

bool
Sedp::Reader::assoc(const DCPS::AssociationData& publication)
{
  return associate(publication, false);
}

// Implementing TransportReceiveListener

static bool decode_parameter_list(
  const DCPS::ReceivedDataSample& sample,
  Serializer& ser,
  DCPS::Extensibility extensibility,
  ParameterList& data)
{
  if (sample.header_.key_fields_only_ && extensibility == DCPS::FINAL) {
    GUID_t guid;
    if (!(ser >> guid)) return false;
    data.length(1);
    data[0].guid(guid);
    data[0]._d(PID_ENDPOINT_GUID);
  } else {
    return ser >> data;
  }
  return true;
}

void
Sedp::Reader::data_received(const DCPS::ReceivedDataSample& sample)
{
  if (shutting_down_ == true) {
    return;
  }

  if (DCPS::DCPS_debug_level >= 9) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::Reader::data_received: from %C\n",
      DCPS::LogGuid(sample.header_.publication_id_).c_str()));
  }

  const DCPS::MessageId id =
    static_cast<DCPS::MessageId>(sample.header_.message_id_);

  switch (id) {
  case DCPS::SAMPLE_DATA:
  case DCPS::DISPOSE_INSTANCE:
  case DCPS::UNREGISTER_INSTANCE:
  case DCPS::DISPOSE_UNREGISTER_INSTANCE: {
    const DCPS::EntityId_t entity_id = sample.header_.publication_id_.entityId;
    const bool full_message = !sample.header_.key_fields_only_;

    // Figure Out Extensibility of Data Based On Entity Id
    const bool is_mutable =
      entity_id == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER ||
#ifdef OPENDDS_SECURITY
      entity_id == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER ||
#endif
      entity_id == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER ||
#ifdef OPENDDS_SECURITY
      entity_id == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER ||
      entity_id == ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER ||
#endif
      false;
    const bool is_final =
      (entity_id == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER && full_message) ||
#ifdef OPENDDS_SECURITY
      (entity_id == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER && full_message) ||
      entity_id == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER ||
      entity_id == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER ||
#endif
      entity_id == ENTITYID_TL_SVC_REQ_WRITER ||
      entity_id == ENTITYID_TL_SVC_REPLY_WRITER ||
#ifdef OPENDDS_SECURITY
      entity_id == ENTITYID_TL_SVC_REQ_WRITER_SECURE ||
      entity_id == ENTITYID_TL_SVC_REPLY_WRITER_SECURE ||
#endif
      false;
    if (is_mutable == is_final) {
      if (is_mutable) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Sedp::Reader::data_received: "
          "entity id extensibility error over %C: both is_final and is_mutable are %d\n",
          to_string(entity_id).c_str(), is_mutable));
      }
      break;
    }
    const DCPS::Extensibility extensibility =
      is_mutable ? DCPS::MUTABLE : DCPS::FINAL;

    // Get Encoding from Encapsulation
    Encoding encoding;
    Serializer ser(sample.sample_.get(), encoding);
    DCPS::EncapsulationHeader encap;
    if (!(ser >> encap)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
        ACE_TEXT("failed to deserialize encapsulation header\n")));
      return;
    }
    if (!encap.to_encoding(encoding, extensibility)) {
      return;
    }
    ser.encoding(encoding);

    data_received_i(sample, entity_id, ser, extensibility);
    break;
  }

  default:
    break;
  }
}

void
Sedp::LivelinessReader::data_received_i(const DCPS::ReceivedDataSample& sample,
  const DCPS::EntityId_t& entity_id,
  DCPS::Serializer& ser,
  DCPS::Extensibility)
{
  const DCPS::MessageId id = static_cast<DCPS::MessageId>(sample.header_.message_id_);
  const bool full_message = !sample.header_.key_fields_only_;

  if (entity_id == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER && full_message) {
    ParticipantMessageData data;
    if (!(ser >> data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::LivelinessReader::data_received_i - ")
                 ACE_TEXT("failed to deserialize data\n")));
      return;
    }
    sedp_.data_received(id, data);

#ifdef OPENDDS_SECURITY
  } else if (entity_id == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER && full_message) {
    ParticipantMessageData data;
    if (!(ser >> data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::LivelinessReader::data_received_i - ")
                 ACE_TEXT("failed to deserialize data\n")));
      return;
    }
    sedp_.received_participant_message_data_secure(id, data);
#endif
  }
}

void
Sedp::SecurityReader::data_received_i(const DCPS::ReceivedDataSample& sample,
  const DCPS::EntityId_t& entity_id,
  DCPS::Serializer& ser,
  DCPS::Extensibility)
{
#ifdef OPENDDS_SECURITY
  const DCPS::MessageId id = static_cast<DCPS::MessageId>(sample.header_.message_id_);

  if (entity_id == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER) {
    DDS::Security::ParticipantStatelessMessage data;
    ser.reset_alignment(); // https://issues.omg.org/browse/DDSIRTP23-63
    if (!(ser >> data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::SecurityReader::data_received_i - ")
                 ACE_TEXT("failed to deserialize data\n")));
      return;
    }
    sedp_.received_stateless_message(id, data);
  } else if (entity_id == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER) {
    DDS::Security::ParticipantVolatileMessageSecure data;
    if (!(ser >> data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::SecurityReader::data_received_i - ")
                 ACE_TEXT("failed to deserialize data\n")));
      return;
    }
    sedp_.received_volatile_message_secure(id, data);
  }
#else
  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(entity_id);
  ACE_UNUSED_ARG(ser);
#endif
}

void
Sedp::DiscoveryReader::data_received_i(const DCPS::ReceivedDataSample& sample,
  const DCPS::EntityId_t& entity_id,
  DCPS::Serializer& ser,
  DCPS::Extensibility extensibility)
{
  const DCPS::MessageId id = static_cast<DCPS::MessageId>(sample.header_.message_id_);

  if (entity_id == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER) {
    ParameterList data;
    if (!decode_parameter_list(sample, ser, extensibility, data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to deserialize data\n")));
      return;
    }

    DiscoveredPublication wdata;
    if (!ParameterListConverter::from_param_list(data, wdata.writer_data_, sedp_.use_xtypes_, wdata.type_info_)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::DiscoveryReader::data_received_i - ")
                 ACE_TEXT("failed to convert from ParameterList ")
                 ACE_TEXT("to DiscoveredWriterData\n")));
      return;
    }
#ifdef OPENDDS_SECURITY
    wdata.have_ice_agent_info_ = false;
    ICE::AgentInfoMap ai_map;
    if (!ParameterListConverter::from_param_list(data, ai_map)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to convert from ParameterList ")
        ACE_TEXT("to ICE Agent info\n")));
      return;
    }
    ICE::AgentInfoMap::const_iterator pos = ai_map.find("DATA");
    if (pos != ai_map.end()) {
      wdata.have_ice_agent_info_ = true;
      wdata.ice_agent_info_ = pos->second;
    }
#endif

    if (wdata.type_info_.minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE ||
        wdata.type_info_.complete.typeid_with_size.type_id.kind() != XTypes::TK_NONE) {
      const GUID_t& remote_guid = wdata.writer_data_.writerProxy.remoteWriterGuid;
      DDS::BuiltinTopicKey_t key = DCPS::repo_id_to_bit_key(remote_guid);
      sedp_.type_lookup_service_->cache_type_info(key, wdata.type_info_);
    }

    sedp_.data_received(id, wdata);

#ifdef OPENDDS_SECURITY
  } else if (entity_id == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    ParameterList data;
    if (!decode_parameter_list(sample, ser, extensibility, data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to deserialize data\n")));
      return;
    }

    DiscoveredPublication_SecurityWrapper wdata_secure = DiscoveredPublication_SecurityWrapper();

    if (!ParameterListConverter::from_param_list(data, wdata_secure, sedp_.use_xtypes_, wdata_secure.type_info)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::DiscoveryReader::data_received_i - ")
                 ACE_TEXT("failed to convert from ParameterList ")
                 ACE_TEXT("to DiscoveredPublication_SecurityWrapper\n")));
      return;
    }

    wdata_secure.have_ice_agent_info = false;
    ICE::AgentInfoMap ai_map;
    if (!ParameterListConverter::from_param_list(data, ai_map)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to convert from ParameterList ")
        ACE_TEXT("to ICE Agent info\n")));
      return;
    }
    ICE::AgentInfoMap::const_iterator pos = ai_map.find("DATA");
    if (pos != ai_map.end()) {
      wdata_secure.have_ice_agent_info = true;
      wdata_secure.ice_agent_info = pos->second;
    }

    if (wdata_secure.type_info.minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE ||
        wdata_secure.type_info.complete.typeid_with_size.type_id.kind() != XTypes::TK_NONE) {
      const GUID_t& remote_guid = wdata_secure.data.writerProxy.remoteWriterGuid;
      DDS::BuiltinTopicKey_t key = DCPS::repo_id_to_bit_key(remote_guid);
      sedp_.type_lookup_service_->cache_type_info(key, wdata_secure.type_info);
    }

    sedp_.data_received(id, wdata_secure);
#endif
  } else if (entity_id == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER) {
    ParameterList data;
    if (!decode_parameter_list(sample, ser, extensibility, data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to deserialize data\n")));
      return;
    }

    DiscoveredSubscription rdata;
    if (!ParameterListConverter::from_param_list(data, rdata.reader_data_, sedp_.use_xtypes_, rdata.type_info_)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR Sedp::DiscoveryReader::data_received_i - ")
                 ACE_TEXT("failed to convert from ParameterList ")
                 ACE_TEXT("to DiscoveredReaderData\n")));
      return;
    }
#ifdef OPENDDS_SECURITY
    rdata.have_ice_agent_info_ = false;
    ICE::AgentInfoMap ai_map;
    if (!ParameterListConverter::from_param_list(data, ai_map)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to convert from ParameterList ")
        ACE_TEXT("to ICE Agent info\n")));
      return;
    }
    ICE::AgentInfoMap::const_iterator pos = ai_map.find("DATA");
    if (pos != ai_map.end()) {
      rdata.have_ice_agent_info_ = true;
      rdata.ice_agent_info_ = pos->second;
    }
#endif
    if (rdata.reader_data_.readerProxy.expectsInlineQos) {
      set_inline_qos(rdata.reader_data_.readerProxy.allLocators);
    }

    if (rdata.type_info_.minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE ||
        rdata.type_info_.complete.typeid_with_size.type_id.kind() != XTypes::TK_NONE) {
      const GUID_t& remote_guid = rdata.reader_data_.readerProxy.remoteReaderGuid;
      DDS::BuiltinTopicKey_t key = DCPS::repo_id_to_bit_key(remote_guid);
      sedp_.type_lookup_service_->cache_type_info(key, rdata.type_info_);
    }

    sedp_.data_received(id, rdata);

#ifdef OPENDDS_SECURITY
  } else if (entity_id == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    ParameterList data;
    if (!decode_parameter_list(sample, ser, extensibility, data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to deserialize data\n")));
      return;
    }

    DiscoveredSubscription_SecurityWrapper rdata_secure = DiscoveredSubscription_SecurityWrapper();
    if (!ParameterListConverter::from_param_list(data, rdata_secure, sedp_.use_xtypes_, rdata_secure.type_info)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR Sedp::DiscoveryReader::data_received_i - ")
                 ACE_TEXT("failed to convert from ParameterList ")
                 ACE_TEXT("to DiscoveredSubscription_SecurityWrapper\n")));
      return;
    }

    rdata_secure.have_ice_agent_info = false;
    ICE::AgentInfoMap ai_map;
    if (!ParameterListConverter::from_param_list(data, ai_map)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to convert from ParameterList ")
        ACE_TEXT("to ICE Agent info\n")));
      return;
    }
    ICE::AgentInfoMap::const_iterator pos = ai_map.find("DATA");
    if (pos != ai_map.end()) {
      rdata_secure.have_ice_agent_info = true;
      rdata_secure.ice_agent_info = pos->second;
    }

    if ((rdata_secure.data).readerProxy.expectsInlineQos) {
      set_inline_qos((rdata_secure.data).readerProxy.allLocators);
    }

    if (rdata_secure.type_info.minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE ||
        rdata_secure.type_info.complete.typeid_with_size.type_id.kind() != XTypes::TK_NONE) {
      const GUID_t& remote_guid = rdata_secure.data.readerProxy.remoteReaderGuid;
      DDS::BuiltinTopicKey_t key = DCPS::repo_id_to_bit_key(remote_guid);
      sedp_.type_lookup_service_->cache_type_info(key, rdata_secure.type_info);
    }

    sedp_.data_received(id, rdata_secure);

  } else if (entity_id == ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER) {

    ParameterList data;
    if (!decode_parameter_list(sample, ser, extensibility, data)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::DiscoveryReader::data_received_i - ")
        ACE_TEXT("failed to deserialize data\n")));
      return;
    }

    Security::SPDPdiscoveredParticipantData pdata;

    if (!ParameterListConverter::from_param_list(data, pdata)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::DiscoveryReader::data_received_i - ")
                 ACE_TEXT("failed to convert from ParameterList ")
                 ACE_TEXT("to Security::SPDPdiscoveredParticipantData\n")));
      return;
    }
    const GUID_t guid = make_part_guid(sample.header_.publication_id_);
    sedp_.spdp_.process_participant_ice(data, pdata, guid);
    sedp_.spdp_.handle_participant_data(id, pdata, DCPS::SequenceNumber::ZERO(), ACE_INET_Addr(), true);

#endif
  }
}

void
Sedp::TypeLookupRequestReader::data_received_i(const DCPS::ReceivedDataSample& sample,
  const DCPS::EntityId_t& entity_id,
  DCPS::Serializer& ser,
  DCPS::Extensibility)
{
  if (DCPS::DCPS_debug_level > 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupRequestReader::data_received_i: from %C\n",
      DCPS::LogGuid(sample.header_.publication_id_).c_str()));
  }

  XTypes::TypeLookup_Reply type_lookup_reply;
  if (!process_type_lookup_request(ser, type_lookup_reply)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupRequestReader::data_received_i - ")
      ACE_TEXT("failed to take type lookup request\n")));
    return;
  }

#ifdef OPENDDS_SECURITY
  if (entity_id == ENTITYID_TL_SVC_REQ_WRITER_SECURE) {
    const DCPS::RepoId reader = make_id(sample.header_.publication_id_, ENTITYID_TL_SVC_REPLY_READER_SECURE);
    if (!sedp_.type_lookup_reply_secure_writer_->send_type_lookup_reply(type_lookup_reply, reader)) {
      if (DCPS::DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupRequestReader::data_received_i - ")
          ACE_TEXT("failed to send secure type lookup reply\n")));
      }
      return;
    }
  } else if (entity_id == ENTITYID_TL_SVC_REQ_WRITER) {
#endif
    const DCPS::RepoId reader = make_id(sample.header_.publication_id_, ENTITYID_TL_SVC_REPLY_READER);
    if (!sedp_.type_lookup_reply_writer_->send_type_lookup_reply(type_lookup_reply, reader)) {
      if (DCPS::DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupRequestReader::data_received_i - ")
          ACE_TEXT("failed to send type lookup reply\n")));
      }
      return;
    }
#ifdef OPENDDS_SECURITY
  }
#else
  ACE_UNUSED_ARG(entity_id);
#endif
}

void Sedp::TypeLookupReplyReader::data_received_i(
  const DCPS::ReceivedDataSample& sample,
  const DCPS::EntityId_t& remote_id,
  DCPS::Serializer& ser,
  DCPS::Extensibility)
{
  if (DCPS::DCPS_debug_level > 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::TypeLookupReplyReader::data_received_i: from %C\n",
      DCPS::LogGuid(sample.header_.publication_id_).c_str()));
  }

#ifdef OPENDDS_SECURITY
  if (remote_id == ENTITYID_TL_SVC_REPLY_WRITER_SECURE) {
    if (!process_type_lookup_reply(sample, ser, true)) {
      if (DCPS::DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupReplyReader::data_received_i - ")
          ACE_TEXT("failed to process secure type lookup reply\n")));
      }
      return;
    }
  } else if (remote_id == ENTITYID_TL_SVC_REPLY_WRITER) {
#endif
    if (!process_type_lookup_reply(sample, ser, false)) {
      if (DCPS::DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::TypeLookupReplyReader::data_received_i - ")
          ACE_TEXT("failed to process type lookup reply\n")));
      }
      return;
    }
#ifdef OPENDDS_SECURITY
  }
#else
  ACE_UNUSED_ARG(remote_id);
#endif
}

void
Sedp::populate_discovered_writer_msg(
  DCPS::DiscoveredWriterData& dwd,
  const RepoId& publication_id,
  const LocalPublication& pub)
{
  // Ignored on the wire dwd.ddsPublicationData.key
  // Ignored on the wire dwd.ddsPublicationData.participant_key
  OPENDDS_STRING topic_name = topic_names_[pub.topic_id_];
  dwd.ddsPublicationData.topic_name = topic_name.c_str();
  TopicDetails& topic_details = topics_[topic_name];
  dwd.ddsPublicationData.type_name = topic_details.local_data_type_name().c_str();
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
  dwd.ddsPublicationData.representation = pub.qos_.representation;
  dwd.ddsPublicationData.presentation = pub.publisher_qos_.presentation;
  dwd.ddsPublicationData.partition = pub.publisher_qos_.partition;
  dwd.ddsPublicationData.topic_data = topic_details.local_qos().topic_data;
  dwd.ddsPublicationData.group_data = pub.publisher_qos_.group_data;

  dwd.writerProxy.remoteWriterGuid = publication_id;
  // Ignore dwd.writerProxy.unicastLocatorList;
  // Ignore dwd.writerProxy.multicastLocatorList;
  dwd.writerProxy.allLocators = pub.trans_info_;
}

void
Sedp::populate_discovered_reader_msg(
  DCPS::DiscoveredReaderData& drd,
  const RepoId& subscription_id,
  const LocalSubscription& sub)
{
  // Ignored on the wire drd.ddsSubscription.key
  // Ignored on the wire drd.ddsSubscription.participant_key
  OPENDDS_STRING topic_name = topic_names_[sub.topic_id_];
  drd.ddsSubscriptionData.topic_name = topic_name.c_str();
  TopicDetails& topic_details = topics_[topic_name];
  drd.ddsSubscriptionData.type_name = topic_details.local_data_type_name().c_str();
  drd.ddsSubscriptionData.durability = sub.qos_.durability;
  drd.ddsSubscriptionData.deadline = sub.qos_.deadline;
  drd.ddsSubscriptionData.latency_budget = sub.qos_.latency_budget;
  drd.ddsSubscriptionData.liveliness = sub.qos_.liveliness;
  drd.ddsSubscriptionData.reliability = sub.qos_.reliability;
  drd.ddsSubscriptionData.ownership = sub.qos_.ownership;
  drd.ddsSubscriptionData.destination_order = sub.qos_.destination_order;
  drd.ddsSubscriptionData.user_data = sub.qos_.user_data;
  drd.ddsSubscriptionData.time_based_filter = sub.qos_.time_based_filter;
  drd.ddsSubscriptionData.representation  = sub.qos_.representation;
  drd.ddsSubscriptionData.presentation = sub.subscriber_qos_.presentation;
  drd.ddsSubscriptionData.partition = sub.subscriber_qos_.partition;
  drd.ddsSubscriptionData.topic_data = topic_details.local_qos().topic_data;
  drd.ddsSubscriptionData.group_data = sub.subscriber_qos_.group_data;
  drd.ddsSubscriptionData.type_consistency = sub.qos_.type_consistency;

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

  const CORBA::ULong len = drd.readerProxy.associatedWriters.length();
  const CORBA::ULong added_len = static_cast<CORBA::ULong>(sub.remote_expectant_opendds_associations_.size());
  drd.readerProxy.associatedWriters.length(len + added_len);

  CORBA::ULong i = 0;
  for (DCPS::RepoIdSet::const_iterator writer =
         sub.remote_expectant_opendds_associations_.begin();
       writer != sub.remote_expectant_opendds_associations_.end();
       ++writer) {
    drd.readerProxy.associatedWriters[len + i] = *writer;
    ++i;
  }
}

void
Sedp::write_durable_publication_data(const RepoId& reader, bool secure)
{
  if (!(spdp_.available_builtin_endpoints() & (DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER
#ifdef OPENDDS_SECURITY
                                               | DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER
#endif
                                               ))) {
    return;
  }

  if (secure) {
#ifdef OPENDDS_SECURITY
    LocalPublicationIter pub, end = local_publications_.end();
    for (pub = local_publications_.begin(); pub != end; ++pub) {
      if (pub->second.security_attribs_.base.is_discovery_protected) {
        write_publication_data_secure(pub->first, pub->second, reader);
      }
    }
    publications_secure_writer_->end_historic_samples(reader);
#endif
  } else {
    LocalPublicationIter pub, end = local_publications_.end();
    for (pub = local_publications_.begin(); pub != end; ++pub) {
#ifdef OPENDDS_SECURITY
      if (!pub->second.security_attribs_.base.is_discovery_protected) {
        write_publication_data(pub->first, pub->second, reader);
      }
#else
      write_publication_data(pub->first, pub->second, reader);
#endif
    }
    publications_writer_->end_historic_samples(reader);
  }
}

void
Sedp::write_durable_subscription_data(const RepoId& reader, bool secure)
{
  if (!(spdp_.available_builtin_endpoints() & (DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER
#ifdef OPENDDS_SECURITY
                                               | DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER
#endif
                                               ))) {
    return;
  }

  if (secure) {
#ifdef OPENDDS_SECURITY
    LocalSubscriptionIter sub, end = local_subscriptions_.end();
    for (sub = local_subscriptions_.begin(); sub != end; ++sub) {
      if (is_security_enabled() && sub->second.security_attribs_.base.is_discovery_protected) {
        write_subscription_data_secure(sub->first, sub->second, reader);
      }
    }
    subscriptions_secure_writer_->end_historic_samples(reader);
#endif
  } else {
    LocalSubscriptionIter sub, end = local_subscriptions_.end();
    for (sub = local_subscriptions_.begin(); sub != end; ++sub) {
#ifdef OPENDDS_SECURITY
      if (!(is_security_enabled() && sub->second.security_attribs_.base.is_discovery_protected)) {
        write_subscription_data(sub->first, sub->second, reader);
      }
#else
      write_subscription_data(sub->first, sub->second, reader);
#endif
    }
    subscriptions_writer_->end_historic_samples(reader);
  }
}

void
Sedp::write_durable_participant_message_data(const RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER)) {
    return;
  }

  LocalParticipantMessageIter part, end = local_participant_messages_.end();
  for (part = local_participant_messages_.begin(); part != end; ++part) {
    write_participant_message_data(part->first, part->second, reader);
  }
  participant_message_writer_->end_historic_samples(reader);
}

#ifdef OPENDDS_SECURITY
void
Sedp::write_durable_participant_message_data_secure(const RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER)) {
    return;
  }

  LocalParticipantMessageIter part, end = local_participant_messages_secure_.end();
  for (part = local_participant_messages_secure_.begin(); part != end; ++part) {
    write_participant_message_data_secure(part->first, part->second, reader);
  }
  participant_message_secure_writer_->end_historic_samples(reader);
}

DDS::ReturnCode_t
Sedp::write_stateless_message(const DDS::Security::ParticipantStatelessMessage& msg,
                              const RepoId& reader)
{
  DCPS::SequenceNumber sequence = DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN();
  return participant_stateless_message_writer_->write_stateless_message(msg, reader, sequence);
}

DDS::ReturnCode_t
Sedp::write_volatile_message(DDS::Security::ParticipantVolatileMessageSecure& msg,
                             const RepoId& reader)
{
  msg.message_identity.sequence_number = static_cast<unsigned long>(participant_volatile_message_secure_writer_->get_seq().getValue());
  DCPS::SequenceNumber sequence = DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN();
  return participant_volatile_message_secure_writer_->write_volatile_message_secure(msg, reader, sequence);
}

void
Sedp::write_durable_dcps_participant_secure(const DCPS::RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER)) {
    return;
  }

  write_dcps_participant_secure(
    spdp_.build_local_pdata(false, Security::DPDK_SECURE), reader);
  dcps_participant_secure_writer_->end_historic_samples(reader);
}

DDS::ReturnCode_t
Sedp::write_dcps_participant_secure(const Security::SPDPdiscoveredParticipantData& msg,
                                    const RepoId& part)
{
  DCPS::RepoId remote_reader(part);
  if (part != GUID_UNKNOWN) {
    remote_reader.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
  }

  return dcps_participant_secure_writer_->write_dcps_participant_secure(msg, remote_reader, participant_secure_sequence_);
}

DDS::ReturnCode_t
Sedp::write_dcps_participant_dispose(const RepoId& part)
{
  return dcps_participant_secure_writer_->write_unregister_dispose(part, PID_PARTICIPANT_GUID);
}
#endif

DDS::ReturnCode_t
Sedp::add_publication_i(const DCPS::RepoId& rid,
                        LocalPublication& pub)
{
  pub.participant_discovered_at_ = spdp_.get_participant_discovered_at();
  pub.transport_context_ = spdp_.config()->participant_flags();
#ifdef OPENDDS_SECURITY
  DCPS::DataWriterCallbacks_rch pl = pub.publication_.lock();
  if (pl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = pl->get_ice_endpoint();
    if (endpoint) {
      pub.have_ice_agent_info = true;
      pub.ice_agent_info = ice_agent_->get_local_agent_info(endpoint);
      ice_agent_->add_local_agent_info_listener(endpoint, rid, DCPS::static_rchandle_cast<ICE::AgentInfoListener>(publication_agent_info_listener_));
      start_ice(rid, pub);
    }
  }
#else
  ACE_UNUSED_ARG(rid);
  ACE_UNUSED_ARG(pub);
#endif
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
Sedp::write_publication_data(
  const RepoId& rid,
  LocalPublication& lp,
  const RepoId& reader)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;

#ifdef OPENDDS_SECURITY
  if (is_security_enabled() && lp.security_attribs_.base.is_discovery_protected) {
    result = write_publication_data_secure(rid, lp, reader);

  } else {
#endif

    result = write_publication_data_unsecure(rid, lp, reader);

#ifdef OPENDDS_SECURITY
  }
#endif

  return result;
}

DDS::ReturnCode_t
Sedp::write_publication_data_unsecure(
  const RepoId& rid,
  LocalPublication& lp,
  const RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER)) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {
    DCPS::DiscoveredWriterData dwd;
    ParameterList plist;
    populate_discovered_writer_msg(dwd, rid, lp);

    // Convert to parameter list
    if (!ParameterListConverter::to_param_list(dwd, plist, use_xtypes_, lp.type_info_, false)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data_unsecure - ")
                 ACE_TEXT("Failed to convert DiscoveredWriterData ")
                 ACE_TEXT(" to ParameterList\n")));
      result = DDS::RETCODE_ERROR;
    }
#ifdef OPENDDS_SECURITY
    if (lp.have_ice_agent_info) {
      ICE::AgentInfoMap ai_map;
      ai_map["DATA"] = lp.ice_agent_info;
      if (!ParameterListConverter::to_param_list(ai_map, plist)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data_unsecure - ")
                   ACE_TEXT("Failed to convert ICE Agent info ")
                   ACE_TEXT("to ParameterList\n")));
        result = DDS::RETCODE_ERROR;
      }
    }
#endif

    if (DDS::RETCODE_OK == result) {
      result = publications_writer_->write_parameter_list(plist, reader, lp.sequence_);
    }
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_publication_data_unsecure - ")
               ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}

#ifdef OPENDDS_SECURITY
DDS::ReturnCode_t
Sedp::write_publication_data_secure(
  const RepoId& rid,
  LocalPublication& lp,
  const RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER)) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {

    DiscoveredPublication_SecurityWrapper dwd;
    ParameterList plist;
    populate_discovered_writer_msg(dwd.data, rid, lp);

    dwd.security_info.endpoint_security_attributes = security_attributes_to_bitmask(lp.security_attribs_);
    dwd.security_info.plugin_endpoint_security_attributes = lp.security_attribs_.plugin_endpoint_attributes;

    // Convert to parameter list
    if (!ParameterListConverter::to_param_list(dwd, plist, use_xtypes_, lp.type_info_, false)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data_secure - ")
                 ACE_TEXT("Failed to convert DiscoveredWriterData ")
                 ACE_TEXT("to ParameterList\n")));
      result = DDS::RETCODE_ERROR;
    }
    if (lp.have_ice_agent_info) {
      ICE::AgentInfoMap ai_map;
      ai_map["DATA"] = lp.ice_agent_info;
      if (!ParameterListConverter::to_param_list(ai_map, plist)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data_secure - ")
                   ACE_TEXT("Failed to convert ICE Agent info ")
                   ACE_TEXT("to ParameterList\n")));
        result = DDS::RETCODE_ERROR;
      }
    }
    if (DDS::RETCODE_OK == result) {
      RepoId effective_reader = reader;
      if (reader != GUID_UNKNOWN)
        effective_reader.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
      result = publications_secure_writer_->write_parameter_list(plist, effective_reader, lp.sequence_);
    }
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_publication_data_secure - ")
               ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}
#endif

DDS::ReturnCode_t
Sedp::add_subscription_i(const DCPS::RepoId& rid,
                         LocalSubscription& sub)
{
  sub.participant_discovered_at_ = spdp_.get_participant_discovered_at();
  sub.transport_context_ = spdp_.config()->participant_flags();
#ifdef OPENDDS_SECURITY
  DCPS::DataReaderCallbacks_rch sl = sub.subscription_.lock();
  if (sl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = sl->get_ice_endpoint();
    if (endpoint) {
      sub.have_ice_agent_info = true;
      sub.ice_agent_info = ice_agent_->get_local_agent_info(endpoint);
      ice_agent_->add_local_agent_info_listener(endpoint, rid, DCPS::static_rchandle_cast<ICE::AgentInfoListener>(subscription_agent_info_listener_));
      start_ice(rid, sub);
    }
  }
#else
  ACE_UNUSED_ARG(rid);
  ACE_UNUSED_ARG(sub);
#endif
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
Sedp::write_subscription_data(
  const RepoId& rid,
  LocalSubscription& ls,
  const RepoId& reader)
{
  DDS::ReturnCode_t result = DDS::RETCODE_OK;

#ifdef OPENDDS_SECURITY
  if (is_security_enabled() && ls.security_attribs_.base.is_discovery_protected) {
    result = write_subscription_data_secure(rid, ls, reader);

  } else {
#endif

    result = write_subscription_data_unsecure(rid, ls, reader);

#ifdef OPENDDS_SECURITY
  }
#endif

  return result;
}

DDS::ReturnCode_t
Sedp::write_subscription_data_unsecure(
  const RepoId& rid,
  LocalSubscription& ls,
  const RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER)) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {
    DCPS::DiscoveredReaderData drd;
    ParameterList plist;
    populate_discovered_reader_msg(drd, rid, ls);

    // Convert to parameter list
    if (!ParameterListConverter::to_param_list(drd, plist, use_xtypes_, ls.type_info_, false)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::write_subscription_data_unsecure - ")
                 ACE_TEXT("Failed to convert DiscoveredReaderData ")
                 ACE_TEXT("to ParameterList\n")));
      result = DDS::RETCODE_ERROR;
    }

#ifdef OPENDDS_SECURITY
    if (ls.have_ice_agent_info) {
      ICE::AgentInfoMap ai_map;
      ai_map["DATA"] = ls.ice_agent_info;
      if (!ParameterListConverter::to_param_list(ai_map, plist)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::write_subscription_data_unsecure - ")
                   ACE_TEXT("Failed to convert ICE Agent info ")
                   ACE_TEXT("to ParameterList\n")));
        result = DDS::RETCODE_ERROR;
      }
    }
#endif
    if (DDS::RETCODE_OK == result) {
      result = subscriptions_writer_->write_parameter_list(plist, reader, ls.sequence_);
    }
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_subscription_data_unsecure - ")
               ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}

#ifdef OPENDDS_SECURITY
DDS::ReturnCode_t
Sedp::write_subscription_data_secure(
  const RepoId& rid,
  LocalSubscription& ls,
  const RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER)) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {

    DiscoveredSubscription_SecurityWrapper drd;
    ParameterList plist;
    populate_discovered_reader_msg(drd.data, rid, ls);

    drd.security_info.endpoint_security_attributes = security_attributes_to_bitmask(ls.security_attribs_);
    drd.security_info.plugin_endpoint_security_attributes = ls.security_attribs_.plugin_endpoint_attributes;

    // Convert to parameter list
    if (!ParameterListConverter::to_param_list(drd, plist, use_xtypes_, ls.type_info_, false)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::write_subscription_data_secure - ")
                 ACE_TEXT("Failed to convert DiscoveredReaderData ")
                 ACE_TEXT("to ParameterList\n")));
      result = DDS::RETCODE_ERROR;
    }
    if (ls.have_ice_agent_info) {
      ICE::AgentInfoMap ai_map;
      ai_map["DATA"] = ls.ice_agent_info;
      if (!ParameterListConverter::to_param_list(ai_map, plist)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::write_subscription_data_secure - ")
                   ACE_TEXT("Failed to convert ICE Agent info ")
                   ACE_TEXT("to ParameterList\n")));
        result = DDS::RETCODE_ERROR;
      }
    }
    if (DDS::RETCODE_OK == result) {
      RepoId effective_reader = reader;
      if (reader != GUID_UNKNOWN)
        effective_reader.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
      result = subscriptions_secure_writer_->write_parameter_list(plist, effective_reader, ls.sequence_);
    }
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_subscription_data_secure - ")
                        ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}
#endif

DDS::ReturnCode_t
Sedp::write_participant_message_data(
  const RepoId& rid,
  LocalParticipantMessage& pm,
  const RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER)) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  //Update liveliness for local subscriptions
  ParticipantMessageData pmd;
  pmd.participantGuid = rid;
  notify_liveliness(pmd);
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {
    result = participant_message_writer_->write_participant_message(pmd, reader, pm.sequence_);
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_participant_message_data - ")
               ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}

#ifdef OPENDDS_SECURITY
DDS::ReturnCode_t
Sedp::write_participant_message_data_secure(
  const RepoId& rid,
  LocalParticipantMessage& pm,
  const RepoId& reader)
{
  if (!(spdp_.available_builtin_endpoints() & DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER)) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  DDS::ReturnCode_t result = DDS::RETCODE_OK;
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {
    ParticipantMessageData pmd;
    pmd.participantGuid = rid;
    result = participant_message_secure_writer_->write_participant_message(pmd, reader, pm.sequence_);
  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_participant_message_data_secure - ")
               ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}
#endif

void
Sedp::set_inline_qos(DCPS::TransportLocatorSeq& locators)
{
  const OPENDDS_STRING rtps_udp = "rtps_udp";
  for (CORBA::ULong i = 0; i < locators.length(); ++i) {
    if (locators[i].transport_type.in() == rtps_udp) {
      DCPS::push_back(locators[i].data, CORBA::Octet(1));
    }
  }
}

bool
Sedp::shutting_down() const
{
  return spdp_.shutting_down();
}

void
Sedp::populate_transport_locator_sequence(DCPS::TransportLocatorSeq& rTls,
                                          DiscoveredSubscriptionIter& dsi,
                                          const RepoId& reader)
{
  DCPS::LocatorSeq locs;
  bool participantExpectsInlineQos = false;
  RepoId remote_participant = reader;
  remote_participant.entityId = ENTITYID_PARTICIPANT;
  const bool participant_found =
    spdp_.get_default_locators(remote_participant, locs,
                               participantExpectsInlineQos);
  if (!rTls.length()) {     // if no locators provided, add the default
    if (!participant_found) {
      return;
    } else if (locs.length()) {
      const Encoding& encoding = get_locators_encoding();
      size_t size = DCPS::serialized_size(encoding, locs);
      DCPS::primitive_serialized_size_boolean(encoding, size);

      ACE_Message_Block mb_locator(size);
      Serializer ser_loc(&mb_locator, encoding);
      ser_loc << locs;
      const bool readerExpectsInlineQos =
        dsi->second.reader_data_.readerProxy.expectsInlineQos;
      ser_loc << ACE_OutputCDR::from_boolean(participantExpectsInlineQos
                                             || readerExpectsInlineQos);

      DCPS::TransportLocator tl;
      tl.transport_type = "rtps_udp";
      message_block_to_sequence(mb_locator, tl.data);
      rTls.length(1);
      rTls[0] = tl;
    } else {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) Sedp::match - ")
                 ACE_TEXT("remote reader found with no locators ")
                 ACE_TEXT("and no default locators\n")));
    }
  }
}

void
Sedp::populate_transport_locator_sequence(DCPS::TransportLocatorSeq& wTls,
                                          DiscoveredPublicationIter& /*dpi*/,
                                          const RepoId& writer)
{
  DCPS::LocatorSeq locs;
  bool participantExpectsInlineQos = false;
  RepoId remote_participant = writer;
  remote_participant.entityId = ENTITYID_PARTICIPANT;
  const bool participant_found =
    spdp_.get_default_locators(remote_participant, locs,
                               participantExpectsInlineQos);
  if (!wTls.length()) {     // if no locators provided, add the default
    if (!participant_found) {
      return;
    } else if (locs.length()) {
      const Encoding& encoding = get_locators_encoding();
      size_t size = DCPS::serialized_size(encoding, locs);
      DCPS::primitive_serialized_size_boolean(encoding, size);

      ACE_Message_Block mb_locator(size);
      Serializer ser_loc(&mb_locator, encoding);
      ser_loc << locs;
      ser_loc << ACE_OutputCDR::from_boolean(participantExpectsInlineQos);

      DCPS::TransportLocator tl;
      tl.transport_type = "rtps_udp";
      message_block_to_sequence(mb_locator, tl.data);
      wTls.length(1);
      wTls[0] = tl;
    } else {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) Sedp::match - ")
                 ACE_TEXT("remote writer found with no locators ")
                 ACE_TEXT("and no default locators\n")));
    }
  }
}

#ifdef OPENDDS_SECURITY
DDS::Security::DatawriterCryptoHandle
Sedp::generate_remote_matched_writer_crypto_handle(const RepoId& writer,
                                                   const RepoId& reader)
{
  DDS::Security::DatawriterCryptoHandle result = get_handle_registry()->get_remote_datawriter_crypto_handle(writer);
  if (result != DDS::HANDLE_NIL) {
    return result;
  }

  const RepoId writer_part = make_id(writer, ENTITYID_PARTICIPANT);
  Spdp::ParticipantCryptoInfoPair info = spdp_.lookup_participant_crypto_info(writer_part);

  if (info.first != DDS::HANDLE_NIL && info.second) {
    const DDS::Security::DatareaderCryptoHandle drch =
      get_handle_registry()->get_local_datareader_crypto_handle(reader);
    const DDS::Security::EndpointSecurityAttributes attribs =
      get_handle_registry()->get_local_datareader_security_attributes(reader);
    DDS::Security::CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();
    DDS::Security::SecurityException se = {"", 0, 0};
    result = key_factory->register_matched_remote_datawriter(drch, info.first, info.second, se);
    get_handle_registry()->insert_remote_datawriter_crypto_handle(writer, result, attribs);
    if (result == DDS::HANDLE_NIL) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::generate_remote_matched_writer_crypto_handle() - ")
        ACE_TEXT("Failure calling register_matched_remote_datawriter(). Security Exception[%d.%d]: %C\n"),
          se.code, se.minor_code, se.message.in()));
    }
  } else {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::generate_remote_matched_writer_crypto_handle() - ")
      ACE_TEXT("Unable to lookup remote participant crypto info.\n")));
  }
  return result;
}

DDS::Security::DatareaderCryptoHandle
Sedp::generate_remote_matched_reader_crypto_handle(const RepoId& reader,
                                                   const RepoId& writer,
                                                   bool relay_only)
{
  DDS::Security::DatareaderCryptoHandle result = get_handle_registry()->get_remote_datareader_crypto_handle(reader);
  if (result != DDS::HANDLE_NIL) {
    return result;
  }

  const RepoId reader_part = make_id(reader, ENTITYID_PARTICIPANT);
  Spdp::ParticipantCryptoInfoPair info = spdp_.lookup_participant_crypto_info(reader_part);

  if (info.first != DDS::HANDLE_NIL && info.second) {
    const DDS::Security::DatawriterCryptoHandle dwch =
      get_handle_registry()->get_local_datawriter_crypto_handle(writer);
    const DDS::Security::EndpointSecurityAttributes attribs =
      get_handle_registry()->get_local_datawriter_security_attributes(writer);
    DDS::Security::CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();
    DDS::Security::SecurityException se = {"", 0, 0};
    result = key_factory->register_matched_remote_datareader(dwch, info.first, info.second, relay_only, se);
    get_handle_registry()->insert_remote_datareader_crypto_handle(reader, result, attribs);
    if (result == DDS::HANDLE_NIL) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::generate_remote_matched_reader_crypto_handle() - ")
        ACE_TEXT("Failure calling register_matched_remote_datareader(). Security Exception[%d.%d]: %C\n"),
          se.code, se.minor_code, se.message.in()));
    }
  } else {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::generate_remote_matched_reader_crypto_handle() - ")
      ACE_TEXT("Unable to lookup remote participant crypto info.\n")));
  }
  return result;
}

void
Sedp::create_datareader_crypto_tokens(const DDS::Security::DatareaderCryptoHandle& drch,
                                      const DDS::Security::DatawriterCryptoHandle& dwch,
                                      DDS::Security::DatareaderCryptoTokenSeq& drcts)
{
  DDS::Security::SecurityException se = {"", 0, 0};
  DDS::Security::CryptoKeyExchange_var key_exchange = spdp_.get_security_config()->get_crypto_key_exchange();

  if (!key_exchange->create_local_datareader_crypto_tokens(drcts, drch, dwch, se)) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("Sedp::create_datareader_crypto_tokens() - ")
      ACE_TEXT("Unable to create local datareader crypto tokens with crypto key exchange plugin. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"), se.code, se.minor_code, se.message.in()));
  }
}

void
Sedp::send_datareader_crypto_tokens(const RepoId& local_reader,
                                    const RepoId& remote_writer,
                                    const DDS::Security::DatareaderCryptoTokenSeq& drcts)
{
  if (drcts.length() != 0) {
    const DCPS::RepoId remote_part = make_id(remote_writer, ENTITYID_PARTICIPANT);
    const DCPS::RepoId local_volatile_writer = make_id(
      participant_id_, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER);
    const DCPS::RepoId remote_volatile_reader = make_id(
      remote_part, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER);

    DDS::Security::ParticipantVolatileMessageSecure msg;
    msg.message_identity.source_guid = local_volatile_writer;
    msg.message_class_id = DDS::Security::GMCLASSID_SECURITY_DATAREADER_CRYPTO_TOKENS;
    msg.destination_participant_guid = remote_part;
    msg.destination_endpoint_guid = remote_writer;
    msg.source_endpoint_guid = local_reader;
    msg.message_data = reinterpret_cast<const DDS::Security::DataHolderSeq&>(drcts);
    msg.related_message_identity = DDS::Security::MessageIdentity();

    if (write_volatile_message(msg, remote_volatile_reader) != DDS::RETCODE_OK) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::send_datareader_crypto_tokens() - ")
        ACE_TEXT("Unable to write volatile message.\n")));
    }
  }
}

void
Sedp::create_datawriter_crypto_tokens(const DDS::Security::DatawriterCryptoHandle& dwch,
                                      const DDS::Security::DatareaderCryptoHandle& drch,
                                      DDS::Security::DatawriterCryptoTokenSeq& dwcts)
{
  DDS::Security::SecurityException se = {"", 0, 0};
  DDS::Security::CryptoKeyExchange_var key_exchange = spdp_.get_security_config()->get_crypto_key_exchange();

  if (key_exchange->create_local_datawriter_crypto_tokens(dwcts, dwch, drch, se) == false) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("Sedp::create_datawriter_crypto_tokens() - ")
      ACE_TEXT("Unable to create local datawriter crypto tokens with crypto key exchange plugin. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"), se.code, se.minor_code, se.message.in()));
  }
}

void
Sedp::send_datawriter_crypto_tokens(const RepoId& local_writer,
                                    const RepoId& remote_reader,
                                    const DDS::Security::DatawriterCryptoTokenSeq& dwcts)
{
  if (dwcts.length() != 0) {
    const DCPS::RepoId remote_part = make_id(remote_reader, ENTITYID_PARTICIPANT);
    const DCPS::RepoId local_volatile_writer = make_id(
      participant_id_, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER);
    const DCPS::RepoId remote_volatile_reader = make_id(
      remote_part, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER);

    DDS::Security::ParticipantVolatileMessageSecure msg;
    msg.message_identity.source_guid = local_volatile_writer;
    msg.message_class_id = DDS::Security::GMCLASSID_SECURITY_DATAWRITER_CRYPTO_TOKENS;
    msg.destination_participant_guid = remote_part;
    msg.destination_endpoint_guid = remote_reader;
    msg.source_endpoint_guid = local_writer;
    msg.message_data = reinterpret_cast<const DDS::Security::DataHolderSeq&>(dwcts);
    msg.related_message_identity = DDS::Security::MessageIdentity();

    if (write_volatile_message(msg, remote_volatile_reader) != DDS::RETCODE_OK) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::send_datawriter_crypto_tokens() - ")
        ACE_TEXT("Unable to write volatile message.\n")));
    }
  }
}

bool
Sedp::handle_datawriter_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg) {
  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Sedp::handle_datawriter_crypto_tokens() %C\n"),
               DCPS::LogGuid(msg.source_endpoint_guid).c_str()));
  }

  DDS::Security::SecurityException se = {"", 0, 0};
  Security::CryptoKeyExchange_var key_exchange = spdp_.get_security_config()->get_crypto_key_exchange();

  ACE_Guard<ACE_Thread_Mutex> g(lock_, false);

  const DDS::Security::DatawriterCryptoHandle dwch =
    get_handle_registry()->get_remote_datawriter_crypto_handle(msg.source_endpoint_guid);
  const DDS::Security::DatareaderCryptoHandle drch =
    get_handle_registry()->get_local_datareader_crypto_handle(msg.destination_endpoint_guid);

  DDS::Security::DatawriterCryptoTokenSeq dwcts;
  dwcts = reinterpret_cast<const DDS::Security::DatawriterCryptoTokenSeq&>(msg.message_data);

  if (dwch == DDS::HANDLE_NIL) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Sedp::handle_datawriter_crypto_tokens() - ")
        ACE_TEXT("received tokens for unknown remote writer %C Caching.\n"),
        DCPS::LogGuid(msg.source_endpoint_guid).c_str()));
    }

    pending_remote_writer_crypto_tokens_[msg.source_endpoint_guid] = dwcts;
    return true;
  }

  if (drch == DDS::HANDLE_NIL) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) Sedp::handle_datawriter_crypto_tokens() - ")
        ACE_TEXT("received tokens for unknown local reader. Ignoring.\n")));
    }
    return false;
  }

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Sedp::handle_datawriter_crypto_tokens() from %C drch %d dwch %d count %d\n"),
               DCPS::LogGuid(msg.source_endpoint_guid).c_str(), drch, dwch, dwcts.length()));
  }

  if (!key_exchange->set_remote_datawriter_crypto_tokens(drch, dwch, dwcts, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::handle_datawriter_crypto_tokens() - ")
      ACE_TEXT("Unable to set remote datawriter crypto tokens with crypto key exchange plugin. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"), se.code, se.minor_code, se.message.in()));
    return false;
  }

  Spdp::DiscoveredParticipantIter iter = spdp_.participants_.find(make_id(msg.source_endpoint_guid, ENTITYID_PARTICIPANT));
  if (iter != spdp_.participants_.end()) {
    process_association_records_i(iter->second);
  }

  return true;
}

bool
Sedp::handle_datareader_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg) {
  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Sedp::handle_datareader_crypto_tokens() %C\n"),
               DCPS::LogGuid(msg.source_endpoint_guid).c_str()));
  }

  DDS::Security::SecurityException se = {"", 0, 0};
  Security::CryptoKeyExchange_var key_exchange = spdp_.get_security_config()->get_crypto_key_exchange();

  ACE_Guard<ACE_Thread_Mutex> g(lock_, false);

  const DDS::Security::DatareaderCryptoHandle drch =
    get_handle_registry()->get_remote_datareader_crypto_handle(msg.source_endpoint_guid);
  const DDS::Security::DatawriterCryptoHandle dwch =
    get_handle_registry()->get_local_datawriter_crypto_handle(msg.destination_endpoint_guid);

  DDS::Security::DatareaderCryptoTokenSeq drcts;
  drcts = reinterpret_cast<const DDS::Security::DatareaderCryptoTokenSeq&>(msg.message_data);

  if (drch == DDS::HANDLE_NIL) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Sedp::handle_datareader_crypto_tokens() - ")
        ACE_TEXT("received tokens for unknown remote reader %C Caching.\n"),
        DCPS::LogGuid(msg.source_endpoint_guid).c_str()));
    }

    pending_remote_reader_crypto_tokens_[msg.source_endpoint_guid] = drcts;
    return true;
  }

  if (dwch == DDS::HANDLE_NIL) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) Sedp::handle_datareader_crypto_tokens() - ")
        ACE_TEXT("received tokens for unknown local writer. Ignoring.\n")));
    }
    return false;
  }

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Sedp::handle_datareader_crypto_tokens() from %C dwch %d drch %d count %d\n"),
               DCPS::LogGuid(msg.source_endpoint_guid).c_str(), dwch, drch, drcts.length()));
  }

  if (!key_exchange->set_remote_datareader_crypto_tokens(dwch, drch, drcts, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Sedp::handle_datareader_crypto_tokens() - ")
      ACE_TEXT("Unable to set remote datareader crypto tokens with crypto key exchange plugin. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"), se.code, se.minor_code, se.message.in()));
    return false;
  }

  Spdp::DiscoveredParticipantIter iter = spdp_.participants_.find(make_id(msg.source_endpoint_guid, ENTITYID_PARTICIPANT));
  if (iter != spdp_.participants_.end()) {
    process_association_records_i(iter->second);
  }

  return true;
}

DDS::DomainId_t Sedp::get_domain_id() const {
  return spdp_.get_domain_id();
}

void Sedp::resend_user_crypto_tokens(const RepoId& id)
{
  const RepoId remote_participant = make_id(id, ENTITYID_PARTICIPANT);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("Sedp::resend_user_crypto_tokens(%C)\n"),
               DCPS::LogGuid(remote_participant).c_str()));
  }

  /*
   * For each user DataReader that has a crypto handle, send tokens to matched
   * DataWriters that have a crypto handle.
   */

  for (LocalSubscriptionCIter reader_pos = local_subscriptions_.begin(),
         reader_limit = local_subscriptions_.end();
       reader_pos != reader_limit; ++reader_pos) {
    if (!DCPS::GuidConverter(reader_pos->first).isUserDomainEntity()) {
      continue;
    }
    const DDS::Security::DatareaderCryptoHandle drch =
      get_handle_registry()->get_local_datareader_crypto_handle(reader_pos->first);
    if (drch == DDS::HANDLE_NIL) {
      continue;
    }
    for (DCPS::RepoIdSet::const_iterator writer_pos = reader_pos->second.matched_endpoints_.begin(),
           writer_limit = reader_pos->second.matched_endpoints_.end();
         writer_pos != writer_limit; ++writer_pos) {
      if (!DCPS::equal_guid_prefixes(*writer_pos, remote_participant)) {
        continue;
      }
      const DDS::Security::DatawriterCryptoHandle dwch =
        get_handle_registry()->get_remote_datawriter_crypto_handle(*writer_pos);
      if (dwch == DDS::HANDLE_NIL) {
        continue;
      }
      create_and_send_datareader_crypto_tokens(drch, reader_pos->first, dwch, *writer_pos);
    }
  }

  /*
   * For each user DataWriter that has a crypto handle, send tokens to matched
   * DataReaders that have a crypto handle.
   */
  for (LocalPublicationCIter writer_pos = local_publications_.begin(),
         writer_limit = local_publications_.end();
       writer_pos != writer_limit; ++writer_pos) {
    if (!DCPS::GuidConverter(writer_pos->first).isUserDomainEntity()) {
      continue;
    }
    const DDS::Security::DatawriterCryptoHandle dwch =
      get_handle_registry()->get_local_datawriter_crypto_handle(writer_pos->first);
    if (dwch == DDS::HANDLE_NIL) {
      continue;
    }
    for (DCPS::RepoIdSet::const_iterator reader_pos = writer_pos->second.matched_endpoints_.begin(),
           reader_limit = writer_pos->second.matched_endpoints_.end();
         reader_pos != reader_limit; ++reader_pos) {
      if (!DCPS::equal_guid_prefixes(*reader_pos, remote_participant)) {
        continue;
      }
      const DDS::Security::DatareaderCryptoHandle drch =
        get_handle_registry()->get_remote_datareader_crypto_handle(*reader_pos);
      if (drch == DDS::HANDLE_NIL) {
        continue;
      }
      create_and_send_datawriter_crypto_tokens(dwch, writer_pos->first, drch, *reader_pos);
    }
  }
}
#endif

void
Sedp::add_assoc_i(const DCPS::RepoId& local_guid, const LocalPublication& lpub,
                  const DCPS::RepoId& remote_guid, const DiscoveredSubscription& dsub) {
#ifdef OPENDDS_SECURITY
  DCPS::DataWriterCallbacks_rch pl = lpub.publication_.lock();
  if (pl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = pl->get_ice_endpoint();
    if (endpoint && dsub.have_ice_agent_info_) {
      ice_agent_->start_ice(endpoint, local_guid, remote_guid, dsub.ice_agent_info_);
    }
  }
#else
  ACE_UNUSED_ARG(local_guid);
  ACE_UNUSED_ARG(lpub);
  ACE_UNUSED_ARG(remote_guid);
  ACE_UNUSED_ARG(dsub);
#endif
}

void
Sedp::remove_assoc_i(const DCPS::RepoId& local_guid, const LocalPublication& lpub,
                     const DCPS::RepoId& remote_guid) {
#ifdef OPENDDS_SECURITY
  DCPS::DataWriterCallbacks_rch pl = lpub.publication_.lock();
  if (pl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = pl->get_ice_endpoint();
    if (endpoint) {
      ice_agent_->stop_ice(endpoint, local_guid, remote_guid);
    }
  }
#else
  ACE_UNUSED_ARG(local_guid);
  ACE_UNUSED_ARG(lpub);
  ACE_UNUSED_ARG(remote_guid);
#endif
}

void
Sedp::add_assoc_i(const DCPS::RepoId& local_guid, const LocalSubscription& lsub,
                  const DCPS::RepoId& remote_guid, const DiscoveredPublication& dpub) {
#ifdef OPENDDS_SECURITY
  DCPS::DataReaderCallbacks_rch sl = lsub.subscription_.lock();
  if (sl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = sl->get_ice_endpoint();
    if (endpoint && dpub.have_ice_agent_info_) {
      ice_agent_->start_ice(endpoint, local_guid, remote_guid, dpub.ice_agent_info_);
    }
  }
#else
  ACE_UNUSED_ARG(local_guid);
  ACE_UNUSED_ARG(lsub);
  ACE_UNUSED_ARG(remote_guid);
  ACE_UNUSED_ARG(dpub);
#endif
}

void
Sedp::remove_assoc_i(const DCPS::RepoId& local_guid, const LocalSubscription& lsub,
                     const DCPS::RepoId& remote_guid) {
#ifdef OPENDDS_SECURITY
  DCPS::DataReaderCallbacks_rch sl = lsub.subscription_.lock();
  if (sl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = sl->get_ice_endpoint();
    if (endpoint) {
      ice_agent_->stop_ice(endpoint, local_guid, remote_guid);
    }
  }
#else
  ACE_UNUSED_ARG(local_guid);
  ACE_UNUSED_ARG(lsub);
  ACE_UNUSED_ARG(remote_guid);
#endif
}

#ifdef OPENDDS_SECURITY
void
Sedp::PublicationAgentInfoListener::update_agent_info(const DCPS::RepoId& a_local_guid,
                                                      const ICE::AgentInfo& a_agent_info)
{
  ACE_GUARD(ACE_Thread_Mutex, g, sedp.lock_);
  LocalPublicationIter pos = sedp.local_publications_.find(a_local_guid);
  if (pos != sedp.local_publications_.end()) {
    pos->second.have_ice_agent_info = true;
    pos->second.ice_agent_info = a_agent_info;
    sedp.write_publication_data(a_local_guid, pos->second);
  }
}

void
Sedp::PublicationAgentInfoListener::remove_agent_info(const DCPS::RepoId& a_local_guid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, sedp.lock_);
  LocalPublicationIter pos = sedp.local_publications_.find(a_local_guid);
  if (pos != sedp.local_publications_.end()) {
    pos->second.have_ice_agent_info = false;
    sedp.write_publication_data(a_local_guid, pos->second);
  }
}

void
Sedp::SubscriptionAgentInfoListener::update_agent_info(const DCPS::RepoId& a_local_guid,
                                                       const ICE::AgentInfo& a_agent_info)
{
  ACE_GUARD(ACE_Thread_Mutex, g, sedp.lock_);
  LocalSubscriptionIter pos = sedp.local_subscriptions_.find(a_local_guid);
  if (pos != sedp.local_subscriptions_.end()) {
    pos->second.have_ice_agent_info = true;
    pos->second.ice_agent_info = a_agent_info;
    sedp.write_subscription_data(a_local_guid, pos->second);
  }
}

void
Sedp::SubscriptionAgentInfoListener::remove_agent_info(const DCPS::RepoId& a_local_guid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, sedp.lock_);
  LocalSubscriptionIter pos = sedp.local_subscriptions_.find(a_local_guid);
  if (pos != sedp.local_subscriptions_.end()) {
    pos->second.have_ice_agent_info = false;
    sedp.write_subscription_data(a_local_guid, pos->second);
  }
}
#endif

void
Sedp::start_ice(const DCPS::RepoId& guid, const LocalPublication& lpub) {
#ifdef OPENDDS_SECURITY
  DCPS::DataWriterCallbacks_rch pl = lpub.publication_.lock();
  if (pl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = pl->get_ice_endpoint();

    if (!endpoint || !lpub.have_ice_agent_info) {
      return;
    }

    for (DCPS::RepoIdSet::const_iterator it = lpub.matched_endpoints_.begin(),
           end = lpub.matched_endpoints_.end(); it != end; ++it) {
      DiscoveredSubscriptionIter dsi = discovered_subscriptions_.find(*it);
      if (dsi != discovered_subscriptions_.end()) {
        if (dsi->second.have_ice_agent_info_) {
          ice_agent_->start_ice(endpoint, guid, dsi->first, dsi->second.ice_agent_info_);
        }
      }
    }
  }
#else
  ACE_UNUSED_ARG(guid);
  ACE_UNUSED_ARG(lpub);
#endif
}

void
Sedp::start_ice(const DCPS::RepoId& guid, const LocalSubscription& lsub) {
#ifdef OPENDDS_SECURITY
  DCPS::DataReaderCallbacks_rch sl = lsub.subscription_.lock();
  if (sl) {
    DCPS::WeakRcHandle<ICE::Endpoint> endpoint = sl->get_ice_endpoint();

    if (!endpoint || !lsub.have_ice_agent_info) {
      return;
    }

    for (DCPS::RepoIdSet::const_iterator it = lsub.matched_endpoints_.begin(),
           end = lsub.matched_endpoints_.end(); it != end; ++it) {
      DiscoveredPublicationIter dpi = discovered_publications_.find(*it);
      if (dpi != discovered_publications_.end()) {
        if (dpi->second.have_ice_agent_info_) {
          ice_agent_->start_ice(endpoint, guid, dpi->first, dpi->second.ice_agent_info_);
        }
      }
    }
  }
#else
  ACE_UNUSED_ARG(guid);
  ACE_UNUSED_ARG(lsub);
#endif
}

void
Sedp::start_ice(const DCPS::RepoId& guid, const DiscoveredPublication& dpub) {
#ifdef OPENDDS_SECURITY
  if (!dpub.have_ice_agent_info_) {
    return;
  }

  for (DCPS::RepoIdSet::const_iterator it = dpub.matched_endpoints_.begin(),
       end = dpub.matched_endpoints_.end(); it != end; ++it) {
    LocalSubscriptionIter lsi = local_subscriptions_.find(*it);
    if (lsi != local_subscriptions_.end() &&
        lsi->second.matched_endpoints_.count(guid)) {
      DCPS::DataReaderCallbacks_rch sl = lsi->second.subscription_.lock();
      if (sl) {
        DCPS::WeakRcHandle<ICE::Endpoint> endpoint = sl->get_ice_endpoint();
        if (endpoint) {
          ice_agent_->start_ice(endpoint, lsi->first, guid, dpub.ice_agent_info_);
        }
      }
    }
  }
#else
  ACE_UNUSED_ARG(guid);
  ACE_UNUSED_ARG(dpub);
#endif
}

void
Sedp::start_ice(const DCPS::RepoId& guid, const DiscoveredSubscription& dsub) {
#ifdef OPENDDS_SECURITY
  if (!dsub.have_ice_agent_info_) {
    return;
  }

  for (DCPS::RepoIdSet::const_iterator it = dsub.matched_endpoints_.begin(),
       end = dsub.matched_endpoints_.end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isWriter()) {
      LocalPublicationIter lpi = local_publications_.find(*it);
      if (lpi != local_publications_.end() &&
          lpi->second.matched_endpoints_.count(guid)) {
        DCPS::DataWriterCallbacks_rch pl = lpi->second.publication_.lock();
        if (pl) {
          DCPS::WeakRcHandle<ICE::Endpoint> endpoint = pl->get_ice_endpoint();
          if (endpoint) {
            ice_agent_->start_ice(endpoint, lpi->first, guid, dsub.ice_agent_info_);
          }
        }
      }
    }
  }
#else
  ACE_UNUSED_ARG(guid);
  ACE_UNUSED_ARG(dsub);
#endif
}

void
Sedp::stop_ice(const DCPS::RepoId& guid, const DiscoveredPublication& dpub)
{
#ifdef OPENDDS_SECURITY
  for (DCPS::RepoIdSet::const_iterator it = dpub.matched_endpoints_.begin(),
       end = dpub.matched_endpoints_.end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isReader()) {
      LocalSubscriptionIter lsi = local_subscriptions_.find(*it);
      if (lsi != local_subscriptions_.end() &&
          lsi->second.matched_endpoints_.count(guid)) {
        DCPS::DataReaderCallbacks_rch sl = lsi->second.subscription_.lock();
        if (sl) {
          DCPS::WeakRcHandle<ICE::Endpoint> endpoint = sl->get_ice_endpoint();
          if (endpoint) {
            ice_agent_->stop_ice(endpoint, lsi->first, guid);
          }
        }
      }
    }
  }
#else
  ACE_UNUSED_ARG(guid);
  ACE_UNUSED_ARG(dpub);
#endif
}

void
Sedp::stop_ice(const DCPS::RepoId& guid, const DiscoveredSubscription& dsub)
{
#ifdef OPENDDS_SECURITY
  for (DCPS::RepoIdSet::const_iterator it = dsub.matched_endpoints_.begin(),
       end = dsub.matched_endpoints_.end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isWriter()) {
      LocalPublicationIter lpi = local_publications_.find(*it);
      if (lpi != local_publications_.end() &&
          lpi->second.matched_endpoints_.count(guid)) {
        DCPS::DataWriterCallbacks_rch pl = lpi->second.publication_.lock();
        if (pl) {
          DCPS::WeakRcHandle<ICE::Endpoint> endpoint = pl->get_ice_endpoint();
          if (endpoint) {
            ice_agent_->stop_ice(endpoint, lpi->first, guid);
          }
        }
      }
    }
  }
#else
  ACE_UNUSED_ARG(guid);
  ACE_UNUSED_ARG(dsub);
#endif
}

void
Sedp::rtps_relay_only_now(bool f)
{
  transport_inst_->rtps_relay_only_now(f);
}

void
Sedp::use_rtps_relay_now(bool f)
{
  transport_inst_->use_rtps_relay_now(f);
}

void
Sedp::use_ice_now(bool f)
{
  transport_inst_->use_ice_now(f);

#ifdef OPENDDS_SECURITY
  if (!f) {
    return;
  }

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  for (LocalPublicationIter pos = local_publications_.begin(), limit = local_publications_.end(); pos != limit; ++pos) {
    LocalPublication& pub = pos->second;
    DCPS::DataWriterCallbacks_rch pl = pub.publication_.lock();
    if (pl) {
      DCPS::WeakRcHandle<ICE::Endpoint> endpoint = pl->get_ice_endpoint();
      if (endpoint) {
        pub.have_ice_agent_info = true;
        pub.ice_agent_info = ice_agent_->get_local_agent_info(endpoint);
        ice_agent_->add_local_agent_info_listener(endpoint, pos->first, DCPS::static_rchandle_cast<ICE::AgentInfoListener>(publication_agent_info_listener_));
        start_ice(pos->first, pub);
      }
    }
  }
  for (LocalSubscriptionIter pos = local_subscriptions_.begin(), limit = local_subscriptions_.end(); pos != limit; ++pos) {
    LocalSubscription& sub = pos->second;
    DCPS::DataReaderCallbacks_rch sl = sub.subscription_.lock();
    if (sl) {
      DCPS::WeakRcHandle<ICE::Endpoint> endpoint = sl->get_ice_endpoint();
      if (endpoint) {
        sub.have_ice_agent_info = true;
        sub.ice_agent_info = ice_agent_->get_local_agent_info(endpoint);
        ice_agent_->add_local_agent_info_listener(endpoint, pos->first, DCPS::static_rchandle_cast<ICE::AgentInfoListener>(subscription_agent_info_listener_));
        start_ice(pos->first, sub);
      }
    }
  }
#endif
}

void
Sedp::rtps_relay_address(const ACE_INET_Addr& address)
{
  DCPS::RtpsUdpInst_rch rtps_inst = DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  {
    ACE_GUARD(ACE_Thread_Mutex, g, rtps_inst->config_lock_);
    rtps_inst->rtps_relay_address_ = address;
  }
  rtps_inst->rtps_relay_address_change();
}

void
Sedp::stun_server_address(const ACE_INET_Addr& address)
{
  DCPS::RtpsUdpInst_rch rtps_inst = DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  ACE_GUARD(ACE_Thread_Mutex, g, rtps_inst->config_lock_);
  rtps_inst->stun_server_address_ = address;
}

void
Sedp::append_transport_statistics(DCPS::TransportStatisticsSequence& seq)
{
  transport_inst_->append_transport_statistics(seq);
}

bool locators_changed(const ParticipantProxy_t& x,
                      const ParticipantProxy_t& y)
{
  return locatorsChanged(x, y);
}

void Sedp::ignore(const GUID_t& to_ignore)
{
  // Locked prior to call from Spdp.
  ignored_guids_.insert(to_ignore);
  {
    const DiscoveredPublicationIter iter =
      discovered_publications_.find(to_ignore);
    if (iter != discovered_publications_.end()) {
      // clean up tracking info
      const String topic_name = iter->second.get_topic_name();
      DCPS::TopicDetails& td = topics_[topic_name];
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
      DCPS::TopicDetails& td = topics_[topic_name];
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
    const OPENDDS_MAP_CMP(GUID_t, String, GUID_tKeyLessThan)::iterator
      iter = topic_names_.find(to_ignore);
    if (iter != topic_names_.end()) {
      ignored_topics_.insert(iter->second);
      // Remove all publications and subscriptions on this topic
      DCPS::TopicDetails& td = topics_[iter->second];
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

DCPS::TopicStatus Sedp::assert_topic(
  GUID_t& topicId, const char* topicName,
  const char* dataTypeName, const DDS::TopicQos& qos,
  bool hasDcpsKey, DCPS::TopicCallbacks* topic_callbacks)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  DCPS::TopicDetailsMap::iterator iter = topics_.find(topicName);
  if (iter != topics_.end()) {
    if (iter->second.local_is_set() && iter->second.local_data_type_name() != dataTypeName) {
      return DCPS::CONFLICTING_TYPENAME;
    }
    topicId = iter->second.topic_id();
    iter->second.set_local(dataTypeName, qos, hasDcpsKey, topic_callbacks);
    return DCPS::FOUND;
  }

  DCPS::TopicDetails& td = topics_[topicName];
  topicId = make_topic_guid();
  td.init(topicName, topicId);
  topic_names_[topicId] = topicName;
  td.set_local(dataTypeName, qos, hasDcpsKey, topic_callbacks);

  return DCPS::CREATED;
}

DCPS::TopicStatus Sedp::find_topic(
  const char* topicName,
  CORBA::String_out dataTypeName,
  DDS::TopicQos_out qos,
  GUID_t& topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  DCPS::TopicDetailsMap::const_iterator iter = topics_.find(topicName);
  if (iter == topics_.end()) {
    return DCPS::NOT_FOUND;
  }

  const DCPS::TopicDetails& td = iter->second;

  dataTypeName = td.local_data_type_name().c_str();
  qos = new DDS::TopicQos(td.local_qos());
  topicId = td.topic_id();
  return DCPS::FOUND;
}

DCPS::TopicStatus Sedp::remove_topic(const GUID_t& topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  TopicNameMap::iterator name_iter = topic_names_.find(topicId);
  if (name_iter == topic_names_.end()) {
    return DCPS::NOT_FOUND;
  }
  const String& name = name_iter->second;
  DCPS::TopicDetails& td = topics_[name];
  td.unset_local();
  if (td.is_dead()) {
    purge_dead_topic(name);
  }

  return DCPS::REMOVED;
}

GUID_t Sedp::add_publication(
  const GUID_t& topicId,
  DCPS::DataWriterCallbacks_rch publication,
  const DDS::DataWriterQos& qos,
  const DCPS::TransportLocatorSeq& transInfo,
  const DDS::PublisherQos& publisherQos,
  const XTypes::TypeInformation& type_info)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, GUID_t());

  GUID_t rid = participant_id_;
  assign_publication_key(rid, topicId, qos);
  LocalPublication& pb = local_publications_[rid];
  pb.topic_id_ = topicId;
  pb.publication_ = publication;
  pb.qos_ = qos;
  pb.trans_info_ = transInfo;
  pb.publisher_qos_ = publisherQos;
  pb.type_info_ = type_info;
  const String& topic_name = topic_names_[topicId];

#ifdef OPENDDS_SECURITY
  if (is_security_enabled()) {
    DDS::Security::SecurityException ex;

    DDS::Security::TopicSecurityAttributes topic_sec_attr;
    DDS::Security::PermissionsHandle permh = get_permissions_handle();
    if (!get_access_control()->get_topic_sec_attributes(permh, topic_name.data(), topic_sec_attr, ex)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Sedp::add_publication: ")
        ACE_TEXT("Unable to get security attributes for topic '%C'. SecurityException[%d.%d]: %C\n"),
          topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
      return GUID_t();
    }

    if (topic_sec_attr.is_write_protected == true) {
      if (!get_access_control()->check_create_datawriter(
            permh, get_domain_id(), topic_name.data(), qos,
            publisherQos.partition, DDS::Security::DataTagQosPolicy(), ex)) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
          ACE_TEXT("Sedp::add_publication: ")
          ACE_TEXT("Permissions check failed for local datawriter on topic '%C'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), topic_name.data(),
            ex.code, ex.minor_code, ex.message.in()));
        return GUID_t();
      }
    }

    if (!get_access_control()->get_datawriter_sec_attributes(permh, topic_name.data(),
        publisherQos.partition, DDS::Security::DataTagQosPolicy(), pb.security_attribs_, ex)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("Sedp::add_publication: ")
                 ACE_TEXT("Unable to get security attributes for local datawriter. ")
                 ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                 ex.code, ex.minor_code, ex.message.in()));
      return GUID_t();
    }

    if (pb.security_attribs_.is_submessage_protected || pb.security_attribs_.is_payload_protected) {
      const DDS::Security::DatawriterCryptoHandle handle =
        get_crypto_key_factory()->register_local_datawriter(
          crypto_handle_, DDS::PropertySeq(), pb.security_attribs_, ex);
      if (handle == DDS::HANDLE_NIL) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("Sedp::add_publication: ")
                   ACE_TEXT("Unable to get local datawriter crypto handle. ")
                   ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                   ex.code, ex.minor_code, ex.message.in()));
      }

      get_handle_registry()->insert_local_datawriter_crypto_handle(rid, handle, pb.security_attribs_);
    }
  }
#endif

  DCPS::TopicDetails& td = topics_[topic_name];
  td.add_local_publication(rid);

  if (DDS::RETCODE_OK != add_publication_i(rid, pb)) {
    return GUID_t();
  }

  if (DDS::RETCODE_OK != write_publication_data(rid, pb)) {
    return GUID_t();
  }

  if (DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::add_publication: ")
               ACE_TEXT("calling match_endpoints\n")));
  }
  match_endpoints(rid, td);

  return rid;
}

void Sedp::remove_publication(const GUID_t& publicationId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationIter iter = local_publications_.find(publicationId);
  if (iter != local_publications_.end()) {
    if (DDS::RETCODE_OK == remove_publication_i(publicationId, iter->second)) {
      String topic_name = topic_names_[iter->second.topic_id_];
#ifdef OPENDDS_SECURITY
      if (is_security_enabled()) {
        cleanup_secure_writer(publicationId);
      }
#endif
      local_publications_.erase(publicationId);
      DCPS::TopicDetailsMap::iterator top_it = topics_.find(topic_name);
      if (top_it != topics_.end()) {
        match_endpoints(publicationId, top_it->second, true /*remove*/);
        top_it->second.remove_local_publication(publicationId);
        // Local, no need to check for dead topic.
      }
    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::remove_publication: ")
                 ACE_TEXT("Failed to publish dispose msg\n")));
    }
  }
}

void Sedp::update_publication_locators(const GUID_t& publicationId,
                                 const DCPS::TransportLocatorSeq& transInfo)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalPublicationIter iter = local_publications_.find(publicationId);
  if (iter != local_publications_.end()) {
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_INFO,
        ACE_TEXT("(%P|%t) Sedp::update_publication_locators: updating locators for %C\n"),
        LogGuid(publicationId).c_str()));
    }
    iter->second.trans_info_ = transInfo;
    write_publication_data(publicationId, iter->second);
  }
}

GUID_t Sedp::add_subscription(
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
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, GUID_t());

  GUID_t rid = participant_id_;
  assign_subscription_key(rid, topicId, qos);
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
  const String& topic_name = topic_names_[topicId];

#ifdef OPENDDS_SECURITY
  if (is_security_enabled()) {
    DDS::Security::SecurityException ex;

    DDS::Security::TopicSecurityAttributes topic_sec_attr;
    if (!get_access_control()->get_topic_sec_attributes(
        get_permissions_handle(), topic_name.data(), topic_sec_attr, ex)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Sedp::add_subscription: ")
        ACE_TEXT("Unable to get security attributes for topic '%C'. ")
        ACE_TEXT("SecurityException[%d.%d]: %C\n"),
          topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
      return GUID_t();
    }

    if (topic_sec_attr.is_read_protected == true) {
      if (!get_access_control()->check_create_datareader(
        get_permissions_handle(), get_domain_id(), topic_name.data(), qos,
        subscriberQos.partition, DDS::Security::DataTagQosPolicy(), ex)) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
          ACE_TEXT("Sedp::add_subscription: ")
          ACE_TEXT("Permissions check failed for local datareader on topic '%C'. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"), topic_name.data(),
            ex.code, ex.minor_code, ex.message.in()));
        return GUID_t();
      }
    }

    if (!get_access_control()->get_datareader_sec_attributes(get_permissions_handle(), topic_name.data(),
        subscriberQos.partition, DDS::Security::DataTagQosPolicy(), sb.security_attribs_, ex)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("Sedp::add_subscription: ")
                 ACE_TEXT("Unable to get security attributes for local datareader. ")
                 ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                 ex.code, ex.minor_code, ex.message.in()));
      return GUID_t();
    }

    if (sb.security_attribs_.is_submessage_protected || sb.security_attribs_.is_payload_protected) {
      DDS::Security::DatareaderCryptoHandle handle =
        get_crypto_key_factory()->register_local_datareader(
          crypto_handle_, DDS::PropertySeq(), sb.security_attribs_, ex);
      if (handle == DDS::HANDLE_NIL) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("Sedp::add_subscription: ")
                   ACE_TEXT("Unable to get local datareader crypto handle. ")
                   ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                   ex.code, ex.minor_code, ex.message.in()));
      }

      get_handle_registry()->insert_local_datareader_crypto_handle(rid, handle, sb.security_attribs_);
    }
  }
#endif

  DCPS::TopicDetails& td = topics_[topic_name];
  td.add_local_subscription(rid);

  if (DDS::RETCODE_OK != add_subscription_i(rid, sb)) {
    return GUID_t();
  }

  if (DDS::RETCODE_OK != write_subscription_data(rid, sb)) {
    return GUID_t();
  }

  if (DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::add_subscription: ")
               ACE_TEXT("calling match_endpoints\n")));
  }
  match_endpoints(rid, td);

  return rid;
}

void Sedp::remove_subscription(const GUID_t& subscriptionId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
  if (iter != local_subscriptions_.end()) {
    if (DDS::RETCODE_OK == remove_subscription_i(subscriptionId, iter->second)) {
      String topic_name = topic_names_[iter->second.topic_id_];
#ifdef OPENDDS_SECURITY
      if (is_security_enabled()) {
        cleanup_secure_reader(subscriptionId);
      }
#endif
      local_subscriptions_.erase(subscriptionId);
      DCPS::TopicDetailsMap::iterator top_it = topics_.find(topic_name);
      if (top_it != topics_.end()) {
        match_endpoints(subscriptionId, top_it->second, true /*remove*/);
        top_it->second.remove_local_subscription(subscriptionId);
        // Local, no need to check for dead topic.
      }
    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::remove_subscription - ")
                 ACE_TEXT("Failed to publish dispose msg\n")));
    }
  }
}

void Sedp::update_subscription_locators(
  const GUID_t& subscriptionId,
  const DCPS::TransportLocatorSeq& transInfo)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  LocalSubscriptionIter iter = local_subscriptions_.find(subscriptionId);
  if (iter != local_subscriptions_.end()) {
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_INFO,
        ACE_TEXT("(%P|%t) Sedp::update_subscription_locators: updating locators for %C\n"),
        LogGuid(subscriptionId).c_str()));
    }
    iter->second.trans_info_ = transInfo;
    write_subscription_data(subscriptionId, iter->second);
  }
}

bool Sedp::remote_knows_about_local_i(const GUID_t& local, const GUID_t& remote) const
{
  if (DCPS_debug_level > 6) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Sedp::remote_knows_about_local_i: local %C remote %C\n"),
               LogGuid(local).c_str(), LogGuid(remote).c_str()));
  }

  const DCPS::GuidConverter gc(local);
  if (gc.isBuiltinDomainEntity()) {
    OPENDDS_ASSERT(DCPS::GuidConverter(remote).isBuiltinDomainEntity());
    // One improvement to returning true here would be waiting until
    // the remote actually sends confirmation that it knows about the
    // local.  A directed SPDP message from the remote would work.
    return true;
  }

  if (gc.isWriter()) {
    LocalPublicationCIter pub = local_publications_.find(local);
    if (pub != local_publications_.end()) {
#ifdef OPENDDS_SECURITY
      if (pub->second.security_attribs_.base.is_discovery_protected) {
        return publications_secure_writer_->is_leading(make_id(remote, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER));
      }
#endif
      return publications_writer_->is_leading(make_id(remote, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER));
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Sedp::remote_knows_about_local_i - could not find local publication %C\n"), LogGuid(local).c_str()));
      return false;
    }
  } else {
    LocalSubscriptionCIter pub = local_subscriptions_.find(local);
    if (pub != local_subscriptions_.end()) {
#ifdef OPENDDS_SECURITY
      if (pub->second.security_attribs_.base.is_discovery_protected) {
        return subscriptions_secure_writer_->is_leading(make_id(remote, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER));
      }
#endif
      return subscriptions_writer_->is_leading(make_id(remote, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER));
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Sedp::remote_knows_about_local_i - could not find local subscription %C\n"), LogGuid(local).c_str()));
      return false;
    }
  }
}

#ifdef OPENDDS_SECURITY
bool Sedp::remote_is_authenticated_i(const GUID_t& local, const GUID_t& remote, const DiscoveredParticipant& participant) const
{
  if (DCPS_debug_level > 6) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Sedp::remote_is_authenticated_i: local %C remote %C\n"),
               LogGuid(local).c_str(), LogGuid(remote).c_str()));
  }

  if (!spdp_.is_security_enabled()) {
    return true;
  }

  if (is_stateless(local)) {
    // Don't need authentication.
    return true;
  }

  if (!has_security_data(participant.pdata_.dataKind)) {
    return participant_sec_attr_.allow_unauthenticated_participants;
  } else {
    return participant.auth_state_ == AUTH_STATE_AUTHENTICATED;
  }
}
#endif

#ifdef OPENDDS_SECURITY
bool Sedp::local_has_remote_participant_token_i(const GUID_t& local, const GUID_t& remote) const
{
  if (DCPS_debug_level > 6) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Sedp::local_has_remote_participant_token_i: local %C remote %C\n"),
               LogGuid(local).c_str(), LogGuid(remote).c_str()));
  }

  if (spdp_.crypto_handle_ == DDS::HANDLE_NIL) {
    return true;
  }

  const GUID_t remote_part = make_id(remote, ENTITYID_PARTICIPANT);

  if (is_stateless_or_volatile(local)) {
    // Don't need the token so okay.
    return true;
  }

  Security::CryptoKeyExchange_var key_exchange = spdp_.security_config_->get_crypto_key_exchange();
  const DDS::Security::ParticipantCryptoHandle dp_crypto_handle =
    get_handle_registry()->get_remote_participant_crypto_handle(remote_part);

  return !key_exchange->have_local_participant_crypto_tokens(crypto_handle_, dp_crypto_handle) ||
    key_exchange->have_remote_participant_crypto_tokens(crypto_handle_, dp_crypto_handle);
}

bool Sedp::remote_has_local_participant_token_i(const GUID_t& local, const GUID_t& remote, const DiscoveredParticipant& participant) const
{
  if (DCPS_debug_level > 6) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Sedp::remote_has_local_participant_token_i: local %C remote %C\n"),
               LogGuid(local).c_str(), LogGuid(remote).c_str()));
  }

  if (spdp_.crypto_handle_ == DDS::HANDLE_NIL) {
    return true;
  }

  const GUID_t remote_part = make_id(remote, ENTITYID_PARTICIPANT);

  if (is_stateless_or_volatile(local)) {
    // Don't need the token so okay.
    return true;
  }

  Security::CryptoKeyExchange_var key_exchange = spdp_.security_config_->get_crypto_key_exchange();
  const DDS::Security::ParticipantCryptoHandle dp_crypto_handle =
    get_handle_registry()->get_remote_participant_crypto_handle(remote_part);

  return !key_exchange->have_local_participant_crypto_tokens(crypto_handle_, dp_crypto_handle) ||
    (participant.participant_tokens_sent_ &&
     participant_volatile_message_secure_writer_->is_leading(make_id(remote, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER)));
}

bool Sedp::local_has_remote_endpoint_token_i(const GUID_t& local, const GUID_t& remote) const
{
  if (DCPS_debug_level > 6) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Sedp::local_has_remote_endpoint_token_i: local %C remote %C\n"),
               LogGuid(local).c_str(), LogGuid(remote).c_str()));
  }

  if (spdp_.crypto_handle_ == DDS::HANDLE_NIL) {
    return true;
  }

  if (is_stateless_or_volatile(local)) {
    // Don't need the token so okay.
    return true;
  }

  Security::CryptoKeyExchange_var key_exchange = spdp_.security_config_->get_crypto_key_exchange();
  if (DCPS::GuidConverter(local).isWriter()) {
    const DDS::Security::DatawriterCryptoHandle local_crypto_handle =
      get_handle_registry()->get_local_datawriter_crypto_handle(local);
    if (local_crypto_handle == DDS::HANDLE_NIL) {
      return true;
    }
    const DDS::Security::DatareaderCryptoHandle remote_crypto_handle =
      get_handle_registry()->get_remote_datareader_crypto_handle(remote);
    const DDS::Security::EndpointSecurityAttributes attribs =
      get_handle_registry()->get_remote_datareader_security_attributes(remote);
    return !attribs.is_submessage_protected ||
      key_exchange->have_remote_datareader_crypto_tokens(local_crypto_handle, remote_crypto_handle);
  } else {
    const DDS::Security::DatareaderCryptoHandle local_crypto_handle =
      get_handle_registry()->get_local_datareader_crypto_handle(local);
    if (local_crypto_handle == DDS::HANDLE_NIL) {
      return true;
    }
    const DDS::Security::DatawriterCryptoHandle remote_crypto_handle =
      get_handle_registry()->get_remote_datawriter_crypto_handle(remote);
    const DDS::Security::EndpointSecurityAttributes attribs =
      get_handle_registry()->get_remote_datawriter_security_attributes(remote);
    return (!attribs.is_submessage_protected && !attribs.is_payload_protected) ||
      key_exchange->have_remote_datawriter_crypto_tokens(local_crypto_handle, remote_crypto_handle);
  }
}

bool Sedp::remote_has_local_endpoint_token_i(const GUID_t& local, bool local_tokens_sent,
                                             const GUID_t& remote) const
{
  if (DCPS_debug_level > 6) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Sedp::remote_has_local_endpoint_token_i: local %C remote %C\n"),
               LogGuid(local).c_str(), LogGuid(remote).c_str()));
  }

  if (spdp_.crypto_handle_ == DDS::HANDLE_NIL) {
    return true;
  }

  if (is_stateless_or_volatile(local)) {
    // Don't need the token so okay.
    return true;
  }

  Security::CryptoKeyExchange_var key_exchange = spdp_.security_config_->get_crypto_key_exchange();
  if (DCPS::GuidConverter(local).isWriter()) {
    const DDS::Security::DatawriterCryptoHandle local_crypto_handle =
      get_handle_registry()->get_local_datawriter_crypto_handle(local);
    const DDS::Security::DatareaderCryptoHandle remote_crypto_handle =
      get_handle_registry()->get_remote_datareader_crypto_handle(remote);
    return !key_exchange->have_local_datawriter_crypto_tokens(local_crypto_handle, remote_crypto_handle) ||
      (local_tokens_sent && participant_volatile_message_secure_writer_->is_leading(make_id(remote, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER)));
  } else {
    const DDS::Security::DatareaderCryptoHandle local_crypto_handle =
      get_handle_registry()->get_local_datareader_crypto_handle(local);
    const DDS::Security::DatawriterCryptoHandle remote_crypto_handle =
      get_handle_registry()->get_remote_datawriter_crypto_handle(remote);
    return !key_exchange->have_local_datareader_crypto_tokens(local_crypto_handle, remote_crypto_handle) ||
      (local_tokens_sent && participant_volatile_message_secure_writer_->is_leading(make_id(remote, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER)));
  }
}

void Sedp::cleanup_secure_writer(const GUID_t& publicationId)
{
  using namespace DDS::Security;

  Security::HandleRegistry_rch handle_registry = get_handle_registry();
  if (!handle_registry) {
    return;
  }
  const DatawriterCryptoHandle dwch =
    handle_registry->get_local_datawriter_crypto_handle(publicationId);
  if (dwch == DDS::HANDLE_NIL) {
    return;
  }

  SecurityException ex = {"", 0, 0};
  if (!get_crypto_key_factory()->unregister_datawriter(dwch, ex)) {
    if (DCPS::security_debug.cleanup_error) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) {cleanup_error} Sedp::cleanup_secure_writer() - ")
                 ACE_TEXT("Failure calling unregister_datawriter. (ch %d)")
                 ACE_TEXT(" Security Exception[%d.%d]: %C\n"),
                 dwch, ex.code, ex.minor_code, ex.message.in()));
    }
  }
  handle_registry->erase_local_datawriter_crypto_handle(publicationId);
}

void Sedp::cleanup_secure_reader(const GUID_t& subscriptionId)
{
  using namespace DDS::Security;

  Security::HandleRegistry_rch handle_registry = get_handle_registry();
  if (!handle_registry) {
    return;
  }
  const DatareaderCryptoHandle drch =
    handle_registry->get_local_datareader_crypto_handle(subscriptionId);
  if (drch == DDS::HANDLE_NIL) {
    return;
  }

  SecurityException ex = {"", 0, 0};
  if (!get_crypto_key_factory()->unregister_datareader(drch, ex)) {
    if (DCPS::security_debug.cleanup_error) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) {cleanup_error} Sedp::cleanup_secure_reader() - ")
                 ACE_TEXT("Failure calling unregister_datareader (ch %d).")
                 ACE_TEXT(" Security Exception[%d.%d]: %C\n"),
                 drch, ex.code, ex.minor_code, ex.message.in()));
    }
  }
  handle_registry->erase_local_datareader_crypto_handle(subscriptionId);
}
#endif

// TODO: This is perhaps too generic since the context probably has the details this function computes.
void Sedp::match_endpoints(GUID_t repoId, const DCPS::TopicDetails& td, bool remove)
{
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::match_endpoints %C%C\n",
      remove ? "remove " : "", LogGuid(repoId).c_str()));
  }

  const bool reader = DCPS::GuidConverter(repoId).isReader();
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
    if (DCPS::GuidConverter(*iter).isReader() != reader) {
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
    if (DCPS::GuidConverter(*iter).isReader() != reader) {
      if (remove) {
        remove_assoc(*iter, repoId);
      } else {
        match(reader ? *iter : repoId, reader ? repoId : *iter);
      }
    }
  }
}

void Sedp::cleanup_writer_association(DCPS::DataWriterCallbacks_wrch callbacks,
                                      const GUID_t& writer,
                                      const GUID_t& reader)
{
  Spdp::DiscoveredParticipantIter part_iter = spdp_.participants_.find(make_id(reader, ENTITYID_PARTICIPANT));
  if (part_iter != spdp_.participants_.end()) {
    for (DiscoveredParticipant::WriterAssociationRecords::iterator pos = part_iter->second.writer_pending_records_.begin(), limit = part_iter->second.writer_pending_records_.end(); pos != limit; ++pos) {
      if ((*pos)->writer_id() == writer && (*pos)->reader_id() == reader) {
        part_iter->second.writer_pending_records_.erase(pos);
        break;
      }
    }

    for (DiscoveredParticipant::WriterAssociationRecords::iterator pos = part_iter->second.writer_associated_records_.begin(), limit = part_iter->second.writer_associated_records_.end(); pos != limit; ++pos) {
      if ((*pos)->writer_id() == writer && (*pos)->reader_id() == reader) {
        job_queue_->enqueue(DCPS::make_rch<WriterRemoveAssociations>(*pos));
        part_iter->second.writer_associated_records_.erase(pos);
        break;
      }
    }
  } else if (equal_guid_prefixes(writer, participant_id_) && equal_guid_prefixes(reader, participant_id_)) {
    DCPS::ReaderAssociation ra = DCPS::ReaderAssociation();
    ra.readerId = reader;
    job_queue_->enqueue(DCPS::make_rch<WriterRemoveAssociations>(DCPS::make_rch<WriterAssociationRecord>(callbacks, writer, ra)));
  }
}

void Sedp::cleanup_reader_association(DCPS::DataReaderCallbacks_wrch callbacks,
                                      const GUID_t& reader,
                                      const GUID_t& writer)
{
  Spdp::DiscoveredParticipantIter part_iter = spdp_.participants_.find(make_id(writer, ENTITYID_PARTICIPANT));
  if (part_iter != spdp_.participants_.end()) {
    for (DiscoveredParticipant::ReaderAssociationRecords::iterator pos = part_iter->second.reader_pending_records_.begin(), limit = part_iter->second.reader_pending_records_.end(); pos != limit; ++pos) {
      if ((*pos)->reader_id() == reader && (*pos)->writer_id() == writer) {
        part_iter->second.reader_pending_records_.erase(pos);
        break;
      }
    }

    for (DiscoveredParticipant::ReaderAssociationRecords::iterator pos = part_iter->second.reader_associated_records_.begin(), limit = part_iter->second.reader_associated_records_.end(); pos != limit; ++pos) {
      if ((*pos)->reader_id() == reader && (*pos)->writer_id() == writer) {
        job_queue_->enqueue(DCPS::make_rch<ReaderRemoveAssociations>(*pos));
        part_iter->second.reader_associated_records_.erase(pos);
        break;
      }
    }
  } else if (equal_guid_prefixes(reader, participant_id_) && equal_guid_prefixes(writer, participant_id_)) {
    DCPS::WriterAssociation wa = DCPS::WriterAssociation();
    wa.writerId = writer;
    job_queue_->enqueue(DCPS::make_rch<ReaderRemoveAssociations>(DCPS::make_rch<ReaderAssociationRecord>(callbacks, reader, wa)));
  }
}

void Sedp::remove_assoc(const GUID_t& remove_from, const GUID_t& removing)
{
  if (DCPS::GuidConverter(remove_from).isReader()) {
    const LocalSubscriptionIter lsi = local_subscriptions_.find(remove_from);
    if (lsi != local_subscriptions_.end()) {
      lsi->second.matched_endpoints_.erase(removing);
      const DiscoveredPublicationIter dpi = discovered_publications_.find(removing);
      if (dpi != discovered_publications_.end()) {
        dpi->second.matched_endpoints_.erase(remove_from);
      }
      const size_t count = lsi->second.remote_expectant_opendds_associations_.erase(removing);
      cleanup_reader_association(lsi->second.subscription_, remove_from, removing);
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
      lpi->second.remote_expectant_opendds_associations_.erase(removing);
      cleanup_writer_association(lpi->second.publication_, remove_from, removing);
      remove_assoc_i(remove_from, lpi->second, removing);
    }
  }
}

void Sedp::match(const GUID_t& writer, const GUID_t& reader)
{
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::match: w: %C r: %C\n",
      LogGuid(writer).c_str(), LogGuid(reader).c_str()));
  }

  // 1. collect type info about the writer, which may be local or discovered
  XTypes::TypeInformation* writer_type_info = 0;

  const LocalPublicationIter lpi = local_publications_.find(writer);
  DiscoveredPublicationIter dpi;
  bool writer_local = false;
  if (lpi != local_publications_.end()) {
    writer_local = true;
    writer_type_info = &lpi->second.type_info_;
  } else if ((dpi = discovered_publications_.find(writer))
             != discovered_publications_.end()) {
    writer_type_info = &dpi->second.type_info_;
  } else {
    if (DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::match: Undiscovered Writer\n"));
    }
    return; // Possible and ok, since lock is released
  }

  // 2. collect type info about the reader, which may be local or discovered
  XTypes::TypeInformation* reader_type_info = 0;

  const LocalSubscriptionIter lsi = local_subscriptions_.find(reader);
  DiscoveredSubscriptionIter dsi;
  bool reader_local = false;
  if (lsi != local_subscriptions_.end()) {
    reader_local = true;
    reader_type_info = &lsi->second.type_info_;
  } else if ((dsi = discovered_subscriptions_.find(reader))
             != discovered_subscriptions_.end()) {
    reader_type_info = &dsi->second.type_info_;
  } else {
    if (DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::match: Undiscovered Reader\n"));
    }
    return; // Possible and ok, since lock is released
  }

  MatchingData md;

  // if the type object is not in cache, send RPC request
  md.time_added_to_map = MonotonicTimePoint::now();

  // NOTE(sonndinh): Is it possible for a discovered endpoint to include only the "complete"
  // part in its TypeInformation? If it's possible, then we may need to handle that case, i.e.,
  // request only the remote complete TypeObject (if it's not already in the cache).
  // The following code assumes when the "minimal" part is not included in the discovered
  // endpoint's TypeInformation, then the "complete" part also is not included.
  if ((writer_type_info->minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE) &&
      (reader_type_info->minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE)) {
    bool need_minimal_tobjs, need_complete_tobjs;
    if (!writer_local && reader_local) {
      need_minimal_and_or_complete_types(writer_type_info, need_minimal_tobjs, need_complete_tobjs);
      if (need_minimal_tobjs || need_complete_tobjs) {
        if (DCPS_debug_level >= 4) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::match: Remote Writer\n"));
        }
        bool is_discovery_protected = false;
#ifdef OPENDDS_SECURITY
        is_discovery_protected = lsi->second.security_attribs_.base.is_discovery_protected;
#endif
        save_matching_data_and_get_typeobjects(writer_type_info, md,
                                               MatchingPair(writer, reader),
                                               writer, is_discovery_protected,
                                               need_minimal_tobjs, need_complete_tobjs);
        return;
      }
    } else if (!reader_local && writer_local) {
      need_minimal_and_or_complete_types(reader_type_info, need_minimal_tobjs, need_complete_tobjs);
      if (need_minimal_tobjs || need_complete_tobjs) {
        if (DCPS_debug_level >= 4) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::match: Remote Reader\n"));
        }
        bool is_discovery_protected = false;
#ifdef OPENDDS_SECURITY
        is_discovery_protected = lpi->second.security_attribs_.base.is_discovery_protected;
#endif
        save_matching_data_and_get_typeobjects(reader_type_info, md,
                                               MatchingPair(writer, reader),
                                               reader, is_discovery_protected,
                                               need_minimal_tobjs, need_complete_tobjs);
        return;
      }
    }
  } else if (reader_local && !writer_local && use_xtypes_ &&
             writer_type_info->minimal.typeid_with_size.type_id.kind() != XTypes::TK_NONE) {
    // We are a recorder trying to associate with a remote xtypes writer
    bool need_minimal_tobjs, need_complete_tobjs;
    need_minimal_and_or_complete_types(writer_type_info, need_minimal_tobjs, need_complete_tobjs);
    if (need_minimal_tobjs || need_complete_tobjs) {
      if (DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::match: Local Recorder matching Remote Writer\n"));
      }
#ifdef OPENDDS_SECURITY
      const bool is_discovery_protected = lsi->second.security_attribs_.base.is_discovery_protected;
#else
      const bool is_discovery_protected = false;
#endif
      save_matching_data_and_get_typeobjects(writer_type_info, md,
                                             MatchingPair(writer, reader),
                                             writer, is_discovery_protected,
                                             need_minimal_tobjs, need_complete_tobjs);
      return;
    }
  }

  match_continue(writer, reader);
}

void Sedp::need_minimal_and_or_complete_types(const XTypes::TypeInformation* type_info,
                                              bool& need_minimal,
                                              bool& need_complete) const
{
  need_minimal = type_lookup_service_ &&
    !type_lookup_service_->type_object_in_cache(type_info->minimal.typeid_with_size.type_id);

  need_complete = use_xtypes_complete_ && type_lookup_service_ &&
    type_info->complete.typeid_with_size.type_id.kind() != XTypes::TK_NONE &&
    !type_lookup_service_->type_object_in_cache(type_info->complete.typeid_with_size.type_id);
}

void Sedp::remove_expired_endpoints(const MonotonicTimePoint& /*now*/)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  const MonotonicTimePoint now = MonotonicTimePoint::now();

  MatchingDataIter end_iter = matching_data_buffer_.end();
  for (MatchingDataIter iter = matching_data_buffer_.begin(); iter != end_iter; ) {
    // Do not try to simplify increment: "associative container erase idiom"
    if (now - iter->second.time_added_to_map >= max_type_lookup_service_reply_period_) {
      if (DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::remove_expired_endpoints: "
          "clean up pending pair w: %C r: %C\n",
          LogGuid(iter->first.writer_).c_str(), LogGuid(iter->first.reader_).c_str()));
      }
      matching_data_buffer_.erase(iter++);
    } else {
      ++iter;
    }
  }

  // Clean up internal data used by getTypeDependencies
  for (OrigSeqNumberMap::iterator it = orig_seq_numbers_.begin(); it != orig_seq_numbers_.end();) {
    if (now - it->second.time_started >= max_type_lookup_service_reply_period_) {
      if (DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::remove_expired_endpoints: "
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

void Sedp::populate_origination_locator(const GUID_t& id, DCPS::TransportLocator& tl)
{
  DCPS::GuidConverter conv(id);
  if (conv.isBuiltinDomainEntity()) {
    DCPS::LocatorSeq locators;
    bool expects_inline_qos = false;
    bool found = spdp_.get_last_recv_locator(make_id(id, ENTITYID_PARTICIPANT), locators, expects_inline_qos);

    if (!found || !locators.length()) {
      return;
    }

    const Encoding& encoding = RTPS::get_locators_encoding();
    size_t size = serialized_size(encoding, locators);
    primitive_serialized_size_boolean(encoding, size);

    ACE_Message_Block mb_locator(size);
    Serializer ser_loc(&mb_locator, encoding);
    ser_loc << locators;
    ser_loc << ACE_OutputCDR::from_boolean(expects_inline_qos);

    tl.transport_type = "rtps_udp";
    RTPS::message_block_to_sequence(mb_locator, tl.data);
  } else {
    if (conv.isReader()) {
      transport_inst_->get_last_recv_locator(make_id(id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER), tl);
    } else {
      transport_inst_->get_last_recv_locator(make_id(id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER), tl);
    }
  }
}

void Sedp::match_continue(const GUID_t& writer, const GUID_t& reader)
{
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Sedp::match_continue: w: %C r: %C\n",
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
  DCPS::ContentFilterProperty_t tempCfp;

  DiscoveredPublicationIter dpi = discovered_publications_.find(writer);
  DiscoveredSubscriptionIter dsi = discovered_subscriptions_.find(reader);
  if (dpi != discovered_publications_.end() && dsi != discovered_subscriptions_.end()) {
    // This is a discovered/discovered match, nothing for us to do
    return;
  }

  // 1. Collect details about the writer, which may be local or discovered
  const DDS::DataWriterQos* dwQos = 0;
  const DDS::PublisherQos* pubQos = 0;
  DCPS::TransportLocatorSeq* wTls = 0;
  ACE_CDR::ULong wTransportContext = 0;
  XTypes::TypeInformation* writer_type_info = 0;
  String topic_name;
  DCPS::MonotonicTime_t writer_participant_discovered_at;

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

    populate_transport_locator_sequence(*wTls, dpi, writer);
  } else {
    return; // Possible and ok, since lock is released
  }

  // 2. Collect details about the reader, which may be local or discovered
  const DDS::DataReaderQos* drQos = 0;
  const DDS::SubscriberQos* subQos = 0;
  DCPS::TransportLocatorSeq* rTls = 0;
  ACE_CDR::ULong rTransportContext = 0;
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  const DCPS::ContentFilterProperty_t* cfProp = 0;
#endif
  XTypes::TypeInformation* reader_type_info = 0;
  DCPS::MonotonicTime_t reader_participant_discovered_at;

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
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    cfProp = &tempCfp;
#endif
    if (!already_matched) {
      already_matched = lsi->second.matched_endpoints_.count(writer);
    }
    reader_participant_discovered_at = lsi->second.participant_discovered_at_;
  } else if (dsi != discovered_subscriptions_.end()) {
    rTls = &dsi->second.reader_data_.readerProxy.allLocators;

    populate_transport_locator_sequence(*rTls, dsi, reader);
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

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    cfProp = &dsi->second.reader_data_.contentFilterProperty;
#endif
    reader_type_info = &dsi->second.type_info_;
    reader_participant_discovered_at = dsi->second.participant_discovered_at_;
  } else {
    return; // Possible and ok, since lock is released
  }

  // 3. Perform type consistency check (XTypes 1.3, Section 7.6.3.4.2)
  bool consistent = false;

  DCPS::TopicDetailsMap::iterator td_iter = topics_.find(topic_name);
  if (td_iter == topics_.end()) {
    ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) Sedp::match_continue - ERROR ")
              ACE_TEXT("Didn't find topic for consistency check\n")));
    return;
  } else {
    const XTypes::TypeIdentifier& writer_type_id =
      writer_type_info->minimal.typeid_with_size.type_id;
    const XTypes::TypeIdentifier& reader_type_id =
      reader_type_info->minimal.typeid_with_size.type_id;
    if (writer_type_id.kind() != XTypes::TK_NONE && reader_type_id.kind() != XTypes::TK_NONE) {
      if (!writer_local || !reader_local) {
        DCPS::Encoding::Kind encoding_kind;
        if (tempDwQos.representation.value.length() > 0 &&
            DCPS::repr_to_encoding_kind(tempDwQos.representation.value[0], encoding_kind) &&
            encoding_kind == DCPS::Encoding::KIND_XCDR1) {
          const XTypes::TypeFlag extensibility_mask = XTypes::IS_APPENDABLE;
          if (type_lookup_service_->extensibility(extensibility_mask, writer_type_id)) {
            if (OpenDDS::DCPS::DCPS_debug_level) {
              ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: "
                "Sedp::match_continue: "
                "Encountered unsupported combination of XCDR1 encoding and appendable "
                "extensibility\n"));
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
        String writer_type_name;
        String reader_type_name;
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
        consistent = reader_type_name.empty() || writer_type_name == reader_type_name;
      }
    }

    if (!consistent) {
      td_iter->second.increment_inconsistent();
      if (DCPS::DCPS_debug_level) {
        ACE_DEBUG((LM_WARNING,
                  ACE_TEXT("(%P|%t) Sedp::match_continue - WARNING ")
                  ACE_TEXT("Data types of topic %C does not match (inconsistent)\n"),
                  topic_name.c_str()));
      }
      return;
    }
  }

  // 4. Check transport and QoS compatibility

  // Copy entries from local publication and local subscription maps
  // prior to releasing lock
  DCPS::DataWriterCallbacks_wrch dwr;
  DCPS::DataReaderCallbacks_wrch drr;
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

  DCPS::IncompatibleQosStatus writerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};
  DCPS::IncompatibleQosStatus readerStatus = {0, 0, 0, DDS::QosPolicyCountSeq()};

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
    WriterAssociationRecord_rch war;
    if (call_writer) {
      war = make_rch<WriterAssociationRecord>(dwr, writer, DCPS::ReaderAssociation());
      DCPS::ReaderAssociation& ra = const_cast<DCPS::ReaderAssociation&>(war->reader_association_);
      ra.readerTransInfo = *rTls;
      populate_origination_locator(reader, ra.readerDiscInfo);
      ra.transportContext = rTransportContext;
      ra.readerId = reader;
      ra.subQos = *subQos;
      ra.readerQos = *drQos;
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
      ra.filterClassName = cfProp->filterClassName;
      ra.filterExpression = cfProp->filterExpression;
      ra.exprParams = cfProp->expressionParameters;
#endif
      XTypes::serialize_type_info(*reader_type_info, ra.serializedTypeInfo);
      ra.participantDiscoveredAt = writer_participant_discovered_at;
    }

    ReaderAssociationRecord_rch rar;
    if (call_reader) {
      rar = make_rch<ReaderAssociationRecord>(drr, reader, DCPS::WriterAssociation());
      DCPS::WriterAssociation& wa = const_cast<DCPS::WriterAssociation&>(rar->writer_association_);
      wa.writerTransInfo = *wTls;
      populate_origination_locator(writer, wa.writerDiscInfo);
      wa.transportContext = wTransportContext;
      wa.writerId = writer;
      wa.pubQos = *pubQos;
      wa.writerQos = *dwQos;
      XTypes::serialize_type_info(*writer_type_info, wa.serializedTypeInfo);
      wa.participantDiscoveredAt = writer_participant_discovered_at;
    }

#ifdef OPENDDS_SECURITY
    if (is_security_enabled()) {
      match_continue_security_enabled(writer, reader, call_writer, call_reader);
    }
#endif

    if (call_reader && call_writer) {
      // Associate immediately.
      job_queue_->enqueue(DCPS::make_rch<ReaderAddAssociation>(rar));
      job_queue_->enqueue(DCPS::make_rch<WriterAddAssociation>(war));
    } else if (call_reader) {
      if (use_xtypes_complete_ && reader_type_info->complete.typeid_with_size.type_id.kind() == XTypes::TK_NONE) {
        // Reader is a local recorder using complete types
        DCPS::DataReaderCallbacks_rch lock = rar->callbacks_.lock();
        OpenDDS::DCPS::RecorderImpl* ri = dynamic_cast<OpenDDS::DCPS::RecorderImpl*>(lock.in());
        if (ri) {
          XTypes::TypeInformation type_info;
          if (XTypes::deserialize_type_info(type_info, rar->writer_association_.serializedTypeInfo)) {
            ri->add_to_dynamic_type_map(rar->writer_id(), type_info.complete.typeid_with_size.type_id);
          }
        }
      }
      Spdp::DiscoveredParticipantIter iter = spdp_.participants_.find(make_id(writer, ENTITYID_PARTICIPANT));
      if (iter != spdp_.participants_.end()) {
        iter->second.reader_pending_records_.push_back(rar);
        process_association_records_i(iter->second);
      }
    } else if (call_writer) {
      Spdp::DiscoveredParticipantIter iter = spdp_.participants_.find(make_id(reader, ENTITYID_PARTICIPANT));
      if (iter != spdp_.participants_.end()) {
        iter->second.writer_pending_records_.push_back(war);
        process_association_records_i(iter->second);
      }
    }

  } else if (already_matched) { // break an existing association
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
    if (writer_local) {
      cleanup_writer_association(dwr, writer, reader);
    }
    if (reader_local) {
      cleanup_reader_association(drr, reader, writer);
    }
  } else { // something was incompatible
    if (writer_local && writerStatus.count_since_last_send) {
      if (DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match_continue - ")
                   ACE_TEXT("writer incompatible\n")));
      }
      DCPS::DataWriterCallbacks_rch dwr_lock = dwr.lock();
      if (dwr_lock) {
        dwr_lock->update_incompatible_qos(writerStatus);
      }
    }
    if (reader_local && readerStatus.count_since_last_send) {
      if (DCPS_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::match_continue - ")
                   ACE_TEXT("reader incompatible\n")));
      }
      DCPS::DataReaderCallbacks_rch drr_lock = drr.lock();
      if (drr_lock) {
        drr_lock->update_incompatible_qos(readerStatus);
      }
    }
  }
}

void Sedp::save_matching_data_and_get_typeobjects(const XTypes::TypeInformation* type_info,
                                                  MatchingData& md, const MatchingPair& mp,
                                                  const RepoId& remote_id,
                                                  bool is_discovery_protected,
                                                  bool get_minimal, bool get_complete)
{
  if (get_minimal) {
    md.rpc_seqnum_minimal = ++type_lookup_service_sequence_number_;
    md.got_minimal = false;
  } else {
    md.rpc_seqnum_minimal = SequenceNumber::SEQUENCENUMBER_UNKNOWN();
    md.got_minimal = true;
  }

  if (get_complete) {
    md.rpc_seqnum_complete = ++type_lookup_service_sequence_number_;
    md.got_complete = false;
  } else {
    md.rpc_seqnum_complete = SequenceNumber::SEQUENCENUMBER_UNKNOWN();
    md.got_complete = true;
  }

  matching_data_buffer_[mp] = md;

  // Send a sequence of requests for minimal remote TypeObjects
  if (get_minimal) {
    if (DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG,
                  "(%P|%t) Sedp::save_matching_data_and_get_typeobjects: "
                  "remote: %C seq: %q\n",
                  LogGuid(remote_id).c_str(), md.rpc_seqnum_minimal.getValue()));
    }
    get_remote_type_objects(type_info->minimal, md, true, remote_id, is_discovery_protected);
  }

  // Send another sequence of requests for complete remote TypeObjects
  if (get_complete) {
    if (DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG,
                  "(%P|%t) Sedp::save_matching_data_and_get_typeobjects: "
                  "remote: %C seq: %q\n",
                  LogGuid(remote_id).c_str(), md.rpc_seqnum_complete.getValue()));
    }
    get_remote_type_objects(type_info->complete, md, false, remote_id, is_discovery_protected);
  }
}

void Sedp::get_remote_type_objects(const XTypes::TypeIdentifierWithDependencies& tid_with_deps,
                                   MatchingData& md, bool get_minimal, const RepoId& remote_id,
                                   bool is_discovery_protected)
{
  // Store an entry for the first request in the sequence.
  TypeIdOrigSeqNumber orig_req_data;
  std::memcpy(orig_req_data.participant, remote_id.guidPrefix, sizeof(GuidPrefix_t));
  orig_req_data.type_id = tid_with_deps.typeid_with_size.type_id;
  const SequenceNumber& orig_seqnum = get_minimal ? md.rpc_seqnum_minimal : md.rpc_seqnum_complete;
  orig_req_data.seq_number = orig_seqnum;
  orig_req_data.secure = false;
#ifdef OPENDDS_SECURITY
  if (is_security_enabled() && is_discovery_protected) {
    orig_req_data.secure = true;
  }
#endif
  orig_req_data.time_started = md.time_added_to_map;
  orig_seq_numbers_.insert(std::make_pair(orig_seqnum, orig_req_data));

  XTypes::TypeIdentifierSeq type_ids;
  if (tid_with_deps.dependent_typeid_count == -1 ||
      tid_with_deps.dependent_typeids.length() < (CORBA::ULong)tid_with_deps.dependent_typeid_count) {
    type_ids.append(tid_with_deps.typeid_with_size.type_id);

    // Get dependencies of the topic type. TypeObjects of both topic type and
    // its dependencies are obtained in subsequent type lookup requests.
    send_type_lookup_request(type_ids, remote_id, is_discovery_protected, false, orig_req_data.seq_number);
  } else {
    type_ids.length(tid_with_deps.dependent_typeid_count + 1);
    type_ids[0] = tid_with_deps.typeid_with_size.type_id;
    for (unsigned i = 1; i <= (unsigned)tid_with_deps.dependent_typeid_count; ++i) {
      type_ids[i] = tid_with_deps.dependent_typeids[i - 1].type_id;
    }

    // Get TypeObjects of topic type and all of its dependencies.
    send_type_lookup_request(type_ids, remote_id, is_discovery_protected, true, orig_req_data.seq_number);
  }
  type_lookup_reply_deadline_processor_->schedule(max_type_lookup_service_reply_period_);
}

#ifdef OPENDDS_SECURITY
void Sedp::match_continue_security_enabled(
  const GUID_t& writer, const GUID_t& reader, bool call_writer, bool call_reader)
{
  DDS::Security::CryptoKeyExchange_var keyexg = get_crypto_key_exchange();
  if (call_reader) {
    const DDS::Security::DatareaderCryptoHandle drch =
      get_handle_registry()->get_local_datareader_crypto_handle(reader);
    const DDS::Security::EndpointSecurityAttributes attribs =
      get_handle_registry()->get_local_datareader_security_attributes(reader);

    // It might not exist due to security attributes, and that's OK
    if (drch != DDS::HANDLE_NIL) {
      DDS::Security::DatawriterCryptoHandle dwch =
        generate_remote_matched_writer_crypto_handle(writer, reader);
      DatawriterCryptoTokenSeqMap::iterator t_iter =
        pending_remote_writer_crypto_tokens_.find(writer);
      if (t_iter != pending_remote_writer_crypto_tokens_.end()) {
        DDS::Security::SecurityException se;
        if (!keyexg->set_remote_datawriter_crypto_tokens(drch, dwch, t_iter->second, se)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
            ACE_TEXT("DiscoveryBase::match_continue_security_enabled: ")
            ACE_TEXT("Unable to set pending remote datawriter crypto tokens with ")
            ACE_TEXT("crypto key exchange plugin. Security Exception[%d.%d]: %C\n"),
              se.code, se.minor_code, se.message.in()));
        }
        pending_remote_writer_crypto_tokens_.erase(t_iter);
      }
      // Yes, this is different for remote datawriters than readers (see 8.8.9.3 vs 8.8.9.2)
      if (attribs.is_submessage_protected) {
        create_and_send_datareader_crypto_tokens(drch, reader, dwch, writer);
      }
    }
  }

  if (call_writer) {
    const DDS::Security::DatawriterCryptoHandle dwch =
      get_handle_registry()->get_local_datawriter_crypto_handle(writer);
    const DDS::Security::EndpointSecurityAttributes attribs =
      get_handle_registry()->get_local_datawriter_security_attributes(writer);

    // It might not exist due to security attributes, and that's OK
    if (dwch != DDS::HANDLE_NIL) {
      DDS::Security::DatareaderCryptoHandle drch =
        generate_remote_matched_reader_crypto_handle(reader, writer,
                                                     relay_only_readers_.count(reader));
      DatareaderCryptoTokenSeqMap::iterator t_iter =
        pending_remote_reader_crypto_tokens_.find(reader);
      if (t_iter != pending_remote_reader_crypto_tokens_.end()) {
        DDS::Security::SecurityException se;
        if (!keyexg->set_remote_datareader_crypto_tokens(dwch, drch, t_iter->second, se)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
            ACE_TEXT("DiscoveryBase::match_continue_security_enabled: ")
            ACE_TEXT("Unable to set pending remote datareader crypto tokens with crypto ")
            ACE_TEXT("key exchange plugin. Security Exception[%d.%d]: %C\n"),
              se.code, se.minor_code, se.message.in()));
        }
        pending_remote_reader_crypto_tokens_.erase(t_iter);
      }
      if (attribs.is_submessage_protected || attribs.is_payload_protected) {
        create_and_send_datawriter_crypto_tokens(dwch, writer, drch, reader);
      }
    }
  }
}
#endif

void Sedp::WriterAddAssociation::execute()
{
  DCPS::DataWriterCallbacks_rch lock = record_->callbacks_.lock();
  if (lock) {
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Sedp::WriterAddAssociation::execute - ")
                 ACE_TEXT("adding writer %C association for reader %C\n"), LogGuid(record_->writer_id()).c_str(),
                 LogGuid(record_->reader_id()).c_str()));
    }
    lock->add_association(record_->writer_id(), record_->reader_association_, true);
  }
}

void Sedp::WriterRemoveAssociations::execute()
{
  DCPS::DataWriterCallbacks_rch lock = record_->callbacks_.lock();
  if (lock) {
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Sedp::WriterRemoveAssociations::execute - ")
                 ACE_TEXT("removing writer %C association for writer %C\n"), LogGuid(record_->writer_id()).c_str(),
                 LogGuid(record_->reader_id()).c_str()));
    }
    DCPS::ReaderIdSeq reader_seq(1);
    reader_seq.length(1);
    reader_seq[0] = record_->reader_id();
    lock->remove_associations(reader_seq, false);
  }
}

void Sedp::ReaderAddAssociation::execute()
{
  DCPS::DataReaderCallbacks_rch lock = record_->callbacks_.lock();
  if (lock) {
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Sedp::ReaderAddAssociation::execute - ")
                 ACE_TEXT("adding reader %C association for writer %C\n"), LogGuid(record_->reader_id()).c_str(),
                 LogGuid(record_->writer_id()).c_str()));
    }
    lock->add_association(record_->reader_id(), record_->writer_association_, false);
  }
}

void Sedp::ReaderRemoveAssociations::execute()
{
  DCPS::DataReaderCallbacks_rch lock = record_->callbacks_.lock();
  if (lock) {
    if (DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Sedp::ReaderRemoveAssociations::execute - ")
                 ACE_TEXT("removing reader %C association for writer %C\n"), LogGuid(record_->reader_id()).c_str(),
                 LogGuid(record_->writer_id()).c_str()));
    }
    DCPS::WriterIdSeq writer_seq(1);
    writer_seq.length(1);
    writer_seq[0] = record_->writer_id();
    lock->remove_associations(writer_seq, false);
  }
}

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
