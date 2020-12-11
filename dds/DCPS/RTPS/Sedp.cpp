/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Sedp.h"

#include "MessageTypes.h"
#include "ParameterListConverter.h"
#include "RtpsDiscovery.h"
#include "RtpsCoreTypeSupportImpl.h"

#ifdef OPENDDS_SECURITY
#include "SecurityHelpers.h"
#endif

#include "Spdp.h"

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
#include "dds/DCPS/SafetyProfileStreams.h"

#ifdef OPENDDS_SECURITY
#include "dds/DdsSecurityCoreTypeSupportImpl.h"
#endif

#include <ace/Reverse_Lock_T.h>
#include <ace/Auto_Ptr.h>

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

}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {
using DCPS::RepoId;
using DCPS::make_rch;
using DCPS::TimeDuration;
using DCPS::MonotonicTimePoint;
using DCPS::SystemTimePoint;

const bool Sedp::host_is_bigendian_(!ACE_CDR_BYTE_ORDER);

Sedp::Sedp(const RepoId& participant_id, Spdp& owner, ACE_Thread_Mutex& lock) :
  DCPS::EndpointManager<ParticipantData_t>(participant_id, lock),
  spdp_(owner),
  publications_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER), ref(*this))),

#ifdef OPENDDS_SECURITY
  publications_secure_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER), ref(*this))),
#endif

  subscriptions_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER), ref(*this))),

#ifdef OPENDDS_SECURITY
  subscriptions_secure_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER), ref(*this))),
#endif

  participant_message_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER), ref(*this))),

#ifdef OPENDDS_SECURITY
  participant_message_secure_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER), ref(*this))),
  participant_stateless_message_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER), ref(*this))),
  dcps_participant_secure_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER), ref(*this), 2)),
  participant_volatile_message_secure_writer_(make_rch<Writer>(
    make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER), ref(*this))),
#endif

  publications_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER),
      ref(*this))),

#ifdef OPENDDS_SECURITY
  publications_secure_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER),
      ref(*this))),
#endif

  subscriptions_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER),
      ref(*this))),

#ifdef OPENDDS_SECURITY
  subscriptions_secure_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER),
      ref(*this))),
#endif

  participant_message_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER),
      ref(*this)))

#ifdef OPENDDS_SECURITY
  , participant_message_secure_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER),
      ref(*this)))
  , participant_stateless_message_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER),
      ref(*this)))
  , participant_volatile_message_secure_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER),
      ref(*this)))
  , dcps_participant_secure_reader_(make_rch<Reader>(
      make_id(participant_id, ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER),
      ref(*this)))
  , publication_agent_info_listener_(*this)
  , subscription_agent_info_listener_(*this)
#endif // OPENDDS_SECURITY
{}

DDS::ReturnCode_t
Sedp::init(const RepoId& guid,
           const RtpsDiscovery& disco,
           DDS::DomainId_t domainId)
{
  char domainStr[16];
  ACE_OS::snprintf(domainStr, 16, "%d", domainId);

  OPENDDS_STRING key = DCPS::GuidConverter(guid).uniqueId();

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
  rtps_inst->max_message_size_ = disco.config()->sedp_max_message_size();

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
#ifdef ACE_HAS_IPV6
  rtps_inst->ipv6_local_address_ = disco.config()->ipv6_sedp_local_address();
#endif

  rtps_relay_address(disco.config()->sedp_rtps_relay_address());
  rtps_inst->use_rtps_relay_ = disco.config()->use_rtps_relay();
  rtps_inst->rtps_relay_only_ = disco.config()->rtps_relay_only();

  stun_server_address(disco.config()->sedp_stun_server_address());
  rtps_inst->use_ice_ = disco.config()->use_ice();

  // Create a config
  OPENDDS_STRING config_name = DCPS::TransportRegistry::DEFAULT_INST_PREFIX +
                            OPENDDS_STRING("_SEDP_TransportCfg_") + key +
                            domainStr;
  transport_cfg_ = TheTransportRegistry->create_config(config_name.c_str());
  transport_cfg_->instances_.push_back(transport_inst_);

  reactor_task_ = transport_inst_->reactor_task();
  ACE_Reactor* reactor = reactor_task_->get_reactor();
  job_queue_ = DCPS::make_rch<DCPS::JobQueue>(reactor);

  // Configure and enable each reader/writer
  rtps_inst->opendds_discovery_default_listener_ = publications_reader_;
  rtps_inst->opendds_discovery_guid_ = guid;
  const bool reliable = true, durable = true;

#ifdef OPENDDS_SECURITY
  const bool besteffort = false, nondurable = false;
#endif

  if (spdp_.available_builtin_endpoints() & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    publications_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  publications_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);

#ifdef OPENDDS_SECURITY
  publications_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
  publications_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
  if (spdp_.available_builtin_endpoints() & DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    publications_secure_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  publications_secure_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);
#endif

  if (spdp_.available_builtin_endpoints() & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    subscriptions_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  subscriptions_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);

#ifdef OPENDDS_SECURITY
  subscriptions_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
  subscriptions_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
  if (spdp_.available_builtin_endpoints() & DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    subscriptions_secure_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  subscriptions_secure_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);
#endif

  if (spdp_.available_builtin_endpoints() & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
    participant_message_writer_->enable_transport_using_config(reliable, durable, transport_cfg_);
  }
  participant_message_reader_->enable_transport_using_config(reliable, durable, transport_cfg_);

#ifdef OPENDDS_SECURITY
  participant_message_secure_writer_->set_crypto_handles(spdp_.crypto_handle());
  participant_message_secure_reader_->set_crypto_handles(spdp_.crypto_handle());
  if (spdp_.available_builtin_endpoints() & DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
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

  set_permissions_handle(perm_handle);
  set_access_control(acl);
  set_crypto_key_factory(key_factory);
  set_crypto_key_exchange(key_exchange);
  crypto_handle_ = crypto_handle;

  // TODO: Handle all exceptions below once error-codes have been defined, etc.
  SecurityException ex = {"", 0, 0};

  bool ok = acl->get_participant_sec_attributes(perm_handle, participant_sec_attr_, ex);
  if (ok) {

    EndpointSecurityAttributes default_sec_attr;
    default_sec_attr.base.is_read_protected = false;
    default_sec_attr.base.is_write_protected = false;
    default_sec_attr.base.is_discovery_protected = false;
    default_sec_attr.base.is_liveliness_protected = false;
    default_sec_attr.is_submessage_protected = false;
    default_sec_attr.is_payload_protected = false;
    default_sec_attr.is_key_protected = false;
    default_sec_attr.plugin_endpoint_attributes = 0;

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
      local_writer_crypto_handles_[pvms_writer] = h;
      local_writer_security_attribs_[pvms_writer] = dw_sec_attr;

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
      local_reader_crypto_handles_[pvms_reader] = h;
      local_reader_security_attribs_[pvms_reader] = dr_sec_attr;
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
      local_writer_crypto_handles_[pms_writer] = h;
      local_writer_security_attribs_[pms_writer] = dw_sec_attr;

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
      local_reader_crypto_handles_[pms_reader] = h;
      local_reader_security_attribs_[pms_reader] = dr_sec_attr;
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
      local_writer_crypto_handles_[ps_writer] = h;
      local_writer_security_attribs_[ps_writer] = dw_sec_attr;

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
      local_reader_crypto_handles_[ps_reader] = h;
      local_reader_security_attribs_[ps_reader] = dr_sec_attr;
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
      local_writer_crypto_handles_[ss_writer] = h;
      local_writer_security_attribs_[ss_writer] = dw_sec_attr;

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
      local_reader_crypto_handles_[ss_reader] = h;
      local_reader_security_attribs_[ss_reader] = dr_sec_attr;
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
      local_writer_crypto_handles_[dps_writer] = h;
      local_writer_security_attribs_[dps_writer] = dw_sec_attr;

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
      local_reader_crypto_handles_[dps_reader] = h;
      local_reader_security_attribs_[dps_reader] = dr_sec_attr;
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

const ACE_INET_Addr&
Sedp::local_address() const
{
  DCPS::RtpsUdpInst_rch rtps_inst =
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  return rtps_inst->local_address_;
}

#ifdef ACE_HAS_IPV6
const ACE_INET_Addr&
Sedp::ipv6_local_address() const
{
  DCPS::RtpsUdpInst_rch rtps_inst =
      DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  return rtps_inst->ipv6_local_address_;
}
#endif

const ACE_INET_Addr&
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

  ACE_Message_Block mb_locator(4 + locator_count * sizeof(DCPS::Locator_t) + 1);
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
  assign(proto.remote_id_.guidPrefix, pdata.participantProxy.guidPrefix);
  populate_locators(proto.remote_data_, pdata);
}

#ifdef OPENDDS_SECURITY
void
Sedp::associate_preauth(Security::SPDPdiscoveredParticipantData& pdata)
{
  // First create a 'prototypical' instance of AssociationData.  It will
  // be copied and modified for each of the (up to) four SEDP Endpoints.
  DCPS::AssociationData proto;
  create_association_data_proto(proto, pdata);
  proto.remote_reliable_ = false;
  proto.remote_durable_ = false;

  const BuiltinEndpointSet_t& avail =
    pdata.participantProxy.availableBuiltinEndpoints;
  /*
   * Stateless messages are associated here because they are the first step in the
   * security-enablement process and as such they are sent in the clear.
   */

  if (avail & DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER &&
      (pdata.associated_endpoints & DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    participant_stateless_message_reader_->assoc(peer);
    pdata.associated_endpoints |= DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER;
  }

  if (avail & DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER &&
      (pdata.associated_endpoints & DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    participant_stateless_message_writer_->assoc(peer);
    pdata.associated_endpoints |= DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER;
  }
}
#endif

void
Sedp::associate(ParticipantData_t& pdata)
{
  // First create a 'prototypical' instance of AssociationData.  It will
  // be copied and modified for each of the (up to) four SEDP Endpoints.
  DCPS::AssociationData proto;
  create_association_data_proto(proto, pdata);

  const BuiltinEndpointSet_t& avail =
    pdata.participantProxy.availableBuiltinEndpoints;

  const BuiltinEndpointQos_t& beq =
    pdata.participantProxy.builtinEndpointQos;

  // See RTPS v2.1 section 8.5.5.1
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER &&
      (pdata.associated_endpoints & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    publications_reader_->assoc(peer);
    pdata.associated_endpoints |= DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR;
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER &&
      (pdata.associated_endpoints & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    subscriptions_reader_->assoc(peer);
    pdata.associated_endpoints |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER &&
      (pdata.associated_endpoints & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    participant_message_reader_->assoc(peer);
    pdata.associated_endpoints |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER;
  }

  if (spdp_.available_builtin_endpoints() & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER &&
      avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR &&
      (pdata.associated_endpoints & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    publications_writer_->assoc(peer);
    pdata.associated_endpoints |= DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER;
  }
  if (spdp_.available_builtin_endpoints() & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER &&
      avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR &&
      (pdata.associated_endpoints & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    subscriptions_writer_->assoc(peer);
    pdata.associated_endpoints |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
  }
  if (spdp_.available_builtin_endpoints() & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER &&
      avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER &&
      (pdata.associated_endpoints & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) == 0) {
    DCPS::AssociationData peer = proto;
    if (beq & BEST_EFFORT_PARTICIPANT_MESSAGE_DATA_READER) {
      peer.remote_reliable_ = false;
    }
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    participant_message_writer_->assoc(peer);
    pdata.associated_endpoints |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER;
  }

  //FUTURE: if/when topic propagation is supported, add it here

  // Process deferred publications and subscriptions.
  for (DeferredSubscriptionMap::iterator pos = deferred_subscriptions_.lower_bound(proto.remote_id_),
         limit = deferred_subscriptions_.upper_bound(proto.remote_id_);
       pos != limit;
       /* Increment in body. */) {
    data_received (pos->second.first, pos->second.second);
    deferred_subscriptions_.erase (pos++);
  }
  for (DeferredPublicationMap::iterator pos = deferred_publications_.lower_bound(proto.remote_id_),
         limit = deferred_publications_.upper_bound(proto.remote_id_);
       pos != limit;
       /* Increment in body. */) {
    data_received (pos->second.first, pos->second.second);
    deferred_publications_.erase (pos++);
  }

  if (spdp_.shutting_down()) { return; }

  proto.remote_id_.entityId = ENTITYID_PARTICIPANT;
  associated_participants_.insert(proto.remote_id_);
}

#ifdef OPENDDS_SECURITY
void Sedp::associate_volatile(Security::SPDPdiscoveredParticipantData& pdata)
{
  using namespace DDS::Security;

  DCPS::AssociationData proto;
  create_association_data_proto(proto, pdata);
  proto.remote_reliable_ = true;
  proto.remote_durable_ = false;

  DCPS::RepoId part = proto.remote_id_;
  part.entityId = ENTITYID_PARTICIPANT;

  const BuiltinEndpointSet_t& avail = pdata.participantProxy.availableBuiltinEndpoints;

  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER &&
      (pdata.associated_endpoints & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    remote_writer_crypto_handles_[peer.remote_id_] = generate_remote_matched_writer_crypto_handle(
      part, participant_volatile_message_secure_reader_->get_endpoint_crypto_handle());
    peer.remote_data_ = add_security_info(
      peer.remote_data_, peer.remote_id_, participant_volatile_message_secure_reader_->get_repo_id());
    participant_volatile_message_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER &&
      (pdata.associated_endpoints & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    remote_reader_crypto_handles_[peer.remote_id_] = generate_remote_matched_reader_crypto_handle(
      part, participant_volatile_message_secure_writer_->get_endpoint_crypto_handle(), false);
    peer.remote_data_ = add_security_info(
      peer.remote_data_, participant_volatile_message_secure_writer_->get_repo_id(), peer.remote_id_);
    participant_volatile_message_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER;
  }
}

void Sedp::disassociate_volatile(Security::SPDPdiscoveredParticipantData& pdata)
{
  using namespace DDS::Security;

  const RepoId part = make_id(pdata.participantProxy.guidPrefix, ENTITYID_PARTICIPANT);

  disassociate_helper(pdata.associated_endpoints,
                      BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER,
                      part,
                      ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER,
                      *participant_volatile_message_secure_writer_);
  disassociate_helper(pdata.associated_endpoints,
                      BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER,
                      part,
                      ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER,
                      *participant_volatile_message_secure_reader_);
}

void Sedp::associate_secure_endpoints(Security::SPDPdiscoveredParticipantData& pdata,
                                      const DDS::Security::ParticipantSecurityAttributes& participant_sec_attr)
{
  // If an endpoint needs a crypto token, then it will be associated after the crypto tokens have been received.

  using namespace DDS::Security;

  DCPS::AssociationData proto;
  create_association_data_proto(proto, pdata);

  const BuiltinEndpointSet_t& avail = pdata.participantProxy.availableBuiltinEndpoints;
  const BuiltinEndpointQos_t& beq = pdata.participantProxy.builtinEndpointQos;

  if (!participant_sec_attr.is_liveliness_protected &&
      avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER &&
      (pdata.associated_endpoints & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    peer.remote_data_ = add_security_info(
                                          peer.remote_data_, peer.remote_id_, participant_message_secure_reader_->get_repo_id());
    participant_message_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
  }
  if (!participant_sec_attr.is_discovery_protected &&
      avail & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER &&
      (pdata.associated_endpoints & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    peer.remote_data_ = add_security_info(
                                          peer.remote_data_, peer.remote_id_, dcps_participant_secure_reader_->get_repo_id());
    dcps_participant_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= SPDP_BUILTIN_PARTICIPANT_SECURE_READER;
  }
  if (!participant_sec_attr.is_discovery_protected &&
      avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER &&
      (pdata.associated_endpoints & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    peer.remote_data_ = add_security_info(
                                          peer.remote_data_, peer.remote_id_, publications_secure_reader_->get_repo_id());
    publications_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
  }
  if (!participant_sec_attr.is_discovery_protected &&
      avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER &&
      (pdata.associated_endpoints & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    peer.remote_data_ = add_security_info(
                                          peer.remote_data_, peer.remote_id_, subscriptions_secure_reader_->get_repo_id());
    subscriptions_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
  }

  if (!participant_sec_attr.is_liveliness_protected &&
      avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER &&
      (pdata.associated_endpoints & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    if (beq & BEST_EFFORT_PARTICIPANT_MESSAGE_DATA_READER) {
      peer.remote_reliable_ = false;
    }
    peer.remote_data_ = add_security_info(
                                          peer.remote_data_, participant_message_secure_writer_->get_repo_id(), peer.remote_id_);
    participant_message_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
  }
  if (!participant_sec_attr.is_discovery_protected &&
      avail & SPDP_BUILTIN_PARTICIPANT_SECURE_READER &&
      (pdata.associated_endpoints & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    peer.remote_data_ = add_security_info(
                                          peer.remote_data_, dcps_participant_secure_writer_->get_repo_id(), peer.remote_id_);
    dcps_participant_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER;
  }
  if (!participant_sec_attr.is_discovery_protected &&
      spdp_.available_builtin_endpoints() & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER &&
      (pdata.associated_endpoints & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    peer.remote_data_ = add_security_info(
                                          peer.remote_data_, publications_secure_writer_->get_repo_id(), peer.remote_id_);
    publications_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
  }
  if (!participant_sec_attr.is_discovery_protected &&
      spdp_.available_builtin_endpoints() & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER &&
      (pdata.associated_endpoints & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) == 0) {
    DCPS::AssociationData peer = proto;
    peer.remote_id_.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    peer.remote_data_ = add_security_info(
                                          peer.remote_data_, subscriptions_secure_writer_->get_repo_id(), peer.remote_id_);
    subscriptions_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
  }
}

#endif // OPENDDS_SECURITY

void Sedp::disassociate_helper(BuiltinEndpointSet_t& associated_endpoints, const CORBA::ULong flags,
                               const RepoId& id, const EntityId_t& ent, DCPS::TransportClient& client)
{
  if (associated_endpoints & flags) {
    client.disassociate(make_id(id, ent));
    associated_endpoints &= ~flags;
  }
}

#ifdef OPENDDS_SECURITY

void Sedp::remove_remote_crypto_handle(const RepoId& participant, const EntityId_t& entity)
{
  using namespace DDS::Security;
  using DCPS::DatareaderCryptoHandleMap;
  using DCPS::DatawriterCryptoHandleMap;

  const RepoId remote = make_id(participant, entity);
  SecurityException se = {"", 0, 0};
  CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();

  const DCPS::GuidConverter traits(remote);
  if (traits.isReader()) {
    const DatareaderCryptoHandleMap::iterator iter = remote_reader_crypto_handles_.find(remote);
    if (iter == remote_reader_crypto_handles_.end()) {
      return;
    }
    if (!key_factory->unregister_datareader(iter->second, se)) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::remove_remote_crypto_handle() - ")
                 ACE_TEXT("Failure calling unregister_datareader(). Security Exception[%d.%d]: %C\n"),
                 se.code, se.minor_code, se.message.in()));
    }
    remote_reader_crypto_handles_.erase(iter);

  } else if (traits.isWriter()) {
    const DatawriterCryptoHandleMap::iterator iter = remote_writer_crypto_handles_.find(remote);
    if (iter == remote_writer_crypto_handles_.end()) {
      return;
    }
    if (!key_factory->unregister_datawriter(iter->second, se)) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::remove_remote_crypto_handle() - ")
                 ACE_TEXT("Failure calling unregister_datawriter(). Security Exception[%d.%d]: %C\n"),
                 se.code, se.minor_code, se.message.in()));
    }
    remote_writer_crypto_handles_.erase(iter);
  }
}

void Sedp::generate_remote_crypto_handles(const Security::SPDPdiscoveredParticipantData& pdata)
{
  using namespace DDS::Security;

  const DCPS::RepoId part = make_id(pdata.participantProxy.guidPrefix, ENTITYID_PARTICIPANT);
  DCPS::RepoId remote_id = part;

  const BuiltinEndpointSet_t& avail = pdata.participantProxy.availableBuiltinEndpoints;

  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    remote_writer_crypto_handles_[remote_id] = generate_remote_matched_writer_crypto_handle(
      part, participant_message_secure_reader_->get_endpoint_crypto_handle());
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    remote_writer_crypto_handles_[remote_id] = generate_remote_matched_writer_crypto_handle(
      part, dcps_participant_secure_reader_->get_endpoint_crypto_handle());
  }
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    remote_writer_crypto_handles_[remote_id] = generate_remote_matched_writer_crypto_handle(
      part, publications_secure_reader_->get_endpoint_crypto_handle());
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    remote_writer_crypto_handles_[remote_id] = generate_remote_matched_writer_crypto_handle(
      part, subscriptions_secure_reader_->get_endpoint_crypto_handle());
  }

  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    remote_id.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    remote_reader_crypto_handles_[remote_id] = generate_remote_matched_reader_crypto_handle(
      part, participant_message_secure_writer_->get_endpoint_crypto_handle(), false);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) {
    remote_id.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    remote_reader_crypto_handles_[remote_id] = generate_remote_matched_reader_crypto_handle(
      part, dcps_participant_secure_writer_->get_endpoint_crypto_handle(), false);
  }
  if (spdp_.available_builtin_endpoints() & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    remote_reader_crypto_handles_[remote_id] = generate_remote_matched_reader_crypto_handle(
      part, publications_secure_writer_->get_endpoint_crypto_handle(), false);
  }
  if (spdp_.available_builtin_endpoints() & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    remote_id.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    remote_reader_crypto_handles_[remote_id] = generate_remote_matched_reader_crypto_handle(
      part, subscriptions_secure_writer_->get_endpoint_crypto_handle(), false);
  }
}

void Sedp::associate_secure_reader_to_writer(const RepoId& remote_writer)
{
  using namespace DDS::Security;

  ParticipantData_t& pdata = spdp_.get_participant_data(remote_writer);

  DCPS::AssociationData peer;
  create_association_data_proto(peer, pdata);
  peer.remote_id_ = remote_writer;

  const BuiltinEndpointSet_t& avail = pdata.participantProxy.availableBuiltinEndpoints;

  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER &&
      remote_writer.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER &&
      (pdata.associated_endpoints & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) == 0) {
    peer.remote_data_ = add_security_info(
      peer.remote_data_, peer.remote_id_, participant_message_secure_reader_->get_repo_id());
    participant_message_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER &&
      remote_writer.entityId == ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER &&
      (pdata.associated_endpoints & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) == 0) {
    peer.remote_data_ = add_security_info(
      peer.remote_data_, peer.remote_id_, dcps_participant_secure_reader_->get_repo_id());
    dcps_participant_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= SPDP_BUILTIN_PARTICIPANT_SECURE_READER;
  }
  if (remote_writer.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER &&
      (pdata.associated_endpoints & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) == 0) {
    peer.remote_data_ = add_security_info(
      peer.remote_data_, peer.remote_id_, publications_secure_reader_->get_repo_id());
    publications_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER &&
      remote_writer.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER &&
      (pdata.associated_endpoints & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) == 0) {
    peer.remote_data_ = add_security_info(
      peer.remote_data_, peer.remote_id_, subscriptions_secure_reader_->get_repo_id());
    subscriptions_secure_reader_->assoc(peer);
    pdata.associated_endpoints |= SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
  }
}

void Sedp::associate_secure_writer_to_reader(const RepoId& remote_reader)
{
  using namespace DDS::Security;

  ParticipantData_t& pdata = spdp_.get_participant_data(remote_reader);

  DCPS::AssociationData peer;
  create_association_data_proto(peer, pdata);
  peer.remote_id_ = remote_reader;

  const BuiltinEndpointSet_t& avail = pdata.participantProxy.availableBuiltinEndpoints;
  const BuiltinEndpointQos_t& beq = pdata.participantProxy.builtinEndpointQos;

  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER &&
      (pdata.associated_endpoints & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) == 0 &&
      remote_reader.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    if (beq & BEST_EFFORT_PARTICIPANT_MESSAGE_DATA_READER) {
      peer.remote_reliable_ = false;
    }
    peer.remote_data_ = add_security_info(
      peer.remote_data_, participant_message_secure_writer_->get_repo_id(), peer.remote_id_);
    participant_message_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_READER &&
      (pdata.associated_endpoints & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) == 0 &&
      remote_reader.entityId == ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER) {
    peer.remote_data_ = add_security_info(
      peer.remote_data_, dcps_participant_secure_writer_->get_repo_id(), peer.remote_id_);
    dcps_participant_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER;
  }
  if (spdp_.available_builtin_endpoints() & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER &&
      (pdata.associated_endpoints & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) == 0 &&
      remote_reader.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    peer.remote_data_ = add_security_info(
      peer.remote_data_, publications_secure_writer_->get_repo_id(), peer.remote_id_);
    publications_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
  }
  if (spdp_.available_builtin_endpoints() & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER &&
      (pdata.associated_endpoints & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) == 0 &&
      remote_reader.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    peer.remote_data_ = add_security_info(
      peer.remote_data_, subscriptions_secure_writer_->get_repo_id(), peer.remote_id_);
    subscriptions_secure_writer_->assoc(peer);
    pdata.associated_endpoints |= SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
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
Sedp::send_builtin_crypto_tokens(
  const DCPS::RepoId& dstParticipant, const DCPS::EntityId_t& dstEntity, const DCPS::RepoId& src)
{
  const DCPS::RepoId dst = make_id(dstParticipant, dstEntity);
  if (DCPS::GuidConverter(src).isReader()) {
    create_and_send_datareader_crypto_tokens(
      local_reader_crypto_handles_[src], src,
      remote_writer_crypto_handles_[dst], dst);
  } else {
    create_and_send_datawriter_crypto_tokens(
      local_writer_crypto_handles_[src], src,
      remote_reader_crypto_handles_[dst], dst);
  }
}

void
Sedp::send_builtin_crypto_tokens(const DCPS::RepoId& remoteId)
{
  using namespace DDS::Security;

  const DCPS::RepoId part = make_id(remoteId, ENTITYID_PARTICIPANT);
  const ParticipantData_t& pdata = spdp_.get_participant_data(part);

  const BuiltinEndpointSet_t& avail = pdata.participantProxy.availableBuiltinEndpoints;

  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
    send_builtin_crypto_tokens(part, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER,
                               participant_message_secure_reader_->get_repo_id());
  }

  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) {
    send_builtin_crypto_tokens(part, ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER,
                               dcps_participant_secure_reader_->get_repo_id());
  }

  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    send_builtin_crypto_tokens(part, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER,
                               publications_secure_reader_->get_repo_id());
  }

  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    send_builtin_crypto_tokens(part, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER,
                               subscriptions_secure_reader_->get_repo_id());
  }

  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    send_builtin_crypto_tokens(part, ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER,
                               participant_message_secure_writer_->get_repo_id());
  }

  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) {
    send_builtin_crypto_tokens(part, ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER,
                               dcps_participant_secure_writer_->get_repo_id());
  }

  if (spdp_.available_builtin_endpoints() & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    send_builtin_crypto_tokens(part, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER,
                               publications_secure_writer_->get_repo_id());
  }

  if (spdp_.available_builtin_endpoints() & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER &&
      avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    send_builtin_crypto_tokens(part, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER,
                               subscriptions_secure_writer_->get_repo_id());
  }
}
#endif

bool
Sedp::disassociate(ParticipantData_t& pdata)
{
  const RepoId part = make_id(pdata.participantProxy.guidPrefix, ENTITYID_PARTICIPANT);

  associated_participants_.erase(part);

  { // Release lock, so we can call into transport
    ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);
    ACE_GUARD_RETURN(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock, false);

    disassociate_helper(pdata.associated_endpoints,
                        DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER,
                        part,
                        ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER,
                        *publications_writer_);
    disassociate_helper(pdata.associated_endpoints,
                        DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR,
                        part,
                        ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER,
                        *publications_reader_);
    disassociate_helper(pdata.associated_endpoints,
                        DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER,
                        part,
                        ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER,
                        *subscriptions_writer_);
    disassociate_helper(pdata.associated_endpoints,
                        DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR,
                        part,
                        ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER,
                        *subscriptions_reader_);
    disassociate_helper(pdata.associated_endpoints,
                        BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER,
                        part,
                        ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER,
                        *participant_message_writer_);
    disassociate_helper(pdata.associated_endpoints,
                        BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER,
                        part,
                        ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER,
                        *participant_message_reader_);

    //FUTURE: if/when topic propagation is supported, add it here

#ifdef OPENDDS_SECURITY
    disassociate_security_builtins(pdata);
#endif
  }

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
    };
    for (size_t i = 0; i < sizeof secure_entities / sizeof secure_entities[0]; ++i) {
      remove_remote_crypto_handle(part, secure_entities[i]);
    }

    const DDS::Security::CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();
    DDS::Security::SecurityException se;

    const RepoId key = make_id(part, ENTITYID_UNKNOWN);
    for (DCPS::DatareaderCryptoHandleMap::iterator pos = remote_reader_crypto_handles_.lower_bound(key);
         pos != remote_reader_crypto_handles_.end() &&
         std::memcmp(pos->first.guidPrefix, pdata.participantProxy.guidPrefix,
                     sizeof(pdata.participantProxy.guidPrefix)) == 0;) {
      if (!key_factory->unregister_datareader(pos->second, se)) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::disassociate() - ")
                   ACE_TEXT("Failure calling unregister_datareader(). Security Exception[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      remote_reader_crypto_handles_.erase(pos++);
    }
    for (DCPS::DatawriterCryptoHandleMap::iterator pos = remote_writer_crypto_handles_.lower_bound(key);
         pos != remote_writer_crypto_handles_.end() &&
         std::memcmp(pos->first.guidPrefix, pdata.participantProxy.guidPrefix,
                     sizeof(pdata.participantProxy.guidPrefix)) == 0;) {
      if (!key_factory->unregister_datawriter(pos->second, se)) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Sedp::disassociate() - ")
                   ACE_TEXT("Failure calling unregister_datawriter(). Security Exception[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      remote_writer_crypto_handles_.erase(pos++);
    }
  }
#endif

  if (spdp_.has_discovered_participant(part)) {
    remove_entities_belonging_to(discovered_publications_, part);
    remove_entities_belonging_to(discovered_subscriptions_, part);
    return true;
  } else {
    return false;
  }
}

#ifdef OPENDDS_SECURITY
void Sedp::disassociate_security_builtins(ParticipantData_t& pdata)
{
  using namespace DDS::Security;

  const RepoId part = make_id(pdata.participantProxy.guidPrefix, ENTITYID_PARTICIPANT);

  disassociate_helper(pdata.associated_endpoints,
                      SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER,
                      part,
                      ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER,
                      *publications_secure_writer_);
  disassociate_helper(pdata.associated_endpoints,
                      SEDP_BUILTIN_PUBLICATIONS_SECURE_READER,
                      part,
                      ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER,
                      *publications_secure_reader_);
  disassociate_helper(pdata.associated_endpoints,
                      SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER,
                      part,
                      ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER,
                      *subscriptions_secure_writer_);
  disassociate_helper(pdata.associated_endpoints,
                      SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER,
                      part,
                      ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER,
                      *subscriptions_secure_reader_);
  disassociate_helper(pdata.associated_endpoints,
                      BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER,
                      part,
                      ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER,
                      *participant_message_secure_writer_);
  disassociate_helper(pdata.associated_endpoints,
                      BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER,
                      part,
                      ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER,
                      *participant_message_secure_reader_);
  disassociate_helper(pdata.associated_endpoints,
                      BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER,
                      part,
                      ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER,
                      *participant_stateless_message_writer_);
  disassociate_helper(pdata.associated_endpoints,
                      BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER,
                      part,
                      ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER,
                      *participant_stateless_message_reader_);
  disassociate_helper(pdata.associated_endpoints,
                      BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER,
                      part,
                      ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER,
                      *participant_volatile_message_secure_writer_);
  disassociate_helper(pdata.associated_endpoints,
                      BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER,
                      part,
                      ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER,
                      *participant_volatile_message_secure_reader_);
  disassociate_helper(pdata.associated_endpoints,
                      SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER,
                      part,
                      ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER,
                      *dcps_participant_secure_writer_);
  disassociate_helper(pdata.associated_endpoints,
                      SPDP_BUILTIN_PARTICIPANT_SECURE_READER,
                      part,
                      ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER,
                      *dcps_participant_secure_reader_);
}
#endif

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
#endif
}

template<typename Map>
void
Sedp::remove_entities_belonging_to(Map& m, RepoId participant)
{
  participant.entityId = ENTITYID_UNKNOWN;
  for (typename Map::iterator i = m.lower_bound(participant);
       i != m.end() && 0 == std::memcmp(i->first.guidPrefix,
                                        participant.guidPrefix,
                                        sizeof(GuidPrefix_t));) {
    OPENDDS_STRING topic_name = get_topic_name(i->second);
    OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
      topics_.find(topic_name);
    if (top_it != topics_.end()) {
      top_it->second.remove_pub_sub(i->first);
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
    remove_from_bit(i->second);
    m.erase(i++);
  }
}

void
Sedp::remove_from_bit_i(const DiscoveredPublication& pub)
{
#ifndef DDS_HAS_MINIMUM_BIT
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);
  ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);

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
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);
  ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);

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
DCPS::TopicBuiltinTopicDataDataReaderImpl*
Sedp::topic_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_TOPIC_TOPIC);
  return dynamic_cast<DCPS::TopicBuiltinTopicDataDataReaderImpl*>(d.in());
}

DCPS::PublicationBuiltinTopicDataDataReaderImpl*
Sedp::pub_bit()
{
  DDS::Subscriber_var sub = spdp_.bit_subscriber();
  if (!sub.in())
    return 0;

  DDS::DataReader_var d =
    sub->lookup_datareader(DCPS::BUILT_IN_PUBLICATION_TOPIC);
  return dynamic_cast<DCPS::PublicationBuiltinTopicDataDataReaderImpl*>(d.in());
}

DCPS::SubscriptionBuiltinTopicDataDataReaderImpl*
Sedp::sub_bit()
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
    for (RepoIdSet::const_iterator topic_endpoints = topic.endpoints().begin();
         topic_endpoints != topic.endpoints().end(); ++topic_endpoints) {

      const RepoId& rid = *topic_endpoints;
      GuidConverter conv(rid);
      if (conv.isWriter()) {
        // This may be our local publication, verify
        LocalPublicationIter lp = local_publications_.find(rid);
        if (lp != local_publications_.end()) {
          write_publication_data(rid, lp->second);
        }
      } else if (conv.isReader()) {
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

DDS::ReturnCode_t
Sedp::remove_publication_i(const RepoId& publicationId, LocalPublication& pub)
{
#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = pub.publication_->get_ice_endpoint();
  if (endpoint) {
    ICE::Agent::instance()->remove_local_agent_info_listener(endpoint, publicationId);
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
Sedp::remove_subscription_i(const RepoId& subscriptionId,
                            LocalSubscription& sub)
{
#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = sub.subscription_->get_ice_endpoint();
  if (endpoint) {
    ICE::Agent::instance()->remove_local_agent_info_listener(endpoint, subscriptionId);
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
  publications_reader_->shutting_down();
  subscriptions_reader_->shutting_down();
  participant_message_reader_->shutting_down();
#ifdef OPENDDS_SECURITY
  publications_secure_reader_->shutting_down();
  subscriptions_secure_reader_->shutting_down();
  participant_message_secure_reader_->shutting_down();
  participant_stateless_message_reader_->shutting_down();
  participant_volatile_message_secure_reader_->shutting_down();
  dcps_participant_secure_reader_->shutting_down();
#endif
  publications_writer_->shutting_down();
  subscriptions_writer_->shutting_down();
  participant_message_writer_->shutting_down();
#ifdef OPENDDS_SECURITY
  publications_secure_writer_->shutting_down();
  subscriptions_secure_writer_->shutting_down();
  participant_message_secure_writer_->shutting_down();
  participant_stateless_message_writer_->shutting_down();
  participant_volatile_message_secure_writer_->shutting_down();
  dcps_participant_secure_writer_->shutting_down();
#endif
}

void Sedp::process_discovered_writer_data(DCPS::MessageId message_id,
                                          const DCPS::DiscoveredWriterData& wdata,
                                          const RepoId& guid
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
      // Must unlock when calling into pub_bit() as it may call back into us
      ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

      { // Reduce scope of pub and td
        DiscoveredPublication prepub(wdata);

#ifdef OPENDDS_SECURITY
        prepub.have_ice_agent_info_ = have_ice_agent_info;
        prepub.ice_agent_info_ = ice_agent_info;
#endif
        topic_name = get_topic_name(prepub);

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

          DCPS::AuthState auth_state = spdp_.lookup_participant_auth_state(participant_id);
          if (auth_state == DCPS::AUTH_STATE_AUTHENTICATED) {

            DDS::Security::PermissionsHandle remote_permissions = spdp_.lookup_participant_permissions(participant_id);

            if (participant_sec_attr_.is_access_protected &&
                !get_access_control()->check_remote_topic(remote_permissions, spdp_.get_domain_id(), data, ex))
            {
              ACE_ERROR((LM_WARNING,
                ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("Sedp::data_received(dwd) - ")
                ACE_TEXT("Unable to check remote topic '%C'. SecurityException[%d.%d]: %C\n"),
                topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }

            DDS::Security::TopicSecurityAttributes topic_sec_attr;
            if (!get_access_control()->get_topic_sec_attributes(remote_permissions, topic_name.data(), topic_sec_attr, ex))
            {
              ACE_ERROR((LM_WARNING,
                ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("Sedp::data_received(dwd) - ")
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
                ACE_TEXT("Sedp::data_received(dwd) - ")
                ACE_TEXT("Unable to check remote datawriter '%C'. SecurityException[%d.%d]: %C\n"),
                topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }
          } else if (auth_state != DCPS::AUTH_STATE_UNAUTHENTICATED) {
            ACE_ERROR((LM_WARNING,
              ACE_TEXT("(%P|%t) WARNING: ")
              ACE_TEXT("Sedp::data_received(dwd) - ")
              ACE_TEXT("Unsupported remote participant authentication state for discovered datawriter '%C'. ")
              ACE_TEXT("SecurityException[%d.%d]: %C\n"),
              topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
            return;
          }
        }
#endif

        DiscoveredPublication& pub = discovered_publications_[guid] = prepub;

        // Create a topic if necessary.
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it = topics_.find(topic_name);
        if (top_it == topics_.end()) {
          top_it = topics_.insert(std::make_pair(topic_name, TopicDetails())).first;
          DCPS::RepoId topic_id = make_topic_guid();
          top_it->second.init(topic_name, topic_id);
          topic_names_[topic_id] = topic_name;
        }

        TopicDetails& td = top_it->second;

        // Upsert the remote topic.
        td.add_pub_sub(guid, wdata.ddsPublicationData.type_name.in());

        assign_bit_key(pub);
        wdata_copy = pub.writer_data_;
      }

      // Iter no longer valid once lock released
      iter = discovered_publications_.end();

      DDS::InstanceHandle_t instance_handle = DDS::HANDLE_NIL;
#ifndef DDS_HAS_MINIMUM_BIT
      {
        // Release lock for call into pub_bit
        DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
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

    } else {
      if (checkAndAssignQos(iter->second.writer_data_.ddsPublicationData,
                            wdata.ddsPublicationData)) { // update existing

#ifndef DDS_HAS_MINIMUM_BIT
        DCPS::PublicationBuiltinTopicDataDataReaderImpl* bit = pub_bit();
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
          iter = discovered_publications_.find(guid);
          if (iter == discovered_publications_.end()) {
            return;
          }
        }
      }

      if (checkAndAssignLocators(iter->second.writer_data_.writerProxy, wdata.writerProxy)) {
        topic_name = get_topic_name(iter->second);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::const_iterator top_it = topics_.find(topic_name);
        using DCPS::RepoIdSet;
        const RepoIdSet& assoc =
          (top_it == topics_.end()) ? RepoIdSet() : top_it->second.endpoints();
        for (RepoIdSet::const_iterator i = assoc.begin(); i != assoc.end(); ++i) {
          if (DCPS::GuidConverter(*i).isWriter()) continue; // publication
          LocalSubscriptionIter lsi = local_subscriptions_.find(*i);
          if (lsi != local_subscriptions_.end()) {
            lsi->second.subscription_->update_locators(guid, wdata.writerProxy.allLocators);
          }
        }
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
        top_it->second.remove_pub_sub(guid);
        match_endpoints(guid, top_it->second, true /*remove*/);
        if (spdp_.shutting_down()) { return; }
        if (top_it->second.is_dead()) {
          purge_dead_topic(topic_name);
        }
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
Sedp::data_received(DCPS::MessageId message_id,
                    const DiscoveredPublication& dpub)
{
  if (spdp_.shutting_down()) { return; }

  const DCPS::DiscoveredWriterData& wdata = dpub.writer_data_;
  const RepoId& guid = wdata.writerProxy.remoteWriterGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

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
    deferred_publications_[guid] = std::make_pair(message_id, dpub);
    return;
  }

  process_discovered_writer_data(message_id, wdata, guid
#ifdef OPENDDS_SECURITY
                                 , dpub.have_ice_agent_info_, dpub.ice_agent_info_
#endif
                                 );
}

#ifdef OPENDDS_SECURITY
void Sedp::data_received(DCPS::MessageId message_id,
                         const DiscoveredPublication_SecurityWrapper& wrapper)
{
  if (spdp_.shutting_down()) { return; }

  const RepoId& guid = wrapper.data.writerProxy.remoteWriterGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(wrapper.data.ddsPublicationData.topic_name)) {
    return;
  }

  process_discovered_writer_data(message_id, wrapper.data, guid, wrapper.have_ice_agent_info, wrapper.ice_agent_info, &wrapper.security_info);
}
#endif

void Sedp::process_discovered_reader_data(DCPS::MessageId message_id,
                                          const DCPS::DiscoveredReaderData& rdata,
                                          const RepoId& guid
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

  // Must unlock when calling into sub_bit() as it may call back into us
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

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
      { // Reduce scope of sub and td
        DiscoveredSubscription presub(rdata);
#ifdef OPENDDS_SECURITY
        presub.have_ice_agent_info_ = have_ice_agent_info;
        presub.ice_agent_info_ = ice_agent_info;
#endif

        topic_name = get_topic_name(presub);

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

          DCPS::AuthState auth_state = spdp_.lookup_participant_auth_state(participant_id);
          if (auth_state == DCPS::AUTH_STATE_AUTHENTICATED) {

            DDS::Security::PermissionsHandle remote_permissions = spdp_.lookup_participant_permissions(participant_id);

            if (participant_sec_attr_.is_access_protected &&
                !get_access_control()->check_remote_topic(remote_permissions, spdp_.get_domain_id(), data, ex))
            {
              ACE_ERROR((LM_WARNING,
                ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("Sedp::data_received(drd) - ")
                ACE_TEXT("Unable to check remote topic '%C'. SecurityException[%d.%d]: %C\n"),
                topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }

            DDS::Security::TopicSecurityAttributes topic_sec_attr;
            if (!get_access_control()->get_topic_sec_attributes(remote_permissions, topic_name.data(), topic_sec_attr, ex))
            {
              ACE_ERROR((LM_WARNING,
                ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("Sedp::data_received(drd) - ")
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
                ACE_TEXT("Sedp::data_received(drd) - ")
                ACE_TEXT("Unable to check remote datareader '%C'. SecurityException[%d.%d]: %C\n"),
                topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
              return;
            }

            if (relay_only) {
              relay_only_readers_.insert(guid);
            } else {
              relay_only_readers_.erase(guid);
            }
          } else if (auth_state != DCPS::AUTH_STATE_UNAUTHENTICATED) {
            ACE_ERROR((LM_WARNING,
              ACE_TEXT("(%P|%t) WARNING: ")
              ACE_TEXT("Sedp::data_received(dwd) - ")
              ACE_TEXT("Unsupported remote participant authentication state for discovered datawriter '%C'. ")
              ACE_TEXT("SecurityException[%d.%d]: %C\n"),
              topic_name.data(), ex.code, ex.minor_code, ex.message.in()));
            return;
          }
        }
#endif

        DiscoveredSubscription& sub = discovered_subscriptions_[guid] = presub;

        // Create a topic if necessary.
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it = topics_.find(topic_name);
        if (top_it == topics_.end()) {
          top_it = topics_.insert(std::make_pair(topic_name, TopicDetails())).first;
          DCPS::RepoId topic_id = make_topic_guid();
          top_it->second.init(topic_name, topic_id);
          topic_names_[topic_id] = topic_name;
        }

        TopicDetails& td = top_it->second;

        // Upsert the remote topic.
        td.add_pub_sub(guid, rdata.ddsSubscriptionData.type_name.in());

        assign_bit_key(sub);
        rdata_copy = sub.reader_data_;
      }

      // Iter no longer valid once lock released
      iter = discovered_subscriptions_.end();

      DDS::InstanceHandle_t instance_handle = DDS::HANDLE_NIL;
#ifndef DDS_HAS_MINIMUM_BIT
      {
        // Release lock for call into sub_bit
        DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
        if (bit) { // bit may be null if the DomainParticipant is shutting down
          ACE_GUARD(ACE_Reverse_Lock< ACE_Thread_Mutex>, rg, rev_lock);
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
      if (checkAndAssignQos(iter->second.reader_data_.ddsSubscriptionData,
                            rdata.ddsSubscriptionData)) {
#ifndef DDS_HAS_MINIMUM_BIT
        DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* bit = sub_bit();
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
          iter = discovered_subscriptions_.find(guid);
          if (iter == discovered_subscriptions_.end()) {
            return;
          }
        }
      }

      if (checkAndAssignParams(iter->second.reader_data_.contentFilterProperty,
                               rdata.contentFilterProperty)) {
        // Let any associated local publications know about the change
        topic_name = get_topic_name(iter->second);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
            topics_.find(topic_name);
        using DCPS::RepoIdSet;
        const RepoIdSet& assoc =
          (top_it == topics_.end()) ? RepoIdSet() : top_it->second.endpoints();
        for (RepoIdSet::const_iterator i = assoc.begin(); i != assoc.end(); ++i) {
          if (DCPS::GuidConverter(*i).isReader()) continue; // subscription
          const LocalPublicationIter lpi = local_publications_.find(*i);
          if (lpi != local_publications_.end()) {
            lpi->second.publication_->update_subscription_params(guid,
              rdata.contentFilterProperty.expressionParameters);
          }
        }
      }

      if (checkAndAssignLocators(iter->second.reader_data_.readerProxy, rdata.readerProxy)) {
        topic_name = get_topic_name(iter->second);
        OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::const_iterator top_it =
          topics_.find(topic_name);
        using DCPS::RepoIdSet;
        const RepoIdSet& assoc =
          (top_it == topics_.end()) ? RepoIdSet() : top_it->second.endpoints();
        for (RepoIdSet::const_iterator i = assoc.begin(); i != assoc.end(); ++i) {
          if (i->entityId.entityKind & 4) continue; // subscription
          LocalPublicationIter lpi = local_publications_.find(*i);
          if (lpi != local_publications_.end()) {
            lpi->second.publication_->update_locators(guid, rdata.readerProxy.allLocators);
          }
        }
      }
    }

    if (is_expectant_opendds(guid)) {
      // For each associated opendds writer to this reader
      CORBA::ULong len = rdata.readerProxy.associatedWriters.length();
      for (CORBA::ULong writerIndex = 0; writerIndex < len; ++writerIndex)
      {
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
      topic_name = get_topic_name(iter->second);
      OPENDDS_MAP(OPENDDS_STRING, TopicDetails)::iterator top_it =
          topics_.find(topic_name);
      if (top_it != topics_.end()) {
        top_it->second.remove_pub_sub(guid);
        if (DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sedp::data_received(drd) - ")
                               ACE_TEXT("calling match_endpoints disp/unreg\n")));
        }
        match_endpoints(guid, top_it->second, true /*remove*/);
        if (top_it->second.is_dead()) {
          purge_dead_topic(topic_name);
        }
        if (spdp_.shutting_down()) { return; }
      }
      remove_from_bit(iter->second);
      discovered_subscriptions_.erase(iter);
    }
  }
}

void
Sedp::data_received(DCPS::MessageId message_id,
                    const DiscoveredSubscription& dsub)
{
  if (spdp_.shutting_down()) { return; }

  const DCPS::DiscoveredReaderData& rdata = dsub.reader_data_;
  const RepoId& guid = rdata.readerProxy.remoteReaderGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(rdata.ddsSubscriptionData.topic_name)) {
    return;
  }

#ifdef OPENDDS_SECURITY
  if (message_id == DCPS::SAMPLE_DATA && should_drop_message(rdata.ddsSubscriptionData.topic_name)) {
    return;
  }
#endif

  if (!spdp_.has_discovered_participant(guid_participant)) {
    deferred_subscriptions_[guid] = std::make_pair(message_id, dsub);
    return;
  }

  process_discovered_reader_data(message_id, rdata, guid
#ifdef OPENDDS_SECURITY
                                 , dsub.have_ice_agent_info_, dsub.ice_agent_info_
#endif
                                 );
}

#ifdef OPENDDS_SECURITY
void Sedp::data_received(DCPS::MessageId message_id,
                         const DiscoveredSubscription_SecurityWrapper& wrapper)
{
  if (spdp_.shutting_down()) { return; }

  const RepoId& guid = wrapper.data.readerProxy.remoteReaderGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid)
      || ignoring(guid_participant)
      || ignoring(wrapper.data.ddsSubscriptionData.topic_name)) {
    return;
  }

  process_discovered_reader_data(message_id, wrapper.data, guid, wrapper.have_ice_agent_info, wrapper.ice_agent_info, &wrapper.security_info);
}
#endif

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

  if (!spdp_.has_discovered_participant(guid_participant)) {
    return;
  }

  for (LocalSubscriptionMap::const_iterator sub_pos = local_subscriptions_.begin(),
         sub_limit = local_subscriptions_.end();
       sub_pos != sub_limit; ++sub_pos) {
    const DCPS::RepoIdSet::const_iterator pos =
      sub_pos->second.matched_endpoints_.lower_bound(prefix);
    if (pos != sub_pos->second.matched_endpoints_.end() &&
        DCPS::GuidPrefixEqual()(pos->guidPrefix, prefix.guidPrefix)) {
      sub_pos->second.subscription_->signal_liveliness(guid_participant);
    }
  }
}

#ifdef OPENDDS_SECURITY
void
Sedp::received_participant_message_data_secure(DCPS::MessageId /*message_id*/,
            const ParticipantMessageData& data)
{
  if (spdp_.shutting_down()) {
      return;
  }

  const RepoId& guid = data.participantGuid;
  RepoId guid_participant = guid;
  guid_participant.entityId = ENTITYID_PARTICIPANT;
  RepoId prefix = data.participantGuid;
  prefix.entityId = EntityId_t(); // Clear the entityId so lower bound will work.

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (ignoring(guid) || ignoring(guid_participant)) {
    return;
  }

  if (!spdp_.has_discovered_participant(guid_participant)) {
    return;
  }

  LocalSubscriptionMap::const_iterator i, n;
  for (i = local_subscriptions_.begin(), n = local_subscriptions_.end(); i != n; ++i) {
    const DCPS::RepoIdSet::const_iterator pos = i->second.matched_endpoints_.lower_bound(prefix);

    if (pos != i->second.matched_endpoints_.end() && DCPS::GuidPrefixEqual()(pos->guidPrefix, prefix.guidPrefix)) {
      (i->second.subscription_)->signal_liveliness(guid_participant);
    }
  }
}

bool Sedp::should_drop_stateless_message(const DDS::Security::ParticipantGenericMessage& msg)
{
  using DCPS::GUID_t;
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
  using DCPS::GUID_t;
  using DCPS::GUID_UNKNOWN;

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, true);

  const GUID_t src_endpoint = msg.source_endpoint_guid;
  const GUID_t dst_participant = msg.destination_participant_guid;
  const GUID_t this_participant = participant_id_;

  if (ignoring(src_endpoint) || !msg.message_data.length()) {
    return true;
  }

  if (dst_participant != GUID_UNKNOWN && dst_participant != this_participant) {
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
    return;
  }

  if (0 == std::strcmp(msg.message_class_id,
                       DDS::Security::GMCLASSID_SECURITY_AUTH_REQUEST)) {
    spdp_.handle_auth_request(msg);

  } else if (0 == std::strcmp(msg.message_class_id,
                              DDS::Security::GMCLASSID_SECURITY_AUTH_HANDSHAKE)) {
    spdp_.handle_handshake_message(msg);
  }
  return;
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
    spdp_.volatile_association_complete(remoteId);
    spdp_.send_participant_crypto_tokens(remoteId);
    send_builtin_crypto_tokens(remoteId);
    resend_user_crypto_tokens(remoteId);
  }
#endif
  if (remoteId.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER) {
    write_durable_publication_data(remoteId, false);
  } else if (remoteId.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER) {
    write_durable_subscription_data(remoteId, false);
  } else if (remoteId.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER) {
    write_durable_participant_message_data(remoteId);
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
    const RepoId& guid = make_id(participant_id_, DCPS::EntityIdConverter(PARTICIPANT_MESSAGE_DATA_KIND_AUTOMATIC_LIVELINESS_UPDATE));
    write_participant_message_data(guid, local_participant_messages_[guid], GUID_UNKNOWN);
    break;
  }

  case DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS: {
    const RepoId& guid = make_id(participant_id_, DCPS::EntityIdConverter(PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE));
    write_participant_message_data(guid, local_participant_messages_[guid], GUID_UNKNOWN);
    break;
  }

  case DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS:
    // Do nothing.
    break;
  }
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
    const RepoId& guid = make_id(participant_id_, DCPS::EntityIdConverter(PARTICIPANT_MESSAGE_DATA_KIND_AUTOMATIC_LIVELINESS_UPDATE));
    write_participant_message_data_secure(guid, local_participant_messages_secure_[guid], GUID_UNKNOWN);
    break;
  }

  case DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS: {
    const RepoId& guid = make_id(participant_id_, DCPS::EntityIdConverter(PARTICIPANT_MESSAGE_DATA_KIND_MANUAL_LIVELINESS_UPDATE));
    write_participant_message_data_secure(guid, local_participant_messages_secure_[guid], GUID_UNKNOWN);
    break;
  }

  case DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS:
    // Do nothing.
    break;
  }
}
#endif

ICE::Endpoint* Sedp::get_ice_endpoint() {
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

void
Sedp::Writer::transport_assoc_done(int flags, const RepoId& remote) {
  if (!(flags & ASSOC_OK)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) Sedp::Writer::transport_assoc_done: ")
               ACE_TEXT("ERROR: transport layer failed to associate %C\n"),
               DCPS::LogGuid(remote).c_str()));
    return;
  }

  if (shutting_down_.value()) {
    return;
  }

  if (is_reliable()) {
    ACE_GUARD(ACE_Thread_Mutex, g, sedp_.lock_);
    sedp_.association_complete_i(repo_id_, remote);
  } else {
    sedp_.association_complete_i(repo_id_, remote);
  }
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

void Sedp::Writer::send_sample(const ACE_Message_Block& data,
                               size_t size,
                               const RepoId& reader,
                               DCPS::SequenceNumber& sequence,
                               bool historic)
{
  DCPS::DataSampleElement* el = new DCPS::DataSampleElement(repo_id_, this, DCPS::PublicationInstance_rch());
  set_header_fields(el->get_header(), size, reader, sequence, historic);

  DCPS::Message_Block_Ptr sample(new ACE_Message_Block(size));
  el->set_sample(DCPS::move(sample));
  *el->get_sample() << el->get_header();
  el->get_sample()->cont(data.duplicate());

  if (reader != GUID_UNKNOWN) {
    el->set_sub_id(0, reader);
    el->set_num_subs(1);
  }

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
  size_t size = 0, padding = 0;
  DCPS::find_size_ulong(size, padding);
  DCPS::gen_find_size(plist, size, padding);

  // Build RTPS message
  ACE_Message_Block payload(DCPS::DataSampleHeader::max_marshaled_size(),
                            ACE_Message_Block::MB_DATA,
                            new ACE_Message_Block(size + padding));
  using DCPS::Serializer;
  Serializer ser(payload.cont(), host_is_bigendian_, Serializer::ALIGN_CDR);
  bool ok = (ser << ACE_OutputCDR::from_octet(0)) &&  // PL_CDR_LE = 0x0003
            (ser << ACE_OutputCDR::from_octet(3)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << plist);

  if (ok) {
    send_sample(payload, size + padding, reader, sequence, reader != GUID_UNKNOWN);

  } else {
    result = DDS::RETCODE_ERROR;
  }

  delete payload.cont();
  return result;
}

DDS::ReturnCode_t
Sedp::Writer::write_participant_message(const ParticipantMessageData& pmd,
                                        const RepoId& reader,
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
                            new ACE_Message_Block(size + padding));
  using DCPS::Serializer;
  Serializer ser(payload.cont(), host_is_bigendian_, Serializer::ALIGN_CDR);
  bool ok = (ser << ACE_OutputCDR::from_octet(0)) &&  // CDR_LE = 0x0001
            (ser << ACE_OutputCDR::from_octet(1)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << pmd);

  if (ok) {
    send_sample(payload, size + padding, reader, sequence, reader != GUID_UNKNOWN);

  } else {
      result = DDS::RETCODE_ERROR;
  }

  delete payload.cont();
  return result;
}

#ifdef OPENDDS_SECURITY
DDS::ReturnCode_t
Sedp::Writer::write_stateless_message(const DDS::Security::ParticipantStatelessMessage& msg,
                                      const RepoId& reader,
                                      DCPS::SequenceNumber& sequence)
{
  using DCPS::Serializer;

  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  size_t size = 0, padding = 0;
  DCPS::find_size_ulong(size, padding);
  DCPS::gen_find_size(msg, size, padding);

  ACE_Message_Block payload(
      DCPS::DataSampleHeader::max_marshaled_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size + padding));

  Serializer ser(payload.cont(), host_is_bigendian_, Serializer::ALIGN_CDR);
  bool ok = (ser << ACE_OutputCDR::from_octet(0)) &&  // CDR_LE = 0x0001
            (ser << ACE_OutputCDR::from_octet(1)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << ACE_OutputCDR::from_octet(0));
  ser.reset_alignment(); // https://issues.omg.org/browse/DDSIRTP23-63
  ok &= (ser << msg);

  if (ok) {
    send_sample(payload, size + padding, reader, sequence);

  } else {
    result = DDS::RETCODE_ERROR;
  }

  delete payload.cont();
  return result;
}

DDS::ReturnCode_t
Sedp::Writer::write_volatile_message_secure(const DDS::Security::ParticipantVolatileMessageSecure& msg,
                                            const RepoId& reader,
                                            DCPS::SequenceNumber& sequence)
{
  using DCPS::Serializer;

  DDS::ReturnCode_t result = DDS::RETCODE_OK;

  size_t size = 0, padding = 0;
  DCPS::find_size_ulong(size, padding);
  DCPS::gen_find_size(msg, size, padding);

  ACE_Message_Block payload(
      DCPS::DataSampleHeader::max_marshaled_size(),
      ACE_Message_Block::MB_DATA,
      new ACE_Message_Block(size + padding));

  Serializer ser(payload.cont(), host_is_bigendian_, Serializer::ALIGN_CDR);
  bool ok = (ser << ACE_OutputCDR::from_octet(0)) &&  // CDR_LE = 0x0001
            (ser << ACE_OutputCDR::from_octet(1)) &&
            (ser << ACE_OutputCDR::from_octet(0)) &&
            (ser << ACE_OutputCDR::from_octet(0));
  ser.reset_alignment(); // https://issues.omg.org/browse/DDSIRTP23-63
  ok &= (ser << msg);

  if (ok) {
    send_sample(payload, size + padding, reader, sequence);

  } else {
    result = DDS::RETCODE_ERROR;
  }

  delete payload.cont();
  return result;
}

DDS::ReturnCode_t
Sedp::Writer::write_dcps_participant_secure(const Security::SPDPdiscoveredParticipantData& msg,
                                            const RepoId& reader, DCPS::SequenceNumber& sequence)
{
  using DCPS::Serializer;

  ParameterList plist;

  if (!ParameterListConverter::to_param_list(msg, plist)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Sedp::write_dcps_participant_secure - ")
               ACE_TEXT("Failed to convert SPDPdiscoveredParticipantData ")
               ACE_TEXT("to ParameterList\n")));

    return DDS::RETCODE_ERROR;
  }

  ICE::AgentInfoMap ai_map;
  ICE::Endpoint* sedp_endpoint = get_ice_endpoint();
  if (sedp_endpoint) {
    ai_map[SEDP_AGENT_INFO_KEY] = ICE::Agent::instance()->get_local_agent_info(sedp_endpoint);
  }
  ICE::Endpoint* spdp_endpoint = sedp_.spdp_.get_ice_endpoint_if_added();
  if (spdp_endpoint) {
    ai_map[SPDP_AGENT_INFO_KEY] = ICE::Agent::instance()->get_local_agent_info(spdp_endpoint);
  }
  if (!ParameterListConverter::to_param_list(ai_map, plist)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("Sedp::write_dcps_participant_secure() - ")
               ACE_TEXT("failed to convert from ICE::AgentInfo ")
               ACE_TEXT("to ParameterList\n")));
    return DDS::RETCODE_ERROR;
  }

  return write_parameter_list(plist, reader, sequence);
}
#endif

DDS::ReturnCode_t
Sedp::Writer::write_unregister_dispose(const RepoId& rid, CORBA::UShort pid)
{
  // Build param list for message
  Parameter param;
  param.guid(rid);
  param._d(pid);
  ParameterList plist;
  plist.length(1);
  plist[0] = param;

  // Determine message length
  size_t size = 0, padding = 0;
  DCPS::find_size_ulong(size, padding);
  DCPS::gen_find_size(plist, size, padding);

  DCPS::Message_Block_Ptr payload(new ACE_Message_Block(DCPS::DataSampleHeader::max_marshaled_size(),
                            ACE_Message_Block::MB_DATA,
                            new ACE_Message_Block(size)));

  if (!payload) {
    ACE_ERROR((LM_ERROR,
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
  const void* pReader = static_cast<const void*>(&reader);
  DCPS::Message_Block_Ptr mb(new ACE_Message_Block (DCPS::DataSampleHeader::max_marshaled_size(),
                                                 ACE_Message_Block::MB_DATA,
                                                 new ACE_Message_Block(static_cast<const char*>(pReader),
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
Sedp::Writer::write_control_msg(DCPS::Message_Block_Ptr payload,
                                size_t size,
                                DCPS::MessageId id,
                                DCPS::SequenceNumber seq)
{
  DCPS::DataSampleHeader header;
  set_header_fields(header, size, GUID_UNKNOWN, seq, false, id);
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

  if (id != DCPS::END_HISTORIC_SAMPLES &&
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

//-------------------------------------------------------------------------

Sedp::Reader::~Reader()
{
}

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

  if (sample.header_.key_fields_only_ && encap < 2) {
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
  if (shutting_down_.value()) {
    return;
  }

  switch (sample.header_.message_id_) {
  case DCPS::SAMPLE_DATA:
  case DCPS::DISPOSE_INSTANCE:
  case DCPS::UNREGISTER_INSTANCE:
  case DCPS::DISPOSE_UNREGISTER_INSTANCE: {
    const DCPS::MessageId id =
      static_cast<DCPS::MessageId>(sample.header_.message_id_);

    DCPS::Serializer ser(sample.sample_.get(),
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    ACE_CDR::Octet encap, dummy;
    ACE_CDR::UShort options;
    const bool ok = (ser >> ACE_InputCDR::to_octet(dummy))
              && (ser >> ACE_InputCDR::to_octet(encap))
              && (ser >> options);
    if (!ok) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                 ACE_TEXT("failed to deserialize encap and options\n")));
      return;
    }

    // Ignore the 'encap' byte order since we use sample.header_.byte_order_
    // to determine whether or not to swap bytes.

    if (sample.header_.publication_id_.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER) {
      ParameterList data;
      if (!decode_parameter_list(sample, ser, encap, data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }

      DiscoveredPublication wdata;
      if (!ParameterListConverter::from_param_list(data, wdata.writer_data_)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to DiscoveredWriterData\n")));
        return;
      }
#ifdef OPENDDS_SECURITY
      wdata.have_ice_agent_info_ = false;
      ICE::AgentInfoMap ai_map;
      if (!ParameterListConverter::from_param_list(data, ai_map)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
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
      sedp_.data_received(id, wdata);

#ifdef OPENDDS_SECURITY
    } else if (sample.header_.publication_id_.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
      ParameterList data;
      if (!decode_parameter_list(sample, ser, encap, data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }

      DiscoveredPublication_SecurityWrapper wdata_secure;

      if (!ParameterListConverter::from_param_list(data, wdata_secure)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to DiscoveredPublication_SecurityWrapper\n")));
        return;
      }

      wdata_secure.have_ice_agent_info = false;
      ICE::AgentInfoMap ai_map;
      if (!ParameterListConverter::from_param_list(data, ai_map)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to ICE Agent info\n")));
        return;
      }
      ICE::AgentInfoMap::const_iterator pos = ai_map.find("DATA");
      if (pos != ai_map.end()) {
        wdata_secure.have_ice_agent_info = true;
        wdata_secure.ice_agent_info = pos->second;
      }
      sedp_.data_received(id, wdata_secure);

#endif

    } else if (sample.header_.publication_id_.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER) {
      ParameterList data;
      if (!decode_parameter_list(sample, ser, encap, data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }

      DiscoveredSubscription rdata;
      if (!ParameterListConverter::from_param_list(data, rdata.reader_data_)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to DiscoveredReaderData\n")));
        return;
      }
#ifdef OPENDDS_SECURITY
      rdata.have_ice_agent_info_ = false;
      ICE::AgentInfoMap ai_map;
      if (!ParameterListConverter::from_param_list(data, ai_map)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
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
      sedp_.data_received(id, rdata);

#ifdef OPENDDS_SECURITY
    } else if (sample.header_.publication_id_.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
      ParameterList data;
      if (!decode_parameter_list(sample, ser, encap, data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }

      DiscoveredSubscription_SecurityWrapper rdata;

      if (!ParameterListConverter::from_param_list(data, rdata)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to DiscoveredSubscription_SecurityWrapper\n")));
        return;
      }

      rdata.have_ice_agent_info = false;
      ICE::AgentInfoMap ai_map;
      if (!ParameterListConverter::from_param_list(data, ai_map)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to ICE Agent info\n")));
        return;
      }
      ICE::AgentInfoMap::const_iterator pos = ai_map.find("DATA");
      if (pos != ai_map.end()) {
        rdata.have_ice_agent_info = true;
        rdata.ice_agent_info = pos->second;
      }

      if ((rdata.data).readerProxy.expectsInlineQos) {
        set_inline_qos((rdata.data).readerProxy.allLocators);
      }
      sedp_.data_received(id, rdata);

#endif

    } else if (sample.header_.publication_id_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER
               && !sample.header_.key_fields_only_) {
      ParticipantMessageData data;
      if (!(ser >> data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }
      sedp_.data_received(id, data);

#ifdef OPENDDS_SECURITY
    } else if (sample.header_.publication_id_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER
               && !sample.header_.key_fields_only_) {

      ParticipantMessageData data;

      if (!(ser >> data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }
      sedp_.received_participant_message_data_secure(id, data);

    } else if (sample.header_.publication_id_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER) {

      DDS::Security::ParticipantStatelessMessage data;
      ser.reset_alignment(); // https://issues.omg.org/browse/DDSIRTP23-63
      if (!(ser >> data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }
      sedp_.received_stateless_message(id, data);

    } else if (sample.header_.publication_id_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER) {

      DDS::Security::ParticipantVolatileMessageSecure data;
      ser.reset_alignment(); // https://issues.omg.org/browse/DDSIRTP23-63
      if (!(ser >> data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }
      sedp_.received_volatile_message_secure(id, data);

    } else if (sample.header_.publication_id_.entityId == ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER) {

      ParameterList data;
      if (!decode_parameter_list(sample, ser, encap, data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to deserialize data\n")));
        return;
      }

      Security::SPDPdiscoveredParticipantData pdata;

      if (!ParameterListConverter::from_param_list(data, pdata)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::Reader::data_received - ")
                   ACE_TEXT("failed to convert from ParameterList ")
                   ACE_TEXT("to Security::SPDPdiscoveredParticipantData\n")));
        return;
      }
      const DCPS::RepoId guid = make_guid(sample.header_.publication_id_.guidPrefix, DCPS::ENTITYID_PARTICIPANT);
      sedp_.spdp_.process_participant_ice(data, pdata, guid);
      sedp_.spdp_.handle_participant_data(id, pdata, DCPS::SequenceNumber::ZERO(), ACE_INET_Addr(), true);

#endif

    }
    break;
  }

  default:
    break;
  }
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
  drd.ddsSubscriptionData.presentation = sub.subscriber_qos_.presentation;
  drd.ddsSubscriptionData.partition = sub.subscriber_qos_.partition;
  drd.ddsSubscriptionData.topic_data = topic_details.local_qos().topic_data;
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
        sub.remote_expectant_opendds_associations_.begin();
       writer != sub.remote_expectant_opendds_associations_.end();
       ++writer) {
    CORBA::ULong len = drd.readerProxy.associatedWriters.length();
    drd.readerProxy.associatedWriters.length(len + 1);
    drd.readerProxy.associatedWriters[len] = *writer;
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
#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = pub.publication_->get_ice_endpoint();
  if (endpoint) {
    pub.have_ice_agent_info = true;
    pub.ice_agent_info = ICE::Agent::instance()->get_local_agent_info(endpoint);
    ICE::Agent::instance()->add_local_agent_info_listener(endpoint, rid, &publication_agent_info_listener_);
    start_ice(rid, pub);
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
    if (!ParameterListConverter::to_param_list(dwd, plist, false)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data - ")
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
                   ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data - ")
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
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_publication_data - ")
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
    if (!ParameterListConverter::to_param_list(dwd, plist, false)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data - ")
                 ACE_TEXT("Failed to convert DiscoveredWriterData ")
                 ACE_TEXT("to ParameterList\n")));
      result = DDS::RETCODE_ERROR;
    }
    if (lp.have_ice_agent_info) {
      ICE::AgentInfoMap ai_map;
      ai_map["DATA"] = lp.ice_agent_info;
      if (!ParameterListConverter::to_param_list(ai_map, plist)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: Sedp::write_publication_data - ")
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
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Sedp::write_publication_data - ")
                        ACE_TEXT("not currently associated, dropping msg.\n")));
  }
  return result;
}
#endif

DDS::ReturnCode_t
Sedp::add_subscription_i(const DCPS::RepoId& rid,
                         LocalSubscription& sub)
{
#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = sub.subscription_->get_ice_endpoint();
  if (endpoint) {
    sub.have_ice_agent_info = true;
    sub.ice_agent_info = ICE::Agent::instance()->get_local_agent_info(endpoint);
    ICE::Agent::instance()->add_local_agent_info_listener(endpoint, rid, &subscription_agent_info_listener_);
    start_ice(rid, sub);
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
    if (!ParameterListConverter::to_param_list(drd, plist, false)) {
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
    if (!ParameterListConverter::to_param_list(drd, plist, false)) {
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
  if (spdp_.associated() && (reader != GUID_UNKNOWN ||
                             !associated_participants_.empty())) {
    ParticipantMessageData pmd;
    pmd.participantGuid = rid;
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
      const CORBA::ULong len = locators[i].data.length();
      locators[i].data.length(len + 1);
      locators[i].data[len] = CORBA::Octet(1);
    }
  }
}

bool
Sedp::shutting_down() const
{
  return spdp_.shutting_down();
}

void
Sedp::populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& rTls,
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
  if (!rTls->length()) {     // if no locators provided, add the default
    if (!participant_found) {
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
                                          const RepoId& writer)
{
  DCPS::LocatorSeq locs;
  bool participantExpectsInlineQos = false;
  RepoId remote_participant = writer;
  remote_participant.entityId = ENTITYID_PARTICIPANT;
  const bool participant_found =
    spdp_.get_default_locators(remote_participant, locs,
                               participantExpectsInlineQos);
  if (!wTls->length()) {     // if no locators provided, add the default
    if (!participant_found) {
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

#ifdef OPENDDS_SECURITY
DCPS::TransportLocatorSeq
Sedp::add_security_info(const DCPS::TransportLocatorSeq& locators,
                        const RepoId& writer, const RepoId& reader)
{
  using DCPS::Serializer;
  using namespace DDS::Security;

  if (std::memcmp(writer.guidPrefix, reader.guidPrefix,
                  sizeof(DCPS::GuidPrefix_t)) == 0) {
    // writer and reader belong to the same participant, no security needed
    return locators;
  }

  DCPS::RepoId remote_part;
  const DCPS::RepoId local_part = spdp_.guid();
  if (std::memcmp(writer.guidPrefix, local_part.guidPrefix, sizeof writer.guidPrefix) == 0) {
    remote_part = reader;
  } else if (std::memcmp(reader.guidPrefix, local_part.guidPrefix, sizeof reader.guidPrefix) == 0) {
    remote_part = writer;
  } else {
    return locators;
  }

  remote_part.entityId = ENTITYID_PARTICIPANT;
  ParticipantCryptoHandle part_handle = spdp_.lookup_participant_crypto_info(remote_part).first;

  if (part_handle == DDS::HANDLE_NIL) {
    // security not enabled for this discovered participant
    return locators;
  }

  DatawriterCryptoHandle dw_handle = DDS::HANDLE_NIL;
  DatareaderCryptoHandle dr_handle = DDS::HANDLE_NIL;
  EndpointSecurityAttributesMask mask = 0;

  if (local_reader_crypto_handles_.find(reader) != local_reader_crypto_handles_.end() &&
      remote_writer_crypto_handles_.find(writer) != remote_writer_crypto_handles_.end()) {
    dr_handle = local_reader_crypto_handles_[reader];
    dw_handle = remote_writer_crypto_handles_[writer];
    mask = security_attributes_to_bitmask(local_reader_security_attribs_[reader]);
  } else if (local_writer_crypto_handles_.find(writer) != local_writer_crypto_handles_.end() &&
             remote_reader_crypto_handles_.find(reader) != remote_reader_crypto_handles_.end()) {
    dw_handle = local_writer_crypto_handles_[writer];
    dr_handle = remote_reader_crypto_handles_[reader];
    mask = security_attributes_to_bitmask(local_writer_security_attribs_[writer]);
  }

  DCPS::TransportLocatorSeq_var newLoc;
  DDS::OctetSeq added;
  for (unsigned int i = 0; i < locators.length(); ++i) {
    if (std::strcmp(locators[i].transport_type.in(), "rtps_udp") == 0) {
      if (!newLoc) {
        newLoc = new DCPS::TransportLocatorSeq(locators);

        DDS::OctetSeq handleOctets = handle_to_octets(part_handle);
        const DDS::BinaryProperty_t prop = {BLOB_PROP_PART_CRYPTO_HANDLE,
                                            handleOctets, true /*serialize*/};
        DDS::OctetSeq handleOctetsDw = handle_to_octets(dw_handle);
        const DDS::BinaryProperty_t dw_p = {BLOB_PROP_DW_CRYPTO_HANDLE,
                                            handleOctetsDw, true /*serialize*/};
        DDS::OctetSeq handleOctetsDr = handle_to_octets(dr_handle);
        const DDS::BinaryProperty_t dr_p = {BLOB_PROP_DR_CRYPTO_HANDLE,
                                            handleOctetsDr, true /*serialize*/};
        DDS::OctetSeq endpointSecAttr(static_cast<unsigned int>(DCPS::max_marshaled_size_ulong()));
        endpointSecAttr.length(endpointSecAttr.maximum());
        std::memcpy(endpointSecAttr.get_buffer(), &mask, endpointSecAttr.length());
        const DDS::BinaryProperty_t esa_p = {BLOB_PROP_ENDPOINT_SEC_ATTR,
                                             endpointSecAttr, true /*serialize*/};
        size_t size = 0, padding = 0;
        DCPS::gen_find_size(prop, size, padding);
        if (dw_handle != DDS::HANDLE_NIL) {
          DCPS::gen_find_size(dw_p, size, padding);
        }
        if (dr_handle != DDS::HANDLE_NIL) {
          DCPS::gen_find_size(dr_p, size, padding);
        }
        DCPS::gen_find_size(esa_p, size, padding);
        ACE_Message_Block mb(size + padding);
        Serializer ser(&mb, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
        ser << prop;
        if (dw_handle != DDS::HANDLE_NIL) {
          ser << dw_p;
        }
        if (dr_handle != DDS::HANDLE_NIL) {
          ser << dr_p;
        }
        ser << esa_p;
        added.length(static_cast<unsigned int>(mb.size()));
        std::memcpy(added.get_buffer(), mb.rd_ptr(), mb.size());
      }

      const unsigned int prevLen = newLoc[i].data.length();
      newLoc[i].data.length(prevLen + added.length());
      std::memcpy(newLoc[i].data.get_buffer() + prevLen, added.get_buffer(),
                  added.length());
    }
  }

  return newLoc ? newLoc : locators;
}
#endif

#ifdef OPENDDS_SECURITY
DDS::Security::DatawriterCryptoHandle
Sedp::generate_remote_matched_writer_crypto_handle(
  const RepoId& writer_part, const DDS::Security::DatareaderCryptoHandle& drch)
{
  DDS::Security::DatawriterCryptoHandle result = DDS::HANDLE_NIL;

  DDS::Security::CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();

  Spdp::ParticipantCryptoInfoPair info = spdp_.lookup_participant_crypto_info(writer_part);

  if (info.first != DDS::HANDLE_NIL && info.second) {
    DDS::Security::SecurityException se = {"", 0, 0};
    result = key_factory->register_matched_remote_datawriter(drch, info.first, info.second, se);
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
Sedp::generate_remote_matched_reader_crypto_handle(const RepoId& reader_part,
                                                   const DDS::Security::DatawriterCryptoHandle& dwch,
                                                   bool relay_only)
{
  DDS::Security::DatareaderCryptoHandle result = DDS::HANDLE_NIL;

  DDS::Security::CryptoKeyFactory_var key_factory = spdp_.get_security_config()->get_crypto_key_factory();

  Spdp::ParticipantCryptoInfoPair info = spdp_.lookup_participant_crypto_info(reader_part);

  if (info.first != DDS::HANDLE_NIL && info.second) {
    DDS::Security::SecurityException se = {"", 0, 0};
    result = key_factory->register_matched_remote_datareader(dwch, info.first, info.second, relay_only, se);
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

  DCPS::DatawriterCryptoHandleMap::const_iterator w_iter = remote_writer_crypto_handles_.find(msg.source_endpoint_guid);
  DCPS::DatareaderCryptoHandleMap::const_iterator r_iter = local_reader_crypto_handles_.find(msg.destination_endpoint_guid);

  DDS::Security::DatawriterCryptoTokenSeq dwcts;
  dwcts = reinterpret_cast<const DDS::Security::DatawriterCryptoTokenSeq&>(msg.message_data);

  if (w_iter == remote_writer_crypto_handles_.end()) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Sedp::handle_datawriter_crypto_tokens() - ")
        ACE_TEXT("received tokens for unknown remote writer %C Caching.\n"),
        DCPS::LogGuid(msg.source_endpoint_guid).c_str()));
    }

    pending_remote_writer_crypto_tokens_[msg.source_endpoint_guid] = dwcts;
    return true;
  }

  if (r_iter == local_reader_crypto_handles_.end()) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) Sedp::handle_datawriter_crypto_tokens() - ")
        ACE_TEXT("received tokens for unknown local reader. Ignoring.\n")));
    }
    return false;
  }

  if (!key_exchange->set_remote_datawriter_crypto_tokens(r_iter->second, w_iter->second, dwcts, se)) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("(%P|%t) WARNING: Sedp::handle_datawriter_crypto_tokens() - ")
      ACE_TEXT("Unable to set remote datawriter crypto tokens with crypto key exchange plugin. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"), se.code, se.minor_code, se.message.in()));
    return false;
  }

  if (DCPS::GuidConverter(msg.source_endpoint_guid).isBuiltinDomainEntity()) {
    associate_secure_reader_to_writer(w_iter->first);
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

  DCPS::DatareaderCryptoHandleMap::const_iterator r_iter = remote_reader_crypto_handles_.find(msg.source_endpoint_guid);
  DCPS::DatawriterCryptoHandleMap::const_iterator w_iter = local_writer_crypto_handles_.find(msg.destination_endpoint_guid);

  DDS::Security::DatareaderCryptoTokenSeq drcts;
  drcts = reinterpret_cast<const DDS::Security::DatareaderCryptoTokenSeq&>(msg.message_data);

  if (r_iter == remote_reader_crypto_handles_.end()) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Sedp::handle_datareader_crypto_tokens() - ")
        ACE_TEXT("received tokens for unknown remote reader %C Caching.\n"),
        DCPS::LogGuid(msg.source_endpoint_guid).c_str()));
    }

    pending_remote_reader_crypto_tokens_[msg.source_endpoint_guid] = drcts;
    return true;
  }

  if (w_iter == local_writer_crypto_handles_.end()) {
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
               DCPS::LogGuid(msg.source_endpoint_guid).c_str(), w_iter->second, r_iter->second, drcts.length()));
  }

  if (!key_exchange->set_remote_datareader_crypto_tokens(w_iter->second, r_iter->second, drcts, se)) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("(%P|%t) WARNING: Sedp::handle_datareader_crypto_tokens() - ")
      ACE_TEXT("Unable to set remote datareader crypto tokens with crypto key exchange plugin. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"), se.code, se.minor_code, se.message.in()));
    return false;
  }

  if (DCPS::GuidConverter(msg.source_endpoint_guid).isBuiltinDomainEntity()) {
    associate_secure_writer_to_reader(r_iter->first);
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

  const DCPS::DatareaderCryptoHandleMap::iterator lrchm_end =
    local_reader_crypto_handles_.end();
  for (DCPS::DatareaderCryptoHandleMap::iterator lrchmi =
        local_reader_crypto_handles_.begin();
      lrchmi != lrchm_end; ++lrchmi) {
    const RepoId& local_reader = lrchmi->first;
    const DDS::Security::DatareaderCryptoHandle local_reader_ch = lrchmi->second;
    if (DCPS::GuidConverter(local_reader).isUserDomainEntity()) {
      const LocalSubscriptionIter lsi = local_subscriptions_.find(local_reader);
      if (lsi != local_subscriptions_.end()) {
        const DCPS::RepoIdSet::iterator me_end = lsi->second.matched_endpoints_.end();
        for (DCPS::RepoIdSet::iterator mei = lsi->second.matched_endpoints_.begin();
            mei != me_end; ++mei) {
          const RepoId& remote_writer = *mei;
          if (DCPS::GuidPrefixEqual()(remote_writer.guidPrefix, remote_participant.guidPrefix)) {
            const DCPS::DatawriterCryptoHandleMap::iterator rdwchmi =
              remote_writer_crypto_handles_.find(remote_writer);
            if (rdwchmi != remote_writer_crypto_handles_.end()) {
              const DDS::Security::DatawriterCryptoHandle remote_writer_ch = rdwchmi->second;
              create_and_send_datareader_crypto_tokens(
                local_reader_ch, local_reader,
                remote_writer_ch, remote_writer);
            }
          }
        }
      }
    }
  }

  /*
   * For each user DataWriter that has a crypto handle, send tokens to matched
   * DataReaders that have a crypto handle.
   */
  const DCPS::DatawriterCryptoHandleMap::iterator lwchm_end =
    local_writer_crypto_handles_.end();
  for (DCPS::DatawriterCryptoHandleMap::iterator lwchmi =
        local_writer_crypto_handles_.begin();
      lwchmi != lwchm_end; ++lwchmi) {
    const RepoId& local_writer = lwchmi->first;
    const DDS::Security::DatawriterCryptoHandle local_writer_ch = lwchmi->second;
    if (DCPS::GuidConverter(local_writer).isUserDomainEntity()) {
      const LocalPublicationIter lpi = local_publications_.find(local_writer);
      if (lpi != local_publications_.end()) {
        const DCPS::RepoIdSet::iterator me_end = lpi->second.matched_endpoints_.end();
        for (DCPS::RepoIdSet::iterator mei = lpi->second.matched_endpoints_.begin();
            mei != me_end; ++mei) {
          const RepoId& remote_reader = *mei;
          if (DCPS::GuidPrefixEqual()(remote_reader.guidPrefix, remote_participant.guidPrefix)) {
            const DCPS::DatareaderCryptoHandleMap::iterator rdrchmi =
              remote_reader_crypto_handles_.find(remote_reader);
            if (rdrchmi != remote_reader_crypto_handles_.end()) {
              const DDS::Security::DatareaderCryptoHandle remote_reader_ch = rdrchmi->second;
              create_and_send_datawriter_crypto_tokens(
                local_writer_ch, local_writer,
                remote_reader_ch, remote_reader);
            }
          }
        }
      }
    }
  }
}
#endif

void
Sedp::add_assoc_i(const DCPS::RepoId& local_guid, const LocalPublication& lpub,
                  const DCPS::RepoId& remote_guid, const DiscoveredSubscription& dsub) {
#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = lpub.publication_->get_ice_endpoint();
  if (endpoint && dsub.have_ice_agent_info_) {
    ICE::Agent::instance()->start_ice(endpoint, local_guid, remote_guid, dsub.ice_agent_info_);
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
  ICE::Endpoint* endpoint = lpub.publication_->get_ice_endpoint();
  if (endpoint) {
    ICE::Agent::instance()->stop_ice(endpoint, local_guid, remote_guid);
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
  ICE::Endpoint* endpoint = lsub.subscription_->get_ice_endpoint();
  if (endpoint && dpub.have_ice_agent_info_) {
    ICE::Agent::instance()->start_ice(endpoint, local_guid, remote_guid, dpub.ice_agent_info_);
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
  ICE::Endpoint* endpoint = lsub.subscription_->get_ice_endpoint();
  if (endpoint) {
    ICE::Agent::instance()->stop_ice(endpoint, local_guid, remote_guid);
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
  ICE::Endpoint* endpoint = lpub.publication_->get_ice_endpoint();

  if (!endpoint || !lpub.have_ice_agent_info) {
    return;
  }

  for (DCPS::RepoIdSet::const_iterator it = lpub.matched_endpoints_.begin(),
         end = lpub.matched_endpoints_.end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isReader()) {
      DiscoveredSubscriptionIter dsi = discovered_subscriptions_.find(*it);
      if (dsi != discovered_subscriptions_.end()) {
        if (dsi->second.have_ice_agent_info_) {
          ICE::Agent::instance()->start_ice(endpoint, guid, dsi->first, dsi->second.ice_agent_info_);
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
  ICE::Endpoint* endpoint = lsub.subscription_->get_ice_endpoint();

  if (!endpoint || !lsub.have_ice_agent_info) {
    return;
  }

  for (DCPS::RepoIdSet::const_iterator it = lsub.matched_endpoints_.begin(),
         end = lsub.matched_endpoints_.end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isWriter()) {
      DiscoveredPublicationIter dpi = discovered_publications_.find(*it);
      if (dpi != discovered_publications_.end()) {
        if (dpi->second.have_ice_agent_info_) {
          ICE::Agent::instance()->start_ice(endpoint, guid, dpi->first, dpi->second.ice_agent_info_);
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

  TopicDetails& td = topics_[get_topic_name(dpub)];
  for (DCPS::RepoIdSet::const_iterator it = td.endpoints().begin(),
         end = td.endpoints().end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isReader()) {
      LocalSubscriptionIter lsi = local_subscriptions_.find(*it);
      if (lsi != local_subscriptions_.end() &&
          lsi->second.matched_endpoints_.count(guid)) {
        ICE::Endpoint* endpoint = lsi->second.subscription_->get_ice_endpoint();
        if (endpoint) {
          ICE::Agent::instance()->start_ice(endpoint, lsi->first, guid, dpub.ice_agent_info_);
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

  TopicDetails& td = topics_[get_topic_name(dsub)];
  for (DCPS::RepoIdSet::const_iterator it = td.endpoints().begin(),
         end = td.endpoints().end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isWriter()) {
      LocalPublicationIter lpi = local_publications_.find(*it);
      if (lpi != local_publications_.end() &&
          lpi->second.matched_endpoints_.count(guid)) {
        ICE::Endpoint* endpoint = lpi->second.publication_->get_ice_endpoint();
        if (endpoint) {
          ICE::Agent::instance()->start_ice(endpoint, lpi->first, guid, dsub.ice_agent_info_);
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
  TopicDetails& td = topics_[get_topic_name(dpub)];
  for (DCPS::RepoIdSet::const_iterator it = td.endpoints().begin(),
         end = td.endpoints().end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isReader()) {
      LocalSubscriptionIter lsi = local_subscriptions_.find(*it);
      if (lsi != local_subscriptions_.end() &&
          lsi->second.matched_endpoints_.count(guid)) {
        ICE::Endpoint* endpoint = lsi->second.subscription_->get_ice_endpoint();
        if (endpoint) {
          ICE::Agent::instance()->stop_ice(endpoint, lsi->first, guid);
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
  TopicDetails& td = topics_[get_topic_name(dsub)];
  for (DCPS::RepoIdSet::const_iterator it = td.endpoints().begin(),
         end = td.endpoints().end(); it != end; ++it) {
    const DCPS::GuidConverter conv(*it);
    if (conv.isWriter()) {
      LocalPublicationIter lpi = local_publications_.find(*it);
      if (lpi != local_publications_.end() &&
          lpi->second.matched_endpoints_.count(guid)) {
        ICE::Endpoint* endpoint = lpi->second.publication_->get_ice_endpoint();
        if (endpoint) {
          ICE::Agent::instance()->stop_ice(endpoint, lpi->first, guid);
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
    ICE::Endpoint* endpoint = pub.publication_->get_ice_endpoint();
    if (endpoint) {
      pub.have_ice_agent_info = true;
      pub.ice_agent_info = ICE::Agent::instance()->get_local_agent_info(endpoint);
      ICE::Agent::instance()->add_local_agent_info_listener(endpoint, pos->first, &publication_agent_info_listener_);
      start_ice(pos->first, pub);
    }
  }
  for (LocalSubscriptionIter pos = local_subscriptions_.begin(), limit = local_subscriptions_.end(); pos != limit; ++pos) {
    LocalSubscription& sub = pos->second;
    ICE::Endpoint* endpoint = sub.subscription_->get_ice_endpoint();
    if (endpoint) {
      sub.have_ice_agent_info = true;
      sub.ice_agent_info = ICE::Agent::instance()->get_local_agent_info(endpoint);
      ICE::Agent::instance()->add_local_agent_info_listener(endpoint, pos->first, &subscription_agent_info_listener_);
      start_ice(pos->first, sub);
    }
  }
#endif
}

void
Sedp::rtps_relay_address(const ACE_INET_Addr& address)
{
  DCPS::RtpsUdpInst_rch rtps_inst = DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  ACE_GUARD(ACE_Thread_Mutex, g, rtps_inst->config_lock_);
  rtps_inst->rtps_relay_address_ = address;
}

void
Sedp::stun_server_address(const ACE_INET_Addr& address)
{
  DCPS::RtpsUdpInst_rch rtps_inst = DCPS::static_rchandle_cast<DCPS::RtpsUdpInst>(transport_inst_);
  ACE_GUARD(ACE_Thread_Mutex, g, rtps_inst->config_lock_);
  rtps_inst->stun_server_address_ = address;
}

bool locators_changed(const ParticipantProxy_t& x,
                      const ParticipantProxy_t& y)
{
  return locatorsChanged(x, y);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
