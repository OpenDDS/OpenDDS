/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Spdp.h"

#include "BaseMessageTypes.h"
#include "MessageTypes.h"
#include "ParameterListConverter.h"
#include "RtpsCoreTypeSupportImpl.h"
#include "RtpsDiscovery.h"

#include "dds/DdsDcpsGuidC.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Qos_Helper.h"

#ifdef OPENDDS_SECURITY
#include "SecurityHelpers.h"
#include "dds/DCPS/security/framework/SecurityRegistry.h"
#endif

#include "ace/Reactor.h"
#include "ace/OS_NS_sys_socket.h" // For setsockopt()

#include <cstring>
#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {
using DCPS::RepoId;

namespace {
  // Multiplier for resend period -> lease duration conversion,
  // if a remote discovery misses this many resends from us it will consider
  // us offline / unreachable.
  const int LEASE_MULT = 10;
  const CORBA::UShort encap_LE = 0x0300; // {PL_CDR_LE} in LE
  const CORBA::UShort encap_BE = 0x0200; // {PL_CDR_BE} in LE

  const ACE_Time_Value MAX_SPDP_TIMER_PERIOD(0, 10000);
  const ACE_Time_Value MAX_AUTH_TIME(3, 0);
  const ACE_Time_Value AUTH_RESEND_PERIOD(0, 25000);

  bool disposed(const ParameterList& inlineQos)
  {
    for (CORBA::ULong i = 0; i < inlineQos.length(); ++i) {
      if (inlineQos[i]._d() == PID_STATUS_INFO) {
        return inlineQos[i].status_info().value[3] & 1;
      }
    }
    return false;
  }

#ifdef OPENDDS_SECURITY
  bool operator==(const DDS::Security::Property_t& rhs, const DDS::Security::Property_t& lhs) {
    return rhs.name == lhs.name && rhs.value == lhs.value && rhs.propagate == lhs.propagate;
  }

  bool operator==(const DDS::Security::BinaryProperty_t& rhs, const DDS::Security::BinaryProperty_t& lhs) {
    return rhs.name == lhs.name && rhs.value == lhs.value && rhs.propagate == lhs.propagate;
  }

  bool operator==(const DDS::Security::PropertySeq& rhs, const DDS::Security::PropertySeq& lhs) {
    bool result = (rhs.length() == lhs.length());
    for (unsigned int i = 0; result && i < rhs.length(); ++i) {
      result = (rhs[i] == lhs[i]);
    }
    return result;
  }

  bool operator==(const DDS::Security::BinaryPropertySeq& rhs, const DDS::Security::BinaryPropertySeq& lhs) {
    bool result = (rhs.length() == lhs.length());
    for (unsigned int i = 0; result && i < rhs.length(); ++i) {
      result = (rhs[i] == lhs[i]);
    }
    return result;
  }

  bool operator==(const DDS::Security::DataHolder& rhs, const DDS::Security::DataHolder& lhs) {
    return rhs.class_id == lhs.class_id && rhs.properties == lhs.properties && rhs.binary_properties == lhs.binary_properties;
  }

  void init_participant_sec_attributes(DDS::Security::ParticipantSecurityAttributes& attr)
  {
    attr.allow_unauthenticated_participants = false;
    attr.is_access_protected = false;
    attr.is_rtps_protected = false;
    attr.is_discovery_protected = false;
    attr.is_liveliness_protected = false;
    attr.plugin_participant_attributes = 0;
    attr.ac_endpoint_properties.length(0);
  }
#endif

  GUID_t make_guid(const DCPS::GuidPrefix_t prefix, const DCPS::EntityId_t entity)
  {
    GUID_t result;
    std::memcpy(result.guidPrefix, prefix, sizeof(GuidPrefix_t));
    std::memcpy(&result.entityId, &entity, sizeof(EntityId_t));
    return result;
  }
}

void Spdp::init(DDS::DomainId_t /*domain*/,
                       DCPS::RepoId& guid,
                       const DDS::DomainParticipantQos& /*qos*/,
                       RtpsDiscovery* disco)
{
  guid = guid_; // may have changed in SpdpTransport constructor
  sedp_.ignore(guid);
  sedp_.init(guid_, *disco, domain_);

  // Append metatraffic unicast locator
  sedp_.unicast_locators(sedp_unicast_);

  if (disco->sedp_multicast()) { // Append metatraffic multicast locator
    const ACE_INET_Addr& mc_addr = sedp_.multicast_group();
    DCPS::Locator_t mc_locator;
    mc_locator.kind = address_to_kind(mc_addr);
    mc_locator.port = mc_addr.get_port_number();
    address_to_bytes(mc_locator.address, mc_addr);
    sedp_multicast_.length(1);
    sedp_multicast_[0] = mc_locator;
  }
}


Spdp::Spdp(DDS::DomainId_t domain,
           RepoId& guid,
           const DDS::DomainParticipantQos& qos,
           RtpsDiscovery* disco)

  : DCPS::LocalParticipant<Sedp>(qos)
  , disco_(disco)
  , domain_(domain)
  , guid_(guid)
  , tport_(new SpdpTransport(this, false))
  , eh_(tport_)
  , eh_shutdown_(false)
  , shutdown_cond_(lock_)
  , shutdown_flag_(false)
  , sedp_(guid_, *this, lock_)
#ifdef OPENDDS_SECURITY
  , security_config_()
  , security_enabled_(false)
#endif
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  init(domain, guid, qos, disco);

#ifdef OPENDDS_SECURITY
  init_participant_sec_attributes(participant_sec_attr_);
#endif

}

#ifdef OPENDDS_SECURITY
Spdp::Spdp(DDS::DomainId_t domain,
           const DCPS::RepoId& guid,
           const DDS::DomainParticipantQos& qos,
           RtpsDiscovery* disco,
           DDS::Security::IdentityHandle identity_handle,
           DDS::Security::PermissionsHandle perm_handle,
           DDS::Security::ParticipantCryptoHandle crypto_handle)

  : DCPS::LocalParticipant<Sedp>(qos)
  , disco_(disco)
  , domain_(domain)
  , guid_(guid)
  , tport_(new SpdpTransport(this, true))
  , eh_(tport_)
  , eh_shutdown_(false)
  , shutdown_cond_(lock_)
  , shutdown_flag_(false)
  , sedp_(guid_, *this, lock_)
  , security_config_(Security::SecurityRegistry::instance()->default_config())
  , security_enabled_(security_config_->get_authentication() && security_config_->get_access_control() && security_config_->get_crypto_key_factory() && security_config_->get_crypto_key_exchange())
  , identity_handle_(identity_handle)
  , permissions_handle_(perm_handle)
  , crypto_handle_(crypto_handle)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  init(domain, guid_, qos, disco);

  DDS::Security::Authentication_var auth = security_config_->get_authentication();
  DDS::Security::AccessControl_var access = security_config_->get_access_control();

  DDS::Security::SecurityException se = {"", 0, 0};

  if (auth->get_identity_token(identity_token_, identity_handle_, se) == false) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to get identity token. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to get identity token");
  }
  if (auth->get_identity_status_token(identity_status_token_, identity_handle_, se) == false) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to get identity status token. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to get identity status token");
  }
  if (access->get_permissions_token(permissions_token_, permissions_handle_, se) == false) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to get permissions handle. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to get permissions token");
  }
  if (access->get_permissions_credential_token(permissions_credential_token_, permissions_handle_, se) == false) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to get permissions credential handle. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to get permissions credential token");
  }

  if (auth->set_permissions_credential_and_token(identity_handle_, permissions_credential_token_, permissions_token_, se) == false) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to set permissions credential and token. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to set permissions credential and token");
  }

  init_participant_sec_attributes(participant_sec_attr_);

  if (access->get_participant_sec_attributes(permissions_handle_, participant_sec_attr_, se) == false) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("failed to retrieve participant security attributes. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to retrieve participant security attributes");
  }

  sedp_.init_security(identity_handle, perm_handle, crypto_handle);
}
#endif

Spdp::~Spdp()
{
  shutdown_flag_ = true;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Spdp::~Spdp ")
                 ACE_TEXT("remove discovered participants\n")));
    }

#ifdef OPENDDS_SECURITY
    try {
      write_secure_disposes();
    } catch (const CORBA::Exception& e) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::~Spdp() - from ")
                 ACE_TEXT("write_secure_disposes: %C\n"), e._info().c_str()));
    }
#endif

    // Iterate through a copy of the repo Ids, rather than the map
    //   as it gets unlocked in remove_discovered_participant()
    DCPS::RepoIdSet participant_ids;
    get_discovered_participant_ids(participant_ids);
    for (DCPS::RepoIdSet::iterator participant_id = participant_ids.begin();
         participant_id != participant_ids.end();
         ++participant_id)
    {
      DiscoveredParticipantIter part = participants_.find(*participant_id);
      if (part != participants_.end()) {
        remove_discovered_participant(part);
      }
    }
  }

  // ensure sedp's task queue is drained before data members are being
  // deleted
  sedp_.shutdown();

  // release lock for reset of event handler, which may delete transport
  tport_->close();
  eh_.reset();
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    while (!eh_shutdown_) {
      shutdown_cond_.wait();
    }
  }
}

#ifdef OPENDDS_SECURITY
void
Spdp::write_secure_updates()
{
  if (shutdown_flag_.value()) { return; }

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  const Security::SPDPdiscoveredParticipantData& pdata =
    build_local_pdata(Security::DPDK_SECURE);

  for (DiscoveredParticipantIter pi = participants_.begin(); pi != participants_.end(); ++pi) {
    if (pi->second.auth_state_ == DCPS::AS_AUTHENTICATED) {
      sedp_.write_dcps_participant_secure(pdata, pi->first);
    }
  }
}

void
Spdp::write_secure_disposes()
{
  sedp_.write_dcps_participant_dispose(guid_);
}
#endif

namespace {
  DDS::ParticipantBuiltinTopicData& partBitData(ParticipantData_t& pdata)
  {
#ifdef OPENDDS_SECURITY
    return pdata.ddsParticipantDataSecure.base.base;
#else
    return pdata.ddsParticipantData;
#endif
  }
}

void
Spdp::handle_participant_data(DCPS::MessageId id, const ParticipantData_t& cpdata)
{
  const ACE_Time_Value now = ACE_OS::gettimeofday();

  // Make a (non-const) copy so we can tweak values below
  ParticipantData_t pdata(cpdata);

  const DCPS::RepoId guid = make_guid(pdata.participantProxy.guidPrefix, DCPS::ENTITYID_PARTICIPANT);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  if (sedp_.ignoring(guid)) {
    // Ignore, this is our domain participant or one that the user has
    // asked us to ignore.
    return;
  }

  // Find the participant - iterator valid only as long as we hold the lock
  DiscoveredParticipantIter iter = participants_.find(guid);

  if (iter == participants_.end()) {

    // Trying to delete something that doesn't exist is a NOOP
    if (id == DCPS::DISPOSE_INSTANCE || id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
      return;
    }

    // copy guid prefix (octet[12]) into BIT key (long[3])
    std::memcpy(partBitData(pdata).key.value,
                pdata.participantProxy.guidPrefix,
                sizeof(DDS::BuiltinTopicKey_t));

    if (DCPS::DCPS_debug_level) {
      DCPS::GuidConverter local(guid_), remote(guid);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Spdp::data_received - %C discovered %C lease %ds\n"),
        OPENDDS_STRING(local).c_str(), OPENDDS_STRING(remote).c_str(),
        pdata.leaseDuration.seconds));
    }

    // add a new participant
    participants_[guid] = DiscoveredParticipant(pdata, now);
    DiscoveredParticipant& dp = participants_[guid];

#ifdef OPENDDS_SECURITY
    if (is_security_enabled()) {
      // Associate the stateless reader / writer for handshakes & auth requests
      sedp_.associate_preauth(dp.pdata_);

      // If we've gotten auth requests for this (previously undiscovered) participant, pull in the tokens now
      PendingRemoteAuthTokenMap::iterator token_iter = pending_remote_auth_tokens_.find(guid);
      if (token_iter != pending_remote_auth_tokens_.end()) {
        dp.remote_auth_request_token_ = token_iter->second;
        pending_remote_auth_tokens_.erase(token_iter);
      }
    }
#endif

    // Since we've just seen a new participant, let's send out our
    // own announcement, so they don't have to wait.
    this->tport_->write_i();

#ifdef OPENDDS_SECURITY
    if (is_security_enabled()) {
      bool has_security_data = dp.pdata_.dataKind == Security::DPDK_ENHANCED ||
                               dp.pdata_.dataKind == Security::DPDK_SECURE;

      if (has_security_data == false) {
        if (participant_sec_attr_.allow_unauthenticated_participants == false) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: Spdp::data_received - ")
            ACE_TEXT("Incompatible security attributes in discovered participant: %C\n"),
            std::string(DCPS::GuidConverter(guid)).c_str()));
            participants_.erase(guid);
        } else { // allow_unauthenticated_participants == true
          dp.auth_state_ = DCPS::AS_UNAUTHENTICATED;
          match_unauthenticated(guid, dp);
        }
      } else { // has_security_data == true
        dp.identity_token_ = pdata.ddsParticipantDataSecure.base.identity_token;
        dp.permissions_token_ = pdata.ddsParticipantDataSecure.base.permissions_token;
        dp.property_qos_ = pdata.ddsParticipantDataSecure.base.property;
        dp.security_info_ = pdata.ddsParticipantDataSecure.base.security_info;

        attempt_authentication(guid, dp);
        if (dp.auth_state_ == DCPS::AS_UNAUTHENTICATED) {
          if (participant_sec_attr_.allow_unauthenticated_participants == false) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: Spdp::data_received - ")
              ACE_TEXT("Incompatible security attributes in discovered participant: %C\n"),
              std::string(DCPS::GuidConverter(guid)).c_str()));
            participants_.erase(guid);
          } else { // allow_unauthenticated_participants == true
            dp.auth_state_ = DCPS::AS_UNAUTHENTICATED;
            match_unauthenticated(guid, dp);
          }
        } else if (dp.auth_state_ == DCPS::AS_AUTHENTICATED) {
          if (match_authenticated(guid, dp) == false) {
            participants_.erase(guid);
          }
        }
        // otherwise just return, since we're waiting for input to finish authentication
      }
    } else {

      dp.auth_state_ = DCPS::AS_UNAUTHENTICATED;
      match_unauthenticated(guid, dp);

    }
#else
    match_unauthenticated(guid, dp);
#endif

  } else {

#ifdef OPENDDS_SECURITY
    // Non-secure updates for authenticated participants are used for liveliness but
    // are otherwise ignored. Non-secure dispose messages are ignored completely.
    if (iter->second.auth_state_ == DCPS::AS_AUTHENTICATED &&
        pdata.dataKind != Security::DPDK_SECURE &&
        id != DCPS::DISPOSE_INSTANCE &&
        id != DCPS::DISPOSE_UNREGISTER_INSTANCE)
    {
      iter->second.last_seen_ = now;
      return;
    }
#endif

    if (id == DCPS::DISPOSE_INSTANCE || id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
      remove_discovered_participant(iter);
      return;
    }

    // Must unlock when calling into part_bit() as it may call back into us
    ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

    // update an existing participant
    DDS::ParticipantBuiltinTopicData& pdataBit = partBitData(pdata);
    DDS::ParticipantBuiltinTopicData& discoveredBit = partBitData(iter->second.pdata_);
    pdataBit.key = discoveredBit.key;
#ifndef OPENDDS_SAFETY_PROFILE
    using DCPS::operator!=;
#endif
    if (discoveredBit.user_data != pdataBit.user_data) {
      discoveredBit.user_data = pdataBit.user_data;
#ifndef DDS_HAS_MINIMUM_BIT
      DCPS::ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
      if (bit) {
        ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
        bit->store_synthetic_data(pdataBit, DDS::NOT_NEW_VIEW_STATE);
      }
#endif /* DDS_HAS_MINIMUM_BIT */
      // Perform search again, so iterator becomes valid
      iter = participants_.find(guid);
    }
    // Participant may have been removed while lock released
    if (iter != participants_.end()) {
      iter->second.pdata_ = pdata;
      iter->second.last_seen_ = now;
    }
  }
}

void
Spdp::data_received(const DataSubmessage& data, const ParameterList& plist)
{
  if (shutdown_flag_.value()) { return; }

  ParticipantData_t pdata;
  if (ParameterListConverter::from_param_list(plist, pdata) < 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::data_received - ")
      ACE_TEXT("failed to convert from ParameterList to ")
      ACE_TEXT("SPDPdiscoveredParticipantData\n")));
    return;
  }

  DCPS::MessageId msg_id = (data.inlineQos.length() && disposed(data.inlineQos)) ? DCPS::DISPOSE_INSTANCE : DCPS::SAMPLE_DATA;

  handle_participant_data(msg_id, pdata);
}

void
Spdp::match_unauthenticated(const DCPS::RepoId& guid, DiscoveredParticipant& dp)
{
  // Must unlock when calling into part_bit() as it may call back into us
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  DDS::InstanceHandle_t bit_instance_handle = DDS::HANDLE_NIL;
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
  if (bit) {
    ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
    bit_instance_handle =
      bit->store_synthetic_data(partBitData(dp.pdata_), DDS::NEW_VIEW_STATE);
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  // notify Sedp of association
  // Sedp may call has_discovered_participant, which is why the participant must be added before this call to associate.
  sedp_.associate(dp.pdata_);

  // Iterator is no longer valid
  DiscoveredParticipantIter iter = participants_.find(guid);
  if (iter != participants_.end()) {
    iter->second.bit_ih_ = bit_instance_handle;
  }
}

#ifdef OPENDDS_SECURITY
void
Spdp::handle_auth_request(const DDS::Security::ParticipantStatelessMessage& msg)
{
  // If this message wasn't intended for us, ignore handshake message
  if (msg.destination_participant_guid != guid_ || msg.message_data.length() == 0) {
    return;
  }

  const ACE_Time_Value time = ACE_OS::gettimeofday();

  RepoId guid = msg.message_identity.source_guid;
  guid.entityId = DCPS::ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (sedp_.ignoring(guid)) {
    // Ignore, this is our domain participant or one that the user has
    // asked us to ignore.
    return;
  }

  DiscoveredParticipantMap::iterator iter = participants_.find(guid);

  if (iter == participants_.end()) {
    // We're simply caching this for later, since we can't actually do much without the SPDP announcement itself
    pending_remote_auth_tokens_[guid] = msg.message_data[0];
  } else {
    iter->second.remote_auth_request_token_ = msg.message_data[0];
  }
}

namespace {
  void set_participant_guid(const GUID_t& guid, ParameterList& param_list)
  {
    Parameter gp_param;
    gp_param.guid(guid);
    gp_param._d(PID_PARTICIPANT_GUID);
    param_list.length(param_list.length() + 1);
    param_list[param_list.length() - 1] = gp_param;
  }
}

void
Spdp::handle_handshake_message(const DDS::Security::ParticipantStatelessMessage& msg)
{
  DDS::Security::SecurityException se = {"", 0, 0};
  Security::Authentication_var auth = security_config_->get_authentication();

  // If this message wasn't intended for us, ignore handshake message
  if (msg.destination_participant_guid != guid_ || !msg.message_data.length()) {
    return;
  }

  RepoId src_participant = msg.message_identity.source_guid;
  src_participant.entityId = DCPS::ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  // If discovery hasn't initialized / validated this participant yet, ignore handshake messages
  DiscoveredParticipantIter iter = participants_.find(src_participant);
  if (iter == participants_.end()) {
    ACE_DEBUG((LM_WARNING,
      ACE_TEXT("(%P|%t) Spdp::handle_handshake_message() - ")
      ACE_TEXT("received handshake for undiscovered participant %C. Ignoring.\n"),
               std::string(DCPS::GuidConverter(src_participant)).c_str()));
    return;
  }

  DiscoveredParticipant& dp = iter->second;

  DCPS::RepoId writer = guid_;
  writer.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;

  DCPS::RepoId reader = src_participant;
  reader.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;

  if (dp.auth_state_ == DCPS::AS_HANDSHAKE_REPLY && msg.related_message_identity.source_guid == GUID_UNKNOWN) {
    DDS::Security::ParticipantBuiltinTopicDataSecure pbtds = {
      {
        {
          DDS::BuiltinTopicKey_t() /*ignored*/,
          qos_.user_data
        },
        identity_token_,
        permissions_token_,
        qos_.property,
        {0, 0}
      },
      identity_status_token_
    };

    pbtds.base.security_info.plugin_participant_security_attributes = participant_sec_attr_.plugin_participant_attributes;
    pbtds.base.security_info.participant_security_attributes = security_attributes_to_bitmask(participant_sec_attr_);

    ParameterList plist;
    set_participant_guid(guid_, plist);
    if (ParameterListConverter::to_param_list(pbtds.base, plist) < 0) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
        ACE_TEXT("Failed to convert from ParticipantBuiltinTopicData to ParameterList\n")));
      return;
    }

    ACE_Message_Block temp_buff(64 * 1024);
    DCPS::Serializer ser(&temp_buff, DCPS::Serializer::SWAP_BE, DCPS::Serializer::ALIGN_INITIALIZE);
    if (!(ser << plist)) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
        ACE_TEXT("Failed to serialize parameter list.\n")));
      return;
    }

    DDS::Security::ParticipantStatelessMessage reply;
    reply.message_identity.source_guid = guid_;
    reply.message_identity.sequence_number = 0;
    reply.message_class_id = DDS::Security::GMCLASSID_SECURITY_AUTH_HANDSHAKE;
    reply.related_message_identity = msg.message_identity;
    reply.destination_participant_guid = src_participant;
    reply.destination_endpoint_guid = reader;
    reply.source_endpoint_guid = GUID_UNKNOWN;
    reply.message_data.length(1);
    reply.message_data[0] = msg.message_data[0];

    const DDS::OctetSeq local_participant(static_cast<unsigned int>(temp_buff.length()), &temp_buff);
    const DDS::Security::ValidationResult_t vr =
      auth->begin_handshake_reply(dp.handshake_handle_, reply.message_data[0], dp.identity_handle_,
                                  identity_handle_, local_participant, se);
    if (vr == DDS::Security::VALIDATION_FAILED) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
        ACE_TEXT("Failed to reply to incoming handshake message. Security Exception[%d.%d]: %C\n"),
          se.code, se.minor_code, se.message.in()));
      return;
    } else if (vr == DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE) {
      if (sedp_.write_stateless_message(reply, reader) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
          ACE_TEXT("Unable to write stateless message for handshake reply.\n")));
        return;
      }
      dp.has_last_stateless_msg_ = true;
      dp.last_stateless_msg_time_ = ACE_OS::gettimeofday();
      dp.last_stateless_msg_ = reply;
      dp.auth_state_ = DCPS::AS_HANDSHAKE_REPLY_SENT;
      return;
    } else if (vr == DDS::Security::VALIDATION_OK_FINAL_MESSAGE) {
      // Theoretically, this shouldn't happen unless handshakes can involve fewer than 3 messages
      if (sedp_.write_stateless_message(reply, reader) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
          ACE_TEXT("Unable to write stateless message for final message.\n")));
        return;
      }
      dp.has_last_stateless_msg_ = false;
      dp.auth_state_ = DCPS::AS_AUTHENTICATED;
      match_authenticated(src_participant, dp);
    } else if (vr == DDS::Security::VALIDATION_OK) {
      // Theoretically, this shouldn't happen unless handshakes can involve fewer than 3 messages
      dp.has_last_stateless_msg_ = false;
      dp.auth_state_ = DCPS::AS_AUTHENTICATED;
      match_authenticated(src_participant, dp);
    }
  }

  if ((dp.auth_state_ == DCPS::AS_HANDSHAKE_REQUEST_SENT || dp.auth_state_ == DCPS::AS_HANDSHAKE_REPLY_SENT) && msg.related_message_identity.source_guid == guid_) {
    DDS::Security::ParticipantStatelessMessage reply;
    reply.message_identity.source_guid = guid_;
    reply.message_identity.sequence_number = 0;
    reply.message_class_id = DDS::Security::GMCLASSID_SECURITY_AUTH_HANDSHAKE;
    reply.related_message_identity = msg.message_identity;
    reply.destination_participant_guid = src_participant;
    reply.destination_endpoint_guid = reader;
    reply.source_endpoint_guid = GUID_UNKNOWN;
    reply.message_data.length(1);

    DDS::Security::ValidationResult_t vr = auth->process_handshake(reply.message_data[0], msg.message_data[0], dp.handshake_handle_, se);
    if (vr == DDS::Security::VALIDATION_FAILED) {
      if (dp.auth_state_ == DCPS::AS_HANDSHAKE_REQUEST_SENT) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
          ACE_TEXT("Failed to process incoming handshake message when expecting reply from %C. Security Exception[%d.%d]: %C\n"),
          std::string(DCPS::GuidConverter(src_participant)).c_str(), se.code, se.minor_code, se.message.in()));
      } else {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
          ACE_TEXT("Failed to process incoming handshake message when expecting final message from %C. Security Exception[%d.%d]: %C\n"),
          std::string(DCPS::GuidConverter(src_participant)).c_str(), se.code, se.minor_code, se.message.in()));
      }
      return;
    } else if (vr == DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE) {
      // Theoretically, this shouldn't happen unless handshakes can involve more than 3 messages
      if (sedp_.write_stateless_message(reply, reader) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
          ACE_TEXT("Unable to write stateless message for handshake reply.\n")));
        return;
      }
      dp.has_last_stateless_msg_ = true;
      dp.last_stateless_msg_time_ = ACE_OS::gettimeofday();
      dp.last_stateless_msg_ = reply;
      // cache the outbound message, but don't change state, since roles shouldn't have changed?
    } else if (vr == DDS::Security::VALIDATION_OK_FINAL_MESSAGE) {
      if (sedp_.write_stateless_message(reply, reader) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: Spdp::handle_handshake_message() - ")
          ACE_TEXT("Unable to write stateless message for final message.\n")));
        return;
      }
      dp.has_last_stateless_msg_ = false;
      dp.auth_state_ = DCPS::AS_AUTHENTICATED;
      match_authenticated(src_participant, dp);
    } else if (vr == DDS::Security::VALIDATION_OK) {
      dp.has_last_stateless_msg_ = false;
      dp.auth_state_ = DCPS::AS_AUTHENTICATED;
      match_authenticated(src_participant, dp);
    }
  }

  return;
}

void
Spdp::check_auth_states(const ACE_Time_Value& tv) {
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  OPENDDS_SET_CMP(RepoId, DCPS::GUID_tKeyLessThan) to_erase;
  for (DiscoveredParticipantIter pi = participants_.begin(); pi != participants_.end(); ++pi) {
    switch (pi->second.auth_state_) {
      case DCPS::AS_HANDSHAKE_REQUEST_SENT:
      case DCPS::AS_HANDSHAKE_REPLY_SENT:
        if (tv > pi->second.auth_started_time_ + MAX_AUTH_TIME) {
          to_erase.insert(pi->first);
        } else if (pi->second.has_last_stateless_msg_ && (tv > (pi->second.last_stateless_msg_time_ + AUTH_RESEND_PERIOD))) {
          RepoId reader = pi->first;
          reader.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
          pi->second.last_stateless_msg_time_ = tv;
          if (sedp_.write_stateless_message(pi->second.last_stateless_msg_, reader) != DDS::RETCODE_OK) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: Spdp::check_auth_states() - ")
              ACE_TEXT("Unable to write stateless message retry.\n")));
          }
        }
        break;
      case DCPS::AS_UNKNOWN:
      case DCPS::AS_VALIDATING_REMOTE:
      case DCPS::AS_HANDSHAKE_REQUEST:
      case DCPS::AS_HANDSHAKE_REPLY:
      case DCPS::AS_AUTHENTICATED:
      case DCPS::AS_UNAUTHENTICATED:
      default:
        break;
    }
  }
  for (OPENDDS_SET_CMP(RepoId, DCPS::GUID_tKeyLessThan)::const_iterator it = to_erase.begin(); it != to_erase.end(); ++it) {
    DiscoveredParticipantIter pit = participants_.find(*it);
    if (pit != participants_.end()) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: Spdp::check_auth_states()      - Removing discovered participant due to authentication timeout: %C\n"), std::string(DCPS::GuidConverter(*it)).c_str()));
      if (participant_sec_attr_.allow_unauthenticated_participants == false) {
        remove_discovered_participant(pit);
      } else {
        pit->second.auth_state_ = DCPS::AS_UNAUTHENTICATED;
        match_unauthenticated(*it, pit->second);
      }
    }
  }
}


void
Spdp::handle_participant_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg) {
  DDS::Security::SecurityException se = {"", 0, 0};
  Security::CryptoKeyExchange_var key_exchange = security_config_->get_crypto_key_exchange();

  // If this message wasn't intended for us, ignore volatile message
  if (msg.destination_participant_guid != guid_ || !msg.message_data.length()) {
    return;
  }

  RepoId src_participant = msg.message_identity.source_guid;
  src_participant.entityId = DCPS::ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  // If discovery hasn't initialized / validated this participant yet, ignore volatile message
  DiscoveredParticipantIter iter = participants_.find(src_participant);
  if (iter == participants_.end()) {
    ACE_DEBUG((LM_WARNING,
      ACE_TEXT("(%P|%t) Spdp::handle_participant_crypto_tokens() - ")
      ACE_TEXT("received tokens for undiscovered participant %C. Ignoring.\n"),
               std::string(DCPS::GuidConverter(src_participant)).c_str()));
    return;
  }
  DiscoveredParticipant& dp = iter->second;

  dp.crypto_tokens_ = reinterpret_cast<const DDS::Security::ParticipantCryptoTokenSeq&>(msg.message_data);

  if (key_exchange->set_remote_participant_crypto_tokens(crypto_handle_, dp.crypto_handle_, dp.crypto_tokens_, se) == false) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("(%P|%t) ERROR: Spdp::handle_participant_crypto_tokens() - ")
      ACE_TEXT("Unable to set remote participant crypto tokens with crypto key exchange plugin. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    return;
  }
}

bool
Spdp::match_authenticated(const DCPS::RepoId& guid, DiscoveredParticipant& dp)
{
  DDS::Security::SecurityException se = {"", 0, 0};

  Security::Authentication_var auth = security_config_->get_authentication();
  Security::AccessControl_var access = security_config_->get_access_control();
  Security::CryptoKeyFactory_var key_factory = security_config_->get_crypto_key_factory();
  Security::CryptoKeyExchange_var key_exchange = security_config_->get_crypto_key_exchange();

  dp.shared_secret_handle_ = auth->get_shared_secret(dp.handshake_handle_, se);
  if (dp.shared_secret_handle_ == 0) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("Spdp::match_authenticated() - ")
      ACE_TEXT("Unable to get shared secret handle. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    return false;
  }

  if (auth->get_authenticated_peer_credential_token(dp.authenticated_peer_credential_token_, dp.handshake_handle_, se) == false) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("Spdp::match_authenticated() - ")
      ACE_TEXT("Unable to get authenticated peer credential token. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    return false;
  }

  dp.permissions_handle_ = access->validate_remote_permissions(auth, identity_handle_, dp.identity_handle_, dp.permissions_token_, dp.authenticated_peer_credential_token_, se);
  if (participant_sec_attr_.is_access_protected == true && dp.permissions_handle_ == DDS::HANDLE_NIL) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("Spdp::match_authenticated() - ")
      ACE_TEXT("Unable to validate remote participant with access control plugin. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    return false;
  }

  if (participant_sec_attr_.is_access_protected == true) {
    if (access->check_remote_participant(dp.permissions_handle_, domain_, dp.pdata_.ddsParticipantDataSecure, se) == false) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
        ACE_TEXT("Spdp::match_authenticated() - ")
        ACE_TEXT("Remote participant check failed. Security Exception[%d.%d]: %C\n"),
          se.code, se.minor_code, se.message.in()));
      return false;
    }
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Spdp::match_authenticated - ")
               ACE_TEXT("auth and access control complete for peer %C\n"),
               std::string(DCPS::GuidConverter(guid)).c_str()));
  }

  dp.crypto_handle_ = key_factory->register_matched_remote_participant(crypto_handle_, dp.identity_handle_, dp.permissions_handle_, dp.shared_secret_handle_, se);
  if (dp.crypto_handle_ == DDS::HANDLE_NIL) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("Spdp::match_authenticated() - ")
      ACE_TEXT("Unable to register remote participant with crypto key factory plugin. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    return false;
  }

  if (key_exchange->create_local_participant_crypto_tokens(crypto_tokens_, crypto_handle_, dp.crypto_handle_, se) == false) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("Spdp::match_authenticated() - ")
      ACE_TEXT("Unable to create local participant crypto tokens with crypto key exchange plugin. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    return false;
  }

  // Must unlock when calling into part_bit() as it may call back into us
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  DDS::InstanceHandle_t bit_instance_handle = DDS::HANDLE_NIL;
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
  if (bit) {
    ACE_GUARD_REACTION(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock, return false);
    bit_instance_handle =
      bit->store_synthetic_data(dp.pdata_.ddsParticipantDataSecure.base.base,
                                DDS::NEW_VIEW_STATE);
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  // notify Sedp of association
  // Sedp may call has_discovered_participant, which is the participant must be added before these calls to associate.
  sedp_.associate(dp.pdata_);
  sedp_.associate_volatile(dp.pdata_);
  sedp_.associate_secure_writers_to_readers(dp.pdata_);
  sedp_.associate_secure_readers_to_writers(dp.pdata_);

  // Iterator is no longer valid
  DiscoveredParticipantIter iter = participants_.find(guid);
  if (iter != participants_.end()) {
    iter->second.bit_ih_ = bit_instance_handle;
  }
  return true;
}

void
Spdp::attempt_authentication(const DCPS::RepoId& guid, DiscoveredParticipant& dp)
{
  DDS::Security::Authentication_var auth = security_config_->get_authentication();
  DDS::Security::SecurityException se = {"", 0, 0};

  if (dp.auth_state_ == DCPS::AS_UNKNOWN) {
    dp.auth_started_time_ = ACE_OS::gettimeofday();
    dp.auth_state_ = DCPS::AS_VALIDATING_REMOTE;
  }

  if (dp.auth_state_ == DCPS::AS_VALIDATING_REMOTE) {
    DDS::Security::ValidationResult_t vr = auth->validate_remote_identity(dp.identity_handle_, dp.local_auth_request_token_, dp.remote_auth_request_token_, identity_handle_, dp.identity_token_, guid, se);

    // Take care of any auth tokens that need to be sent before handling return value
    if (!(dp.local_auth_request_token_ == DDS::Security::Token())) {
      DDS::Security::ParticipantStatelessMessage msg;
      msg.message_identity.source_guid = guid_;
      msg.message_class_id = DDS::Security::GMCLASSID_SECURITY_AUTH_REQUEST;
      msg.destination_participant_guid = guid;
      msg.destination_endpoint_guid = GUID_UNKNOWN;
      msg.source_endpoint_guid = GUID_UNKNOWN;
      msg.related_message_identity.source_guid = GUID_UNKNOWN;
      msg.related_message_identity.sequence_number = 0;
      msg.message_data.length(1);
      msg.message_data[0] = dp.local_auth_request_token_;

      DCPS::RepoId reader = guid;
      reader.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;

      if (sedp_.write_stateless_message(msg, reader) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::attempt_authentication() - ")
          ACE_TEXT("Unable to write stateless message (auth request).\n")));
      }
    }
    switch (vr) {
      case DDS::Security::VALIDATION_OK: {
        dp.auth_state_ = DCPS::AS_AUTHENTICATED;
        return;
      }
      case DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE: {
        dp.auth_state_ = DCPS::AS_HANDSHAKE_REPLY;
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: Spdp::attempt_authentication() - Attempting authentication (expecting reply) for participant:   %C\n"), std::string(DCPS::GuidConverter(guid)).c_str()));
        return; // We'll need to wait for an inbound handshake request from the remote participant
      }
      case DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST: {
        dp.auth_state_ = DCPS::AS_HANDSHAKE_REQUEST;
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: Spdp::attempt_authentication() - Attempting authentication (sending request) for participant:   %C\n"), std::string(DCPS::GuidConverter(guid)).c_str()));
        break; // We've got more to do, move on to handshake request
      }
      case DDS::Security::VALIDATION_FAILED: {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: Spdp::attempt_authentication() - ")
          ACE_TEXT("Remote participant identity is invalid. Security Exception[%d.%d]: %C\n"),
            se.code, se.minor_code, se.message.in()));
        dp.auth_state_ = DCPS::AS_UNAUTHENTICATED;
        return;
      }
      default: {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: Spdp::attempt_authentication() - ")
          ACE_TEXT("Unexpected return value while validating remote identity. Security Exception[%d.%d]: %C\n"),
            se.code, se.minor_code, se.message.in()));
        dp.auth_state_ = DCPS::AS_UNAUTHENTICATED;
        return;
      }
    }
  }

  if (dp.auth_state_ == DCPS::AS_HANDSHAKE_REQUEST) {
    DDS::Security::ParticipantBuiltinTopicDataSecure pbtds = {
      {
        {
          DDS::BuiltinTopicKey_t() /*ignored*/,
          qos_.user_data
        },
        identity_token_,
        permissions_token_,
        qos_.property,
        {0, 0}
      },
      identity_status_token_
    };

    pbtds.base.security_info.plugin_participant_security_attributes = participant_sec_attr_.plugin_participant_attributes;
    pbtds.base.security_info.participant_security_attributes = DDS::Security::PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;
    if (participant_sec_attr_.is_rtps_protected) {
      pbtds.base.security_info.participant_security_attributes |= DDS::Security::PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED;
    }
    if (participant_sec_attr_.is_discovery_protected) {
      pbtds.base.security_info.participant_security_attributes |= DDS::Security::PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;
    }
    if (participant_sec_attr_.is_liveliness_protected) {
      pbtds.base.security_info.participant_security_attributes |= DDS::Security::PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED;
    }

    ParameterList plist;
    set_participant_guid(guid_, plist);
    if (ParameterListConverter::to_param_list(pbtds.base, plist) < 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::attempt_authentication() - ")
        ACE_TEXT("Failed to convert from ParticipantBuiltinTopicData to ParameterList\n")));
      return;
    }

    ACE_Message_Block temp_buff(64 * 1024);
    DCPS::Serializer ser(&temp_buff, DCPS::Serializer::SWAP_BE, DCPS::Serializer::ALIGN_INITIALIZE);
    if (!(ser << plist)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::attempt_authentication() - ")
        ACE_TEXT("Failed to serialize parameter list.\n")));
      return;
    }

    DDS::Security::HandshakeMessageToken hs_mt;
    const DDS::OctetSeq local_participant(static_cast<unsigned int>(temp_buff.length()), &temp_buff);
    if (auth->begin_handshake_request(dp.handshake_handle_, hs_mt, identity_handle_, dp.identity_handle_,
                                      local_participant, se)
        != DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::attempt_authentication() - ")
        ACE_TEXT("Failed to begin handshake_request. Security Exception[%d.%d]: %C\n"),
          se.code, se.minor_code, se.message.in()));
      return;
    }

    DCPS::RepoId writer = guid_;
    writer.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;

    DCPS::RepoId reader = guid;
    reader.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;

    DDS::Security::ParticipantStatelessMessage msg;
    msg.message_identity.source_guid = guid_;
    msg.message_class_id = DDS::Security::GMCLASSID_SECURITY_AUTH_HANDSHAKE;
    msg.destination_participant_guid = guid;
    msg.destination_endpoint_guid = reader;
    msg.source_endpoint_guid = GUID_UNKNOWN;
    msg.related_message_identity.source_guid = GUID_UNKNOWN;
    msg.related_message_identity.sequence_number = 0;
    msg.message_data.length(1);
    msg.message_data[0] = hs_mt;

    if (sedp_.write_stateless_message(msg, reader) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::attempt_authentication() - ")
        ACE_TEXT("Unable to write stateless message (handshake).\n")));
      return;
    }
    dp.has_last_stateless_msg_ = true;
    dp.last_stateless_msg_time_ = ACE_OS::gettimeofday();
    dp.last_stateless_msg_ = msg;
    dp.auth_state_ = DCPS::AS_HANDSHAKE_REQUEST_SENT;
  }

  return;
}
#endif

void
Spdp::remove_expired_participants()
{
  // Find and remove any expired discovered participant
  ACE_GUARD (ACE_Thread_Mutex, g, lock_);
  // Iterate through a copy of the repo Ids, rather than the map
  //   as it gets unlocked in remove_discovered_participant()
  DCPS::RepoIdSet participant_ids;
  get_discovered_participant_ids(participant_ids);
  for (DCPS::RepoIdSet::iterator participant_id = participant_ids.begin();
       participant_id != participant_ids.end();
       ++participant_id)
  {
    DiscoveredParticipantIter part = participants_.find(*participant_id);
    if (part != participants_.end()) {
      if (part->second.last_seen_ <
          ACE_OS::gettimeofday() -
          ACE_Time_Value(part->second.pdata_.leaseDuration.seconds)) {
        if (DCPS::DCPS_debug_level > 1) {
          DCPS::GuidConverter conv(part->first);
          ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) Spdp::remove_expired_participants() - ")
            ACE_TEXT("participant %C exceeded lease duration, removing\n"),
            OPENDDS_STRING(conv).c_str()));
        }
        remove_discovered_participant(part);
      }
    }
  }
}

void
Spdp::init_bit(const DDS::Subscriber_var& bit_subscriber)
{
  bit_subscriber_ = bit_subscriber;
  tport_->open();
}

void
Spdp::fini_bit()
{
  bit_subscriber_ = 0;
  wait_for_acks_.reset();
  // request for SpdpTransport(actually Reactor) thread and Sedp::Task
  // to acknowledge
  tport_->acknowledge();
  sedp_.acknowledge();
  // wait for the 2 acknowledgements
  wait_for_acks_.wait_for_acks(2);
}

#ifndef DDS_HAS_MINIMUM_BIT
DCPS::ParticipantBuiltinTopicDataDataReaderImpl*
Spdp::part_bit()
{
  if (!bit_subscriber_.in())
    return 0;

  DDS::DataReader_var d =
    bit_subscriber_->lookup_datareader(DCPS::BUILT_IN_PARTICIPANT_TOPIC);
  return dynamic_cast<DCPS::ParticipantBuiltinTopicDataDataReaderImpl*>(d.in());
}
#endif /* DDS_HAS_MINIMUM_BIT */

ACE_Reactor*
Spdp::reactor() const
{
  return disco_->reactor();
}

WaitForAcks&
Spdp::wait_for_acks()
{
  return wait_for_acks_;
}

bool
Spdp::is_opendds(const GUID_t& participant) const
{
  const DiscoveredParticipantConstIter iter = participants_.find(participant);
  if (iter == participants_.end()) {
    return false;
  }
  return 0 == std::memcmp(&iter->second.pdata_.participantProxy.vendorId,
                          DCPS::VENDORID_OCI, sizeof(VendorId_t));
}

ParticipantData_t
Spdp::build_local_pdata(
#ifdef OPENDDS_SECURITY
                        Security::DiscoveredParticipantDataKind kind
#endif
                        )
{
  BuiltinEndpointSet_t availableBuiltinEndpoints =
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER
    ;

#ifdef OPENDDS_SECURITY
  if (is_security_enabled()) {
    availableBuiltinEndpoints |=
      DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER |
      DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_READER |
      DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER |
      DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER |
      DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER |
      DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER |
      DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER |
      DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER |
      DDS::Security::BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER |
      DDS::Security::BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER |
      DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER |
      DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_READER
    ;
  }
#endif

  // The RTPS spec has no constants for the builtinTopics{Writer,Reader}

  // This locator list should not be empty, but we won't actually be using it.
  // The OpenDDS publication/subscription data will have locators included.
  DCPS::LocatorSeq nonEmptyList(1);
  nonEmptyList.length(1);
  nonEmptyList[0].kind = LOCATOR_KIND_UDPv4;
  nonEmptyList[0].port = 12345;
  std::memset(nonEmptyList[0].address, 0, 12);
  nonEmptyList[0].address[12] = 127;
  nonEmptyList[0].address[13] = 0;
  nonEmptyList[0].address[14] = 0;
  nonEmptyList[0].address[15] = 1;

  const GuidPrefix_t& gp = guid_.guidPrefix;

#ifdef OPENDDS_SECURITY
  const Security::SPDPdiscoveredParticipantData pdata = {
    kind,
    { // ParticipantBuiltinTopicDataSecure
      { // ParticipantBuiltinTopicData (security enhanced)
        { // ParticipantBuiltinTopicData (original)
          DDS::BuiltinTopicKey_t() /*ignored*/,
          qos_.user_data
        },
        identity_token_,
        permissions_token_,
        qos_.property,
        {
          security_attributes_to_bitmask(participant_sec_attr_),
          participant_sec_attr_.plugin_participant_attributes
        }
      },
      identity_status_token_
    },
#else
  const SPDPdiscoveredParticipantData pdata = {
    {
      DDS::BuiltinTopicKey_t() /*ignored*/,
      qos_.user_data
    },
#endif
    { // ParticipantProxy_t
      PROTOCOLVERSION,
      {gp[0], gp[1], gp[2], gp[3], gp[4], gp[5],
       gp[6], gp[7], gp[8], gp[9], gp[10], gp[11]},
      VENDORID_OPENDDS,
      false /*expectsIQoS*/,
      availableBuiltinEndpoints,
      sedp_unicast_,
      sedp_multicast_,
      nonEmptyList /*defaultMulticastLocatorList*/,
      nonEmptyList /*defaultUnicastLocatorList*/,
      {0 /*manualLivelinessCount*/}   //FUTURE: implement manual liveliness
    },
    { // Duration_t (leaseDuration)
      static_cast<CORBA::Long>((disco_->resend_period() * LEASE_MULT).sec()),
      0 // we are not supporting fractional seconds in the lease duration
    }
  };

  return pdata;
}

bool Spdp::announce_domain_participant_qos()
{

#ifdef OPENDDS_SECURITY
  if (is_security_enabled())
    write_secure_updates();
#endif

  return true;
}

Spdp::SpdpTransport::SpdpTransport(Spdp* outer, bool securityGuids)
  : outer_(outer), lease_duration_(outer_->disco_->resend_period() * LEASE_MULT)
  , buff_(64 * 1024)
  , wbuff_(64 * 1024)
{
  hdr_.prefix[0] = 'R';
  hdr_.prefix[1] = 'T';
  hdr_.prefix[2] = 'P';
  hdr_.prefix[3] = 'S';
  hdr_.version = PROTOCOLVERSION;
  hdr_.vendorId = VENDORID_OPENDDS;
  std::memcpy(hdr_.guidPrefix, outer_->guid_.guidPrefix, sizeof(GuidPrefix_t));
  data_.smHeader.submessageId = DATA;
  data_.smHeader.flags = FLAG_E | FLAG_D;
  data_.smHeader.submessageLength = 0; // last submessage in the Message
  data_.extraFlags = 0;
  data_.octetsToInlineQos = DATA_OCTETS_TO_IQOS;
  data_.readerId = ENTITYID_UNKNOWN;
  data_.writerId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;
  data_.writerSN.high = 0;
  data_.writerSN.low = 0;

  // Ports are set by the formulas in RTPS v2.1 Table 9.8
  const u_short port_common = outer_->disco_->pb() +
                              (outer_->disco_->dg() * outer_->domain_),
    mc_port = port_common + outer_->disco_->d0();

  // with security enabled the meaning of the bytes in guidPrefix changes
  u_short participantId = securityGuids ? 0
    : (hdr_.guidPrefix[10] << 8) | hdr_.guidPrefix[11];

#ifdef OPENDDS_SAFETY_PROFILE
  const u_short startingParticipantId = participantId;
#endif

  while (!open_unicast_socket(port_common, participantId)) {
    ++participantId;
  }

#ifdef OPENDDS_SAFETY_PROFILE
  if (participantId > startingParticipantId && ACE_OS::getpid() == -1) {
    // Since pids are not available, use the fact that we had to increment
    // participantId to modify the GUID's pid bytes.  This avoids GUID conflicts
    // between processes on the same host which start at the same time
    // (resulting in the same seed value for the random number generator).
    hdr_.guidPrefix[8] = static_cast<CORBA::Octet>(participantId >> 8);
    hdr_.guidPrefix[9] = static_cast<CORBA::Octet>(participantId & 0xFF);
    outer_->guid_.guidPrefix[8] = hdr_.guidPrefix[8];
    outer_->guid_.guidPrefix[9] = hdr_.guidPrefix[9];
  }
#endif

  OPENDDS_STRING mc_addr = outer_->disco_->default_multicast_group();
  ACE_INET_Addr default_multicast;
  if (0 != default_multicast.set(mc_port, mc_addr.c_str())) {
    ACE_DEBUG((
          LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::SpdpTransport() - ")
          ACE_TEXT("failed setting default_multicast address %C:%hu %p\n"),
          mc_addr.c_str(), mc_port, ACE_TEXT("ACE_INET_Addr::set")));
    throw std::runtime_error("failed to set default_multicast address");
  }

  const OPENDDS_STRING& net_if = outer_->disco_->multicast_interface();

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Spdp::SpdpTransport::SpdpTransport ")
               ACE_TEXT("joining group %C %C:%hu\n"),
               net_if.c_str (),
               mc_addr.c_str (),
               mc_port));
  }

#ifdef ACE_HAS_MAC_OSX
  multicast_socket_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                         ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#endif

  if (0 != multicast_socket_.join(default_multicast, 1,
                                  net_if.empty() ? 0 :
                                  ACE_TEXT_CHAR_TO_TCHAR(net_if.c_str()))) {
    ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::SpdpTransport() - ")
        ACE_TEXT("failed to join multicast group %C:%hu %p\n"),
        mc_addr.c_str(), mc_port, ACE_TEXT("ACE_SOCK_Dgram_Mcast::join")));
    throw std::runtime_error("failed to join multicast group");
  }

  send_addrs_.insert(default_multicast);

  typedef RtpsDiscovery::AddrVec::iterator iter;
  for (iter it = outer_->disco_->spdp_send_addrs().begin(),
       end = outer_->disco_->spdp_send_addrs().end(); it != end; ++it) {
    send_addrs_.insert(ACE_INET_Addr(it->c_str()));
  }
}

void
Spdp::SpdpTransport::open()
{
  ACE_Reactor* reactor = outer_->reactor();
  if (reactor->register_handler(unicast_socket_.get_handle(),
                                this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register unicast input handler");
  }

  if (reactor->register_handler(multicast_socket_.get_handle(),
                                this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register multicast input handler");
  }

  disco_resend_period_ = outer_->disco_->resend_period();
  last_disco_resend_ = 0;

  ACE_Time_Value timer_period = disco_resend_period_ < MAX_SPDP_TIMER_PERIOD ? disco_resend_period_ : MAX_SPDP_TIMER_PERIOD;

  if (-1 == reactor->schedule_timer(this, 0, ACE_Time_Value(0), timer_period)) {
    throw std::runtime_error("failed to schedule timer with reactor");
  }
}

Spdp::SpdpTransport::~SpdpTransport()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SpdpTransport::~SpdpTransport\n")));
  }
  try {
    dispose_unregister();
  }
  catch (const CORBA::Exception& ex) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) WARNING: Exception caught in ")
        ACE_TEXT("SpdpTransport::~SpdpTransport: %C\n"),
        ex._info().c_str()));
    }
  }
  {
    // Acquire lock for modification of condition variable
    ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
    outer_->eh_shutdown_ = true;
  }
  outer_->shutdown_cond_.signal();
  unicast_socket_.close();
  multicast_socket_.close();
}

void
Spdp::SpdpTransport::dispose_unregister()
{
  // Send the dispose/unregister SPDP sample
  data_.writerSN.high = seq_.getHigh();
  data_.writerSN.low = seq_.getLow();
  data_.smHeader.flags = FLAG_E | FLAG_Q | FLAG_K_IN_DATA;
  data_.inlineQos.length(1);
  static const StatusInfo_t dispose_unregister = { {0, 0, 0, 3} };
  data_.inlineQos[0].status_info(dispose_unregister);

  ParameterList plist(1);
  plist.length(1);
  plist[0].guid(outer_->guid_);
  plist[0]._d(PID_PARTICIPANT_GUID);

  wbuff_.reset();
  DCPS::Serializer ser(&wbuff_, false, DCPS::Serializer::ALIGN_CDR);
  CORBA::UShort options = 0;
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap_LE) || !(ser << options)
      || !(ser << plist)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::dispose_unregister() - ")
      ACE_TEXT("failed to serialize headers for dispose/unregister\n")));
    return;
  }

  typedef OPENDDS_SET(ACE_INET_Addr)::const_iterator iter_t;
  for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
    const ssize_t res =
      unicast_socket_.send(wbuff_.rd_ptr(), wbuff_.length(), *iter);
    if (res < 0) {
      ACE_TCHAR addr_buff[256] = {};
      iter->addr_to_string(addr_buff, 256, 0);
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::dispose_unregister() - ")
        ACE_TEXT("destination %s failed %p\n"), addr_buff, ACE_TEXT("send")));
    }
  }
}

void
Spdp::SpdpTransport::close()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SpdpTransport::close\n")));
  }
  ACE_Reactor* reactor = outer_->reactor();
  reactor->cancel_timer(this);
  const ACE_Reactor_Mask mask =
    ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL;
  reactor->remove_handler(multicast_socket_.get_handle(), mask);
  reactor->remove_handler(unicast_socket_.get_handle(), mask);
}

void
Spdp::SpdpTransport::write()
{
  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
  write_i();
}

void
Spdp::SpdpTransport::write_i()
{
  const ParticipantData_t pdata = outer_->build_local_pdata(
#ifdef OPENDDS_SECURITY
     outer_->is_security_enabled() ? Security::DPDK_ENHANCED : Security::DPDK_ORIGINAL
#endif
                                                            );

  data_.writerSN.high = seq_.getHigh();
  data_.writerSN.low = seq_.getLow();
  ++seq_;

  ParameterList plist;
  if (ParameterListConverter::to_param_list(pdata, plist) < 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to convert from SPDPdiscoveredParticipantData ")
      ACE_TEXT("to ParameterList\n")));
    return;
  }

  wbuff_.reset();
  CORBA::UShort options = 0;
  DCPS::Serializer ser(&wbuff_, false, DCPS::Serializer::ALIGN_CDR);
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap_LE) || !(ser << options)
      || !(ser << plist)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to serialize headers for SPDP\n")));
    return;
  }

  typedef OPENDDS_SET(ACE_INET_Addr)::const_iterator iter_t;
  for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
    const ssize_t res =
      unicast_socket_.send(wbuff_.rd_ptr(), wbuff_.length(), *iter);
    if (res < 0) {
      ACE_TCHAR addr_buff[256] = {};
      iter->addr_to_string(addr_buff, 256, 0);
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write() - ")
        ACE_TEXT("destination %s failed %p\n"), addr_buff, ACE_TEXT("send")));
    }
  }
}

int
Spdp::SpdpTransport::handle_timeout(const ACE_Time_Value& tv, const void*)
{
  if (tv > last_disco_resend_ + disco_resend_period_) {
    write();
    outer_->remove_expired_participants();
    last_disco_resend_ = tv;
  }

#ifdef OPENDDS_SECURITY
  outer_->check_auth_states(tv);
#endif

  return 0;
}

int
Spdp::SpdpTransport::handle_input(ACE_HANDLE h)
{
  const ACE_SOCK_Dgram& socket = (h == unicast_socket_.get_handle())
                                 ? unicast_socket_ : multicast_socket_;
  ACE_INET_Addr remote;
  buff_.reset();
  const ssize_t bytes = socket.recv(buff_.wr_ptr(), buff_.space(), remote);

  if (bytes > 0) {
    buff_.wr_ptr(bytes);
  } else if (bytes == 0) {
    return -1;
  } else {
    ACE_DEBUG((
          LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
          ACE_TEXT("error reading from %C socket %p\n")
          , (h == unicast_socket_.get_handle()) ? "unicast" : "multicast",
          ACE_TEXT("ACE_SOCK_Dgram::recv")));
    return -1;
  }

  // Handle some RTI protocol multicast to the same address
  if ((buff_.size() >= 4) && (!ACE_OS::memcmp(buff_.rd_ptr(), "RTPX", 4))) {
    return 0; // Ignore
  }

  DCPS::Serializer ser(&buff_, false, DCPS::Serializer::ALIGN_CDR);
  Header header;
  if (!(ser >> header)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
               ACE_TEXT("failed to deserialize RTPS header for SPDP\n")));
    return 0;
  }

  while (buff_.length() > 3) {
    const char subm = buff_.rd_ptr()[0], flags = buff_.rd_ptr()[1];
    ser.swap_bytes((flags & FLAG_E) != ACE_CDR_BYTE_ORDER);
    const size_t start = buff_.length();
    CORBA::UShort submessageLength = 0;
    switch (subm) {
    case DATA: {
      DataSubmessage data;
      if (!(ser >> data)) {
        ACE_ERROR((
              LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
              ACE_TEXT("failed to deserialize DATA header for SPDP\n")));
        return 0;
      }
      submessageLength = data.smHeader.submessageLength;

      if (data.writerId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER) {
        // Not our message: this could be the same multicast group used
        // for SEDP and other traffic.
        break;
      }

      ParameterList plist;
      if (data.smHeader.flags & (FLAG_D | FLAG_K_IN_DATA)) {
        ser.swap_bytes(!ACE_CDR_BYTE_ORDER); // read "encap" itself in LE
        CORBA::UShort encap, options;
        if (!(ser >> encap) || (encap != encap_LE && encap != encap_BE)) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
            ACE_TEXT("failed to deserialize encapsulation header for SPDP\n")));
          return 0;
        }
        ser >> options;
        // bit 8 in encap is on if it's PL_CDR_LE
        ser.swap_bytes(((encap & 0x100) >> 8) != ACE_CDR_BYTE_ORDER);
        if (!(ser >> plist)) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
            ACE_TEXT("failed to deserialize data payload for SPDP\n")));
          return 0;
        }
      } else {
        plist.length(1);
        RepoId guid;
        std::memcpy(guid.guidPrefix, header.guidPrefix, sizeof(GuidPrefix_t));
        guid.entityId = ENTITYID_PARTICIPANT;
        plist[0].guid(guid);
        plist[0]._d(PID_PARTICIPANT_GUID);
      }

      outer_->data_received(data, plist);
      break;
    }
    default:
      SubmessageHeader smHeader;
      if (!(ser >> smHeader)) {
        ACE_ERROR((
              LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
              ACE_TEXT("failed to deserialize SubmessageHeader for SPDP\n")));
        return 0;
      }
      submessageLength = smHeader.submessageLength;
      break;
    }
    if (submessageLength && buff_.length()) {
      const size_t read = start - buff_.length();
      if (read < static_cast<size_t>(submessageLength + SMHDR_SZ)) {
        if (!ser.skip(static_cast<CORBA::UShort>(submessageLength + SMHDR_SZ
                                                 - read))) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
            ACE_TEXT("failed to skip sub message length\n")));
          return 0;
        }
      }
    } else if (!submessageLength) {
      break; // submessageLength of 0 indicates the last submessage
    }
  }

  return 0;
}

int
Spdp::SpdpTransport::handle_exception(ACE_HANDLE)
{
  outer_->wait_for_acks().ack();
  return 0;
}

void
Spdp::SpdpTransport::acknowledge()
{
  ACE_Reactor* reactor = outer_->reactor();
  reactor->notify(this);
}

void
Spdp::signal_liveliness(DDS::LivelinessQosPolicyKind kind)
{
  sedp_.signal_liveliness(kind);
}

bool
Spdp::SpdpTransport::open_unicast_socket(u_short port_common,
                                         u_short participant_id)
{
  const u_short uni_port = port_common + outer_->disco_->d1() +
                           (outer_->disco_->pg() * participant_id);

  ACE_INET_Addr local_addr;
  OPENDDS_STRING spdpaddr = outer_->disco_->spdp_local_address().c_str();

  if (spdpaddr.empty()) {
    spdpaddr = "0.0.0.0";
  }

  if (0 != local_addr.set(uni_port, spdpaddr.c_str())) {
    ACE_DEBUG((
          LM_ERROR,
          ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
          ACE_TEXT("failed setting unicast local_addr to port %d %p\n"),
          uni_port, ACE_TEXT("ACE_INET_Addr::set")));
    throw std::runtime_error("failed to set unicast local address");
  }

  if (!DCPS::open_appropriate_socket_type(unicast_socket_, local_addr)) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((
            LM_WARNING,
            ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
            ACE_TEXT("failed to open_appropriate_socket_type unicast socket on port %d %p.  ")
            ACE_TEXT("Trying next participantId...\n"),
            uni_port, ACE_TEXT("ACE_SOCK_Dgram::open")));
    }
    return false;

  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((
          LM_INFO,
          ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
          ACE_TEXT("opened unicast socket on port %d\n"),
          uni_port));
  }

  if (!DCPS::set_socket_multicast_ttl(unicast_socket_, outer_->disco_->ttl())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket() - ")
               ACE_TEXT("failed to set TTL value to %d ")
               ACE_TEXT("for port:%hu %p\n"),
               outer_->disco_->ttl(), uni_port, ACE_TEXT("DCPS::set_socket_multicast_ttl:")));
    throw std::runtime_error("failed to set TTL");
  }
  return true;
}

bool
Spdp::get_default_locators(const RepoId& part_id, DCPS::LocatorSeq& target,
                           bool& inlineQos)
{
  DiscoveredParticipantIter part_iter = participants_.find(part_id);
  if (part_iter == participants_.end()) {
    return false;
  } else {
    inlineQos = part_iter->second.pdata_.participantProxy.expectsInlineQos;
    DCPS::LocatorSeq& mc_source =
          part_iter->second.pdata_.participantProxy.defaultMulticastLocatorList;
    DCPS::LocatorSeq& uc_source =
          part_iter->second.pdata_.participantProxy.defaultUnicastLocatorList;
    CORBA::ULong mc_source_len = mc_source.length();
    CORBA::ULong uc_source_len = uc_source.length();
    CORBA::ULong target_len = target.length();
    target.length(mc_source_len + uc_source_len + target_len);
    // Copy multicast
    for (CORBA::ULong mci = 0; mci < mc_source.length(); ++mci) {
      target[target_len + mci] = mc_source[mci];
    }
    // Copy unicast
    for (CORBA::ULong uci = 0; uci < uc_source.length(); ++uci) {
      target[target_len + mc_source_len + uci] = uc_source[uci];
    }
  }
  return true;
}

bool
Spdp::associated() const
{
  return !participants_.empty();
}

bool
Spdp::has_discovered_participant(const DCPS::RepoId& guid)
{
  return participants_.find(guid) != participants_.end();
}


void
Spdp::get_discovered_participant_ids(DCPS::RepoIdSet& results) const
{
  DiscoveredParticipantMap::const_iterator idx;
  for (idx = participants_.begin(); idx != participants_.end(); ++idx)
  {
    results.insert(idx->first);
  }
}

#ifdef OPENDDS_SECURITY
Spdp::ParticipantCryptoInfoPair
Spdp::lookup_participant_crypto_info(const DCPS::RepoId& id) const
{
  ParticipantCryptoInfoPair result = ParticipantCryptoInfoPair(DDS::HANDLE_NIL, DDS::Security::SharedSecretHandle_var());

  ACE_Guard<ACE_Thread_Mutex> g(lock_, false);
  DiscoveredParticipantConstIter pi = participants_.find(id);
  if (pi != participants_.end()) {
    result.first = pi->second.crypto_handle_;
    result.second = pi->second.shared_secret_handle_;
  }
  return result;
}

void
Spdp::send_participant_crypto_tokens(const DCPS::RepoId& id)
{
  if (crypto_tokens_.length() != 0) {
    DCPS::RepoId writer = guid_;
    writer.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;

    DCPS::RepoId reader = id;
    reader.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;

    DDS::Security::ParticipantVolatileMessageSecure msg;
    msg.message_identity.source_guid = writer;
    msg.message_class_id = DDS::Security::GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS;
    msg.destination_participant_guid = id;
    msg.destination_endpoint_guid = GUID_UNKNOWN; // unknown = whole participant
    msg.source_endpoint_guid = GUID_UNKNOWN;
    msg.message_data = reinterpret_cast<const DDS::Security::DataHolderSeq&>(crypto_tokens_);

    if (sedp_.write_volatile_message(msg, reader) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::send_participant_crypto_tokens() - ")
        ACE_TEXT("Unable to write volatile message.\n")));
    }
  }
  return;
}

DDS::Security::PermissionsHandle
Spdp::lookup_participant_permissions(const DCPS::RepoId& id) const
{
  DDS::Security::PermissionsHandle result = DDS::HANDLE_NIL;

  ACE_Guard<ACE_Thread_Mutex> g(lock_, false);
  DiscoveredParticipantConstIter pi = participants_.find(id);
  if (pi != participants_.end()) {
    result = pi->second.permissions_handle_;
  }
  return result;
}

DCPS::AuthState
Spdp::lookup_participant_auth_state(const DCPS::RepoId& id) const
{
  DCPS::AuthState result = DCPS::AS_UNKNOWN;

  ACE_Guard<ACE_Thread_Mutex> g(lock_, false);
  DiscoveredParticipantConstIter pi = participants_.find(id);
  if (pi != participants_.end()) {
    result = pi->second.auth_state_;
  }
  return result;
}
#endif

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
