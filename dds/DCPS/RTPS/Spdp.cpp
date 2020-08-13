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
#ifdef OPENDDS_SECURITY
#  include "SecurityHelpers.h"
#endif

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/Qos_Helper.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/framework/SecurityRegistry.h>
#endif

#include <dds/DdsDcpsGuidC.h>

#include <ace/Reactor.h>
#include <ace/OS_NS_sys_socket.h> // For setsockopt()
#include <ace/OS_NS_strings.h>
#include <ace/Auto_Ptr.h>

#include <cstring>
#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace RTPS {
using DCPS::RepoId;
using DCPS::MonotonicTimePoint;
using DCPS::TimeDuration;
using DCPS::Serializer;
using DCPS::Encoding;
using DCPS::ENDIAN_BIG;
using DCPS::ENDIAN_LITTLE;

namespace {
  const CORBA::UShort encap_LE = 0x0300; // {PL_CDR_LE} in LE
  const CORBA::UShort encap_BE = 0x0200; // {PL_CDR_BE} in LE

  const Encoding encoding_plain_big(Encoding::KIND_XCDR1, ENDIAN_BIG);
  const Encoding encoding_plain_native(Encoding::KIND_XCDR1);

  bool disposed(const ParameterList& inlineQos)
  {
    for (CORBA::ULong i = 0; i < inlineQos.length(); ++i) {
      if (inlineQos[i]._d() == PID_STATUS_INFO) {
        return inlineQos[i].status_info().value[3] & 1;
      }
    }
    return false;
  }

  DCPS::ParticipantLocation compute_location_mask(const ACE_INET_Addr& address, bool from_relay)
  {
    if (address.get_type() == AF_INET6) {
      return from_relay ? DCPS::LOCATION_RELAY6 : DCPS::LOCATION_LOCAL6;
    }
    return from_relay ? DCPS::LOCATION_RELAY : DCPS::LOCATION_LOCAL;
  }

  DCPS::ParticipantLocation compute_ice_location_mask(const ACE_INET_Addr& address)
  {
    if (address.get_type() == AF_INET6) {
      return DCPS::LOCATION_ICE6;
    }
    return DCPS::LOCATION_ICE;
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
}

void Spdp::init(DDS::DomainId_t /*domain*/,
                       DCPS::RepoId& guid,
                       const DDS::DomainParticipantQos& qos,
                       RtpsDiscovery* disco)
{
  bool enable_writers = true;

  const DDS::PropertySeq& properties = qos.property.value;
  for (unsigned int idx = 0; idx != properties.length(); ++idx) {
    const char* name = properties[idx].name.in();
    if (std::strcmp(RTPS_DISCOVERY_ENDPOINT_ANNOUNCEMENTS, name) == 0) {
      if (ACE_OS::strcasecmp(properties[idx].value.in(), "0") == 0 ||
          ACE_OS::strcasecmp(properties[idx].value.in(), "false") == 0) {
        enable_writers = false;
      }
    }
  }

  available_builtin_endpoints_ =
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER
    ;

  if (enable_writers) {
    available_builtin_endpoints_ |=
      DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
      DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
      DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
  }

#ifdef OPENDDS_SECURITY
  if (is_security_enabled()) {
    available_builtin_endpoints_ |=
      DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_READER |
      DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER |
      DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER |
      DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER |
      DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER |
      DDS::Security::BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER |
      DDS::Security::BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER |
      DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER |
      DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_READER
      ;
    if (enable_writers) {
      available_builtin_endpoints_ |=
        DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER |
        DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER |
        DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    }
  }
#endif

  guid = guid_; // may have changed in SpdpTransport constructor
  sedp_.ignore(guid);
  sedp_.init(guid_, *disco, domain_);

#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = sedp_.get_ice_endpoint();
  if (endpoint) {
    RepoId l = guid_;
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    ICE::Agent::instance()->add_local_agent_info_listener(endpoint, l, this);
  }
#endif
}

Spdp::Spdp(DDS::DomainId_t domain,
           RepoId& guid,
           const DDS::DomainParticipantQos& qos,
           RtpsDiscovery* disco)

  : DCPS::LocalParticipant<Sedp>(qos)
  , disco_(disco)
  , config_(disco_->config())
  , domain_(domain)
  , guid_(guid)
  , tport_(new SpdpTransport(this))
  , eh_(tport_)
  , eh_shutdown_(false)
  , shutdown_cond_(lock_)
  , shutdown_flag_(false)
  , available_builtin_endpoints_(0)
  , sedp_(guid_, *this, lock_)
#ifdef OPENDDS_SECURITY
  , security_config_()
  , security_enabled_(false)
  , identity_handle_(DDS::HANDLE_NIL)
  , permissions_handle_(DDS::HANDLE_NIL)
  , crypto_handle_(DDS::HANDLE_NIL)
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
  , config_(disco_->config())
  , domain_(domain)
  , guid_(guid)
  , tport_(new SpdpTransport(this))
  , eh_(tport_)
  , eh_shutdown_(false)
  , shutdown_cond_(lock_)
  , shutdown_flag_(false)
  , available_builtin_endpoints_(0)
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
#ifdef OPENDDS_SECURITY
        ICE::Endpoint* sedp_endpoint = sedp_.get_ice_endpoint();
        if (sedp_endpoint) {
          stop_ice(sedp_endpoint, part->first, part->second.pdata_.participantProxy.availableBuiltinEndpoints);
        }
        ICE::Endpoint* spdp_endpoint = tport_->get_ice_endpoint();
        if (spdp_endpoint) {
          ICE::Agent::instance()->stop_ice(spdp_endpoint, guid_, part->first);
        }
        purge_handshake_deadlines(part);
#endif
        remove_discovered_participant(part);
      }
    }
  }

#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = sedp_.get_ice_endpoint();
  if (endpoint) {
    RepoId l = guid_;
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    ICE::Agent::instance()->remove_local_agent_info_listener(endpoint, l);
  }
#endif

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

  const Security::SPDPdiscoveredParticipantData pdata =
    build_local_pdata(Security::DPDK_SECURE);

  sedp_.write_dcps_participant_secure(pdata, GUID_UNKNOWN);
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

#ifndef DDS_HAS_MINIMUM_BIT
void
Spdp::enqueue_location_update_i(DiscoveredParticipantIter iter,
                                DCPS::ParticipantLocation mask,
                                const ACE_INET_Addr& from)
{
  // We have the global lock.
  iter->second.location_updates_.push_back(DiscoveredParticipant::LocationUpdate(mask, from));
}

void
Spdp::process_location_updates_i(DiscoveredParticipantIter iter)
{
  // We have the global lock.

  if (iter->second.bit_ih_ == DDS::HANDLE_NIL) {
    // Not in the built-in topics.
    return;
  }

  const RepoId guid = iter->first;

  DiscoveredParticipant::LocationUpdateList location_updates;
  std::swap(iter->second.location_updates_, location_updates);

  for (DiscoveredParticipant::LocationUpdateList::const_iterator pos = location_updates.begin(),
         limit = location_updates.end(); pos != limit; ++pos) {
    DCPS::ParticipantLocationBuiltinTopicData& location_data = iter->second.location_data_;

    OPENDDS_STRING addr = "";
    ACE_TCHAR buffer[256];

    const DCPS::ParticipantLocation old_mask = location_data.location;

    if (pos->from_ != ACE_INET_Addr()) {
      location_data.location |= pos->mask_;
      pos->from_.addr_to_string(buffer, 256);
      addr = ACE_TEXT_ALWAYS_CHAR(buffer);
    } else {
      location_data.location &= ~(pos->mask_);
    }

    location_data.change_mask = pos->mask_;

    const DCPS::SystemTimePoint now = DCPS::SystemTimePoint::now();

    bool address_change = false;
    switch (pos->mask_) {
    case DCPS::LOCATION_LOCAL:
      address_change = addr.compare(location_data.local_addr.in()) != 0;
      location_data.local_addr = addr.c_str();
      location_data.local_timestamp = now.to_dds_time();
      break;
    case DCPS::LOCATION_ICE:
      address_change = addr.compare(location_data.ice_addr.in()) != 0;
      location_data.ice_addr = addr.c_str();
      location_data.ice_timestamp = now.to_dds_time();
      break;
    case DCPS::LOCATION_RELAY:
      address_change = addr.compare(location_data.relay_addr.in()) != 0;
      location_data.relay_addr = addr.c_str();
      location_data.relay_timestamp = now.to_dds_time();
      break;
    case DCPS::LOCATION_LOCAL6:
      address_change = addr.compare(location_data.local6_addr.in()) != 0;
      location_data.local6_addr = addr.c_str();
      location_data.local6_timestamp = now.to_dds_time();
      break;
    case DCPS::LOCATION_ICE6:
      address_change = addr.compare(location_data.ice6_addr.in()) != 0;
      location_data.ice6_addr = addr.c_str();
      location_data.ice6_timestamp = now.to_dds_time();
      break;
    case DCPS::LOCATION_RELAY6:
      address_change = addr.compare(location_data.relay6_addr.in()) != 0;
      location_data.relay6_addr = addr.c_str();
      location_data.relay6_timestamp = now.to_dds_time();
      break;
    }

    const DDS::Time_t expr = (now - rtps_duration_to_time_duration(
                                                                   iter->second.pdata_.leaseDuration,
                                                                   iter->second.pdata_.participantProxy.protocolVersion,
                                                                   iter->second.pdata_.participantProxy.vendorId)).to_dds_time();
    if ((location_data.location & DCPS::LOCATION_LOCAL) && DCPS::operator<(location_data.local_timestamp, expr)) {
      location_data.location &= ~(DCPS::LOCATION_LOCAL);
      location_data.change_mask |= DCPS::LOCATION_LOCAL;
      location_data.local_timestamp = now.to_dds_time();
    }
    if ((location_data.location & DCPS::LOCATION_RELAY) && DCPS::operator<(location_data.relay_timestamp, expr)) {
      location_data.location &= ~(DCPS::LOCATION_RELAY);
      location_data.change_mask |= DCPS::LOCATION_RELAY;
      location_data.relay_timestamp = now.to_dds_time();
    }
    if ((location_data.location & DCPS::LOCATION_LOCAL6) && DCPS::operator<(location_data.local6_timestamp, expr)) {
      location_data.location &= ~(DCPS::LOCATION_LOCAL6);
      location_data.change_mask |= DCPS::LOCATION_LOCAL6;
      location_data.local6_timestamp = now.to_dds_time();
    }
    if ((location_data.location & DCPS::LOCATION_RELAY6) && DCPS::operator<(location_data.relay6_timestamp, expr)) {
      location_data.location &= ~(DCPS::LOCATION_RELAY6);
      location_data.change_mask |= DCPS::LOCATION_RELAY6;
      location_data.relay6_timestamp = now.to_dds_time();
    }

    if (old_mask != location_data.location || address_change) {
      DCPS::ParticipantLocationBuiltinTopicDataDataReaderImpl* locbit = part_loc_bit();
      if (locbit) {
        DDS::InstanceHandle_t handle = DDS::HANDLE_NIL;
        {
          const DCPS::ParticipantLocationBuiltinTopicData ld_copy(location_data);
          ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);
          ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
          handle = locbit->store_synthetic_data(ld_copy, DDS::NEW_VIEW_STATE);
        }
        iter = participants_.find(guid);
        if (iter != participants_.end()) {
          iter->second.location_ih_ = handle;
        } else {
          return;
        }
      }
    }
  }
}
#endif

ICE::Endpoint*
Spdp::get_ice_endpoint_if_added()
{
  return tport_->ice_endpoint_added_ ? tport_->get_ice_endpoint() : 0;
}

void
Spdp::handle_participant_data(DCPS::MessageId id,
                              const ParticipantData_t& cpdata,
                              const DCPS::SequenceNumber& seq,
                              const ACE_INET_Addr& from,
                              bool from_sedp)
{
  const MonotonicTimePoint now = MonotonicTimePoint::now();

  // Make a (non-const) copy so we can tweak values below
  ParticipantData_t pdata(cpdata);

  const DCPS::RepoId guid = make_guid(pdata.participantProxy.guidPrefix, DCPS::ENTITYID_PARTICIPANT);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  if (sedp_.ignoring(guid)) {
    // Ignore, this is our domain participant or one that the user has
    // asked us to ignore.
    return;
  }

  const bool from_relay = from == config_->spdp_rtps_relay_address();
  const DCPS::ParticipantLocation location_mask = compute_location_mask(from, from_relay);

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
    std::pair<DiscoveredParticipantIter, bool> p = participants_.insert(std::make_pair(guid, DiscoveredParticipant(pdata, now, seq)));
    iter = p.first;

#ifndef DDS_HAS_MINIMUM_BIT
    if (!from_sedp) {
      enqueue_location_update_i(iter, location_mask, from);
    }
#endif

#ifdef OPENDDS_SECURITY
    if (is_security_enabled()) {
      // Associate the stateless reader / writer for handshakes & auth requests
      sedp_.associate_preauth(iter->second.pdata_);
    }
#endif

    // Since we've just seen a new participant, let's send out our
    // own announcement, so they don't have to wait.
    if (from != ACE_INET_Addr()) {
      if (from_relay) {
        tport_->write_i(guid, SpdpTransport::SEND_TO_RELAY);
      } else {
        tport_->shorten_local_sender_delay_i();
      }
    }

#ifdef OPENDDS_SECURITY
    if (is_security_enabled()) {
      bool has_security_data = iter->second.pdata_.dataKind == Security::DPDK_ENHANCED ||
        iter->second.pdata_.dataKind == Security::DPDK_SECURE;

      if (has_security_data == false) {
        if (participant_sec_attr_.allow_unauthenticated_participants == false) {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::handle_participant_data - ")
              ACE_TEXT("Incompatible security attributes in discovered participant: %C\n"),
              OPENDDS_STRING(DCPS::GuidConverter(guid)).c_str()));
          }
          participants_.erase(guid);
        } else { // allow_unauthenticated_participants == true
          iter->second.auth_state_ = DCPS::AUTH_STATE_UNAUTHENTICATED;
          match_unauthenticated(guid, iter);
        }
      } else { // has_security_data == true
        iter->second.identity_token_ = pdata.ddsParticipantDataSecure.base.identity_token;
        iter->second.permissions_token_ = pdata.ddsParticipantDataSecure.base.permissions_token;
        iter->second.property_qos_ = pdata.ddsParticipantDataSecure.base.property;
        iter->second.security_info_ = pdata.ddsParticipantDataSecure.base.security_info;

        attempt_authentication(iter, true);
        if (iter->second.auth_state_ == DCPS::AUTH_STATE_UNAUTHENTICATED) {
          if (participant_sec_attr_.allow_unauthenticated_participants == false) {
            if (DCPS::security_debug.auth_debug) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::handle_participant_data - ")
                ACE_TEXT("Incompatible security attributes in discovered participant: %C\n"),
                OPENDDS_STRING(DCPS::GuidConverter(guid)).c_str()));
            }
            participants_.erase(guid);
          } else { // allow_unauthenticated_participants == true
            iter->second.auth_state_ = DCPS::AUTH_STATE_UNAUTHENTICATED;
            match_unauthenticated(guid, iter);
          }
        } else if (iter->second.auth_state_ == DCPS::AUTH_STATE_AUTHENTICATED) {
          if (match_authenticated(guid, iter) == false) {
            participants_.erase(guid);
          }
        }
        // otherwise just return, since we're waiting for input to finish authentication
      }
    } else {
      iter->second.auth_state_ = DCPS::AUTH_STATE_UNAUTHENTICATED;
      match_unauthenticated(guid, iter);
    }
#else
    match_unauthenticated(guid, iter);
#endif

  } else { // Existing Participant
#ifndef DDS_HAS_MINIMUM_BIT
    if (!from_sedp) {
      enqueue_location_update_i(iter, location_mask, from);
    }
#endif
#ifdef OPENDDS_SECURITY
    // Non-secure updates for authenticated participants are used for liveliness but
    // are otherwise ignored. Non-secure dispose messages are ignored completely.
    if (is_security_enabled() && iter->second.auth_state_ == DCPS::AUTH_STATE_AUTHENTICATED && !from_sedp) {
      iter->second.last_seen_ = now;
#ifndef DDS_HAS_MINIMUM_BIT
      process_location_updates_i(iter);
#endif
      return;
    }
#endif

    if (id == DCPS::DISPOSE_INSTANCE || id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
#ifdef OPENDDS_SECURITY
      ICE::Endpoint* sedp_endpoint = sedp_.get_ice_endpoint();
      if (sedp_endpoint) {
        stop_ice(sedp_endpoint, iter->first, iter->second.pdata_.participantProxy.availableBuiltinEndpoints);
      }
      ICE::Endpoint* spdp_endpoint = tport_->get_ice_endpoint();
      if (spdp_endpoint) {
        ICE::Agent::instance()->stop_ice(spdp_endpoint, guid_, iter->first);
      }
      purge_handshake_deadlines(iter);
#endif
      remove_discovered_participant(iter);
      return;
    }

    // Check if sequence numbers are increasing
    if (validateSequenceNumber(seq, iter)) {
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
        if (locators_changed(iter->second.pdata_.participantProxy, pdata.participantProxy)) {
          sedp_.update_locators(pdata);
        }
        iter->second.pdata_ = pdata;
        iter->second.last_seen_ = now;

#ifndef DDS_HAS_MINIMUM_BIT
        process_location_updates_i(iter);
#endif
      }
    // Else a reset has occured and check if we should remove the participant
    } else if (iter->second.seq_reset_count_ >= config_->max_spdp_sequence_msg_reset_check()) {
#ifdef OPENDDS_SECURITY
      purge_handshake_deadlines(iter);
#endif
      remove_discovered_participant(iter);
    }
  }
}

bool
Spdp::validateSequenceNumber(const DCPS::SequenceNumber& seq, DiscoveredParticipantIter& iter)
{
  if (seq.getValue() != 0 && iter->second.last_seq_ != DCPS::SequenceNumber::MAX_VALUE) {
    if (seq < iter->second.last_seq_) {
      ++iter->second.seq_reset_count_;
      return false;
    } else if (iter->second.seq_reset_count_ > 0) {
      --iter->second.seq_reset_count_;
    }
  }
  iter->second.last_seq_ = seq;
  return true;
}

void
Spdp::data_received(const DataSubmessage& data,
                    const ParameterList& plist,
                    const ACE_INET_Addr& from)
{
  if (shutdown_flag_.value()) { return; }

  ParticipantData_t pdata;

  pdata.participantProxy.domainId = domain_;

  if (!ParameterListConverter::from_param_list(plist, pdata)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::data_received - ")
      ACE_TEXT("failed to convert from ParameterList to ")
      ACE_TEXT("SPDPdiscoveredParticipantData\n")));
    return;
  }

  // Remote domain ID, if populated, has to match
  if (pdata.participantProxy.domainId != domain_) {
    return;
  }

  const DCPS::RepoId guid = make_guid(pdata.participantProxy.guidPrefix, DCPS::ENTITYID_PARTICIPANT);
  if (guid == guid_) {
    // About us, stop.
    return;
  }

  DCPS::SequenceNumber seq;
  seq.setValue(data.writerSN.high, data.writerSN.low);
  handle_participant_data((data.inlineQos.length() && disposed(data.inlineQos)) ? DCPS::DISPOSE_INSTANCE : DCPS::SAMPLE_DATA,
                          pdata, seq, from, false);

#ifdef OPENDDS_SECURITY
  if (!is_security_enabled()) {
    process_participant_ice(plist, pdata, guid);
  }
#endif
}

void
Spdp::match_unauthenticated(const DCPS::RepoId& guid, DiscoveredParticipantIter& dp_iter)
{
  // Must unlock when calling into part_bit() as it may call back into us
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  DDS::InstanceHandle_t bit_instance_handle = DDS::HANDLE_NIL;
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
  if (bit) {
    DDS::ParticipantBuiltinTopicData pbtd = partBitData(dp_iter->second.pdata_);
    ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
    bit_instance_handle =
      bit->store_synthetic_data(pbtd, DDS::NEW_VIEW_STATE);
    rg.release();
    dp_iter = participants_.find(guid);
    if (dp_iter == participants_.end()) {
      return;
    }
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  // notify Sedp of association
  // Sedp may call has_discovered_participant, which is why the participant must be added before this call to associate.
  sedp_.associate(dp_iter->second.pdata_);

  dp_iter->second.bit_ih_ = bit_instance_handle;
#ifndef DDS_HAS_MINIMUM_BIT
  process_location_updates_i(dp_iter);
#endif
}

#ifdef OPENDDS_SECURITY

void
Spdp::handle_auth_request(const DDS::Security::ParticipantStatelessMessage& msg)
{
  // If this message wasn't intended for us, ignore handshake message
  if (msg.destination_participant_guid != guid_ || msg.message_data.length() == 0) {
    return;
  }

  const RepoId guid = make_id(msg.message_identity.source_guid, DCPS::ENTITYID_PARTICIPANT);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (sedp_.ignoring(guid)) {
    // Ignore, this is our domain participant or one that the user has
    // asked us to ignore.
    return;
  }

  pending_remote_auth_tokens_[guid] = msg.message_data[0];
  DiscoveredParticipantMap::iterator iter = participants_.find(guid);

  if (iter != participants_.end()) {
    if (msg.message_identity.sequence_number <= iter->second.auth_req_sequence_number_) {
      return;
    }
    iter->second.auth_req_sequence_number_ = msg.message_identity.sequence_number;

    attempt_authentication(iter, false);
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

DDS::OctetSeq Spdp::local_participant_data_as_octets() const
{
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

  if (!ParameterListConverter::to_param_list(pbtds.base, plist)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::local_participant_data_as_octets() - ")
               ACE_TEXT("Failed to convert from ParticipantBuiltinTopicData to ParameterList\n")));
    return DDS::OctetSeq();
  }

  ACE_Message_Block temp_buff(DCPS::serialized_size(encoding_plain_big, plist));
  DCPS::Serializer ser(&temp_buff, encoding_plain_big);
  if (!(ser << plist)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::local_participant_data_as_octets() - ")
               ACE_TEXT("Failed to serialize parameter list.\n")));
    return DDS::OctetSeq();
  }

  DDS::OctetSeq seq(static_cast<unsigned int>(temp_buff.length()));
  seq.length(seq.maximum());
  std::memcpy(seq.get_buffer(), temp_buff.rd_ptr(), temp_buff.length());
  return seq;
}

void
Spdp::send_handshake_request(const DCPS::RepoId& guid, DiscoveredParticipant& dp)
{
  OPENDDS_ASSERT(dp.handshake_state_ == DCPS::HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST);

  Security::Authentication_var auth = security_config_->get_authentication();
  DDS::Security::SecurityException se = {"", 0, 0};
  if (dp.handshake_handle_ != DDS::HANDLE_NIL) {
    // Return the handle for reauth.
    if (!auth->return_handshake_handle(dp.handshake_handle_, se)) {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                   ACE_TEXT("Spdp::send_handshake_request() - ")
                   ACE_TEXT("Unable to return handshake handle. ")
                   ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return;
    }
    dp.handshake_handle_ = DDS::HANDLE_NIL;
  }

  const DDS::OctetSeq local_participant = local_participant_data_as_octets();
  if (!local_participant.length()) {
    return; // already logged in local_participant_data_as_octets()
  }

  DDS::Security::HandshakeMessageToken hs_mt;
  if (auth->begin_handshake_request(dp.handshake_handle_, hs_mt, identity_handle_, dp.identity_handle_,
                                    local_participant, se)
      != DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::send_handshake_request() - ")
               ACE_TEXT("Failed to begin handshake_request. Security Exception[%d.%d]: %C\n"),
               se.code, se.minor_code, se.message.in()));
    return;
  }

  dp.handshake_state_ = DCPS::HANDSHAKE_STATE_PROCESS_HANDSHAKE;

  DDS::Security::ParticipantStatelessMessage msg;
  msg.message_identity.source_guid = guid_;
  msg.message_class_id = DDS::Security::GMCLASSID_SECURITY_AUTH_HANDSHAKE;
  msg.destination_participant_guid = guid;
  msg.destination_endpoint_guid = GUID_UNKNOWN;
  msg.source_endpoint_guid = GUID_UNKNOWN;
  msg.related_message_identity.source_guid = GUID_UNKNOWN;
  msg.related_message_identity.sequence_number = 0;
  msg.message_data.length(1);
  msg.message_data[0] = hs_mt;

  if (send_handshake_message(guid, dp, msg) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::send_handshake_request() - ")
               ACE_TEXT("Unable to write stateless message (handshake).\n")));
    return;
  } else if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::send_handshake_request() - ")
               ACE_TEXT("Sent handshake request message for participant: %C\n"),
               OPENDDS_STRING(DCPS::GuidConverter(guid)).c_str()));
  }
}

void
Spdp::attempt_authentication(const DiscoveredParticipantIter& iter, bool from_discovery)
{
  const DCPS::RepoId& guid = iter->first;
  DiscoveredParticipant& dp = iter->second;

  PendingRemoteAuthTokenMap::iterator token_iter = pending_remote_auth_tokens_.find(guid);
  if (token_iter == pending_remote_auth_tokens_.end()) {
    dp.remote_auth_request_token_ = DDS::Security::Token();
  } else {
    dp.remote_auth_request_token_ = token_iter->second;
    pending_remote_auth_tokens_.erase(token_iter);
  }

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication for %C from_discovery=%d have_remote_token=%d auth_state=%d handshake_state=%d\n", OPENDDS_STRING(DCPS::GuidConverter(guid)).c_str(), from_discovery, !(dp.remote_auth_request_token_ == DDS::Security::Token()), dp.auth_state_, dp.handshake_state_));
  }

  if (!from_discovery && dp.handshake_state_ != DCPS::HANDSHAKE_STATE_WAITING_FOR_TOKEN && dp.handshake_state_ != DCPS::HANDSHAKE_STATE_DONE) {
    // Ignore auth reqs when already in progress.
    return;
  }

  // Reset.
  purge_handshake_deadlines(iter);
  dp.handshake_deadline_ = DCPS::MonotonicTimePoint::now() + config_->max_auth_time();
  handshake_deadlines_.insert(std::make_pair(dp.handshake_deadline_, guid));
  tport_->handshake_deadline_processor_->schedule(config_->max_auth_time());

  DDS::Security::Authentication_var auth = security_config_->get_authentication();
  DDS::Security::SecurityException se = {"", 0, 0};

  const DDS::Security::ValidationResult_t vr = auth->validate_remote_identity(dp.identity_handle_,
                                                                              dp.local_auth_request_token_, dp.remote_auth_request_token_, identity_handle_, dp.identity_token_, guid, se);

  dp.have_auth_req_msg_ = !(dp.local_auth_request_token_ == DDS::Security::Token());
  if (dp.have_auth_req_msg_) {
    dp.auth_req_msg_.message_identity.source_guid = guid_;
    dp.auth_req_msg_.message_identity.sequence_number = (++stateless_sequence_number_).getValue();
    dp.auth_req_msg_.message_class_id = DDS::Security::GMCLASSID_SECURITY_AUTH_REQUEST;
    dp.auth_req_msg_.destination_participant_guid = guid;
    dp.auth_req_msg_.destination_endpoint_guid = GUID_UNKNOWN;
    dp.auth_req_msg_.source_endpoint_guid = GUID_UNKNOWN;
    dp.auth_req_msg_.related_message_identity.source_guid = GUID_UNKNOWN;
    dp.auth_req_msg_.related_message_identity.sequence_number = 0;
    dp.auth_req_msg_.message_data.length(1);
    dp.auth_req_msg_.message_data[0] = dp.local_auth_request_token_;
    // Send the auth req immediately to reset the remote if they are
    // still authenticated with us.
    if (sedp_.write_stateless_message(dp.auth_req_msg_, make_id(guid, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER)) != DDS::RETCODE_OK) {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::attempt_authentication() - ")
                   ACE_TEXT("Unable to write auth req message.\n")));
      }
    } else {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                   ACE_TEXT("Sent auth req message for participant: %C\n"),
                   OPENDDS_STRING(DCPS::GuidConverter(guid)).c_str()));
      }
    }
    schedule_handshake_resend(config_->auth_resend_period(), guid);
  }

  switch (vr) {
  case DDS::Security::VALIDATION_OK: {
    dp.auth_state_ = DCPS::AUTH_STATE_AUTHENTICATED;
    dp.handshake_state_ = DCPS::HANDSHAKE_STATE_DONE;
    purge_handshake_deadlines(iter);
    return;
  }
  case DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE: {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                 ACE_TEXT("Attempting authentication (expecting request) for participant: %C\n"),
                 OPENDDS_STRING(DCPS::GuidConverter(guid)).c_str()));
    }
    dp.handshake_state_ = DCPS::HANDSHAKE_STATE_BEGIN_HANDSHAKE_REPLY;
    dp.is_requester_ = true;
    return; // We'll need to wait for an inbound handshake request from the remote participant
  }
  case DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST: {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                 ACE_TEXT("Attempting authentication (sending request/expecting reply) for participant: %C\n"),
                 OPENDDS_STRING(DCPS::GuidConverter(guid)).c_str()));
    }
    dp.handshake_state_ = DCPS::HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST;
    send_handshake_request(guid, dp);
    return;
  }
  case DDS::Security::VALIDATION_FAILED: {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                 ACE_TEXT("Remote participant identity is invalid. Security Exception[%d.%d]: %C\n"),
                 se.code, se.minor_code, se.message.in()));
    }
    dp.auth_state_ = DCPS::AUTH_STATE_UNAUTHENTICATED;
    dp.handshake_state_ = DCPS::HANDSHAKE_STATE_DONE;
    purge_handshake_deadlines(iter);
    return;
  }
  default: {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                 ACE_TEXT("Unexpected return value while validating remote identity. Security Exception[%d.%d]: %C\n"),
                 se.code, se.minor_code, se.message.in()));
    }
    dp.auth_state_ = DCPS::AUTH_STATE_UNAUTHENTICATED;
    dp.handshake_state_ = DCPS::HANDSHAKE_STATE_DONE;
    purge_handshake_deadlines(iter);
    return;
  }
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

  const RepoId src_participant = make_id(msg.message_identity.source_guid, DCPS::ENTITYID_PARTICIPANT);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  // If discovery hasn't initialized / validated this participant yet, ignore handshake messages
  DiscoveredParticipantIter iter = participants_.find(src_participant);
  if (iter == participants_.end()) {
    if (DCPS::security_debug.auth_warn) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) {auth_warn} Spdp::handle_handshake_message() - ")
        ACE_TEXT("received handshake for undiscovered participant %C. Ignoring.\n"),
                 OPENDDS_STRING(DCPS::GuidConverter(src_participant)).c_str()));
    }
    return;
  }

  DiscoveredParticipant& dp = iter->second;

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message for %C auth_state=%d handshake_state=%d\n", OPENDDS_STRING(DCPS::GuidConverter(src_participant)).c_str(), dp.auth_state_, dp.handshake_state_));
  }

  if (msg.message_identity.sequence_number <= iter->second.handshake_sequence_number_) {
    return;
  }
  iter->second.handshake_sequence_number_ = msg.message_identity.sequence_number;

  // We have received a handshake message from the remote which means
  // we don't need to send the auth req.
  dp.have_auth_req_msg_ = false;

  switch (dp.handshake_state_) {
  case DCPS::HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST:
  case DCPS::HANDSHAKE_STATE_WAITING_FOR_TOKEN:
  case DCPS::HANDSHAKE_STATE_DONE: {
    if (DCPS::security_debug.auth_warn) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) {auth_warn} Spdp::handle_handshake_message() - ")
                 ACE_TEXT("Invalid handshake state\n")));
    }
    return;
  }

  case DCPS::HANDSHAKE_STATE_BEGIN_HANDSHAKE_REPLY: {
    DDS::Security::ParticipantStatelessMessage reply;
    reply.message_identity.source_guid = guid_;
    reply.message_class_id = DDS::Security::GMCLASSID_SECURITY_AUTH_HANDSHAKE;
    reply.related_message_identity = msg.message_identity;
    reply.destination_participant_guid = src_participant;
    reply.destination_endpoint_guid = GUID_UNKNOWN;
    reply.source_endpoint_guid = GUID_UNKNOWN;
    reply.message_data.length(1);
    reply.message_data[0] = msg.message_data[0];

    if (dp.handshake_handle_ != DDS::HANDLE_NIL) {
      // Return the handle for reauth.
      if (!auth->return_handshake_handle(dp.handshake_handle_, se)) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                     ACE_TEXT("Spdp::handke_handshake_message() - ")
                     ACE_TEXT("Unable to return handshake handle. ")
                     ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                     se.code, se.minor_code, se.message.in()));
        }
        return;
      }
      dp.handshake_handle_ = DDS::HANDLE_NIL;
    }

    const DDS::OctetSeq local_participant = local_participant_data_as_octets();
    if (!local_participant.length()) {
      return; // already logged in local_participant_data_as_octets()
    }
    const DDS::Security::ValidationResult_t vr =
      auth->begin_handshake_reply(dp.handshake_handle_, reply.message_data[0], dp.identity_handle_,
                                  identity_handle_, local_participant, se);

    switch (vr) {
    case DDS::Security::VALIDATION_OK: {
      // Theoretically, this shouldn't happen unless handshakes can involve fewer than 3 messages
      dp.auth_state_ = DCPS::AUTH_STATE_AUTHENTICATED;
      dp.handshake_state_ = DCPS::HANDSHAKE_STATE_DONE;
      purge_handshake_deadlines(iter);
      match_authenticated(src_participant, iter);
      return;
    }
    case DDS::Security::VALIDATION_FAILED: {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} Spdp::handle_handshake_message() - ")
                   ACE_TEXT("Failed to reply to incoming handshake message. Security Exception[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return;
    }
    case DDS::Security::VALIDATION_PENDING_RETRY: {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} WARNING: Spdp::handle_handshake_message() - ")
                   ACE_TEXT("Unexpected validation pending retry\n")));
      }
      return;
    }
    case DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST: {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} WARNING: Spdp::handle_handshake_message() - ")
                   ACE_TEXT("Unexpected validation pending handshake request\n")));
      }
      return;
    }
    case DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE: {
      if (send_handshake_message(src_participant, dp, reply) != DDS::RETCODE_OK) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} Spdp::handle_handshake_message() - ")
                     ACE_TEXT("Unable to write stateless message for handshake reply.\n")));
        }
        return;
      } else {
        if (DCPS::security_debug.auth_debug) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - ")
                     ACE_TEXT("Sent handshake reply for participant: %C\n"),
                     OPENDDS_STRING(DCPS::GuidConverter(src_participant)).c_str()));
        }
      }
      dp.handshake_state_ = DCPS::HANDSHAKE_STATE_PROCESS_HANDSHAKE;
      return;
    }
    case DDS::Security::VALIDATION_OK_FINAL_MESSAGE: {
      // Theoretically, this shouldn't happen unless handshakes can involve fewer than 3 messages
      if (send_handshake_message(src_participant, dp, reply) != DDS::RETCODE_OK) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} Spdp::handle_handshake_message() - ")
                     ACE_TEXT("Unable to write stateless message for final message.\n")));
        }
        return;
      } else {
        if (DCPS::security_debug.auth_debug) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - ")
                     ACE_TEXT("Sent handshake final for participant: %C\n"),
                     OPENDDS_STRING(DCPS::GuidConverter(src_participant)).c_str()));
        }
      }
      dp.auth_state_ = DCPS::AUTH_STATE_AUTHENTICATED;
      dp.handshake_state_ = DCPS::HANDSHAKE_STATE_PROCESS_HANDSHAKE;
      purge_handshake_deadlines(iter);
      match_authenticated(src_participant, iter);
      return;
    }
    }
    return;
  }

  case DCPS::HANDSHAKE_STATE_PROCESS_HANDSHAKE: {
    DDS::Security::ParticipantStatelessMessage reply;
    reply.message_identity.source_guid = guid_;
    reply.message_class_id = DDS::Security::GMCLASSID_SECURITY_AUTH_HANDSHAKE;
    reply.related_message_identity = msg.message_identity;
    reply.destination_participant_guid = src_participant;
    reply.destination_endpoint_guid = GUID_UNKNOWN;
    reply.source_endpoint_guid = GUID_UNKNOWN;
    reply.message_data.length(1);

    DDS::Security::ValidationResult_t vr = auth->process_handshake(reply.message_data[0], msg.message_data[0],
                                                                   dp.handshake_handle_, se);
    switch (vr) {
    case DDS::Security::VALIDATION_FAILED: {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} WARNING: ")
                   ACE_TEXT("Spdp::handle_handshake_message() - ")
                   ACE_TEXT("Failed to process incoming handshake message when ")
                   ACE_TEXT("expecting %C from %C. Security Exception[%d.%d]: %C\n"),
                   dp.is_requester_ ? "final" : "reply",
                   OPENDDS_STRING(DCPS::GuidConverter(src_participant)).c_str(),
                   se.code, se.minor_code, se.message.in()));
      }
      return;
    }
    case DDS::Security::VALIDATION_PENDING_RETRY: {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} WARNING: Spdp::handle_handshake_message() - ")
                   ACE_TEXT("Unexpected validation pending retry\n")));
      }
      return;
    }
    case DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST: {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} WARNING: Spdp::handle_handshake_message() - ")
                   ACE_TEXT("Unexpected validation pending handshake request\n")));
      }
      return;
    }
    case DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE: {
      // Theoretically, this shouldn't happen unless handshakes can involve more than 3 messages
      if (send_handshake_message(src_participant, dp, reply) != DDS::RETCODE_OK) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} WARNING: Spdp::handle_handshake_message() - ")
                     ACE_TEXT("Unable to write stateless message for handshake reply.\n")));
        }
        return;
      } else {
        if (DCPS::security_debug.auth_debug) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - ")
                     ACE_TEXT("Sent handshake unknown message for participant: %C\n"),
                     OPENDDS_STRING(DCPS::GuidConverter(src_participant)).c_str()));
        }
      }
      return;
    }
    case DDS::Security::VALIDATION_OK_FINAL_MESSAGE: {
      dp.auth_state_ = DCPS::AUTH_STATE_AUTHENTICATED;
      dp.handshake_state_ = DCPS::HANDSHAKE_STATE_WAITING_FOR_TOKEN;
      // Install the shared secret before sending the final so that
      // we are prepared to receive the crypto tokens from the
      // replier.
      dp.seen_some_crypto_tokens_ = false;
      // match_authenticated releases the lock which means (1) iter
      // may become invalid and (2) The resend timer may fire and send
      // the request again.  So, disable the resend.
      dp.have_handshake_msg_ = false;
      match_authenticated(src_participant, iter);
      if (iter == participants_.end()) {
        return;
      }

      if (send_handshake_message(src_participant, iter->second, reply) != DDS::RETCODE_OK) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} WARNING: Spdp::handle_handshake_message() - ")
                     ACE_TEXT("Unable to write stateless message for final message.\n")));
        }
        return;
      } else {
        if (DCPS::security_debug.auth_debug) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - ")
                     ACE_TEXT("Sent handshake final for participant: %C\n"),
                     OPENDDS_STRING(DCPS::GuidConverter(src_participant)).c_str()));
        }
      }
      return;
    }
    case DDS::Security::VALIDATION_OK: {
      dp.auth_state_ = DCPS::AUTH_STATE_AUTHENTICATED;
      dp.handshake_state_ = DCPS::HANDSHAKE_STATE_DONE;
      purge_handshake_deadlines(iter);
      match_authenticated(src_participant, iter);
      return;
    }
    }
  }
  }
}

void
Spdp::process_handshake_deadlines(const DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  for (TimeQueue::iterator pos = handshake_deadlines_.begin(),
        limit = handshake_deadlines_.upper_bound(now); pos != limit;) {

    DiscoveredParticipantIter pit = participants_.find(pos->second);
    if (pit != participants_.end()) {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::process_handshake_deadlines() - ")
                   ACE_TEXT("Removing discovered participant due to authentication timeout: %C\n"),
                   OPENDDS_STRING(DCPS::GuidConverter(pos->second)).c_str()));
      }
      const DCPS::MonotonicTimePoint ptime = pos->first;
      const RepoId part_id = pos->second;
      if (participant_sec_attr_.allow_unauthenticated_participants == false) {
        ICE::Endpoint* sedp_endpoint = sedp_.get_ice_endpoint();
        if (sedp_endpoint) {
          stop_ice(sedp_endpoint, pit->first, pit->second.pdata_.participantProxy.availableBuiltinEndpoints);
        }
        ICE::Endpoint* spdp_endpoint = tport_->get_ice_endpoint();
        if (spdp_endpoint) {
          ICE::Agent::instance()->stop_ice(spdp_endpoint, guid_, pit->first);
        }
        handshake_deadlines_.erase(pos);
        remove_discovered_participant(pit);
      } else {
        purge_handshake_resends(pit);
        pit->second.auth_state_ = DCPS::AUTH_STATE_UNAUTHENTICATED;
        pit->second.handshake_state_ = DCPS::HANDSHAKE_STATE_DONE;
        handshake_deadlines_.erase(pos);
        match_unauthenticated(part_id, pit);
      }
      pos = handshake_deadlines_.lower_bound(ptime);
      limit = handshake_deadlines_.upper_bound(now);
    } else {
      handshake_deadlines_.erase(pos++);
    }
  }

  if (!handshake_deadlines_.empty()) {
    tport_->handshake_deadline_processor_->schedule(handshake_deadlines_.begin()->first - now);
  }
}

void
Spdp::process_handshake_resends(const DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  for (TimeQueue::iterator pos = handshake_resends_.begin(), limit = handshake_resends_.end();
       pos != limit && pos->first <= now;) {

    DiscoveredParticipantIter pit = participants_.find(pos->second);
    if (pit != participants_.end() &&
        pit->second.stateless_msg_deadline_ <= now) {
      RepoId reader = pit->first;
      reader.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
      pit->second.stateless_msg_deadline_ = now + config_->auth_resend_period();
      // Send the auth req first to reset the remote if necessary.
      if (pit->second.have_auth_req_msg_) {
        if (sedp_.write_stateless_message(pit->second.auth_req_msg_, reader) != DDS::RETCODE_OK) {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::process_handshake_resends() - ")
                       ACE_TEXT("Unable to write auth req message retry.\n")));
          }
        } else {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::process_handshake_resends() - ")
                       ACE_TEXT("Sent auth req message for participant: %C\n"),
                       OPENDDS_STRING(DCPS::GuidConverter(pit->first)).c_str()));
          }
        }
      }
      if (pit->second.have_handshake_msg_) {
        if (sedp_.write_stateless_message(pit->second.handshake_msg_, reader) != DDS::RETCODE_OK) {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::process_handshake_resends() - ")
                       ACE_TEXT("Unable to write handshake message retry.\n")));
          }
        } else {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::process_handshake_resends() - ")
                       ACE_TEXT("Sent handshake message for participant: %C\n"),
                       OPENDDS_STRING(DCPS::GuidConverter(pit->first)).c_str()));
          }
        }
      }

      handshake_resends_.insert(std::make_pair(pit->second.stateless_msg_deadline_, pit->first));
    }

    handshake_resends_.erase(pos++);
  }

  if (!handshake_resends_.empty()) {
    tport_->handshake_resend_processor_->schedule(handshake_resends_.begin()->first - now);
  }
}

bool
Spdp::handle_participant_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg,
                                       bool& send_our_tokens)
{
  DDS::Security::SecurityException se = {"", 0, 0};
  Security::CryptoKeyExchange_var key_exchange = security_config_->get_crypto_key_exchange();

  // If this message wasn't intended for us, ignore volatile message
  if (msg.destination_participant_guid != guid_ || !msg.message_data.length()) {
    return false;
  }

  const RepoId src_participant = make_id(msg.message_identity.source_guid, DCPS::ENTITYID_PARTICIPANT);

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);

  if (crypto_handle_ == DDS::HANDLE_NIL) {
    // not configured for RTPS Protection, therefore doesn't support participant crypto tokens
    return false;
  }

  // If discovery hasn't initialized / validated this participant yet, ignore volatile message
  DiscoveredParticipantIter iter = participants_.find(src_participant);
  if (iter == participants_.end()) {
    if (DCPS::security_debug.auth_warn) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) {auth_warn} Spdp::handle_participant_crypto_tokens() - ")
        ACE_TEXT("received tokens for undiscovered participant %C. Ignoring.\n"),
        OPENDDS_STRING(DCPS::GuidConverter(src_participant)).c_str()));
    }
    return false;
  }
  DiscoveredParticipant& dp = iter->second;

  const DDS::Security::ParticipantCryptoTokenSeq& inboundTokens =
    reinterpret_cast<const DDS::Security::ParticipantCryptoTokenSeq&>(msg.message_data);

  if (!key_exchange->set_remote_participant_crypto_tokens(crypto_handle_, dp.crypto_handle_, inboundTokens, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::handle_participant_crypto_tokens() - ")
      ACE_TEXT("Unable to set remote participant crypto tokens with crypto key exchange plugin. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    return false;
  }

  send_our_tokens = !dp.is_requester_ && !dp.seen_some_crypto_tokens_;
  dp.seen_some_crypto_tokens_ = true;
  if (dp.handshake_state_ == DCPS::HANDSHAKE_STATE_WAITING_FOR_TOKEN) {
    dp.handshake_state_ = DCPS::HANDSHAKE_STATE_DONE;
    purge_handshake_deadlines(iter);
  }

  return true;
}

bool Spdp::seen_crypto_tokens_from(const RepoId& sender)
{
  const RepoId src_participant = make_id(sender.guidPrefix, ENTITYID_PARTICIPANT);
  const DiscoveredParticipantIter iter = participants_.find(src_participant);
  if (iter == participants_.end()) {
    return false;
  }
  DiscoveredParticipant& dp = iter->second;
  const bool send_our_tokens = !dp.is_requester_ && !dp.seen_some_crypto_tokens_;
  dp.seen_some_crypto_tokens_ = true;
  if (dp.handshake_state_ == DCPS::HANDSHAKE_STATE_WAITING_FOR_TOKEN) {
    dp.handshake_state_ = DCPS::HANDSHAKE_STATE_DONE;
    purge_handshake_deadlines(iter);
  }

  return send_our_tokens;
}

DDS::ReturnCode_t
Spdp::send_handshake_message(const DCPS::RepoId& guid,
                             DiscoveredParticipant& dp,
                             const DDS::Security::ParticipantStatelessMessage& msg)
{
  dp.handshake_msg_ = msg;
  dp.handshake_msg_.message_identity.sequence_number = (++stateless_sequence_number_).getValue();

  const DCPS::RepoId reader = make_id(guid, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER);
  const DDS::ReturnCode_t retval = sedp_.write_stateless_message(dp.handshake_msg_, reader);
  dp.have_handshake_msg_ = true;
  dp.stateless_msg_deadline_ = schedule_handshake_resend(config_->auth_resend_period(), guid);
  return retval;
}

MonotonicTimePoint Spdp::schedule_handshake_resend(const TimeDuration& time, const RepoId& guid)
{
  const MonotonicTimePoint deadline = MonotonicTimePoint::now() + time;
  handshake_resends_.insert(std::make_pair(deadline, guid));
  tport_->handshake_resend_processor_->schedule(time);
  return deadline;
}

bool
Spdp::match_authenticated(const DCPS::RepoId& guid, DiscoveredParticipantIter& iter)
{
  if (iter->second.handshake_handle_ == DDS::HANDLE_NIL) {
    return true;
  }

  DDS::Security::SecurityException se = {"", 0, 0};

  Security::Authentication_var auth = security_config_->get_authentication();
  Security::AccessControl_var access = security_config_->get_access_control();
  Security::CryptoKeyFactory_var key_factory = security_config_->get_crypto_key_factory();
  Security::CryptoKeyExchange_var key_exchange = security_config_->get_crypto_key_exchange();

  if (iter->second.shared_secret_handle_ != 0) {
    // Return the shared secret.
    if (!auth->return_sharedsecret_handle(iter->second.shared_secret_handle_, se)) {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                   ACE_TEXT("Spdp::match_authenticated() - ")
                   ACE_TEXT("Unable to return shared secret handle. Security Exception[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return false;
    }

    // Get the new shared secret.
    iter->second.shared_secret_handle_ = auth->get_shared_secret(iter->second.handshake_handle_, se);
    if (iter->second.shared_secret_handle_ == 0) {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                   ACE_TEXT("Spdp::match_authenticated() - ")
                   ACE_TEXT("Unable to get shared secret handle. Security Exception[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return false;
    }

    sedp_.rekey_volatile(iter->second.pdata_);

    if (!auth->return_handshake_handle(iter->second.handshake_handle_, se)) {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                   ACE_TEXT("Spdp::send_handshake_request() - ")
                   ACE_TEXT("Unable to return handshake handle. ")
                   ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
      return false;
    }

    iter->second.handshake_handle_ = DDS::HANDLE_NIL;
    return true;
  }

  iter->second.shared_secret_handle_ = auth->get_shared_secret(iter->second.handshake_handle_, se);
  if (iter->second.shared_secret_handle_ == 0) {
    if (DCPS::security_debug.auth_warn) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
        ACE_TEXT("Spdp::match_authenticated() - ")
        ACE_TEXT("Unable to get shared secret handle. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    }
    return false;
  }

  if (!auth->get_authenticated_peer_credential_token(
      iter->second.authenticated_peer_credential_token_, iter->second.handshake_handle_, se)) {
    if (DCPS::security_debug.auth_warn) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
        ACE_TEXT("Spdp::match_authenticated() - ")
        ACE_TEXT("Unable to get authenticated peer credential token. ")
        ACE_TEXT("Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    }
    return false;
  }

  iter->second.permissions_handle_ = access->validate_remote_permissions(
    auth, identity_handle_, iter->second.identity_handle_,
    iter->second.permissions_token_, iter->second.authenticated_peer_credential_token_, se);
  if (participant_sec_attr_.is_access_protected &&
      iter->second.permissions_handle_ == DDS::HANDLE_NIL) {
    if (DCPS::security_debug.auth_warn) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
        ACE_TEXT("Spdp::match_authenticated() - ")
        ACE_TEXT("Unable to validate remote participant with access control plugin. ")
        ACE_TEXT("Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    }
    return false;
  }

  if (participant_sec_attr_.is_access_protected) {
    if (access->check_remote_participant(iter->second.permissions_handle_, domain_,
        iter->second.pdata_.ddsParticipantDataSecure, se) == false) {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
          ACE_TEXT("Spdp::match_authenticated() - ")
          ACE_TEXT("Remote participant check failed. Security Exception[%d.%d]: %C\n"),
            se.code, se.minor_code, se.message.in()));
      }
      return false;
    }
  }

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::match_authenticated - ")
               ACE_TEXT("auth and access control complete for peer %C\n"),
               OPENDDS_STRING(DCPS::GuidConverter(guid)).c_str()));
  }

  if (iter->second.crypto_handle_ == DDS::HANDLE_NIL) {
    iter->second.crypto_handle_ = key_factory->register_matched_remote_participant(
      crypto_handle_, iter->second.identity_handle_, iter->second.permissions_handle_,
      iter->second.shared_secret_handle_, se);
    if (iter->second.crypto_handle_ == DDS::HANDLE_NIL) {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
          ACE_TEXT("Spdp::match_authenticated() - Unable to register remote ")
          ACE_TEXT("participant with crypto key factory plugin. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"),
          se.code, se.minor_code, se.message.in()));
      }
      return false;
    }
  }

  if (crypto_handle_ != DDS::HANDLE_NIL) {
    if (key_exchange->create_local_participant_crypto_tokens(
        iter->second.crypto_tokens_, crypto_handle_, iter->second.crypto_handle_, se) == false) {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_debug} ")
          ACE_TEXT("Spdp::match_authenticated() - ")
          ACE_TEXT("Unable to create local participant crypto ")
          ACE_TEXT("tokens with crypto key exchange plugin. ")
          ACE_TEXT("Security Exception[%d.%d]: %C\n"),
          se.code, se.minor_code, se.message.in()));
      }
      return false;
    }
  }

  // Must unlock when calling into part_bit() as it may call back into us
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  DDS::InstanceHandle_t bit_instance_handle = DDS::HANDLE_NIL;
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
  if (bit) {
    DDS::ParticipantBuiltinTopicData pbtd = partBitData(iter->second.pdata_);
    ACE_GUARD_RETURN(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock, false);
    bit_instance_handle =
      bit->store_synthetic_data(pbtd, DDS::NEW_VIEW_STATE);
    rg.release();
    iter = participants_.find(guid);
    if (iter == participants_.end()) {
      return false;
    }
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  // notify Sedp of association
  // Sedp may call has_discovered_participant, which is the participant must be added before these calls to associate.

  sedp_.associate(iter->second.pdata_);
  sedp_.associate_volatile(iter->second.pdata_);
  sedp_.associate_secure_writers_to_readers(iter->second.pdata_);
  sedp_.associate_secure_readers_to_writers(iter->second.pdata_);
  iter->second.security_builtins_associated_ = true;

  iter->second.bit_ih_ = bit_instance_handle;
#ifndef DDS_HAS_MINIMUM_BIT
  process_location_updates_i(iter);
#endif
  return true;
}

bool Spdp::security_builtins_associated(const DCPS::RepoId& remoteParticipant) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  if (!security_enabled_) {
    return false;
  }

  const DiscoveredParticipantConstIter iter = participants_.find(remoteParticipant);
  return iter == participants_.end() ? false : iter->second.security_builtins_associated_;
}

void Spdp::update_agent_info(const DCPS::RepoId&, const ICE::AgentInfo&)
{
  if (is_security_enabled()) {
    write_secure_updates();
  }
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
      const MonotonicTimePoint expr(
        MonotonicTimePoint::now() -
        rtps_duration_to_time_duration(
          part->second.pdata_.leaseDuration,
          part->second.pdata_.participantProxy.protocolVersion,
          part->second.pdata_.participantProxy.vendorId
        )
      );
      if (part->second.last_seen_ < expr) {
        if (DCPS::DCPS_debug_level > 1) {
          DCPS::GuidConverter conv(part->first);
          ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) Spdp::remove_expired_participants() - ")
            ACE_TEXT("participant %C exceeded lease duration, removing\n"),
            OPENDDS_STRING(conv).c_str()));
        }
#ifdef OPENDDS_SECURITY
        ICE::Endpoint* sedp_endpoint = sedp_.get_ice_endpoint();
        if (sedp_endpoint) {
          stop_ice(sedp_endpoint, part->first, part->second.pdata_.participantProxy.availableBuiltinEndpoints);
        }
        ICE::Endpoint* spdp_endpoint = tport_->get_ice_endpoint();
        if (spdp_endpoint) {
          ICE::Agent::instance()->stop_ice(spdp_endpoint, guid_, part->first);
        }
        purge_handshake_deadlines(part);
#endif
        remove_discovered_participant(part);
      }
    }
  }
}

void
Spdp::remove_discovered_participant_i(DiscoveredParticipantIter iter)
{
  ACE_UNUSED_ARG(iter);

#ifdef OPENDDS_SECURITY
  if (security_config_) {
    DDS::Security::SecurityException se = {"", 0, 0};
    DDS::Security::Authentication_var auth = security_config_->get_authentication();

    if (iter->second.identity_handle_ != DDS::HANDLE_NIL) {
      if (!auth->return_identity_handle(iter->second.identity_handle_, se)) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                     ACE_TEXT("DiscoveryBase::remove_discovered_participant() - ")
                     ACE_TEXT("Unable to return identity handle. ")
                     ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                     se.code, se.minor_code, se.message.in()));
        }
      }
    }

    if (iter->second.handshake_handle_ != DDS::HANDLE_NIL) {
      if (!auth->return_handshake_handle(iter->second.handshake_handle_, se)) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                     ACE_TEXT("DiscoveryBase::remove_discovered_participant() - ")
                     ACE_TEXT("Unable to return handshake handle. ")
                     ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                     se.code, se.minor_code, se.message.in()));
        }
      }
    }

    if (iter->second.shared_secret_handle_ != 0) {
      if (!auth->return_sharedsecret_handle(iter->second.shared_secret_handle_, se)) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                     ACE_TEXT("Spdp::match_authenticated() - ")
                     ACE_TEXT("Unable to return sharedsecret handle. ")
                     ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                     se.code, se.minor_code, se.message.in()));
        }
      }
    }
  }

  // TODO:  What other security related clean up needs to be performed? (is every register unregistered)
  // TODO:  Is a local participant that is destroyed being cleaned up?
  // TODO:  How should this be split between here and DiscoveryBase?
#endif
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

WaitForAcks&
Spdp::wait_for_acks()
{
  return wait_for_acks_;
}

bool
Spdp::is_expectant_opendds(const GUID_t& participant) const
{
  const DiscoveredParticipantConstIter iter = participants_.find(participant);
  if (iter == participants_.end()) {
    return false;
  }
  const bool is_opendds = 0 == std::memcmp(&iter->second.pdata_.participantProxy.vendorId,
                                           DCPS::VENDORID_OCI, sizeof(VendorId_t));
  return is_opendds && ((iter->second.pdata_.participantProxy.opendds_participant_flags.bits & RTPS::PFLAGS_NO_ASSOCIATED_WRITERS) == 0);
}

ParticipantData_t
Spdp::build_local_pdata(
#ifdef OPENDDS_SECURITY
                        Security::DiscoveredParticipantDataKind kind
#endif
                        )
{
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
      domain_,
      "",
      PROTOCOLVERSION,
      {gp[0], gp[1], gp[2], gp[3], gp[4], gp[5],
       gp[6], gp[7], gp[8], gp[9], gp[10], gp[11]},
      VENDORID_OPENDDS,
      false /*expectsIQoS*/,
      available_builtin_endpoints_,
      0,
      sedp_.unicast_locators(),
      sedp_.multicast_locators(),
      nonEmptyList /*defaultMulticastLocatorList*/,
      nonEmptyList /*defaultUnicastLocatorList*/,
      {0 /*manualLivelinessCount*/},   //FUTURE: implement manual liveliness
      qos_.property,
      {PFLAGS_NO_ASSOCIATED_WRITERS} // opendds_participant_flags
    },
    { // Duration_t (leaseDuration)
      static_cast<CORBA::Long>(config_->lease_duration().value().sec()),
      0 // we are not supporting fractional seconds in the lease duration
    }
  };

  return pdata;
}


bool Spdp::announce_domain_participant_qos()
{

#ifdef OPENDDS_SECURITY
  if (is_security_enabled()) {
    write_secure_updates();
  }
#endif

  return true;
}

#if !defined _MSC_VER || _MSC_VER > 1700
const Spdp::SpdpTransport::WriteFlags Spdp::SpdpTransport::SEND_TO_LOCAL;
const Spdp::SpdpTransport::WriteFlags Spdp::SpdpTransport::SEND_TO_RELAY;
#endif

Spdp::SpdpTransport::SpdpTransport(Spdp* outer)
  : outer_(outer)
  , lease_duration_(outer_->config_->lease_duration())
  , buff_(64 * 1024)
  , wbuff_(64 * 1024)
  , reactor_task_(false)
  , network_is_unreachable_(false)
  , ice_endpoint_added_(false)
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

#ifdef ACE_HAS_MAC_OSX
  multicast_socket_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                         ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#ifdef ACE_HAS_IPV6
  multicast_ipv6_socket_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                              ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#endif
#endif

  multicast_interface_ = outer_->disco_->multicast_interface();

  // Ports are set by the formulas in RTPS v2.1 Table 9.8
  const u_short port_common = outer_->config_->pb() +
    (outer_->config_->dg() * outer_->domain_);
  mc_port_ = port_common + outer_->config_->d0();

  multicast_address_ = outer_->config_->default_multicast_group();
  multicast_address_.set_port_number(mc_port_);

#ifdef ACE_HAS_IPV6
  multicast_ipv6_address_ = outer_->config_->ipv6_default_multicast_group();
  multicast_ipv6_address_.set_port_number(mc_port_);
#endif

  send_addrs_.insert(multicast_address_);
#ifdef ACE_HAS_IPV6
  send_addrs_.insert(multicast_ipv6_address_);
#endif

  typedef RtpsDiscovery::AddrVec::const_iterator iter;
  const RtpsDiscovery::AddrVec addrs = outer_->config_->spdp_send_addrs();
  for (iter it = addrs.begin(),
       end = addrs.end(); it != end; ++it) {
    send_addrs_.insert(ACE_INET_Addr(it->c_str()));
  }

  u_short participantId = 0;

#ifdef OPENDDS_SAFETY_PROFILE
  const u_short startingParticipantId = participantId;
#endif

  while (!open_unicast_socket(port_common, participantId)) {
    ++participantId;
  }
#ifdef ACE_HAS_IPV6
  u_short port = uni_port_;

  while (!open_unicast_ipv6_socket(port)) {
    ++port;
  }
#endif

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
}

void
Spdp::SpdpTransport::open()
{
  reactor_task_.open(0);

#ifdef OPENDDS_SECURITY
  // Add the endpoint before any sending and receiving occurs.
  ICE::Endpoint* endpoint = get_ice_endpoint();
  if (endpoint) {
    ICE::Agent::instance()->add_endpoint(endpoint);
    ice_endpoint_added_ = true;
  }
#endif

  ACE_Reactor* reactor = reactor_task_.get_reactor();
  if (reactor->register_handler(unicast_socket_.get_handle(),
                                this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register unicast input handler");
  }

#ifdef ACE_HAS_IPV6
  if (reactor->register_handler(unicast_ipv6_socket_.get_handle(),
                                this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register unicast IPv6 input handler");
  }
#endif

  job_queue_ = DCPS::make_rch<DCPS::JobQueue>(reactor);

#ifdef OPENDDS_SECURITY
  // Now that the endpoint is added, SEDP can write the SPDP info.
  if (outer_->is_security_enabled()) {
    outer_->write_secure_updates();
  }
#endif

  local_sender_ = DCPS::make_rch<SpdpMulti>(reactor_task_.interceptor(), outer_->config_->resend_period(), ref(*this), &SpdpTransport::send_local);
  local_sender_->enable(outer_->config_->resend_period());

#ifdef OPENDDS_SECURITY
  handshake_deadline_processor_ = DCPS::make_rch<SpdpSporadic>(reactor_task_.interceptor(), ref(*this), &SpdpTransport::process_handshake_deadlines);
  handshake_resend_processor_ = DCPS::make_rch<SpdpSporadic>(reactor_task_.interceptor(), ref(*this), &SpdpTransport::process_handshake_resends);
#endif

  relay_sender_ = DCPS::make_rch<SpdpPeriodic>(reactor_task_.interceptor(), ref(*this), &SpdpTransport::send_relay);
  if (outer_->config_->spdp_rtps_relay_address() != ACE_INET_Addr() ||
      outer_->config_->use_rtps_relay()) {
    relay_sender_->enable(false, outer_->config_->spdp_rtps_relay_send_period());
  }

  relay_beacon_ = DCPS::make_rch<SpdpPeriodic>(reactor_task_.interceptor(), ref(*this), &SpdpTransport::send_relay_beacon);
  if (outer_->config_->spdp_rtps_relay_address() != ACE_INET_Addr() ||
      outer_->config_->use_rtps_relay()) {
    relay_beacon_->enable(false, outer_->config_->spdp_rtps_relay_beacon_period());
  }

  DCPS::NetworkConfigMonitor_rch ncm = TheServiceParticipant->network_config_monitor();
  if (outer_->config_->use_ncm() && ncm) {
    ncm->add_listener(*this);
  } else {
    DCPS::NetworkInterface nic(0, multicast_interface_, true);
    nic.add_default_addrs();
    const bool all = multicast_interface_.empty();
    job_queue_->enqueue(DCPS::make_rch<ChangeMulticastGroup>(rchandle_from(this), nic,
                                                             ChangeMulticastGroup::CMG_JOIN, all));
  }
}

Spdp::SpdpTransport::~SpdpTransport()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SpdpTransport::~SpdpTransport\n")));
  }
  try {
    ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
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
#ifdef ACE_HAS_IPV6
  unicast_ipv6_socket_.close();
  multicast_ipv6_socket_.close();
#endif
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
  DCPS::Serializer ser(&wbuff_, encoding_plain_native);
  CORBA::UShort options = 0;
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap_LE) || !(ser << options)
      || !(ser << plist)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::dispose_unregister() - ")
      ACE_TEXT("failed to serialize headers for dispose/unregister\n")));
    return;
  }

  send(SEND_TO_LOCAL | SEND_TO_RELAY);
}

void
Spdp::SpdpTransport::close()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SpdpTransport::close\n")));
  }

  DCPS::NetworkConfigMonitor_rch ncm = TheServiceParticipant->network_config_monitor();
  if (ncm) {
    ncm->remove_listener(*this);
  }

#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = get_ice_endpoint();
  if (endpoint) {
    ICE::Agent::instance()->remove_endpoint(endpoint);
    ice_endpoint_added_ = false;
  }

  if (handshake_deadline_processor_) {
    handshake_deadline_processor_->cancel_and_wait();
  }
  if (handshake_resend_processor_) {
    handshake_resend_processor_->cancel_and_wait();
  }
#endif
  if (relay_sender_) {
    relay_sender_->disable_and_wait();
  }
  if (relay_beacon_) {
    relay_beacon_->disable_and_wait();
  }
  if (local_sender_) {
    local_sender_->disable_and_wait();
  }

  ACE_Reactor* reactor = reactor_task_.get_reactor();
  const ACE_Reactor_Mask mask =
    ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL;
  reactor->remove_handler(multicast_socket_.get_handle(), mask);
  reactor->remove_handler(unicast_socket_.get_handle(), mask);
#ifdef ACE_HAS_IPV6
  reactor->remove_handler(multicast_ipv6_socket_.get_handle(), mask);
  reactor->remove_handler(unicast_ipv6_socket_.get_handle(), mask);
#endif
  reactor_task_.stop();
}

void
Spdp::SpdpTransport::shorten_local_sender_delay_i()
{
  if (local_sender_) {
    const TimeDuration quick_resend = outer_->config_->resend_period() * outer_->config_->quick_resend_ratio();
    const TimeDuration min_resend = outer_->config_->min_resend_delay();
    local_sender_->enable(std::max(quick_resend, min_resend));
  }
}

void
Spdp::SpdpTransport::write(WriteFlags flags)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
  write_i(flags);
}

void
Spdp::SpdpTransport::write_i(WriteFlags flags)
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
  if (!ParameterListConverter::to_param_list(pdata, plist)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to convert from SPDPdiscoveredParticipantData ")
      ACE_TEXT("to ParameterList\n")));
    return;
  }

#ifdef OPENDDS_SECURITY
  if (!outer_->is_security_enabled()) {
    ICE::AgentInfoMap ai_map;
    ICE::Endpoint* sedp_endpoint = outer_->sedp_.get_ice_endpoint();
    if (sedp_endpoint) {
      ai_map["SEDP"] = ICE::Agent::instance()->get_local_agent_info(sedp_endpoint);
    }
    ICE::Endpoint* spdp_endpoint = get_ice_endpoint();
    if (spdp_endpoint) {
      ai_map["SPDP"] = ICE::Agent::instance()->get_local_agent_info(spdp_endpoint);
    }

    if (!ParameterListConverter::to_param_list(ai_map, plist)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("Spdp::SpdpTransport::write() - ")
                 ACE_TEXT("failed to convert from ICE::AgentInfo ")
                 ACE_TEXT("to ParameterList\n")));
      return;
    }
  }
#endif

  wbuff_.reset();
  CORBA::UShort options = 0;
  DCPS::Serializer ser(&wbuff_, encoding_plain_native);
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap_LE) || !(ser << options)
      || !(ser << plist)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to serialize headers for SPDP\n")));
    return;
  }

  send(flags);
}


void
Spdp::SpdpTransport::write_i(const DCPS::RepoId& guid, WriteFlags flags)
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
  if (!ParameterListConverter::to_param_list(pdata, plist)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to convert from SPDPdiscoveredParticipantData ")
      ACE_TEXT("to ParameterList\n")));
    return;
  }

#ifdef OPENDDS_SECURITY
  if (!outer_->is_security_enabled()) {
    ICE::AgentInfoMap ai_map;
    ICE::Endpoint* sedp_endpoint = outer_->sedp_.get_ice_endpoint();
    if (sedp_endpoint) {
      ai_map["SEDP"] = ICE::Agent::instance()->get_local_agent_info(sedp_endpoint);
    }
    ICE::Endpoint* spdp_endpoint = get_ice_endpoint();
    if (spdp_endpoint) {
      ai_map["SPDP"] = ICE::Agent::instance()->get_local_agent_info(spdp_endpoint);
    }

  if (!ParameterListConverter::to_param_list(ai_map, plist)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("Spdp::SpdpTransport::write() - ")
                 ACE_TEXT("failed to convert from ICE::AgentInfo ")
                 ACE_TEXT("to ParameterList\n")));
      return;
    }
  }
#endif

  InfoDestinationSubmessage info_dst;
  info_dst.smHeader.submessageId = INFO_DST;
  info_dst.smHeader.flags = FLAG_E;
  info_dst.smHeader.submessageLength = sizeof(guid.guidPrefix);
  std::memcpy(info_dst.guidPrefix, guid.guidPrefix, sizeof(guid.guidPrefix));

  wbuff_.reset();
  CORBA::UShort options = 0;
  DCPS::Serializer ser(&wbuff_, encoding_plain_native);
  if (!(ser << hdr_) || !(ser << info_dst) || !(ser << data_) || !(ser << encap_LE) || !(ser << options)
      || !(ser << plist)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to serialize headers for SPDP\n")));
    return;
  }

  send(flags);
}

void
Spdp::SpdpTransport::send(WriteFlags flags)
{
  if ((flags & SEND_TO_LOCAL) && !outer_->config_->rtps_relay_only()) {
    typedef OPENDDS_SET(ACE_INET_Addr)::const_iterator iter_t;
    for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
      send(*iter);
    }
  }

  const ACE_INET_Addr relay_address = outer_->config_->spdp_rtps_relay_address();
  if (((flags & SEND_TO_RELAY) || outer_->config_->rtps_relay_only()) &&
      relay_address != ACE_INET_Addr()) {
    send(relay_address);
  }
}

const ACE_SOCK_Dgram&
Spdp::SpdpTransport::choose_send_socket(const ACE_INET_Addr& addr) const
{
#ifdef ACE_HAS_IPV6
  if (addr.get_type() == AF_INET6) {
    return unicast_ipv6_socket_;
  }
#endif
  ACE_UNUSED_ARG(addr);
  return unicast_socket_;
}

void
Spdp::SpdpTransport::send(const ACE_INET_Addr& addr)
{
  ssize_t res;

  const ACE_SOCK_Dgram& socket = choose_send_socket(addr);
  res = socket.send(wbuff_.rd_ptr(), wbuff_.length(), addr);

  if (res < 0) {
    const int err = errno;
    if (err != ENETUNREACH || !network_is_unreachable_) {
      ACE_TCHAR addr_buff[256] = {};
      addr.addr_to_string(addr_buff, 256);
      errno = err;
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: Spdp::SpdpTransport::send() - ")
                 ACE_TEXT("destination %s failed send: %m\n"), addr_buff));
    }
    if (err == ENETUNREACH) {
      network_is_unreachable_ = true;
    }
  } else {
    network_is_unreachable_ = false;
  }
}

const ACE_SOCK_Dgram&
Spdp::SpdpTransport::choose_recv_socket(ACE_HANDLE h) const
{
#ifdef ACE_HAS_IPV6
  if (h == unicast_ipv6_socket_.get_handle()) {
    return unicast_ipv6_socket_;
  }
  if (h == multicast_ipv6_socket_.get_handle()) {
    return multicast_ipv6_socket_;
  }
#endif
  if (h == multicast_socket_.get_handle()) {
    return multicast_socket_;
  }

  return unicast_socket_;
}

int
Spdp::SpdpTransport::handle_input(ACE_HANDLE h)
{
  const ACE_SOCK_Dgram& socket = choose_recv_socket(h);

  ACE_INET_Addr remote;
  buff_.reset();

#ifdef ACE_LACKS_SENDMSG
  const ssize_t bytes = socket.recv(buff_.wr_ptr(), buff_.space(), remote);
#else
  ACE_INET_Addr local;

  iovec iov[1];
  iov[0].iov_base = buff_.wr_ptr();
#ifdef _MSC_VER
#pragma warning(push)
  // iov_len is 32-bit on 64-bit VC++, but we don't want a cast here
  // since on other platforms iov_len is 64-bit
#pragma warning(disable : 4267)
#endif
  iov[0].iov_len = buff_.space();
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  const ssize_t bytes = socket.recv(iov, 1, remote, 0
#if defined(ACE_RECVPKTINFO) || defined(ACE_RECVPKTINFO6)
                                    , &local
#endif
                                    );
#endif

  if (bytes > 0) {
    buff_.wr_ptr(bytes);
  } else if (bytes == 0) {
    return 0;
  } else {
    ACE_DEBUG((
          LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: Spdp::SpdpTransport::handle_input() - ")
          ACE_TEXT("error reading from %C socket %p\n")
          , (h == unicast_socket_.get_handle()) ? "unicast" : "multicast",
          ACE_TEXT("ACE_SOCK_Dgram::recv")));
    return 0;
  }

  // Handle some RTI protocol multicast to the same address
  if ((buff_.size() >= 4) && (!ACE_OS::memcmp(buff_.rd_ptr(), "RTPX", 4))) {
    return 0; // Ignore
  }

  ICE::Endpoint* endpoint = get_ice_endpoint();
  if (endpoint && (buff_.size() >= 4) && ACE_OS::memcmp(buff_.rd_ptr(), "RTPS", 4)) {
# ifndef ACE_RECVPKTINFO
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() ")
      ACE_TEXT("potential STUN message received but this version of the ACE ")
      ACE_TEXT("library doesn't support the local_address extension in ")
      ACE_TEXT("ACE_SOCK_Dgram::recv\n")));
    ACE_NOTSUP_RETURN(0);
# else

#ifdef OPENDDS_SECURITY
    // Assume STUN
    DCPS::Serializer serializer(&buff_, STUN::encoding);
    STUN::Message message;
    message.block = &buff_;
    if (serializer >> message) {
      ICE::Agent::instance()->receive(endpoint, local, remote, message);
    }
#endif
    return 0;
# endif
  }
  DCPS::Serializer ser(&buff_, encoding_plain_native);
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

      outer_->data_received(data, plist, remote);
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
  ACE_Reactor* reactor = reactor_task_.get_reactor();
  reactor->notify(this);
}

ICE::Endpoint*
Spdp::SpdpTransport::get_ice_endpoint()
{
#ifdef OPENDDS_SECURITY
  return (outer_->config_->use_ice()) ? this : 0;
#else
  return 0;
#endif
}

#ifdef OPENDDS_SECURITY
ICE::AddressListType
Spdp::SpdpTransport::host_addresses() const
{
  ICE::AddressListType addresses;
  ACE_INET_Addr addr;

  unicast_socket_.get_local_addr(addr);
  if (addr != ACE_INET_Addr()) {
    if (addr.is_any()) {
      ICE::AddressListType addrs;
      DCPS::get_interface_addrs(addrs);
      for (ICE::AddressListType::iterator pos = addrs.begin(), limit = addrs.end(); pos != limit; ++pos) {
        if (pos->get_type() == AF_INET) {
          pos->set_port_number(addr.get_port_number());
          addresses.push_back(*pos);
        }
      }
    } else {
      addresses.push_back(addr);
    }
  }

#ifdef ACE_HAS_IPV6
  unicast_ipv6_socket_.get_local_addr(addr);
  if (addr != ACE_INET_Addr()) {
    if (addr.is_any()) {
      ICE::AddressListType addrs;
      DCPS::get_interface_addrs(addrs);
      for (ICE::AddressListType::iterator pos = addrs.begin(), limit = addrs.end(); pos != limit; ++pos) {
        if (pos->get_type() == AF_INET6) {
          pos->set_port_number(addr.get_port_number());
          addresses.push_back(*pos);
        }
      }
    } else {
      addresses.push_back(addr);
    }
  }
#endif

  return addresses;
}

void
Spdp::SpdpTransport::send(const ACE_INET_Addr& address, const STUN::Message& message)
{
  DCPS::RcHandle<DCPS::JobQueue> job_queue = outer_->job_queue();
  if (job_queue) {
    job_queue->enqueue(DCPS::make_rch<SendStun>(rchandle_from(this), address, message));
  }
}

void
Spdp::SendStun::execute()
{
  ACE_GUARD(ACE_Thread_Mutex, g, tport_->outer_->lock_);
  tport_->wbuff_.reset();
  Serializer serializer(&tport_->wbuff_, STUN::encoding);
  const_cast<STUN::Message&>(message_).block = &tport_->wbuff_;
  serializer << message_;

  ssize_t res;
  const ACE_SOCK_Dgram& socket = tport_->choose_send_socket(address_);
  res = socket.send(tport_->wbuff_.rd_ptr(), tport_->wbuff_.length(), address_);

  if (res < 0) {
    const int e = errno;
    ACE_TCHAR addr_buff[256] = {};
    address_.addr_to_string(addr_buff, 256);
    errno = e;
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::send() - destination %s failed %m\n"), addr_buff));
  }
}

ACE_INET_Addr
Spdp::SpdpTransport::stun_server_address() const
{
  return outer_->config_->sedp_stun_server_address();
}

#ifndef DDS_HAS_MINIMUM_BIT
void
Spdp::SpdpTransport::ice_connect(const ICE::GuidSetType& guids, const ACE_INET_Addr& addr)
{
  job_queue_->enqueue(DCPS::make_rch<IceConnect>(rchandle_from(this->outer_), guids, addr, true));
}

void
Spdp::IceConnect::execute()
{
  ACE_GUARD(ACE_Thread_Mutex, g, spdp_->lock_);
  for (ICE::GuidSetType::const_iterator pos = guids_.begin(), limit = guids_.end(); pos != limit; ++pos) {
    DiscoveredParticipantIter iter = spdp_->participants_.find(pos->remote);
    if (iter != spdp_->participants_.end()) {
      spdp_->enqueue_location_update_i(iter, compute_ice_location_mask(addr_), connect_ ? addr_ : ACE_INET_Addr());
      spdp_->process_location_updates_i(iter);
    }
  }
}

void
Spdp::SpdpTransport::ice_disconnect(const ICE::GuidSetType& guids, const ACE_INET_Addr& addr)
{
  job_queue_->enqueue(DCPS::make_rch<IceConnect>(rchandle_from(this->outer_), guids, addr, false));
}
#endif /* DDS_HAS_MINIMUM_BIT */
#endif /* OPENDDS_SECURITY */

void
Spdp::signal_liveliness(DDS::LivelinessQosPolicyKind kind)
{
  sedp_.signal_liveliness(kind);
}

bool
Spdp::SpdpTransport::open_unicast_socket(u_short port_common,
                                         u_short participant_id)
{
  uni_port_ = port_common + outer_->config_->d1() + (outer_->config_->pg() * participant_id);

  ACE_INET_Addr local_addr = outer_->config_->spdp_local_address();
  local_addr.set_port_number(uni_port_);

  if (unicast_socket_.open(local_addr, PF_INET) != 0) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((
            LM_WARNING,
            ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
            ACE_TEXT("failed to open_appropriate_socket_type unicast socket on port %d %p.  ")
            ACE_TEXT("Trying next participantId...\n"),
            uni_port_, ACE_TEXT("ACE_SOCK_Dgram::open")));
    }
    return false;
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((
          LM_INFO,
          ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
          ACE_TEXT("opened unicast socket on port %d\n"),
          uni_port_));
  }

  if (!DCPS::set_socket_multicast_ttl(unicast_socket_, outer_->config_->ttl())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket() - ")
               ACE_TEXT("failed to set TTL value to %d ")
               ACE_TEXT("for port:%hu %p\n"),
               outer_->config_->ttl(), uni_port_, ACE_TEXT("DCPS::set_socket_multicast_ttl:")));
    throw std::runtime_error("failed to set TTL");
  }

#ifdef ACE_RECVPKTINFO
  int sockopt = 1;
  if (unicast_socket_.set_option(IPPROTO_IP, ACE_RECVPKTINFO, &sockopt, sizeof sockopt) == -1) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket: set_option: %m\n")), false);
  }
#endif

  return true;
}

#ifdef ACE_HAS_IPV6
bool
Spdp::SpdpTransport::open_unicast_ipv6_socket(u_short port)
{
  ipv6_uni_port_ = port;

  ACE_INET_Addr local_addr = outer_->config_->ipv6_spdp_local_address();
  local_addr.set_port_number(ipv6_uni_port_);

  if (unicast_ipv6_socket_.open(local_addr, PF_INET6) != 0) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((
                 LM_WARNING,
                 ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_ipv6_socket() - ")
                 ACE_TEXT("failed to open_appropriate_socket_type unicast ipv6 socket on port %d %p.  ")
                 ACE_TEXT("Trying next port...\n"),
                 uni_port_, ACE_TEXT("ACE_SOCK_Dgram::open")));
    }
    return false;
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((
          LM_INFO,
          ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_ipv6_socket() - ")
          ACE_TEXT("opened unicast ipv6 socket on port %d\n"),
          ipv6_uni_port_));
  }

  if (!DCPS::set_socket_multicast_ttl(unicast_ipv6_socket_, outer_->config_->ttl())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_ipv6_socket() - ")
               ACE_TEXT("failed to set TTL value to %d ")
               ACE_TEXT("for port:%hu %p\n"),
               outer_->config_->ttl(), ipv6_uni_port_, ACE_TEXT("DCPS::set_socket_multicast_ttl:")));
    throw std::runtime_error("failed to set TTL");
  }

#ifdef ACE_RECVPKTINFO6
  int sockopt = 1;
  if (unicast_ipv6_socket_.set_option(IPPROTO_IPV6, ACE_RECVPKTINFO6, &sockopt, sizeof sockopt) == -1) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_ipv6_socket: set_option: %m\n")), false);
  }
#endif

  return true;
}
#endif /* ACE_HAS_IPV6 */

void
Spdp::SpdpTransport::join_multicast_group(const DCPS::NetworkInterface& nic,
                                          bool all_interfaces)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);

  if (nic.exclude_from_multicast(multicast_interface_.c_str())) {
    return;
  }

  if (joined_interfaces_.count(nic.name()) == 0 && nic.has_ipv4()) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_TCHAR buff[256];
      multicast_address_.addr_to_string(buff, 256);
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Spdp::SpdpTransport::join_multicast_group ")
                 ACE_TEXT("joining group %s on %C\n"),
                 buff,
                 all_interfaces ? "all interfaces" : nic.name().c_str()));
    }

    if (0 == multicast_socket_.join(multicast_address_, 1, all_interfaces ? 0 : ACE_TEXT_CHAR_TO_TCHAR(nic.name().c_str()))) {
      joined_interfaces_.insert(nic.name());

      if (reactor()->register_handler(multicast_socket_.get_handle(),
                                    this, ACE_Event_Handler::READ_MASK) != 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Spdp::SpdpTransport::join_multicast_group failed to register multicast input handler\n")));
        return;
      }

      write_i(SEND_TO_LOCAL);
    } else {
      ACE_TCHAR buff[256];
      multicast_address_.addr_to_string(buff, 256);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::join_multicast_group() - ")
                 ACE_TEXT("failed to join multicast group %s on %C: %p\n"),
                 buff,
                 all_interfaces ? "all interfaces" : nic.name().c_str(),
                 ACE_TEXT("ACE_SOCK_Dgram_Mcast::join")));
    }
  }

#ifdef ACE_HAS_IPV6
  if (joined_ipv6_interfaces_.count(nic.name()) == 0 && nic.has_ipv6()) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_TCHAR buff[256];
      multicast_ipv6_address_.addr_to_string(buff, 256);
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Spdp::SpdpTransport::join_multicast_group ")
                 ACE_TEXT("joining group %s on %C\n"),
                 buff,
                 all_interfaces ? "all interfaces" : nic.name().c_str()));
    }

    if (0 == multicast_ipv6_socket_.join(multicast_ipv6_address_, 1, all_interfaces ? 0 : ACE_TEXT_CHAR_TO_TCHAR(nic.name().c_str()))) {
      joined_ipv6_interfaces_.insert(nic.name());

      if (reactor()->register_handler(multicast_ipv6_socket_.get_handle(),
                                    this, ACE_Event_Handler::READ_MASK) != 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Spdp::SpdpTransport::join_multicast_group failed to register multicast ipv6 input handler\n")));
        return;
      }

      write_i(SEND_TO_LOCAL);
    } else {
      ACE_TCHAR buff[256];
      multicast_ipv6_address_.addr_to_string(buff, 256);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::join_multicast_group() - ")
                 ACE_TEXT("failed to join multicast group %s on %C: %p\n"),
                 buff,
                 all_interfaces ? "all interfaces" : nic.name().c_str(),
                 ACE_TEXT("ACE_SOCK_Dgram_Mcast::join")));
    }
  }
#endif
}

void
Spdp::SpdpTransport::leave_multicast_group(const DCPS::NetworkInterface& nic)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);

  if (joined_interfaces_.count(nic.name()) != 0 && !nic.has_ipv4()) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_TCHAR buff[256];
      multicast_address_.addr_to_string(buff, 256);
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Spdp::SpdpTransport::leave_multicast_group ")
                 ACE_TEXT("leaving group %s on %C\n"),
                 buff,
                 nic.name().c_str()));
    }

    if (0 != multicast_socket_.leave(multicast_address_, ACE_TEXT_CHAR_TO_TCHAR(nic.name().c_str()))) {
      ACE_TCHAR buff[256];
      multicast_address_.addr_to_string(buff, 256);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::leave_multicast_group() - ")
                 ACE_TEXT("failed to leave multicast group %s on %C: %p\n"),
                 buff,
                 nic.name().c_str(),
                 ACE_TEXT("ACE_SOCK_Dgram_Mcast::leave")));
    }
    joined_interfaces_.erase(nic.name());
  }

#ifdef ACE_HAS_IPV6
  if (joined_ipv6_interfaces_.count(nic.name()) != 0 && !nic.has_ipv6()) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_TCHAR buff[256];
      multicast_ipv6_address_.addr_to_string(buff, 256);
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Spdp::SpdpTransport::leave_multicast_group ")
                 ACE_TEXT("leaving group %s on %C\n"),
                 buff,
                 nic.name().c_str()));
    }

    if (0 != multicast_ipv6_socket_.leave(multicast_ipv6_address_, ACE_TEXT_CHAR_TO_TCHAR(nic.name().c_str()))) {
      ACE_TCHAR buff[256];
      multicast_ipv6_address_.addr_to_string(buff, 256);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::leave_multicast_group() - ")
                 ACE_TEXT("failed to leave multicast ipv6 group %s on %C: %p\n"),
                 buff,
                 nic.name().c_str(),
                 ACE_TEXT("ACE_SOCK_Dgram_Mcast::leave")));
    }
    joined_ipv6_interfaces_.erase(nic.name());
  }
#endif
}

void
Spdp::SpdpTransport::add_address(const DCPS::NetworkInterface& nic,
                                 const ACE_INET_Addr&)
{
  job_queue_->enqueue(DCPS::make_rch<ChangeMulticastGroup>(rchandle_from(this), nic,
                                                           ChangeMulticastGroup::CMG_JOIN));
}

void Spdp::SpdpTransport::remove_address(const DCPS::NetworkInterface& nic,
                                         const ACE_INET_Addr&)
{
  job_queue_->enqueue(DCPS::make_rch<ChangeMulticastGroup>(rchandle_from(this), nic,
                                                           ChangeMulticastGroup::CMG_LEAVE));
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
  const DCPS::RepoId peer = make_id(id, ENTITYID_PARTICIPANT);
  const DiscoveredParticipantConstIter iter = participants_.find(peer);
  if (iter == participants_.end()) {
    const DCPS::GuidConverter conv(peer);
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::send_participant_crypto_tokens() - ")
               ACE_TEXT("Discovered participant %C not found.\n"), OPENDDS_STRING(conv).c_str()));
    return;
  }

  const DDS::Security::ParticipantCryptoTokenSeq& pcts = iter->second.crypto_tokens_;

  if (pcts.length() != 0) {
    DCPS::RepoId writer = guid_;
    writer.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;

    DCPS::RepoId reader = peer;
    reader.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;

    DDS::Security::ParticipantVolatileMessageSecure msg;
    msg.message_identity.source_guid = writer;
    msg.message_class_id = DDS::Security::GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS;
    msg.destination_participant_guid = peer;
    msg.destination_endpoint_guid = GUID_UNKNOWN; // unknown = whole participant
    msg.source_endpoint_guid = GUID_UNKNOWN;
    msg.message_data = reinterpret_cast<const DDS::Security::DataHolderSeq&>(pcts);

    if (sedp_.write_volatile_message(msg, reader) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::send_participant_crypto_tokens() - ")
        ACE_TEXT("Unable to write volatile message.\n")));
    }
  }
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
  ACE_Guard<ACE_Thread_Mutex> g(lock_, false);
  DiscoveredParticipantConstIter pi = participants_.find(id);
  if (pi != participants_.end()) {
    return pi->second.auth_state_;
  }
  return DCPS::AUTH_STATE_HANDSHAKE;
}
#endif

#ifndef OPENDDS_SAFETY_PROFILE
bool
operator==(const DCPS::Locator_t& x, const DCPS::Locator_t& y)
{
  return x.kind == y.kind && x.port == y.port && x.address == y.address;
}

bool
operator!=(const DCPS::Locator_t& x, const DCPS::Locator_t& y)
{
  return x.kind != y.kind && x.port != y.port && x.address != y.address;
}
#endif

#ifdef OPENDDS_SECURITY
void Spdp::start_ice(ICE::Endpoint* endpoint, RepoId r, const BuiltinEndpointSet_t& avail, const ICE::AgentInfo& agent_info) {
  RepoId l = guid_;

  // See RTPS v2.1 section 8.5.5.1
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }

  using namespace DDS::Security;
  // See DDS-Security v1.1 section 7.3.7.1
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) {
    l.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    r.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) {
    l.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    r.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    ICE::Agent::instance()->start_ice(endpoint, l, r, agent_info);
  }
}

void Spdp::stop_ice(ICE::Endpoint* endpoint, DCPS::RepoId r, const BuiltinEndpointSet_t& avail) {
  RepoId l = guid_;

  // See RTPS v2.1 section 8.5.5.1
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }

  using namespace DDS::Security;
  // See DDS-Security v1.1 section 7.3.7.1
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) {
    l.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    r.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) {
    l.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    r.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    ICE::Agent::instance()->stop_ice(endpoint, l, r);
  }
}

DDS::Security::ParticipantCryptoHandle
Spdp::remote_crypto_handle(const DCPS::RepoId& remote_participant) const
{
  DiscoveredParticipantMap::const_iterator pos = participants_.find(remote_participant);
  if (pos != participants_.end()) {
    return pos->second.crypto_handle_;
  }
  return DDS::HANDLE_NIL;
}
#endif

void Spdp::SpdpTransport::send_relay_beacon(const MonotonicTimePoint& /*now*/)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);

  if (outer_->config_->spdp_rtps_relay_address() == ACE_INET_Addr()) {
    return;
  }

  static const PadSubmessage pad = { { PAD, FLAG_E, 0 } };

  wbuff_.reset();
  DCPS::Serializer ser(&wbuff_, encoding_plain_native);
  if (!(ser << hdr_) || !(ser << pad)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::send_relay_beacon() - ")
               ACE_TEXT("failed to serialize headers for SPDP\n")));
    return;
  }

  send(SEND_TO_RELAY);
}

void Spdp::SpdpTransport::send_relay(const DCPS::MonotonicTimePoint& /*now*/)
{
  if (outer_->config_->spdp_rtps_relay_address() == ACE_INET_Addr()) {
    return;
  }

  write(SEND_TO_RELAY);
}

void Spdp::SpdpTransport::send_local(const DCPS::MonotonicTimePoint& /*now*/)
{
  write(SEND_TO_LOCAL);
  outer_->remove_expired_participants();
}

#ifdef OPENDDS_SECURITY
void Spdp::SpdpTransport::process_handshake_deadlines(const DCPS::MonotonicTimePoint& now)
{
  outer_->process_handshake_deadlines(now);
}

void Spdp::SpdpTransport::process_handshake_resends(const DCPS::MonotonicTimePoint& now)
{
  outer_->process_handshake_resends(now);
}

void Spdp::purge_handshake_deadlines(DiscoveredParticipantIter iter)
{
  if (iter == participants_.end()) {
    return;
  }

  purge_handshake_resends(iter);

  std::pair<TimeQueue::iterator, TimeQueue::iterator> range = handshake_deadlines_.equal_range(iter->second.handshake_deadline_);
  for (; range.first != range.second; ++range.first) {
    if (range.first->second == iter->first) {
      handshake_deadlines_.erase(range.first);
      break;
    }
  }
}

void Spdp::purge_handshake_resends(DiscoveredParticipantIter iter)
{
  if (iter == participants_.end()) {
    return;
  }

  iter->second.have_auth_req_msg_ = false;
  iter->second.have_handshake_msg_ = false;

  std::pair<TimeQueue::iterator, TimeQueue::iterator> range = handshake_resends_.equal_range(iter->second.stateless_msg_deadline_);
  for (; range.first != range.second; ++range.first) {
    if (range.first->second == iter->first) {
      handshake_resends_.erase(range.first);
      break;
    }
  }
}

void Spdp::process_participant_ice(const ParameterList& plist,
                                   const ParticipantData_t& pdata,
                                   const DCPS::RepoId& guid)
{
  ICE::AgentInfoMap ai_map;
  if (!ParameterListConverter::from_param_list(plist, ai_map)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::data_received - ")
               ACE_TEXT("failed to convert from ParameterList to ")
               ACE_TEXT("ICE::AgentInfo\n")));
    return;
  }

  ICE::Endpoint* sedp_endpoint = sedp_.get_ice_endpoint();
  if (sedp_endpoint) {
    ICE::AgentInfoMap::const_iterator sedp_pos = ai_map.find("SEDP");
    if (sedp_pos != ai_map.end()) {
      start_ice(sedp_endpoint, guid, pdata.participantProxy.availableBuiltinEndpoints, sedp_pos->second);
    } else {
      stop_ice(sedp_endpoint, guid, pdata.participantProxy.availableBuiltinEndpoints);
    }
  }
  ICE::Endpoint* spdp_endpoint = tport_->get_ice_endpoint();
  if (spdp_endpoint) {
    ICE::AgentInfoMap::const_iterator spdp_pos = ai_map.find("SPDP");
    if (spdp_pos != ai_map.end()) {
      ICE::Agent::instance()->start_ice(spdp_endpoint, guid_, guid, spdp_pos->second);
    } else {
      ICE::Agent::instance()->stop_ice(spdp_endpoint, guid_, guid);
#ifndef DDS_HAS_MINIMUM_BIT
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      DiscoveredParticipantIter iter = participants_.find(guid);
      if (iter != participants_.end()) {
        enqueue_location_update_i(iter, DCPS::LOCATION_ICE, ACE_INET_Addr());
        process_location_updates_i(iter);
      }
#endif
    }
  }
}

bool Spdp::remote_is_requester(const DCPS::RepoId& guid) const
{
  DiscoveredParticipantConstIter iter = participants_.find(make_id(guid, DCPS::ENTITYID_PARTICIPANT));
  if (iter != participants_.end()) {
    return iter->second.is_requester_;
  }
  return false;
}

const ParticipantData_t& Spdp::get_participant_data(const DCPS::RepoId& guid) const
{
  DiscoveredParticipantConstIter iter = participants_.find(make_id(guid, DCPS::ENTITYID_PARTICIPANT));
  return iter->second.pdata_;
}

#endif

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
