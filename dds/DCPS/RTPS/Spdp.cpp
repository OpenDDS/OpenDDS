/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Spdp.h"

#include "MessageUtils.h"
#include "MessageTypes.h"
#include "ParameterListConverter.h"
#include "RtpsDiscovery.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/Ice.h>
#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Logging.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/ConnectionRecords.h>
#include <dds/DCPS/transport/framework/TransportDebug.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/framework/SecurityRegistry.h>
#endif

#include <dds/DdsDcpsGuidC.h>

#include <ace/Reactor.h>
#include <ace/OS_NS_sys_socket.h> // For setsockopt()
#include <ace/OS_NS_strings.h>

#include <cstring>
#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace RTPS {
using DCPS::GUID_t;
using DCPS::MonotonicTimePoint;
using DCPS::TimeDuration;
using DCPS::Serializer;
using DCPS::Encoding;
using DCPS::ENDIAN_BIG;
using DCPS::ENDIAN_LITTLE;
using DCPS::LogLevel;
using DCPS::log_level;
using DCPS::LogAddr;

namespace {
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

#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantLocation compute_location_mask(const DCPS::NetworkAddress& address, bool from_relay)
  {
    if (address.get_type() == AF_INET6) {
      return from_relay ? DCPS::LOCATION_RELAY6 : DCPS::LOCATION_LOCAL6;
    }
    return from_relay ? DCPS::LOCATION_RELAY : DCPS::LOCATION_LOCAL;
  }
#endif

#ifdef OPENDDS_SECURITY

#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantLocation compute_ice_location_mask(const DCPS::NetworkAddress& address)
  {
    if (address.get_type() == AF_INET6) {
      return DCPS::LOCATION_ICE6;
    }
    return DCPS::LOCATION_ICE;
  }
#endif

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

  inline bool prop_to_bool(const DDS::Property_t& prop)
  {
    const char* const value = prop.value.in();
    return std::strcmp(value, "0") && ACE_OS::strcasecmp(value, "false");
  }
}

void Spdp::init(DDS::DomainId_t /*domain*/,
                DCPS::GUID_t& guid,
                const DDS::DomainParticipantQos& qos,
                XTypes::TypeLookupService_rch tls)
{
  type_lookup_service_ = tls;

  bool enable_endpoint_announcements = true;
  bool enable_type_lookup_service = config_->use_xtypes();

  const DDS::PropertySeq& properties = qos.property.value;
  for (unsigned int idx = 0; idx != properties.length(); ++idx) {
    const DDS::Property_t& prop = properties[idx];
    if (std::strcmp(RTPS_DISCOVERY_ENDPOINT_ANNOUNCEMENTS, prop.name.in()) == 0) {
      enable_endpoint_announcements = prop_to_bool(prop);
    } else if (std::strcmp(RTPS_DISCOVERY_TYPE_LOOKUP_SERVICE, prop.name.in()) == 0) {
      enable_type_lookup_service = prop_to_bool(prop);
    } else if (std::strcmp(RTPS_RELAY_APPLICATION_PARTICIPANT, prop.name.in()) == 0) {
      is_application_participant_ = prop_to_bool(prop);
    } else if (std::strcmp(RTPS_REFLECT_HEARTBEAT_COUNT, prop.name.in()) == 0) {
      const CORBA::ULong old_flags = config_->participant_flags();
      const CORBA::ULong new_flags = prop_to_bool(prop) ? (old_flags | PFLAGS_REFLECT_HEARTBEAT_COUNT) : (old_flags & ~PFLAGS_REFLECT_HEARTBEAT_COUNT);
      config_->participant_flags(new_flags);
    }
  }

  available_builtin_endpoints_ =
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER;

  if (enable_endpoint_announcements) {
    available_builtin_endpoints_ |=
      DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
      DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER |
      BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER;
  }

  if (enable_type_lookup_service) {
    available_builtin_endpoints_ |=
      BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_READER |
      BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER |
      BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_WRITER |
      BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_WRITER;
  }

#ifdef OPENDDS_SECURITY
  if (is_security_enabled()) {
    using namespace DDS::Security;

    available_builtin_endpoints_ |=
      SEDP_BUILTIN_PUBLICATIONS_SECURE_READER |
      SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER |
      BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER |
      BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER |
      BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER |
      BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER |
      BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER |
      SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER |
      SPDP_BUILTIN_PARTICIPANT_SECURE_READER;

    if (enable_endpoint_announcements) {
      available_builtin_endpoints_ |=
        SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER |
        SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER |
        BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    }

    if (enable_type_lookup_service) {
      available_extended_builtin_endpoints_ =
        TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE |
        TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE |
        TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE |
        TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE;
    }
  }
#endif

  guid = guid_; // may have changed in SpdpTransport constructor
  sedp_->ignore(guid);
}

Spdp::Spdp(DDS::DomainId_t domain,
           GUID_t& guid,
           const DDS::DomainParticipantQos& qos,
           RtpsDiscovery* disco,
           XTypes::TypeLookupService_rch tls)
  : qos_(qos)
  , disco_(disco)
  , config_(disco_->config())
  , participant_flags_(disco->config()->participant_flags())
  , resend_period_(disco->config()->resend_period())
  , quick_resend_ratio_(disco_->config()->quick_resend_ratio())
  , min_resend_delay_(disco_->config()->min_resend_delay())
  , lease_duration_(disco_->config()->lease_duration())
  , lease_extension_(disco_->config()->lease_extension())
  , max_lease_duration_(disco_->config()->max_lease_duration())
  , max_spdp_sequence_msg_reset_check_(disco->config()->max_spdp_sequence_msg_reset_check())
  , check_source_ip_(disco->config()->check_source_ip())
  , undirected_spdp_(disco->config()->undirected_spdp())
#ifdef OPENDDS_SECURITY
  , max_participants_in_authentication_(disco->config()->max_participants_in_authentication())
  , security_unsecure_lease_duration_(disco->config()->security_unsecure_lease_duration())
  , auth_resend_period_(disco->config()->auth_resend_period())
  , max_auth_time_(disco->config()->max_auth_time())
  , secure_participant_user_data_(disco->config()->secure_participant_user_data())
#endif
  , domain_(domain)
  , guid_(guid)
  , participant_discovered_at_(MonotonicTimePoint::now().to_monotonic_time())
  , is_application_participant_(false)
  , tport_(DCPS::make_rch<SpdpTransport>(rchandle_from(this)))
  , initialized_flag_(false)
  , eh_shutdown_(false)
  , shutdown_cond_(lock_)
  , shutdown_flag_(false)
  , available_builtin_endpoints_(0)
  , sedp_(DCPS::make_rch<Sedp>(guid_, DCPS::ref(*this), DCPS::ref(lock_)))
#ifdef OPENDDS_SECURITY
  , available_extended_builtin_endpoints_(0)
  , security_config_()
  , security_enabled_(false)
  , identity_handle_(DDS::HANDLE_NIL)
  , permissions_handle_(DDS::HANDLE_NIL)
  , crypto_handle_(DDS::HANDLE_NIL)
  , ice_agent_(ICE::Agent::instance())
  , n_participants_in_authentication_(0)
#endif
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  init(domain, guid, qos, tls);

#ifdef OPENDDS_SECURITY
  init_participant_sec_attributes(participant_sec_attr_);
#endif

}

#ifdef OPENDDS_SECURITY
Spdp::Spdp(DDS::DomainId_t domain,
           const DCPS::GUID_t& guid,
           const DDS::DomainParticipantQos& qos,
           RtpsDiscovery* disco,
           XTypes::TypeLookupService_rch tls,
           DDS::Security::IdentityHandle identity_handle,
           DDS::Security::PermissionsHandle perm_handle,
           DDS::Security::ParticipantCryptoHandle crypto_handle)
  : qos_(qos)
  , disco_(disco)
  , config_(disco_->config())
  , participant_flags_(disco->config()->participant_flags())
  , resend_period_(disco->config()->resend_period())
  , quick_resend_ratio_(disco_->config()->quick_resend_ratio())
  , min_resend_delay_(disco_->config()->min_resend_delay())
  , lease_duration_(disco_->config()->lease_duration())
  , lease_extension_(disco_->config()->lease_extension())
  , max_lease_duration_(disco_->config()->max_lease_duration())
  , max_spdp_sequence_msg_reset_check_(disco->config()->max_spdp_sequence_msg_reset_check())
  , check_source_ip_(disco->config()->check_source_ip())
  , undirected_spdp_(disco->config()->undirected_spdp())
  , max_participants_in_authentication_(disco->config()->max_participants_in_authentication())
  , security_unsecure_lease_duration_(disco->config()->security_unsecure_lease_duration())
  , auth_resend_period_(disco->config()->auth_resend_period())
  , max_auth_time_(disco->config()->max_auth_time())
  , secure_participant_user_data_(disco->config()->secure_participant_user_data())
  , domain_(domain)
  , guid_(guid)
  , participant_discovered_at_(MonotonicTimePoint::now().to_monotonic_time())
  , is_application_participant_(false)
  , tport_(DCPS::make_rch<SpdpTransport>(rchandle_from(this)))
  , initialized_flag_(false)
  , eh_shutdown_(false)
  , shutdown_cond_(lock_)
  , shutdown_flag_(false)
  , available_builtin_endpoints_(0)
  , sedp_(DCPS::make_rch<Sedp>(guid_, DCPS::ref(*this), DCPS::ref(lock_)))
  , available_extended_builtin_endpoints_(0)
  , security_config_(Security::SecurityRegistry::instance()->default_config())
  , security_enabled_(security_config_->get_authentication() && security_config_->get_access_control() &&
    security_config_->get_crypto_key_factory() && security_config_->get_crypto_key_exchange())
  , identity_handle_(identity_handle)
  , permissions_handle_(perm_handle)
  , crypto_handle_(crypto_handle)
  , ice_agent_(ICE::Agent::instance())
  , n_participants_in_authentication_(0)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  init(domain, guid_, qos, tls);

  DDS::Security::Authentication_var auth = security_config_->get_authentication();
  DDS::Security::AccessControl_var access = security_config_->get_access_control();

  DDS::Security::SecurityException se = {"", 0, 0};

  if (!auth->get_identity_token(identity_token_, identity_handle_, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to get identity token. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to get identity token");
  }
  if (!auth->get_identity_status_token(identity_status_token_, identity_handle_, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to get identity status token. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to get identity status token");
  }
  if (!access->get_permissions_token(permissions_token_, permissions_handle_, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to get permissions handle. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to get permissions token");
  }
  if (!access->get_permissions_credential_token(permissions_credential_token_, permissions_handle_, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to get permissions credential handle. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to get permissions credential token");
  }

  if (!auth->set_permissions_credential_and_token(identity_handle_, permissions_credential_token_, permissions_token_, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("unable to set permissions credential and token. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to set permissions credential and token");
  }

  init_participant_sec_attributes(participant_sec_attr_);

  if (!access->get_participant_sec_attributes(permissions_handle_, participant_sec_attr_, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::Spdp() - ")
      ACE_TEXT("failed to retrieve participant security attributes. Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    throw std::runtime_error("unable to retrieve participant security attributes");
  }

  sedp_->init_security(identity_handle, perm_handle, crypto_handle);
}
#endif

void
Spdp::shutdown()
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    shutdown_flag_ = true;
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

    for (DiscoveredParticipantIter part = participants_.begin(); part != participants_.end(); ++part) {
#ifdef OPENDDS_SECURITY
      DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = sedp_->get_ice_endpoint();
      if (sedp_endpoint) {
        stop_ice(sedp_endpoint, part->first, part->second.pdata_.participantProxy.availableBuiltinEndpoints,
                 part->second.pdata_.participantProxy.availableExtendedBuiltinEndpoints);
      }
      DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint = tport_->get_ice_endpoint();
      if (spdp_endpoint) {
        ice_agent_->stop_ice(spdp_endpoint, guid_, part->first);
      }
      purge_handshake_deadlines(part);
#endif
      purge_discovered_participant(part);
    }
  }

#ifdef OPENDDS_SECURITY
  DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = sedp_->get_ice_endpoint();
  if (sedp_endpoint) {
    const GUID_t l = make_id(guid_, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER);
    ice_agent_->remove_local_agent_info_listener(sedp_endpoint, l);
  }
#endif

  // Clean up.
  tport_->close(sedp_->reactor_task());

  // Reactor task and job queue are gone.
  sedp_->shutdown();

  // release lock for reset of event handler, which may delete transport
  tport_.reset();
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    DCPS::ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
    while (!eh_shutdown_) {
      shutdown_cond_.wait(thread_status_manager);
    }
  }
}

Spdp::~Spdp()
{
}

#ifdef OPENDDS_SECURITY
void
Spdp::write_secure_updates()
{
  if (!initialized_flag_ || shutdown_flag_) {
    return;
  }

  const Security::SPDPdiscoveredParticipantData pdata =
    build_local_pdata(false, Security::DPDK_SECURE);

  sedp_->write_dcps_participant_secure(pdata, GUID_UNKNOWN);
}

void
Spdp::write_secure_disposes()
{
  sedp_->write_dcps_participant_dispose(guid_);
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
                                const DCPS::NetworkAddress& from,
                                const char* reason)
{
  // We have the global lock.
  iter->second.location_updates_.push_back(DiscoveredParticipant::LocationUpdate(mask, from, DCPS::SystemTimePoint::now()));

  if (DCPS::log_bits) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Spdp::enqueue_location_update_i: %@ for %C size=%B reason=%C\n", this, LogGuid(iter->first).c_str(), iter->second.location_updates_.size(), reason));
  }

}

void Spdp::process_location_updates_i(const DiscoveredParticipantIter& iter, const char* reason, bool force_publish)
{
  // We have the global lock.

  if (iter == participants_.end()) {
    if (DCPS::log_bits) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Spdp::process_location_updates_i: %@ iterator invalid, returning\n", this));
    }
    return;
  }

  if (iter->second.bit_ih_ == DDS::HANDLE_NIL) {
    // Do not process updates until the participant exists in the built-in topics.
    if (DCPS::log_bits) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Spdp::process_location_updates_i: %@ %C does not exist in participant bit, returning\n", this, LogGuid(iter->first).c_str()));
    }
    return;
  }

  DiscoveredParticipant::LocationUpdateList location_updates;
  std::swap(iter->second.location_updates_, location_updates);

  if (DCPS::log_bits) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Spdp::process_location_updates_i: %@ %C has %B location update(s) force_publish=%d reason=%C\n", this, LogGuid(iter->first).c_str(), location_updates.size(), force_publish, reason));
  }

  const DCPS::TimeDuration leaseDuration = rtps_duration_to_time_duration(iter->second.pdata_.leaseDuration,
                                             iter->second.pdata_.participantProxy.protocolVersion,
                                             iter->second.pdata_.participantProxy.vendorId);

  DCPS::ParticipantLocationBuiltinTopicData& location_data = iter->second.location_data_;
  location_data.lease_duration = leaseDuration.to_dds_duration();

  bool published = false;
  for (DiscoveredParticipant::LocationUpdateList::const_iterator pos = location_updates.begin(),
         limit = location_updates.end(); pos != limit; ++pos) {

    OPENDDS_STRING addr = "";
    const DCPS::ParticipantLocation old_mask = location_data.location;
    if (pos->from_) {
      location_data.location |= pos->mask_;
      addr = DCPS::LogAddr(pos->from_).str();
    } else {
      location_data.location &= ~(pos->mask_);
    }

    location_data.change_mask = pos->mask_;

    bool address_change = false;
    switch (pos->mask_) {
    case DCPS::LOCATION_LOCAL:
      address_change = addr.compare(location_data.local_addr.in()) != 0;
      location_data.local_addr = addr.c_str();
      location_data.local_timestamp = pos->timestamp_.to_dds_time();
      break;
    case DCPS::LOCATION_ICE:
      address_change = addr.compare(location_data.ice_addr.in()) != 0;
      location_data.ice_addr = addr.c_str();
      location_data.ice_timestamp = pos->timestamp_.to_dds_time();
      break;
    case DCPS::LOCATION_RELAY:
      address_change = addr.compare(location_data.relay_addr.in()) != 0;
      location_data.relay_addr = addr.c_str();
      location_data.relay_timestamp = pos->timestamp_.to_dds_time();
      break;
    case DCPS::LOCATION_LOCAL6:
      address_change = addr.compare(location_data.local6_addr.in()) != 0;
      location_data.local6_addr = addr.c_str();
      location_data.local6_timestamp = pos->timestamp_.to_dds_time();
      break;
    case DCPS::LOCATION_ICE6:
      address_change = addr.compare(location_data.ice6_addr.in()) != 0;
      location_data.ice6_addr = addr.c_str();
      location_data.ice6_timestamp = pos->timestamp_.to_dds_time();
      break;
    case DCPS::LOCATION_RELAY6:
      address_change = addr.compare(location_data.relay6_addr.in()) != 0;
      location_data.relay6_addr = addr.c_str();
      location_data.relay6_timestamp = pos->timestamp_.to_dds_time();
      break;
    }

    const DDS::Time_t expr = (pos->timestamp_ - leaseDuration).to_dds_time();

    if ((location_data.location & DCPS::LOCATION_LOCAL) && DCPS::operator<(location_data.local_timestamp, expr)) {
      location_data.location &= ~(DCPS::LOCATION_LOCAL);
      location_data.change_mask |= DCPS::LOCATION_LOCAL;
      location_data.local_timestamp = pos->timestamp_.to_dds_time();
    }
    if ((location_data.location & DCPS::LOCATION_RELAY) && DCPS::operator<(location_data.relay_timestamp, expr)) {
      location_data.location &= ~(DCPS::LOCATION_RELAY);
      location_data.change_mask |= DCPS::LOCATION_RELAY;
      location_data.relay_timestamp = pos->timestamp_.to_dds_time();
    }
    if ((location_data.location & DCPS::LOCATION_LOCAL6) && DCPS::operator<(location_data.local6_timestamp, expr)) {
      location_data.location &= ~(DCPS::LOCATION_LOCAL6);
      location_data.change_mask |= DCPS::LOCATION_LOCAL6;
      location_data.local6_timestamp = pos->timestamp_.to_dds_time();
    }
    if ((location_data.location & DCPS::LOCATION_RELAY6) && DCPS::operator<(location_data.relay6_timestamp, expr)) {
      location_data.location &= ~(DCPS::LOCATION_RELAY6);
      location_data.change_mask |= DCPS::LOCATION_RELAY6;
      location_data.relay6_timestamp = pos->timestamp_.to_dds_time();
    }

    if (old_mask != location_data.location || address_change) {
      if (DCPS::log_bits) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Spdp::process_location_updates_i: %@ publishing %C update\n", this, LogGuid(iter->first).c_str()));
      }
      publish_location_update_i(iter);
      published = true;
    }
  }

  if (force_publish && !published) {
    if (DCPS::log_bits) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Spdp::process_location_updates_i: %@ publishing %C forced\n", this, LogGuid(iter->first).c_str()));
    }
    publish_location_update_i(iter);
    published = true;
  }

  if (!published && DCPS::log_bits) {
    if (DCPS::log_bits) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Spdp::process_location_updates_i: %@ not published\n", this, LogGuid(iter->first).c_str()));
    }
  }
}

void
Spdp::publish_location_update_i(const DiscoveredParticipantIter& iter)
{
  iter->second.location_ih_ = bit_subscriber_->add_participant_location(iter->second.location_data_, DDS::NEW_VIEW_STATE);
  if (DCPS::log_bits) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: Spdp::publish_location_update_i: %@ participant %C has participant location handle %d\n", this, LogGuid(iter->first).c_str(), iter->second.location_ih_));
  }
}
#endif

DCPS::WeakRcHandle<ICE::Endpoint>
Spdp::get_ice_endpoint_if_added()
{
  return tport_->ice_endpoint_added_ ? tport_->get_ice_endpoint() : DCPS::WeakRcHandle<ICE::Endpoint>();
}

bool cmp_ip4(const ACE_INET_Addr& a, const DCPS::Locator_t& locator)
{
  struct sockaddr_in* sa = static_cast<struct sockaddr_in*>(a.get_addr());
  if (sa->sin_family == AF_INET && locator.kind == LOCATOR_KIND_UDPv4) {
    const unsigned char* ip = reinterpret_cast<const unsigned char*>(&sa->sin_addr);
    const unsigned char* la = reinterpret_cast<const unsigned char*>(locator.address) + 12;
    return ACE_OS::memcmp(ip, la, 4) == 0;
  }
  return false;
}

#if defined (ACE_HAS_IPV6)
bool cmp_ip6(const ACE_INET_Addr& a, const DCPS::Locator_t& locator)
{
  struct sockaddr_in6* in6 = static_cast<struct sockaddr_in6*>(a.get_addr());
  if (in6->sin6_family == AF_INET6 && locator.kind == LOCATOR_KIND_UDPv6) {
    const unsigned char* ip = reinterpret_cast<const unsigned char*>(&in6->sin6_addr);
    const unsigned char* la = reinterpret_cast<const unsigned char*>(locator.address);
    return ACE_OS::memcmp(ip, la, 16) == 0;
  }
  return false;
}
#endif // ACE_HAS_IPV6

bool is_ip_equal(const ACE_INET_Addr& a, const DCPS::Locator_t& locator)
{
#ifdef ACE_HAS_IPV6
  if (a.get_type() == AF_INET6) {
    return cmp_ip6(a, locator);
  }
#endif
  return cmp_ip4(a, locator);
}

void print_locator(const CORBA::ULong i, const DCPS::Locator_t& o){
  const unsigned char* a = reinterpret_cast<const unsigned char*>(o.address);
  ACE_INET_Addr addr;
  bool b = locator_to_address(addr, o, false) == 0;
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("locator%d(kind:%d)[%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d] locator_to_address:%C\n"),
    i, o.kind, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15],
    (b ? DCPS::LogAddr(addr).c_str() : "failed")));
}

bool ip_in_locator_list(const DCPS::NetworkAddress& from, const DCPS::LocatorSeq& locators)
{
  if (DCPS::DCPS_debug_level >= 8) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ip_in_locator_list - from (type %d): %C\n"), from.get_type(), DCPS::LogAddr(from).c_str()));
  }
  for (CORBA::ULong i = 0; i < locators.length(); ++i) {
    if (DCPS::DCPS_debug_level >= 8) {
      print_locator(i, locators[i]);
    }
    if (is_ip_equal(from.to_addr(), locators[i])) {
      return true;
    }
  }
  return false;
}

#ifdef OPENDDS_SECURITY
bool ip_in_AgentInfo(const DCPS::NetworkAddress& from, const ParameterList& plist)
{
  bool found = false;
  ICE::AgentInfoMap ai_map;
  if (!ParameterListConverter::from_param_list(plist, ai_map)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ip_in_AgentInfo - failed to convert ParameterList to AgentInfoMap\n")));
    return found;
  }
  ICE::AgentInfoMap::const_iterator sedp_i = ai_map.find(SEDP_AGENT_INFO_KEY);
  if (sedp_i != ai_map.end()) {
    const ICE::AgentInfo::CandidatesType& cs = sedp_i->second.candidates;
    for (ICE::AgentInfo::const_iterator i = cs.begin(); i != cs.end(); ++i) {
      if (from.to_addr().is_ip_equal(i->address)) {
        found = true;
        break;
      }
    }
  }
  ICE::AgentInfoMap::const_iterator spdp_i = ai_map.find(SPDP_AGENT_INFO_KEY);
  if (!found && spdp_i != ai_map.end()) {
    const ICE::AgentInfo::CandidatesType& cs = spdp_i->second.candidates;
    for (ICE::AgentInfo::const_iterator i = cs.begin(); i != cs.end(); ++i) {
      if (from.to_addr().is_ip_equal(i->address)) {
        found = true;
        break;
      }
    }
  }
  return found;
}
#endif

void
Spdp::handle_participant_data(DCPS::MessageId id,
                              const ParticipantData_t& cpdata,
                              const DCPS::MonotonicTimePoint& now,
                              const DCPS::SequenceNumber& seq,
                              const DCPS::NetworkAddress& from,
                              bool from_sedp)
{
  // Make a (non-const) copy so we can tweak values below
  ParticipantData_t pdata(cpdata);

  const GUID_t guid = DCPS::make_part_guid(pdata.participantProxy.guidPrefix);

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (!initialized_flag_ || shutdown_flag_) {
    return;
  }

  if (sedp_->ignoring(guid)) {
    // Ignore, this is our domain participant or one that the user has
    // asked us to ignore.
    return;
  }

  const bool from_relay = sedp_->core().from_relay(from);

#ifndef DDS_HAS_MINIMUM_BIT
  const DCPS::ParticipantLocation location_mask = compute_location_mask(from, from_relay);
#endif

  // Don't trust SPDP for the RtpsRelay application participant.
  // Otherwise, anyone can reset the application participant.
#ifdef OPENDDS_SECURITY
  if (is_security_enabled() && !from_sedp) {
    pdata.participantProxy.opendds_rtps_relay_application_participant = false;
  }
#endif

  // Find the participant - iterator valid only as long as we hold the lock
  DiscoveredParticipantIter iter = participants_.find(guid);

  if (iter == participants_.end()) {
    // Trying to delete something that doesn't exist is a NOOP
    if (id == DCPS::DISPOSE_INSTANCE || id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
      return;
    }

#ifdef OPENDDS_SECURITY
    if (max_participants_in_authentication_ &&
        n_participants_in_authentication_ >= max_participants_in_authentication_) {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG,
                   "(%P|%t) {auth_debug} DEBUG: Spdp::handle_participant_data - participants_in_authentication: %B >= max: %B\n",
                   n_participants_in_authentication_, max_participants_in_authentication_));
      }
      return;
    }
#endif

    partBitData(pdata).key = guid_to_bit_key(guid);

    TimeDuration effective_lease(pdata.leaseDuration.seconds);

    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Spdp::handle_participant_data - %C discovered %C lease %C from %C (%B)\n"),
        DCPS::LogGuid(guid_).c_str(), DCPS::LogGuid(guid).c_str(),
        effective_lease.str(0).c_str(), DCPS::LogAddr(from).c_str(),
        participants_.size()));
    }

    if (!from_sedp) {
#ifdef OPENDDS_SECURITY
      if (is_security_enabled()) {
        effective_lease = security_unsecure_lease_duration_;
      } else {
#endif
        const TimeDuration maxLeaseDuration = max_lease_duration_;
        if (maxLeaseDuration && effective_lease > maxLeaseDuration) {
          if (DCPS::DCPS_debug_level >= 2) {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Spdp::handle_participant_data - overwriting %C lease %C from %C with %C\n"),
              DCPS::LogGuid(guid).c_str(), effective_lease.str(0).c_str(),
              DCPS::LogAddr(from).c_str(), maxLeaseDuration.str(0).c_str()));
          }
          effective_lease = maxLeaseDuration;
        }
#ifdef OPENDDS_SECURITY
      }
#endif
    }

    pdata.leaseDuration.seconds = static_cast<ACE_CDR::Long>(effective_lease.value().sec());

    if (tport_->directed_send_task_) {
      if (tport_->directed_guids_.empty()) {
        tport_->directed_send_task_->schedule(TimeDuration::zero_value);
      }
      tport_->directed_guids_.push_back(guid);
    }

    // add a new participant

#ifdef OPENDDS_SECURITY
    std::pair<DiscoveredParticipantIter, bool> p = participants_.insert(std::make_pair(guid, DiscoveredParticipant(pdata, seq, auth_resend_period_)));
    ++n_participants_in_authentication_;
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) {auth_debug} DEBUG: Spdp::handle_participant_data() %B participants in authentication\n",
                 n_participants_in_authentication_));
    }
#else
    std::pair<DiscoveredParticipantIter, bool> p = participants_.insert(std::make_pair(guid, DiscoveredParticipant(pdata, seq, TimeDuration())));
#endif
    iter = p.first;
    iter->second.discovered_at_ = now;
    update_lease_expiration_i(iter, now);
    update_rtps_relay_application_participant_i(iter, p.second);

    if (!from_relay && from) {
      iter->second.last_recv_address_ = from;
    }

    if (DCPS::transport_debug.log_progress) {
      log_progress("participant discovery", guid_, guid, iter->second.discovered_at_.to_monotonic_time());
    }

#ifndef DDS_HAS_MINIMUM_BIT
    if (!from_sedp) {
      enqueue_location_update_i(iter, location_mask, from, "new participant");
    }
#endif

    sedp_->associate(iter->second
#ifdef OPENDDS_SECURITY
                     , participant_sec_attr_
#endif
                     );

    // Since we've just seen a new participant, let's send out our
    // own announcement, so they don't have to wait.
    if (from) {
      if (from_relay) {
        tport_->write_i(guid, iter->second.last_recv_address_, SpdpTransport::SEND_RELAY);
      } else {
        tport_->write_i(guid, iter->second.local_address_, SpdpTransport::SEND_DIRECT);
      }
    }

#ifdef OPENDDS_SECURITY
    if (is_security_enabled()) {
      if (!iter->second.has_security_data()) {
        if (!participant_sec_attr_.allow_unauthenticated_participants) {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::handle_participant_data - ")
              ACE_TEXT("Incompatible security attributes in discovered participant: %C\n"),
              DCPS::LogGuid(guid).c_str()));
          }
          // FUTURE: This is probably not a good idea since it will just get rediscovered.
          purge_discovered_participant(iter);
          participants_.erase(iter);
          iter = participants_.end();
        } else { // allow_unauthenticated_participants == true
          set_auth_state(iter->second, AUTH_STATE_UNAUTHENTICATED);
          match_unauthenticated(iter);
        }
      } else {
        iter->second.identity_token_ = pdata.ddsParticipantDataSecure.base.identity_token;
        iter->second.permissions_token_ = pdata.ddsParticipantDataSecure.base.permissions_token;
        iter->second.property_qos_ = pdata.ddsParticipantDataSecure.base.property;
        iter->second.security_info_ = pdata.ddsParticipantDataSecure.base.security_info;
        iter->second.extended_builtin_endpoints_ = pdata.ddsParticipantDataSecure.base.extended_builtin_endpoints;

        // The remote needs to see our SPDP before attempting authentication.
        tport_->write_i(guid, iter->second.last_recv_address_, from_relay ? SpdpTransport::SEND_RELAY : SpdpTransport::SEND_DIRECT);

        attempt_authentication(iter, true);

        if (iter->second.auth_state_ == AUTH_STATE_UNAUTHENTICATED) {
          if (!participant_sec_attr_.allow_unauthenticated_participants) {
            if (DCPS::security_debug.auth_debug) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::handle_participant_data - ")
                ACE_TEXT("Incompatible security attributes in discovered participant: %C\n"),
                DCPS::LogGuid(guid).c_str()));
            }
            purge_discovered_participant(iter);
            participants_.erase(iter);
            iter = participants_.end();
          } else { // allow_unauthenticated_participants == true
            set_auth_state(iter->second, AUTH_STATE_UNAUTHENTICATED);
            match_unauthenticated(iter);
          }
        } else if (iter->second.auth_state_ == AUTH_STATE_AUTHENTICATED) {
          if (!match_authenticated(guid, iter)) {
            purge_discovered_participant(iter);
            participants_.erase(iter);
            iter = participants_.end();
          }
        }
        // otherwise just return, since we're waiting for input to finish authentication
      }
    } else {
      set_auth_state(iter->second, AUTH_STATE_UNAUTHENTICATED);
      match_unauthenticated(iter);
    }
#else
    match_unauthenticated(iter);
#endif

  } else { // Existing Participant
    if (from_sedp && DCPS::transport_debug.log_progress) {
      log_progress("secure participant discovery", guid_, guid, iter->second.discovered_at_.to_monotonic_time());
    }

#ifndef DDS_HAS_MINIMUM_BIT
    if (!from_sedp) {
      enqueue_location_update_i(iter, location_mask, from, "existing participant");
    }
#endif
#ifdef OPENDDS_SECURITY
    // Non-secure updates for authenticated participants are used for liveliness but
    // are otherwise ignored. Non-secure dispose messages are ignored completely.
    if (is_security_enabled() && iter->second.auth_state_ == AUTH_STATE_AUTHENTICATED && !from_sedp) {
      update_lease_expiration_i(iter, now);
      if (!from_relay && from) {
        iter->second.last_recv_address_ = from;
      }
#ifndef DDS_HAS_MINIMUM_BIT
      process_location_updates_i(iter, "non-secure liveliness");
#endif
      return;
    }
#endif

    if (id == DCPS::DISPOSE_INSTANCE || id == DCPS::DISPOSE_UNREGISTER_INSTANCE) {
#ifdef OPENDDS_SECURITY
      DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = sedp_->get_ice_endpoint();
      if (sedp_endpoint) {
        stop_ice(sedp_endpoint, iter->first, iter->second.pdata_.participantProxy.availableBuiltinEndpoints,
                 iter->second.pdata_.participantProxy.availableExtendedBuiltinEndpoints);
      }
      DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint = tport_->get_ice_endpoint();
      if (spdp_endpoint) {
        ice_agent_->stop_ice(spdp_endpoint, guid_, iter->first);
      }
      purge_handshake_deadlines(iter);
#endif
#ifndef DDS_HAS_MINIMUM_BIT
      process_location_updates_i(iter, "dispose/unregister");
#endif
      if (iter != participants_.end()) {
        purge_discovered_participant(iter);
        participants_.erase(iter);
      }
      return;
    }

    // Check if sequence numbers are increasing
    if (validateSequenceNumber(now, seq, iter)) {
      // update an existing participant
      DDS::ParticipantBuiltinTopicData& pdataBit = partBitData(pdata);
      DDS::ParticipantBuiltinTopicData& discoveredBit = partBitData(iter->second.pdata_);
      pdataBit.key = discoveredBit.key;

#ifndef OPENDDS_SAFETY_PROFILE
      using DCPS::operator!=;
#endif
      if (discoveredBit.user_data != pdataBit.user_data ||
          (from_sedp && iter->second.bit_ih_ == DDS::HANDLE_NIL)) {
        discoveredBit.user_data = pdataBit.user_data;

        // If secure user data, this is the first time we should be
        // seeing the real user data.
        iter->second.bit_ih_ = bit_subscriber_->add_participant(pdataBit, secure_part_user_data() ? DDS::NEW_VIEW_STATE : DDS::NOT_NEW_VIEW_STATE);
      }
      if (locators_changed(iter->second.pdata_.participantProxy, pdata.participantProxy)) {
        sedp_->update_locators(pdata);
      }
      const DCPS::MonotonicTime_t da = iter->second.pdata_.discoveredAt;
      iter->second.pdata_ = pdata;
      iter->second.pdata_.discoveredAt = da;
      update_lease_expiration_i(iter, now);
      update_rtps_relay_application_participant_i(iter, false);
      if (!from_relay && from) {
        iter->second.last_recv_address_ = from;
      }

#ifndef DDS_HAS_MINIMUM_BIT
      /*
       * If secure user data, force update location bit because we just gave
       * the first data on the participant. Readers might have been ignoring
       * location samples on the participant until now.
       */
      process_location_updates_i(iter, "valid SPDP", secure_part_user_data());
#endif
    // Else a reset has occurred and check if we should remove the participant
    } else if (iter->second.seq_reset_count_ >= max_spdp_sequence_msg_reset_check_) {
#ifdef OPENDDS_SECURITY
      purge_handshake_deadlines(iter);
#endif
#ifndef DDS_HAS_MINIMUM_BIT
      process_location_updates_i(iter, "reset");
#endif
      if (iter != participants_.end()) {
        purge_discovered_participant(iter);
        participants_.erase(iter);
      }
      return;
    }
  }

#ifndef DDS_HAS_MINIMUM_BIT
  if (iter != participants_.end()) {
    process_location_updates_i(iter, "catch all");
  }
#endif
}

bool
Spdp::validateSequenceNumber(const DCPS::MonotonicTimePoint& now, const DCPS::SequenceNumber& seq, DiscoveredParticipantIter& iter)
{
  if (seq.getValue() != 0 && iter->second.max_seq_ != DCPS::SequenceNumber::MAX_VALUE) {
    if (seq < iter->second.max_seq_) {
      const bool honeymoon_period = now < iter->second.discovered_at_ + min_resend_delay_;
      if (!honeymoon_period) {
        ++iter->second.seq_reset_count_;
      }
      return false;
    } else if (iter->second.seq_reset_count_ > 0) {
      --iter->second.seq_reset_count_;
    }
  }
  iter->second.max_seq_ = std::max(iter->second.max_seq_, seq);
  return true;
}

void
Spdp::data_received(const DataSubmessage& data,
                    const ParameterList& plist,
                    const DCPS::NetworkAddress& from)
{
  ACE_Guard<ACE_Thread_Mutex> guard(lock_);
  if (!initialized_flag_ || shutdown_flag_) {
    return;
  }

  const MonotonicTimePoint now = MonotonicTimePoint::now();
  ParticipantData_t pdata;

  pdata.participantProxy.domainId = domain_;
  pdata.discoveredAt = now.to_monotonic_time();
#ifdef OPENDDS_SECURITY
  pdata.ddsParticipantDataSecure.base.base.key = DCPS::BUILTIN_TOPIC_KEY_UNKNOWN;
#endif

  if (!ParameterListConverter::from_param_list(plist, pdata)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::data_received - ")
        ACE_TEXT("failed to convert from ParameterList to ")
        ACE_TEXT("SPDPdiscoveredParticipantData\n")));
    }
    return;
  }

  // Remote domain ID, if populated, has to match
  if (pdata.participantProxy.domainId != domain_) {
    return;
  }

  const GUID_t guid = DCPS::make_part_guid(pdata.participantProxy.guidPrefix);
  if (guid == guid_) {
    // About us, stop.
    return;
  }

  const DCPS::MessageId msg_id = (data.inlineQos.length() && disposed(data.inlineQos)) ? DCPS::DISPOSE_INSTANCE : DCPS::SAMPLE_DATA;

#ifdef OPENDDS_SECURITY
  const bool from_relay = sedp_->core().from_relay(from);

  if (check_source_ip_ && msg_id == DCPS::SAMPLE_DATA && !from_relay && !ip_in_locator_list(from, pdata.participantProxy.metatrafficUnicastLocatorList) && !ip_in_AgentInfo(from, plist)) {
    if (DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) Spdp::data_received - dropped IP: %C\n"), DCPS::LogAddr(from).c_str()));
    }
    return;
  }

  const bool security_enabled = is_security_enabled();
  guard.release();

  if (!security_enabled) {
    process_participant_ice(plist, pdata, guid);
  }
#elif !defined OPENDDS_SAFETY_PROFILE
  const bool from_relay = sedp_->core().from_relay(from);
  guard.release();

  if (check_source_ip_ && msg_id == DCPS::SAMPLE_DATA && !from_relay && !ip_in_locator_list(from, pdata.participantProxy.metatrafficUnicastLocatorList)) {
    if (DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) Spdp::data_received - IP not in locator list: %C\n"), DCPS::LogAddr(from).c_str()));
    }
    return;
  }
#else
  guard.release();
#endif

  handle_participant_data(msg_id, pdata, now, to_opendds_seqnum(data.writerSN), from, false);
}

void
Spdp::match_unauthenticated(const DiscoveredParticipantIter& dp_iter)
{
#ifndef DDS_HAS_MINIMUM_BIT
  if (!secure_part_user_data()) { // else the user data is assumed to be blank
    dp_iter->second.bit_ih_ = bit_subscriber_->add_participant(partBitData(dp_iter->second.pdata_), DDS::NEW_VIEW_STATE);
  }

  process_location_updates_i(dp_iter, "match_unauthenticated");
#else
  ACE_UNUSED_ARG(dp_iter);
#endif /* DDS_HAS_MINIMUM_BIT */
}

#ifdef OPENDDS_SECURITY

void
Spdp::handle_auth_request(const DDS::Security::ParticipantStatelessMessage& msg)
{
  const GUID_t guid = make_id(msg.message_identity.source_guid, DCPS::ENTITYID_PARTICIPANT);

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_auth_request() - ")
               ACE_TEXT("%C -> %C local %C\n"),
               DCPS::LogGuid(guid).c_str(),
               DCPS::LogGuid(msg.destination_participant_guid).c_str(),
               DCPS::LogGuid(guid_).c_str()));
  }

  // If this message wasn't intended for us, ignore handshake message
  if (msg.destination_participant_guid != guid_) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_auth_request() - ")
                 ACE_TEXT("Dropped not recipient\n")));
    }
    return;
  }

  if (msg.message_data.length() == 0) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_auth_request() - ")
                 ACE_TEXT("Dropped no data\n")));
    }
    return;
  }

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (!initialized_flag_ || shutdown_flag_) {
    return;
  }

  if (sedp_->ignoring(guid)) {
    // Ignore, this is our domain participant or one that the user has
    // asked us to ignore.
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_auth_request() - ")
                 ACE_TEXT("Explicitly ignoring\n")));
    }
    return;
  }


  DiscoveredParticipantMap::iterator iter = participants_.find(guid);

  if (iter != participants_.end()) {
    if (msg.message_identity.sequence_number <= iter->second.auth_req_sequence_number_) {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_auth_request() - ")
                   ACE_TEXT("Dropped due to old sequence number\n")));
      }
      return;
    }

    iter->second.remote_auth_request_token_ = msg.message_data[0];
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
      get_part_bit_data(false),
      identity_token_,
      permissions_token_,
      qos_.property,
      {0, 0},
      0
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
    pbtds.base.extended_builtin_endpoints =
      DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE | DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE |
      DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE | DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE;
  }
  if (participant_sec_attr_.is_liveliness_protected) {
    pbtds.base.security_info.participant_security_attributes |= DDS::Security::PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED;
  }

  ParameterList plist;
  set_participant_guid(guid_, plist);

  if (!ParameterListConverter::to_param_list(pbtds.base, plist)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::local_participant_data_as_octets() - ")
                ACE_TEXT("Failed to convert from ParticipantBuiltinTopicData to ParameterList\n")));
    }
    return DDS::OctetSeq();
  }

  ACE_Message_Block temp_buff(DCPS::serialized_size(encoding_plain_big, plist));
  DCPS::Serializer ser(&temp_buff, encoding_plain_big);
  if (!(ser << plist)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::local_participant_data_as_octets() - ")
                ACE_TEXT("Failed to serialize parameter list.\n")));
    }
    return DDS::OctetSeq();
  }

  DDS::OctetSeq seq(static_cast<unsigned int>(temp_buff.length()));
  seq.length(seq.maximum());
  std::memcpy(seq.get_buffer(), temp_buff.rd_ptr(), temp_buff.length());
  return seq;
}

void
Spdp::send_handshake_request(const DCPS::GUID_t& guid, DiscoveredParticipant& dp)
{
  OPENDDS_ASSERT(dp.handshake_state_ == HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST);

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

  dp.handshake_state_ = HANDSHAKE_STATE_PROCESS_HANDSHAKE;

  DDS::Security::ParticipantStatelessMessage msg = DDS::Security::ParticipantStatelessMessage();
  msg.message_identity.source_guid = guid_;
  msg.message_identity.sequence_number = 0;
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
               DCPS::LogGuid(guid).c_str()));
  }
}

void
Spdp::attempt_authentication(const DiscoveredParticipantIter& iter, bool from_discovery)
{
  const DCPS::GUID_t& guid = iter->first;
  DiscoveredParticipant& dp = iter->second;

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication "
      "for %C from_discovery=%d have_remote_token=%d auth_state=%d handshake_state=%d\n",
      DCPS::LogGuid(guid).c_str(),
      from_discovery, !(dp.remote_auth_request_token_ == DDS::Security::Token()),
      dp.auth_state_, dp.handshake_state_));
  }

  dp.handshake_resend_falloff_.set(auth_resend_period_);

  if (!from_discovery && dp.handshake_state_ != HANDSHAKE_STATE_DONE) {
    // Ignore auth reqs when already in progress.
    schedule_handshake_resend(DCPS::TimeDuration::zero_value, guid);
    return;
  }

  // Reset.
  purge_handshake_deadlines(iter);
  dp.handshake_deadline_ = DCPS::MonotonicTimePoint::now() + max_auth_time_;
  handshake_deadlines_.insert(std::make_pair(dp.handshake_deadline_, guid));
  tport_->handshake_deadline_task_->schedule(max_auth_time_);

  DDS::Security::Authentication_var auth = security_config_->get_authentication();
  DDS::Security::SecurityException se = {"", 0, 0};

  const DDS::Security::ValidationResult_t vr = auth->validate_remote_identity(
    dp.identity_handle_, dp.local_auth_request_token_, dp.remote_auth_request_token_,
    identity_handle_, dp.identity_token_, guid, se);

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
    if (sedp_->write_stateless_message(dp.auth_req_msg_, make_id(guid, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER)) != DDS::RETCODE_OK) {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::attempt_authentication() - ")
                   ACE_TEXT("Unable to write auth req message.\n")));
      }
    } else {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                   ACE_TEXT("Sent auth req message for participant: %C\n"),
                   DCPS::LogGuid(guid).c_str()));
      }
    }
    schedule_handshake_resend(dp.handshake_resend_falloff_.get(), guid);
  }

  switch (vr) {
  case DDS::Security::VALIDATION_OK: {
    set_auth_state(dp, AUTH_STATE_AUTHENTICATED);
    dp.handshake_state_ = HANDSHAKE_STATE_DONE;
    purge_handshake_deadlines(iter);
    return;
  }
  case DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE: {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                 ACE_TEXT("Attempting authentication (expecting request) for participant: %C\n"),
                 DCPS::LogGuid(guid).c_str()));
    }
    dp.handshake_state_ = HANDSHAKE_STATE_BEGIN_HANDSHAKE_REPLY;
    dp.is_requester_ = true;
    return; // We'll need to wait for an inbound handshake request from the remote participant
  }
  case DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST: {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                 ACE_TEXT("Attempting authentication (sending request/expecting reply) for participant: %C\n"),
                 DCPS::LogGuid(guid).c_str()));
    }
    dp.handshake_state_ = HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST;
    send_handshake_request(guid, dp);
    return;
  }
  case DDS::Security::VALIDATION_FAILED: {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                 ACE_TEXT("Remote participant identity is invalid. Security Exception[%d.%d]: %C\n"),
                 se.code, se.minor_code, se.message.in()));
    }
    set_auth_state(dp, AUTH_STATE_UNAUTHENTICATED);
    dp.handshake_state_ = HANDSHAKE_STATE_DONE;
    purge_handshake_deadlines(iter);
    return;
  }
  default: {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::attempt_authentication() - ")
                 ACE_TEXT("Unexpected return value while validating remote identity. Security Exception[%d.%d]: %C\n"),
                 se.code, se.minor_code, se.message.in()));
    }
    set_auth_state(dp, AUTH_STATE_UNAUTHENTICATED);
    dp.handshake_state_ = HANDSHAKE_STATE_DONE;
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

  const GUID_t src_participant = make_id(msg.message_identity.source_guid, DCPS::ENTITYID_PARTICIPANT);

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - ")
               ACE_TEXT("%C -> %C local %C\n"),
               DCPS::LogGuid(src_participant).c_str(),
               DCPS::LogGuid(msg.destination_participant_guid).c_str(),
               DCPS::LogGuid(guid_).c_str()));
  }

  // If this message wasn't intended for us, ignore handshake message
  if (msg.destination_participant_guid != guid_) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - ")
                 ACE_TEXT("Dropped not recipient\n")));
    }
    return;
  }

  if (msg.message_data.length() == 0) {
    if (DCPS::security_debug.auth_debug) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - ")
                 ACE_TEXT("Dropped no data\n")));
    }
    return;
  }

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (!initialized_flag_ || shutdown_flag_ ) {
    return;
  }

  // If discovery hasn't initialized / validated this participant yet, ignore handshake messages
  DiscoveredParticipantIter iter = participants_.find(src_participant);
  if (iter == participants_.end()) {
    if (DCPS::security_debug.auth_warn) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) {auth_warn} Spdp::handle_handshake_message() - ")
        ACE_TEXT("received handshake for undiscovered participant %C. Ignoring.\n"),
        DCPS::LogGuid(src_participant).c_str()));
    }
    return;
  }

  DiscoveredParticipant& dp = iter->second;

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - "
      "for %C auth_state=%d handshake_state=%d\n",
      DCPS::LogGuid(src_participant).c_str(),
      dp.auth_state_, dp.handshake_state_));
  }

  // We have received a handshake message from the remote which means
  // we don't need to send the auth req.
  dp.have_auth_req_msg_ = false;

  dp.handshake_resend_falloff_.set(auth_resend_period_);

  if (dp.handshake_state_ == HANDSHAKE_STATE_DONE && !dp.is_requester_) {
    // Remote is still sending a reply, so resend the final.
    const GUID_t reader = make_id(iter->first, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER);
    if (sedp_->write_stateless_message(dp.handshake_msg_, reader) != DDS::RETCODE_OK) {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::handle_handshake_message() - ")
                   ACE_TEXT("Unable to write handshake message.\n")));
      }
    } else {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::handle_handshake_message() - ")
                   ACE_TEXT("Sent handshake message for participant: %C\n"),
                   DCPS::LogGuid(iter->first).c_str()));
      }
    }
    return;
  }

  if (msg.message_identity.sequence_number <= iter->second.handshake_sequence_number_) {
    return;
  }
  iter->second.handshake_sequence_number_ = msg.message_identity.sequence_number;

  switch (dp.handshake_state_) {
  case HANDSHAKE_STATE_DONE:
    // Handled above.
    return;

  case HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST: {
    if (DCPS::security_debug.auth_warn) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) {auth_warn} Spdp::handle_handshake_message() - ")
                 ACE_TEXT("Invalid handshake state\n")));
    }
    return;
  }

  case HANDSHAKE_STATE_BEGIN_HANDSHAKE_REPLY: {
    DDS::Security::ParticipantStatelessMessage reply = DDS::Security::ParticipantStatelessMessage();
    reply.message_identity.source_guid = guid_;
    reply.message_identity.sequence_number = 0;
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
      set_auth_state(dp, AUTH_STATE_AUTHENTICATED);
      dp.handshake_state_ = HANDSHAKE_STATE_DONE;
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
                     DCPS::LogGuid(src_participant).c_str()));
        }
      }
      dp.handshake_state_ = HANDSHAKE_STATE_PROCESS_HANDSHAKE;
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
                     DCPS::LogGuid(src_participant).c_str()));
        }
      }
      set_auth_state(dp, AUTH_STATE_AUTHENTICATED);
      dp.handshake_state_ = HANDSHAKE_STATE_PROCESS_HANDSHAKE;
      purge_handshake_deadlines(iter);
      match_authenticated(src_participant, iter);
      return;
    }
    }
    return;
  }

  case HANDSHAKE_STATE_PROCESS_HANDSHAKE: {
    DDS::Security::ParticipantStatelessMessage reply = DDS::Security::ParticipantStatelessMessage();
    reply.message_identity.source_guid = guid_;
    reply.message_identity.sequence_number = 0;
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
                   DCPS::LogGuid(src_participant).c_str(),
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
                     DCPS::LogGuid(src_participant).c_str()));
        }
      }
      return;
    }
    case DDS::Security::VALIDATION_OK_FINAL_MESSAGE: {
      set_auth_state(dp, AUTH_STATE_AUTHENTICATED);
      dp.handshake_state_ = HANDSHAKE_STATE_DONE;
      // Install the shared secret before sending the final so that
      // we are prepared to receive the crypto tokens from the
      // replier.

      // Send the final first because match_authenticated takes forever.
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
                     DCPS::LogGuid(src_participant).c_str()));
        }
      }

      purge_handshake_deadlines(iter);
      match_authenticated(src_participant, iter);
      return;
    }
    case DDS::Security::VALIDATION_OK: {
      set_auth_state(dp, AUTH_STATE_AUTHENTICATED);
      dp.handshake_state_ = HANDSHAKE_STATE_DONE;
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

  if (!initialized_flag_ || shutdown_flag_) {
    return;
  }

  for (TimeQueue::iterator pos = handshake_deadlines_.begin(),
        limit = handshake_deadlines_.upper_bound(now); pos != limit;) {

    DiscoveredParticipantIter pit = participants_.find(pos->second);
    if (pit != participants_.end()) {
      if (DCPS::security_debug.auth_debug) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::process_handshake_deadlines() - ")
                   ACE_TEXT("Removing discovered participant due to authentication timeout: %C\n"),
                   DCPS::LogGuid(pos->second).c_str()));
      }
      const DCPS::MonotonicTimePoint ptime = pos->first;
      if (!participant_sec_attr_.allow_unauthenticated_participants) {
        DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = sedp_->get_ice_endpoint();
        if (sedp_endpoint) {
          stop_ice(sedp_endpoint, pit->first, pit->second.pdata_.participantProxy.availableBuiltinEndpoints,
                   pit->second.pdata_.participantProxy.availableExtendedBuiltinEndpoints);
        }
        DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint = tport_->get_ice_endpoint();
        if (spdp_endpoint) {
          ice_agent_->stop_ice(spdp_endpoint, guid_, pit->first);
        }
        handshake_deadlines_.erase(pos);
        purge_discovered_participant(pit);
        participants_.erase(pit);
      } else {
        purge_handshake_resends(pit);
        set_auth_state(pit->second, AUTH_STATE_UNAUTHENTICATED);
        pit->second.handshake_state_ = HANDSHAKE_STATE_DONE;
        handshake_deadlines_.erase(pos);
        match_unauthenticated(pit);
      }
      pos = handshake_deadlines_.lower_bound(ptime);
      limit = handshake_deadlines_.upper_bound(now);
    } else {
      handshake_deadlines_.erase(pos++);
    }
  }

  if (!handshake_deadlines_.empty()) {
    tport_->handshake_deadline_task_->schedule(handshake_deadlines_.begin()->first - now);
  }
}

void
Spdp::process_handshake_resends(const DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (!initialized_flag_ || shutdown_flag_) {
    return;
  }

  bool processor_needs_cancel = false;
  for (TimeQueue::iterator pos = handshake_resends_.begin(), limit = handshake_resends_.end();
       pos != limit && pos->first <= now;) {

    DiscoveredParticipantIter pit = participants_.find(pos->second);
    if (pit != participants_.end() &&
        pit->second.stateless_msg_deadline_ <= now) {
      const GUID_t reader = make_id(pit->first, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER);
      pit->second.stateless_msg_deadline_ = now + pit->second.handshake_resend_falloff_.get();

      // Send the auth req first to reset the remote if necessary.
      if (pit->second.have_auth_req_msg_) {
        // Send the SPDP announcement in case it got lost.
        tport_->write_i(pit->first, pit->second.last_recv_address_, SpdpTransport::SEND_RELAY | SpdpTransport::SEND_DIRECT);
        sedp_->core().writer_resend_count(make_id(guid_, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER), 1);
        if (sedp_->write_stateless_message(pit->second.auth_req_msg_, reader) != DDS::RETCODE_OK) {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::process_handshake_resends() - ")
                       ACE_TEXT("Unable to write auth req message retry.\n")));
          }
        } else {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::process_handshake_resends() - ")
                       ACE_TEXT("Sent auth req message for participant: %C\n"),
                       DCPS::LogGuid(pit->first).c_str()));
          }
        }
      }
      if (pit->second.have_handshake_msg_) {
        sedp_->core().writer_resend_count(make_id(guid_, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER), 1);
        if (sedp_->write_stateless_message(pit->second.handshake_msg_, reader) != DDS::RETCODE_OK) {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} Spdp::process_handshake_resends() - ")
                       ACE_TEXT("Unable to write handshake message retry.\n")));
          }
        } else {
          if (DCPS::security_debug.auth_debug) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {auth_debug} DEBUG: Spdp::process_handshake_resends() - ")
                       ACE_TEXT("Sent handshake message for participant: %C\n"),
                       DCPS::LogGuid(pit->first).c_str()));
          }
        }
      }
      pit->second.handshake_resend_falloff_.advance(max_auth_time_);

      handshake_resends_.insert(std::make_pair(pit->second.stateless_msg_deadline_, pit->first));
      if (pit->second.stateless_msg_deadline_ < handshake_resends_.begin()->first) {
        processor_needs_cancel = true;
      }
    }

    handshake_resends_.erase(pos++);
  }

  if (!handshake_resends_.empty()) {
    if (processor_needs_cancel) {
      tport_->handshake_resend_task_->cancel();
    }
    tport_->handshake_resend_task_->schedule(handshake_resends_.begin()->first - now);
  }
}

bool
Spdp::handle_participant_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg)
{
  const GUID_t src_participant = make_id(msg.message_identity.source_guid, DCPS::ENTITYID_PARTICIPANT);

  if (DCPS::security_debug.auth_debug) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Spdp::handle_participant_crypto_tokens() from %C\n"),
               DCPS::LogGuid(src_participant).c_str()));
  }

  DDS::Security::SecurityException se = {"", 0, 0};
  Security::CryptoKeyExchange_var key_exchange = security_config_->get_crypto_key_exchange();

  // If this message wasn't intended for us, ignore volatile message
  if (msg.destination_participant_guid != guid_ || !msg.message_data.length()) {
    return false;
  }

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
        DCPS::LogGuid(src_participant).c_str()));
    }
    return false;
  }

  if (DCPS::transport_debug.log_progress) {
    log_progress("participant crypto token", guid_, src_participant, iter->second.discovered_at_.to_monotonic_time());
  }

  const DDS::Security::ParticipantCryptoTokenSeq& inboundTokens =
    reinterpret_cast<const DDS::Security::ParticipantCryptoTokenSeq&>(msg.message_data);
  const DDS::Security::ParticipantCryptoHandle dp_crypto_handle =
    sedp_->get_handle_registry()->get_remote_participant_crypto_handle(iter->first);

  if (!key_exchange->set_remote_participant_crypto_tokens(crypto_handle_, dp_crypto_handle, inboundTokens, se)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::handle_participant_crypto_tokens() - ")
      ACE_TEXT("Unable to set remote participant crypto tokens with crypto key exchange plugin. ")
      ACE_TEXT("Security Exception[%d.%d]: %C\n"),
        se.code, se.minor_code, se.message.in()));
    return false;
  }

  sedp_->process_association_records_i(iter->second);

  return true;
}

DDS::ReturnCode_t
Spdp::send_handshake_message(const DCPS::GUID_t& guid,
                             DiscoveredParticipant& dp,
                             const DDS::Security::ParticipantStatelessMessage& msg)
{
  dp.handshake_msg_ = msg;
  dp.handshake_msg_.message_identity.sequence_number = (++stateless_sequence_number_).getValue();

  const DCPS::GUID_t reader = make_id(guid, ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER);
  const DDS::ReturnCode_t retval = sedp_->write_stateless_message(dp.handshake_msg_, reader);
  dp.have_handshake_msg_ = true;
  dp.stateless_msg_deadline_ = schedule_handshake_resend(dp.handshake_resend_falloff_.get(), guid);
  return retval;
}

MonotonicTimePoint Spdp::schedule_handshake_resend(const TimeDuration& time, const GUID_t& guid)
{
  const MonotonicTimePoint deadline = MonotonicTimePoint::now() + time;
  handshake_resends_.insert(std::make_pair(deadline, guid));
  if (deadline < handshake_resends_.begin()->first) {
    tport_->handshake_resend_task_->cancel();
  }
  tport_->handshake_resend_task_->schedule(time);
  return deadline;
}

bool
Spdp::match_authenticated(const DCPS::GUID_t& guid, DiscoveredParticipantIter& iter)
{
  if (iter->second.handshake_handle_ == DDS::HANDLE_NIL) {
    return true;
  }

  DDS::Security::SecurityException se = {"", 0, 0};

  Security::Authentication_var auth = security_config_->get_authentication();
  Security::AccessControl_var access = security_config_->get_access_control();
  Security::CryptoKeyFactory_var key_factory = security_config_->get_crypto_key_factory();
  Security::CryptoKeyExchange_var key_exchange = security_config_->get_crypto_key_exchange();
  Security::HandleRegistry_rch handle_registry = security_config_->get_handle_registry(guid_);

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

    sedp_->disassociate_volatile(iter->second);
    sedp_->cleanup_volatile_crypto(iter->first);
    sedp_->associate_volatile(iter->second);
    sedp_->generate_remote_matched_crypto_handles(iter->second);
    sedp_->process_association_records_i(iter->second);

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
  handle_registry->insert_remote_participant_permissions_handle(guid, iter->second.permissions_handle_);

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
    if (!access->check_remote_participant(iter->second.permissions_handle_, domain_,
        iter->second.pdata_.ddsParticipantDataSecure, se)) {
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
               DCPS::LogGuid(guid).c_str()));
  }

  if (DCPS::transport_debug.log_progress) {
    log_progress("authentication", guid_, guid, iter->second.discovered_at_.to_monotonic_time());
  }

  DDS::Security::ParticipantCryptoHandle dp_crypto_handle =
    sedp_->get_handle_registry()->get_remote_participant_crypto_handle(iter->first);

  if (dp_crypto_handle == DDS::HANDLE_NIL) {
    dp_crypto_handle = key_factory->register_matched_remote_participant(
      crypto_handle_, iter->second.identity_handle_, iter->second.permissions_handle_,
      iter->second.shared_secret_handle_, se);
    sedp_->get_handle_registry()->insert_remote_participant_crypto_handle(iter->first, dp_crypto_handle);
    if (dp_crypto_handle == DDS::HANDLE_NIL) {
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
    if (!key_exchange->create_local_participant_crypto_tokens(
        iter->second.crypto_tokens_, crypto_handle_, dp_crypto_handle, se)) {
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

  sedp_->generate_remote_matched_crypto_handles(iter->second);

  // Auth is now complete.
  sedp_->process_association_records_i(iter->second);

#ifndef DDS_HAS_MINIMUM_BIT
  process_location_updates_i(iter, "match_authenticated");
#endif
  return true;
}

void Spdp::update_agent_info(const DCPS::GUID_t&, const ICE::AgentInfo&)
{
  if (is_security_enabled()) {
    write_secure_updates();
  }
}

void Spdp::remove_agent_info(const DCPS::GUID_t&)
{
  if (is_security_enabled()) {
    write_secure_updates();
  }
}
#endif

void
Spdp::init_bit(DCPS::RcHandle<DCPS::BitSubscriber> bit_subscriber)
{
  OPENDDS_ASSERT(bit_subscriber);

  bit_subscriber_ = bit_subscriber;

  // Defer initilization until we have the bit subscriber.
  sedp_->init(guid_, *disco_, domain_, type_lookup_service_);
  tport_->open(sedp_->reactor_task(), sedp_->job_queue());

#ifdef OPENDDS_SECURITY
  DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = sedp_->get_ice_endpoint();
  if (sedp_endpoint) {
    const GUID_t l = make_id(guid_, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER);
    ice_agent_->add_local_agent_info_listener(sedp_endpoint, l, DCPS::static_rchandle_cast<AgentInfoListener>(rchandle_from(this)));
  }
#endif

  initialized_flag_ = true;
  tport_->enable_periodic_tasks();
}

void
Spdp::fini_bit()
{
  bit_subscriber_->clear();
}

namespace {
  bool is_opendds(const ParticipantProxy_t& participant)
  {
    return 0 == std::memcmp(&participant.vendorId, DCPS::VENDORID_OCI, sizeof(VendorId_t));
  }
}

bool
Spdp::is_expectant_opendds(const GUID_t& participant) const
{
  const DiscoveredParticipantConstIter iter = participants_.find(participant);
  if (iter == participants_.end()) {
    return false;
  }
  return is_opendds(iter->second.pdata_.participantProxy) &&
    (iter->second.pdata_.participantProxy.opendds_participant_flags.bits & PFLAGS_NO_ASSOCIATED_WRITERS) == 0;
}

ParticipantData_t Spdp::build_local_pdata(
#ifdef OPENDDS_SECURITY
  bool always_in_the_clear,
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

  const DCPS::LocatorSeq unicast_locators = sedp_->unicast_locators();
  const DCPS::LocatorSeq multicast_locators = sedp_->multicast_locators();

  if (unicast_locators.length() == 0 && multicast_locators.length() == 0) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: ")
                ACE_TEXT("Spdp::build_local_pdata: ")
                ACE_TEXT("no locators\n")));
    }
  }

#ifdef OPENDDS_SECURITY
  const Security::SPDPdiscoveredParticipantData pdata = {
    kind,
    { // ParticipantBuiltinTopicDataSecure
      { // ParticipantBuiltinTopicData (security enhanced)
        get_part_bit_data(!always_in_the_clear),
        identity_token_,
        permissions_token_,
        qos_.property,
        {
          security_attributes_to_bitmask(participant_sec_attr_),
          participant_sec_attr_.plugin_participant_attributes
        },
        available_extended_builtin_endpoints_
      },
      identity_status_token_
    },
#else
  const SPDPdiscoveredParticipantData pdata = {
    get_part_bit_data(false),
#endif
    { // ParticipantProxy_t
      domain_
      , ""
      , PROTOCOLVERSION
      , {gp[0], gp[1], gp[2], gp[3], gp[4], gp[5],
       gp[6], gp[7], gp[8], gp[9], gp[10], gp[11]}
      , VENDORID_OPENDDS
      , false /*expectsIQoS*/
      , available_builtin_endpoints_
      , 0
      , unicast_locators
      , multicast_locators
      , nonEmptyList /*defaultMulticastLocatorList*/
      , nonEmptyList /*defaultUnicastLocatorList*/
      , {0 /*manualLivelinessCount*/}   //FUTURE: implement manual liveliness
      , qos_.property
      , { participant_flags_ } // opendds_participant_flags
      , is_application_participant_
#ifdef OPENDDS_SECURITY
      , available_extended_builtin_endpoints_
#endif
    },
    { // Duration_t (leaseDuration)
      static_cast<CORBA::Long>(lease_duration_.value().sec()),
      0 // we are not supporting fractional seconds in the lease duration
    },
    participant_discovered_at_
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

#if !defined _MSC_VER || _MSC_VER >= 1900
const Spdp::SpdpTransport::WriteFlags Spdp::SpdpTransport::SEND_MULTICAST;
const Spdp::SpdpTransport::WriteFlags Spdp::SpdpTransport::SEND_RELAY;
const Spdp::SpdpTransport::WriteFlags Spdp::SpdpTransport::SEND_DIRECT;
#endif

Spdp::SpdpTransport::SpdpTransport(DCPS::RcHandle<Spdp> outer)
  : outer_(outer)
  , buff_(64 * 1024)
  , wbuff_(64 * 1024)
  , network_is_unreachable_(false)
  , ice_endpoint_added_(false)
{
  hdr_.prefix[0] = 'R';
  hdr_.prefix[1] = 'T';
  hdr_.prefix[2] = 'P';
  hdr_.prefix[3] = 'S';
  hdr_.version = PROTOCOLVERSION;
  hdr_.vendorId = VENDORID_OPENDDS;
  DCPS::assign(hdr_.guidPrefix, outer->guid_.guidPrefix);
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

  multicast_interface_ = outer->disco_->multicast_interface();

  const u_short port_common = outer->config_->port_common(outer->domain_);
  multicast_address_ = outer->config_->multicast_address(port_common, outer->domain_);

#ifdef ACE_HAS_IPV6
  multicast_ipv6_address_ = outer->config_->ipv6_multicast_address(port_common);
#endif

  send_addrs_.insert(multicast_address_);
#ifdef ACE_HAS_IPV6
  send_addrs_.insert(multicast_ipv6_address_);
#endif

  const DCPS::NetworkAddressSet addrs = outer->config_->spdp_send_addrs();
  send_addrs_.insert(addrs.begin(), addrs.end());

  u_short participantId = 0;

#ifdef OPENDDS_SAFETY_PROFILE
  const u_short startingParticipantId = participantId;
#endif

  const u_short max_part_id = 119; // RTPS 2.5 9.6.2.3
  while (!open_unicast_socket(port_common, participantId)) {
    if (participantId == max_part_id && log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Spdp::SpdpTransport: "
        "participant id is going above max %u allowed by RTPS spec\n", max_part_id));
      // As long as it doesn't result in an invalid port, going past this
      // shouldn't cause a problem, but it could be a sign that OpenDDS has a
      // limited number of ports at its disposal. Also another implementation
      // could use this as a hard limit, but that's much less of a concern.
    }
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
    outer->guid_.guidPrefix[8] = hdr_.guidPrefix[8];
    outer->guid_.guidPrefix[9] = hdr_.guidPrefix[9];
  }
#endif

  const ACE_CDR::ULong userTag = outer->config_->spdp_user_tag();
  if (userTag) {
    user_tag_.smHeader.submessageId = SUBMESSAGE_KIND_USER_TAG;
    user_tag_.smHeader.flags = FLAG_E;
    user_tag_.smHeader.submessageLength = DCPS::uint32_cdr_size;
    user_tag_.userTag = userTag;
  } else {
    user_tag_.smHeader.submessageId = 0;
    user_tag_.smHeader.flags = 0;
    user_tag_.smHeader.submessageLength = 0;
    user_tag_.userTag = 0;
  }
}

void
Spdp::SpdpTransport::open(const DCPS::ReactorTask_rch& reactor_task,
                          const DCPS::JobQueue_rch& job_queue)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

#ifdef OPENDDS_SECURITY
  // Add the endpoint before any sending and receiving occurs.
  DCPS::WeakRcHandle<ICE::Endpoint> endpoint = get_ice_endpoint();
  if (endpoint) {
    outer->ice_agent_->add_endpoint(endpoint);
    ice_endpoint_added_ = true;
    outer->ice_agent_->add_local_agent_info_listener(endpoint, outer->guid_, DCPS::static_rchandle_cast<ICE::AgentInfoListener>(outer));
  }
#endif

  reactor(reactor_task->get_reactor());
  reactor_task->interceptor()->execute_or_enqueue(DCPS::make_rch<RegisterHandlers>(rchandle_from(this), reactor_task));

#ifdef OPENDDS_SECURITY
  // Now that the endpoint is added, SEDP can write the SPDP info.
  if (outer->is_security_enabled()) {
    outer->write_secure_updates();
  }
#endif

  local_send_task_ = DCPS::make_rch<SpdpMulti>(reactor_task->interceptor(), outer->config_->resend_period(), rchandle_from(this), &SpdpTransport::send_local);

  if (outer->config_->periodic_directed_spdp()) {
    directed_send_task_ =
      DCPS::make_rch<SpdpSporadic>(TheServiceParticipant->time_source(), reactor_task->interceptor(),
                                   rchandle_from(this), &SpdpTransport::send_directed);
  }

  lease_expiration_task_ =
    DCPS::make_rch<SpdpSporadic>(TheServiceParticipant->time_source(), reactor_task->interceptor(),
                                 rchandle_from(this), &SpdpTransport::process_lease_expirations);

#ifdef OPENDDS_SECURITY
  handshake_deadline_task_ =
    DCPS::make_rch<SpdpSporadic>(TheServiceParticipant->time_source(), reactor_task->interceptor(),
                                 rchandle_from(this), &SpdpTransport::process_handshake_deadlines);
  handshake_resend_task_ =
    DCPS::make_rch<SpdpSporadic>(TheServiceParticipant->time_source(), reactor_task->interceptor(),
                                 rchandle_from(this), &SpdpTransport::process_handshake_resends);

  relay_spdp_task_ =
    DCPS::make_rch<SpdpSporadic>(TheServiceParticipant->time_source(), reactor_task->interceptor(),
                                 rchandle_from(this), &SpdpTransport::send_relay);
  relay_stun_task_ =
    DCPS::make_rch<SpdpSporadic>(TheServiceParticipant->time_source(), reactor_task->interceptor(),
                                 rchandle_from(this), &SpdpTransport::relay_stun_task);
#endif

#ifndef DDS_HAS_MINIMUM_BIT
  // internal thread bit reporting
  if (TheServiceParticipant->get_thread_status_manager().update_thread_status()) {
    thread_status_task_ = DCPS::make_rch<SpdpPeriodic>(reactor_task->interceptor(), ref(*this), &SpdpTransport::thread_status_task);
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  // Connect the listeners last so that the tasks are created.
  DCPS::ConfigListener::job_queue(job_queue);
  config_reader_ = DCPS::make_rch<DCPS::ConfigReader>(DCPS::ConfigStoreImpl::datareader_qos(), rchandle_from(this));
  TheServiceParticipant->config_topic()->connect(config_reader_);

  DCPS::InternalDataReaderListener<DCPS::NetworkInterfaceAddress>::job_queue(job_queue);
  network_interface_address_reader_ = DCPS::make_rch<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> >(DCPS::DataReaderQosBuilder().reliability_reliable().durability_transient_local(), rchandle_from(this));
  TheServiceParticipant->network_interface_address_topic()->connect(network_interface_address_reader_);
}

Spdp::SpdpTransport::~SpdpTransport()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SpdpTransport::~SpdpTransport\n")));
  }

  DCPS::RcHandle<Spdp> outer = outer_.lock();

  if (outer) {
    ACE_GUARD(ACE_Thread_Mutex, g, outer->lock_);
    try {
      dispose_unregister();
    } catch (const CORBA::BAD_PARAM&) {}
    outer->eh_shutdown_ = true;
    outer->shutdown_cond_.notify_all();
  }

  unicast_socket_.close();
  multicast_socket_.close();
#ifdef ACE_HAS_IPV6
  unicast_ipv6_socket_.close();
  multicast_ipv6_socket_.close();
#endif
}

void Spdp::SpdpTransport::register_unicast_socket(
  ACE_Reactor* reactor, ACE_SOCK_Dgram& socket, const char* what)
{
#ifdef ACE_WIN32
  // By default Winsock will cause reads to fail with "connection reset"
  // when UDP sends result in ICMP "port unreachable" messages.
  // The transport framework is not set up for this since returning <= 0
  // from our receive_bytes causes the framework to close down the datalink
  // which in this case is used to receive from multiple peers.
  {
    BOOL recv_udp_connreset = FALSE;
    socket.control(SIO_UDP_CONNRESET, &recv_udp_connreset);
  }
#endif

  if (reactor->register_handler(socket.get_handle(),
                                this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error(
      (DCPS::String("failed to register ") + what + " unicast input handler").c_str());
  }
}

void Spdp::SpdpTransport::register_handlers(const DCPS::ReactorTask_rch& reactor_task)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) {
    return;
  }
  ACE_GUARD(ACE_Thread_Mutex, g, outer->lock_);

  if (outer->shutdown_flag_) {
    return;
  }

  ACE_Reactor* const reactor = reactor_task->get_reactor();
  register_unicast_socket(reactor, unicast_socket_, "IPV4");
#ifdef ACE_HAS_IPV6
  register_unicast_socket(reactor, unicast_ipv6_socket_, "IPV6");
#endif
}

void
Spdp::SpdpTransport::enable_periodic_tasks()
{
  if (local_send_task_) {
    local_send_task_->enable(TimeDuration::zero_value);
  }

#ifdef OPENDDS_SECURITY
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  outer->sedp_->core().reset_relay_spdp_task_falloff();
  relay_spdp_task_->schedule(TimeDuration::zero_value);

  outer->sedp_->core().reset_relay_stun_task_falloff();
  relay_stun_task_->schedule(TimeDuration::zero_value);
#endif

#ifndef DDS_HAS_MINIMUM_BIT
  const DCPS::ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  if (thread_status_manager.update_thread_status()) {
    thread_status_task_->enable(false, thread_status_manager.thread_status_interval());
  }
#endif /* DDS_HAS_MINIMUM_BIT */
}

void
Spdp::SpdpTransport::dispose_unregister()
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  // Send the dispose/unregister SPDP sample
  data_.writerSN = to_rtps_seqnum(seq_);
  data_.smHeader.flags = FLAG_E | FLAG_Q | FLAG_K_IN_DATA;
  data_.inlineQos.length(1);
  static const StatusInfo_t dispose_unregister = { {0, 0, 0, 3} };
  data_.inlineQos[0].status_info(dispose_unregister);

  ParameterList plist(1);
  plist.length(1);
  plist[0].guid(outer->guid_);
  plist[0]._d(PID_PARTICIPANT_GUID);

  wbuff_.reset();
  DCPS::Serializer ser(&wbuff_, encoding_plain_native);
  DCPS::EncapsulationHeader encap(ser.encoding(), DCPS::MUTABLE);
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap) || !(ser << plist)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::dispose_unregister() - ")
        ACE_TEXT("failed to serialize headers for dispose/unregister\n")));
    }
    return;
  }

  send(SEND_MULTICAST | SEND_RELAY);
}

void
Spdp::SpdpTransport::close(const DCPS::ReactorTask_rch& reactor_task)
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SpdpTransport::close\n")));
  }

  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  TheServiceParticipant->network_interface_address_topic()->disconnect(network_interface_address_reader_);

#ifdef OPENDDS_SECURITY
  DCPS::WeakRcHandle<ICE::Endpoint> endpoint = get_ice_endpoint();
  if (endpoint) {
    outer->ice_agent_->remove_endpoint(endpoint);
    ice_endpoint_added_ = false;
  }

  if (handshake_deadline_task_) {
    handshake_deadline_task_->cancel();
  }
  if (handshake_resend_task_) {
    handshake_resend_task_->cancel();
  }
  if (relay_spdp_task_) {
    relay_spdp_task_->cancel();
  }
  if (relay_stun_task_) {
    relay_stun_task_->cancel();
  }
#endif
  if (local_send_task_) {
    local_send_task_->disable();
  }
  if (directed_send_task_) {
    directed_send_task_->cancel();
  }
  if (lease_expiration_task_) {
    lease_expiration_task_->cancel();
  }
  if (thread_status_task_) {
    thread_status_task_->disable();
  }

  ACE_Reactor* reactor = reactor_task->get_reactor();
  const ACE_Reactor_Mask mask =
    ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL;
  reactor->remove_handler(multicast_socket_.get_handle(), mask);
  reactor->remove_handler(unicast_socket_.get_handle(), mask);
#ifdef ACE_HAS_IPV6
  reactor->remove_handler(multicast_ipv6_socket_.get_handle(), mask);
  reactor->remove_handler(unicast_ipv6_socket_.get_handle(), mask);
#endif

  if (config_reader_) {
    TheServiceParticipant->config_topic()->disconnect(config_reader_);
  }
}

void
Spdp::SpdpTransport::shorten_local_sender_delay_i()
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  if (local_send_task_) {
    const TimeDuration quick_resend = outer->resend_period_ * outer->quick_resend_ratio_;
    local_send_task_->enable(std::max(quick_resend, outer->min_resend_delay_));
  }
}

void
Spdp::SpdpTransport::write(WriteFlags flags)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  ACE_GUARD(ACE_Thread_Mutex, g, outer->lock_);
  write_i(flags);
}

void
Spdp::SpdpTransport::write_i(WriteFlags flags)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  if (!outer->undirected_spdp_) {
    return;
  }

  const ParticipantData_t pdata = outer->build_local_pdata(
#ifdef OPENDDS_SECURITY
    true, outer->is_security_enabled() ? Security::DPDK_ENHANCED : Security::DPDK_ORIGINAL
#endif
  );

  data_.writerSN = to_rtps_seqnum(seq_);
  ++seq_;

  ParameterList plist;
  if (!ParameterListConverter::to_param_list(pdata, plist)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Spdp::SpdpTransport::write_i: ")
        ACE_TEXT("failed to convert from SPDPdiscoveredParticipantData ")
        ACE_TEXT("to ParameterList\n")));
    }
    return;
  }

#ifdef OPENDDS_SECURITY
  if (!outer->is_security_enabled()) {
    ICE::AgentInfoMap ai_map;
    DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = outer->sedp_->get_ice_endpoint();
    if (sedp_endpoint) {
      ai_map[SEDP_AGENT_INFO_KEY] = outer->ice_agent_->get_local_agent_info(sedp_endpoint);
    }
    DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint = get_ice_endpoint();
    if (spdp_endpoint) {
      ai_map[SPDP_AGENT_INFO_KEY] = outer->ice_agent_->get_local_agent_info(spdp_endpoint);
    }

    if (!ParameterListConverter::to_param_list(ai_map, plist)) {
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                  ACE_TEXT("Spdp::SpdpTransport::write_i: ")
                  ACE_TEXT("failed to convert from ICE::AgentInfo ")
                  ACE_TEXT("to ParameterList\n")));
      }
      return;
    }
  }
#endif

  wbuff_.reset();
  DCPS::Serializer ser(&wbuff_, encoding_plain_native);
  DCPS::EncapsulationHeader encap(ser.encoding(), DCPS::MUTABLE);
  if (!(ser << hdr_)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write_i: ")
        ACE_TEXT("failed to serialize RTPS header for SPDP\n")));
    }
    return;
  }
  // The implementation-specific UserTagSubmessage is designed to directly
  // follow the RTPS Message Header.  No other submessages should be added
  // before it.  This enables filtering based on a fixed offset.
  if (user_tag_.smHeader.submessageId && !(ser << user_tag_)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write_i: ")
        ACE_TEXT("failed to serialize user tag for SPDP\n")));
    }
    return;
  }
  if (!(ser << data_) || !(ser << encap) || !(ser << plist)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write_i: ")
        ACE_TEXT("failed to serialize data submessage for SPDP\n")));
    }
    return;
  }

  send(flags);
}

void
Spdp::update_rtps_relay_application_participant_i(DiscoveredParticipantIter iter, bool new_participant)
{
  if (!iter->second.pdata_.participantProxy.opendds_rtps_relay_application_participant) {
    return;
  }

  if (new_participant) {
#ifdef OPENDDS_SECURITY
    tport_->relay_spdp_task_->cancel();
    sedp_->core().reset_relay_spdp_task_falloff();
    tport_->relay_spdp_task_->schedule(TimeDuration::zero_value);
#endif
  }

  if (DCPS::DCPS_debug_level) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Spdp::update_rtps_relay_application_participant - %C is an RtpsRelay application participant\n"),
               DCPS::LogGuid(iter->first).c_str()));
  }

  for (DiscoveredParticipantIter pos = participants_.begin(), limit = participants_.end(); pos != limit;) {
    if (pos != iter && pos->second.pdata_.participantProxy.opendds_rtps_relay_application_participant) {
      if (DCPS::DCPS_debug_level) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) Spdp::update_rtps_relay_application_participant - removing previous RtpsRelay application participant %C\n"),
                   DCPS::LogGuid(pos->first).c_str()));
      }
      purge_discovered_participant(pos);
      participants_.erase(pos++);
    } else {
      ++pos;
    }
  }
}

void
Spdp::append_transport_statistics(DCPS::TransportStatisticsSequence& seq)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    sedp_->core().append_transport_statistics(seq);
  }
  sedp_->append_transport_statistics(seq);
}

void
Spdp::SpdpTransport::write_i(const DCPS::GUID_t& guid, const DCPS::NetworkAddress& local_address, WriteFlags flags)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  const ParticipantData_t pdata = outer->build_local_pdata(
#ifdef OPENDDS_SECURITY
    true, outer->is_security_enabled() ? Security::DPDK_ENHANCED : Security::DPDK_ORIGINAL
#endif
  );

  data_.writerSN = to_rtps_seqnum(seq_);
  ++seq_;

  ParameterList plist;
  if (!ParameterListConverter::to_param_list(pdata, plist)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
        ACE_TEXT("Spdp::SpdpTransport::write_i() - ")
        ACE_TEXT("failed to convert from SPDPdiscoveredParticipantData ")
        ACE_TEXT("to ParameterList\n")));
    }
    return;
  }

#ifdef OPENDDS_SECURITY
  if (!outer->is_security_enabled()) {
    ICE::AgentInfoMap ai_map;
    DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = outer->sedp_->get_ice_endpoint();
    if (sedp_endpoint) {
      ai_map[SEDP_AGENT_INFO_KEY] = outer->ice_agent_->get_local_agent_info(sedp_endpoint);
    }
    DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint = get_ice_endpoint();
    if (spdp_endpoint) {
      ai_map[SPDP_AGENT_INFO_KEY] = outer->ice_agent_->get_local_agent_info(spdp_endpoint);
    }

    if (!ParameterListConverter::to_param_list(ai_map, plist)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("Spdp::SpdpTransport::write_i() - ")
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
  DCPS::assign(info_dst.guidPrefix, guid.guidPrefix);

  wbuff_.reset();
  DCPS::Serializer ser(&wbuff_, encoding_plain_native);
  DCPS::EncapsulationHeader encap(ser.encoding(), DCPS::MUTABLE);
  if (!(ser << hdr_) || !(ser << info_dst) || !(ser << data_) || !(ser << encap)
      || !(ser << plist)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write_i() - ")
        ACE_TEXT("failed to serialize headers for SPDP\n")));
    }
    return;
  }

  send(flags, local_address);
}

void
Spdp::SpdpTransport::send(WriteFlags flags, const DCPS::NetworkAddress& local_address)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  if ((flags & SEND_MULTICAST) && !outer->sedp_->core().rtps_relay_only()) {
    typedef DCPS::NetworkAddressSet::const_iterator iter_t;
    for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
      send(*iter);
    }
  }

  if (((flags & SEND_DIRECT) && !outer->sedp_->core().rtps_relay_only()) &&
      local_address) {
    send(local_address);
  }

  if ((flags & SEND_RELAY) || outer->sedp_->core().rtps_relay_only()) {
    const DCPS::NetworkAddress relay_address = outer->sedp_->core().spdp_rtps_relay_address();
    if (relay_address) {
      send(relay_address);
    }
  }
}

const ACE_SOCK_Dgram&
Spdp::SpdpTransport::choose_send_socket(const DCPS::NetworkAddress& addr) const
{
#ifdef ACE_HAS_IPV6
  if (addr.get_type() == AF_INET6) {
    return unicast_ipv6_socket_;
  }
#endif
  ACE_UNUSED_ARG(addr);
  return unicast_socket_;
}

ssize_t
Spdp::SpdpTransport::send(const DCPS::NetworkAddress& addr)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return -1;

#ifdef OPENDDS_TESTING_FEATURES
  if (outer->sedp_->core().should_drop(wbuff_.length())) {
    return wbuff_.length();
  }
#endif

  const ACE_SOCK_Dgram& socket = choose_send_socket(addr);
  const ssize_t res = socket.send(wbuff_.rd_ptr(), wbuff_.length(), addr.to_addr());
  outer->sedp_->core().writer_resend_count(make_id(outer->guid_, ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER), 1);
  if (res < 0) {
    outer->sedp_->core().send_fail(addr, DCPS::MCK_RTPS, wbuff_.length());
    const int err = errno;
    if (err != ENETUNREACH || !network_is_unreachable_) {
      errno = err;
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_WARNING,
                  ACE_TEXT("(%P|%t) WARNING: Spdp::SpdpTransport::send() - ")
                  ACE_TEXT("destination %C failed send: %m\n"), DCPS::LogAddr(addr).c_str()));
      }
    }
    if (err == ENETUNREACH) {
      network_is_unreachable_ = true;
    }
  } else {
    outer->sedp_->core().send(addr, DCPS::MCK_RTPS, wbuff_.length());
    network_is_unreachable_ = false;
  }

  return res;
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

bool valid_size(const ACE_INET_Addr& a)
{
  return a.get_size() <= a.get_addr_size();
}

int
Spdp::SpdpTransport::handle_input(ACE_HANDLE h)
{
  DCPS::ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

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

  if (!valid_size(remote)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - invalid address size\n")));
    return 0;
  }

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

  DCPS::RcHandle<Spdp> outer = outer_.lock();

  if (!outer) {
    return 0;
  }

  const DCPS::NetworkAddress remote_na(remote);

  // Ignore messages from the relay when not using it.
  if (outer->sedp_->core().ignore_from_relay(remote_na)) {
    return 0;
  }

  if ((buff_.size() >= 4) && ACE_OS::memcmp(buff_.rd_ptr(), "RTPS", 4) == 0) {
    RTPS::Message message;

    DCPS::Serializer ser(&buff_, encoding_plain_native);
    Header header;
    if (!(ser >> header)) {
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
                  ACE_TEXT("failed to deserialize RTPS header for SPDP\n")));
      }
      return 0;
    }

    outer->sedp_->core().recv(remote_na, DCPS::MCK_RTPS, bytes);

    if (DCPS::transport_debug.log_messages) {
      message.hdr = header;
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
          if (DCPS::DCPS_debug_level > 0) {
            ACE_ERROR((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
                      ACE_TEXT("failed to deserialize DATA header for SPDP\n")));
          }
          return 0;
        }
        submessageLength = data.smHeader.submessageLength;

        if (DCPS::transport_debug.log_messages) {
          append_submessage(message, data);
        }

        if (data.writerId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER) {
          // Not our message: this could be the same multicast group used
          // for SEDP and other traffic.
          break;
        }

        ParameterList plist;
        if (data.smHeader.flags & (FLAG_D | FLAG_K_IN_DATA)) {
          DCPS::EncapsulationHeader encap;
          DCPS::Encoding enc;
          if (!(ser >> encap) || !encap.to_encoding(enc, DCPS::MUTABLE) || enc.kind() != Encoding::KIND_XCDR1) {
            if (DCPS::DCPS_debug_level > 0) {
              ACE_ERROR((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
                        ACE_TEXT("failed to deserialize encapsulation header for SPDP\n")));
            }
            return 0;
          }
          ser.encoding(enc);
          if (!(ser >> plist)) {

            if (DCPS::DCPS_debug_level > 0) {
              ACE_ERROR((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
                        ACE_TEXT("failed to deserialize data payload for SPDP\n")));
            }
            return 0;
          }
        } else {
          plist.length(1);
          const GUID_t guid = make_id(header.guidPrefix, ENTITYID_PARTICIPANT);
          plist[0].guid(guid);
          plist[0]._d(PID_PARTICIPANT_GUID);
        }

        DCPS::RcHandle<Spdp> outer = outer_.lock();
        if (outer) {
          outer->data_received(data, plist, remote_na);
        }
        break;
      }
      case INFO_DST: {
        if (DCPS::transport_debug.log_messages) {
          InfoDestinationSubmessage sm;
          if (!(ser >> sm)) {
            if (DCPS::DCPS_debug_level > 0) {
              ACE_ERROR((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
                        ACE_TEXT("failed to deserialize INFO_DST header for SPDP\n")));
            }
            return 0;
          }
          submessageLength = sm.smHeader.submessageLength;
          append_submessage(message, sm);
          break;
        }
      }
      // fallthrough
      default:
        SubmessageHeader smHeader;
        if (!(ser >> smHeader)) {
          if (DCPS::DCPS_debug_level > 0) {
            ACE_ERROR((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
                      ACE_TEXT("failed to deserialize SubmessageHeader for SPDP\n")));
          }
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
            if (DCPS::DCPS_debug_level > 0) {
              ACE_ERROR((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
                        ACE_TEXT("failed to skip sub message length\n")));
            }
            return 0;
          }
        }
      } else if (!submessageLength) {
        break; // submessageLength of 0 indicates the last submessage
      }
    }

  } else if ((buff_.size() >= 4) && (ACE_OS::memcmp(buff_.rd_ptr(), "RTPX", 4) == 0)) {
    // Handle some RTI protocol multicast to the same address
    return 0; // Ignore
  }

#ifdef OPENDDS_SECURITY
  // Assume STUN
  if (!outer->initialized() || outer->shutting_down()) {
    return 0;
  }

#ifndef ACE_RECVPKTINFO
  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() ")
             ACE_TEXT("potential STUN message received but this version of the ACE ")
             ACE_TEXT("library doesn't support the local_address extension in ")
             ACE_TEXT("ACE_SOCK_Dgram::recv\n")));
  ACE_NOTSUP_RETURN(0);
#else

  DCPS::Serializer serializer(&buff_, STUN::encoding);
  STUN::Message message;
  message.block = &buff_;
  if (serializer >> message) {
    outer->sedp_->core().recv(remote_na, DCPS::MCK_STUN, bytes);

    if (relay_srsm_.is_response(message)) {
      process_relay_sra(relay_srsm_.receive(message));
    } else {
      DCPS::WeakRcHandle<ICE::Endpoint> endpoint = get_ice_endpoint();
      if (endpoint) {
        outer->ice_agent_->receive(endpoint, local, remote, message);
      }
    }
  }
#endif
#endif

  return 0;
}

DCPS::WeakRcHandle<ICE::Endpoint>
Spdp::SpdpTransport::get_ice_endpoint()
{
#ifdef OPENDDS_SECURITY
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  return outer && outer->sedp_->core().use_ice() ? DCPS::static_rchandle_cast<ICE::Endpoint>(rchandle_from(this)) : DCPS::WeakRcHandle<ICE::Endpoint>();
#else
  return DCPS::WeakRcHandle<ICE::Endpoint>();
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
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  DCPS::RcHandle<DCPS::JobQueue> job_queue = outer->sedp_->job_queue();
  if (job_queue) {
    job_queue->enqueue(DCPS::make_rch<SendStun>(rchandle_from(this), DCPS::NetworkAddress(address), message));
  }
}

void
Spdp::SendStun::execute()
{
  DCPS::RcHandle<SpdpTransport> tport = tport_.lock();
  if (!tport) return;

  DCPS::RcHandle<Spdp> outer = tport->outer_.lock();
  if (!outer) return;

  ACE_GUARD(ACE_Thread_Mutex, g, outer->lock_);
  tport->wbuff_.reset();
  Serializer serializer(&tport->wbuff_, STUN::encoding);
  const_cast<STUN::Message&>(message_).block = &tport->wbuff_;
  serializer << message_;

#ifdef OPENDDS_TESTING_FEATURES
  if (outer->sedp_->core().should_drop(tport->wbuff_.length())) {
    return;
  }
#endif

  const ACE_SOCK_Dgram& socket = tport->choose_send_socket(address_);
  const ssize_t res = socket.send(tport->wbuff_.rd_ptr(), tport->wbuff_.length(), address_.to_addr());
  if (res < 0) {
    outer->sedp_->core().send_fail(address_, DCPS::MCK_STUN, tport->wbuff_.length());
    const int err = errno;
    if (err != ENETUNREACH || !tport->network_is_unreachable_) {
      errno = err;
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_WARNING,
                  ACE_TEXT("(%P|%t) WARNING: Spdp::SendStun::execute() - ")
                  ACE_TEXT("destination %C failed send: %m\n"), DCPS::LogAddr(address_).c_str()));
      }
    }
    if (err == ENETUNREACH) {
      tport->network_is_unreachable_ = true;
    }
  } else {
    outer->sedp_->core().send(address_, DCPS::MCK_STUN, tport->wbuff_.length());
    tport->network_is_unreachable_ = false;
  }
}

ACE_INET_Addr
Spdp::SpdpTransport::stun_server_address() const
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  return outer ? outer->sedp_->core().spdp_stun_server_address().to_addr() : ACE_INET_Addr();
}

#ifndef DDS_HAS_MINIMUM_BIT
void
Spdp::SpdpTransport::ice_connect(const ICE::GuidSetType& guids, const ACE_INET_Addr& addr)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  outer->sedp_->job_queue()->enqueue(DCPS::make_rch<IceConnect>(outer, guids, DCPS::NetworkAddress(addr), true));
}

void
Spdp::IceConnect::execute()
{
  ACE_GUARD(ACE_Thread_Mutex, g, spdp_->lock_);
  for (ICE::GuidSetType::const_iterator pos = guids_.begin(), limit = guids_.end(); pos != limit; ++pos) {
    DiscoveredParticipantIter iter = spdp_->participants_.find(pos->remote);
    if (iter != spdp_->participants_.end()) {
      spdp_->enqueue_location_update_i(iter, compute_ice_location_mask(addr_), connect_ ? addr_ : DCPS::NetworkAddress(), "ICE connect");
      spdp_->process_location_updates_i(iter, "ICE connect");
    }
  }
}

void
Spdp::SpdpTransport::ice_disconnect(const ICE::GuidSetType& guids, const ACE_INET_Addr& addr)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  outer->sedp_->job_queue()->enqueue(DCPS::make_rch<IceConnect>(outer, guids, DCPS::NetworkAddress(addr), false));
}
#endif /* DDS_HAS_MINIMUM_BIT */
#endif /* OPENDDS_SECURITY */

void
Spdp::signal_liveliness(DDS::LivelinessQosPolicyKind kind)
{
  sedp_->signal_liveliness(kind);
}

bool
Spdp::SpdpTransport::open_unicast_socket(u_short port_common,
                                         u_short participant_id)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) {
    throw std::runtime_error("couldn't get Spdp");
  }

  DCPS::NetworkAddress local_addr = outer->config_->spdp_local_address();
  const bool fixed_port = local_addr.get_port_number();

  if (fixed_port) {
    uni_port_ = local_addr.get_port_number();
  } else if (!outer->config_->spdp_request_random_port()) {
    const ACE_UINT32 port = static_cast<ACE_UINT32>(port_common) + outer->config_->d1() +
      outer->config_->pg() * participant_id;
    if (port > 65535) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket: "
                   "port %u is too high\n", port));
      }
      throw std::runtime_error("failed to open unicast port for SPDP (port too high)");
    }
    uni_port_ = static_cast<unsigned short>(port);
    local_addr.set_port_number(uni_port_);
  }

  if (unicast_socket_.open(local_addr.to_addr(), PF_INET) != 0) {
    if (fixed_port) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket: "
                   "failed to open %C %p.\n",
                   LogAddr(local_addr).c_str(), ACE_TEXT("ACE_SOCK_Dgram::open")));
      }
      throw std::runtime_error("failed to open unicast port for SPDP");
    }
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
                 ACE_TEXT("failed to open %C %p.  ")
                 ACE_TEXT("Trying next participantId...\n"),
                 DCPS::LogAddr(local_addr).c_str(), ACE_TEXT("ACE_SOCK_Dgram::open")));
    }
    return false;
  }

  if (!fixed_port && outer->config_->spdp_request_random_port()) {
    ACE_INET_Addr addr;
    if (unicast_socket_.get_local_addr(addr) == 0) {
      uni_port_ = addr.get_port_number();
    }
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
               ACE_TEXT("opened unicast socket on port %d\n"),
               uni_port_));
  }

  if (!DCPS::set_socket_multicast_ttl(unicast_socket_, outer->config_->ttl())) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket() - ")
                ACE_TEXT("failed to set TTL value to %d ")
                ACE_TEXT("for port:%hu %p\n"),
                outer->config_->ttl(), uni_port_, ACE_TEXT("DCPS::set_socket_multicast_ttl:")));
    }
    throw std::runtime_error("failed to set TTL");
  }

  const int send_buffer_size = outer->config()->send_buffer_size();
  if (send_buffer_size > 0) {
    if (unicast_socket_.set_option(SOL_SOCKET,
                                   SO_SNDBUF,
                                   (void *) &send_buffer_size,
                                   sizeof(send_buffer_size)) < 0
        && errno != ENOTSUP) {
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket() - failed to set the send buffer size to %d errno %m\n"), send_buffer_size));
      }
      throw std::runtime_error("failed to set send buffer size");
    }
  }

  const int recv_buffer_size = outer->config()->recv_buffer_size();
  if (recv_buffer_size > 0) {
    if (unicast_socket_.set_option(SOL_SOCKET,
                                   SO_RCVBUF,
                                   (void *) &recv_buffer_size,
                                   sizeof(recv_buffer_size)) < 0
        && errno != ENOTSUP) {
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket() - failed to set the recv buffer size to %d errno %m\n"), recv_buffer_size));
      }
      throw std::runtime_error("failed to set recv buffer size");
    }
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
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return false;

  DCPS::NetworkAddress local_addr = outer->config_->ipv6_spdp_local_address();
  const bool fixed_port = local_addr.get_port_number();

  if (fixed_port) {
    ipv6_uni_port_ = local_addr.get_port_number();
  } else {
    ipv6_uni_port_ = port;
    local_addr.set_port_number(ipv6_uni_port_);
  }

  if (unicast_ipv6_socket_.open(local_addr.to_addr(), PF_INET6) != 0) {
    if (fixed_port) {
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                  ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_ipv6_socket() - ")
                  ACE_TEXT("failed to open %C %p.\n"),
                  DCPS::LogAddr(local_addr).c_str(), ACE_TEXT("ACE_SOCK_Dgram::open")));
      }
      throw std::runtime_error("failed to open ipv6 unicast port for SPDP");
    }
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_ipv6_socket() - ")
                 ACE_TEXT("failed to open %C %p.  ")
                 ACE_TEXT("Trying next port...\n"),
                 DCPS::LogAddr(local_addr).c_str(), ACE_TEXT("ACE_SOCK_Dgram::open")));
    }
    return false;
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_ipv6_socket() - ")
               ACE_TEXT("opened unicast ipv6 socket on port %d\n"),
               ipv6_uni_port_));
  }

  if (!DCPS::set_socket_multicast_ttl(unicast_ipv6_socket_, outer->config_->ttl())) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_ipv6_socket() - ")
                ACE_TEXT("failed to set TTL value to %d ")
                ACE_TEXT("for port:%hu %p\n"),
                outer->config_->ttl(), ipv6_uni_port_, ACE_TEXT("DCPS::set_socket_multicast_ttl:")));
    }
    throw std::runtime_error("failed to set TTL");
  }

  const int send_buffer_size = outer->config()->send_buffer_size();
  if (send_buffer_size > 0) {
    if (unicast_ipv6_socket_.set_option(SOL_SOCKET,
                                        SO_SNDBUF,
                                        (void *) &send_buffer_size,
                                        sizeof(send_buffer_size)) < 0
        && errno != ENOTSUP) {
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_ipv6_socket() - failed to set the send buffer size to %d errno %m\n"), send_buffer_size));
      }
      throw std::runtime_error("failed to set send buffer size");
    }
  }

  const int recv_buffer_size = outer->config()->recv_buffer_size();
  if (recv_buffer_size > 0) {
    if (unicast_ipv6_socket_.set_option(SOL_SOCKET,
                                        SO_RCVBUF,
                                        (void *) &recv_buffer_size,
                                        sizeof(recv_buffer_size)) < 0
        && errno != ENOTSUP) {
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_ipv6_socket() - failed to set the recv buffer size to %d errno %m\n"), recv_buffer_size));
      }
      throw std::runtime_error("failed to set recv buffer size");
    }
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

void Spdp::SpdpTransport::on_data_available(DCPS::RcHandle<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> >)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  ACE_GUARD(ACE_Thread_Mutex, g, outer->lock_);
  if (outer->shutting_down()) {
    return;
  }

  if (outer->shutdown_flag_) {
    return;
  }

  DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;

  network_interface_address_reader_->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  if (multicast_manager_.process(samples,
                                 infos,
                                 multicast_interface_,
                                 reactor(),
                                 this,
                                 multicast_address_,
                                 multicast_socket_
#ifdef ACE_HAS_IPV6
                                 , multicast_ipv6_address_,
                                 multicast_ipv6_socket_
#endif
                                 )) {
    shorten_local_sender_delay_i();
  }
}

void Spdp::SpdpTransport::on_data_available(DCPS::ConfigReader_rch)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  ACE_GUARD(ACE_Thread_Mutex, g, outer->lock_);
  if (outer->shutting_down()) {
    return;
  }

  if (outer->shutdown_flag_) {
    return;
  }

  DCPS::RcHandle<RtpsDiscoveryConfig> config = outer->config();
  RtpsDiscoveryCore& core = outer->sedp_->core();

  const String& config_prefix = config->config_prefix();
  bool has_prefix = false;

  DCPS::InternalDataReader<DCPS::ConfigPair>::SampleSequence samples;
  DCPS::InternalSampleInfoSequence infos;
  config_reader_->take(samples, infos, DDS::LENGTH_UNLIMITED,
                       DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const DCPS::ConfigPair& sample = samples[idx];

    if (sample.key_has_prefix(config_prefix)) {
      has_prefix = true;
#ifdef OPENDDS_SECURITY
      if (config->config_key("RTPS_RELAY_ONLY") == sample.key()) {
        const bool flag = config->rtps_relay_only();
        core.rtps_relay_only(flag);
        outer->sedp_->rtps_relay_only_now(flag);

        if (flag) {
          core.reset_relay_spdp_task_falloff();
          relay_spdp_task_->schedule(TimeDuration::zero_value);

          core.reset_relay_stun_task_falloff();
          relay_stun_task_->schedule(TimeDuration::zero_value);

#ifndef DDS_HAS_MINIMUM_BIT
          const DCPS::ParticipantLocation mask =
            DCPS::LOCATION_LOCAL |
            DCPS::LOCATION_LOCAL6 |
            DCPS::LOCATION_ICE |
            DCPS::LOCATION_ICE6;

          for (DiscoveredParticipantIter iter = outer->participants_.begin();
               iter != outer->participants_.end();
               ++iter) {
            outer->enqueue_location_update_i(iter, mask, DCPS::NetworkAddress(), "rtps_relay_only_now");
            outer->process_location_updates_i(iter, "rtps_relay_only_now");
          }
#endif
        } else {
          if (!core.use_rtps_relay()) {
            if (relay_spdp_task_) {
              relay_spdp_task_->cancel();
            }
            if (relay_stun_task_) {
              disable_relay_stun_task();
            }
          }
        }
      } else if (config->config_key("USE_RTPS_RELAY") == sample.key()) {
        const bool flag = config->use_rtps_relay();
        core.use_rtps_relay(flag);
        outer->sedp_->use_rtps_relay_now(flag);

        if (flag) {
          core.reset_relay_spdp_task_falloff();
          relay_spdp_task_->schedule(TimeDuration::zero_value);

          core.reset_relay_stun_task_falloff();
          relay_stun_task_->schedule(TimeDuration::zero_value);
        } else {
          if (!core.rtps_relay_only()) {
            if (relay_spdp_task_) {
              relay_spdp_task_->cancel();
            }
            if (relay_stun_task_) {
              disable_relay_stun_task();
            }
          }

#ifndef DDS_HAS_MINIMUM_BIT
          const DCPS::ParticipantLocation mask =
            DCPS::LOCATION_RELAY |
            DCPS::LOCATION_RELAY6;

          for (DiscoveredParticipantIter iter = outer->participants_.begin();
               iter != outer->participants_.end();
               ++iter) {
            outer->enqueue_location_update_i(iter, mask, DCPS::NetworkAddress(), "use_rtps_relay_now");
            outer->process_location_updates_i(iter, "use_rtps_relay_now");
          }
#endif
        }
      } else if (config->config_key("USE_ICE") == sample.key()) {
        const bool flag = config->use_ice();
        core.use_ice(flag);
        outer->sedp_->use_ice_now(flag);

        if (flag) {
          DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint = get_ice_endpoint();
          DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = outer->sedp_->get_ice_endpoint();

          if (sedp_endpoint) {
            const GUID_t l = make_id(outer->guid_, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER);
            outer->ice_agent_->add_local_agent_info_listener(sedp_endpoint, l, DCPS::static_rchandle_cast<ICE::AgentInfoListener>(outer));
          }

          outer->ice_agent_->add_endpoint(DCPS::static_rchandle_cast<ICE::Endpoint>(rchandle_from(this)));
          ice_endpoint_added_ = true;
          if (spdp_endpoint) {
            outer->ice_agent_->add_local_agent_info_listener(spdp_endpoint, outer->guid_, DCPS::static_rchandle_cast<ICE::AgentInfoListener>(outer));
          }

          for (DiscoveredParticipantConstIter pos = outer->participants_.begin(), limit = outer->participants_.end(); pos != limit; ++pos) {
            if (spdp_endpoint && pos->second.have_spdp_info_) {
              outer->ice_agent_->start_ice(spdp_endpoint, outer->guid_, pos->first, pos->second.spdp_info_);
            }

            if (sedp_endpoint && pos->second.have_sedp_info_) {
              outer->start_ice(sedp_endpoint, pos->first, pos->second.pdata_.participantProxy.availableBuiltinEndpoints,
                               pos->second.pdata_.participantProxy.availableExtendedBuiltinEndpoints, pos->second.sedp_info_);
            }
          }
        } else {
          outer->ice_agent_->remove_endpoint(DCPS::static_rchandle_cast<ICE::Endpoint>(rchandle_from(this)));
          ice_endpoint_added_ = false;

#ifndef DDS_HAS_MINIMUM_BIT
          const DCPS::ParticipantLocation mask =
            DCPS::LOCATION_ICE |
            DCPS::LOCATION_ICE6;

          for (DiscoveredParticipantIter part = outer->participants_.begin();
               part != outer->participants_.end();
               ++part) {
            outer->enqueue_location_update_i(part, mask, DCPS::NetworkAddress(), "use_ice_now");
            outer->process_location_updates_i(part, "use_ice_now");
          }
#endif
        }

        if (outer->is_security_enabled()) {
          outer->write_secure_updates();
        }
      } else if (config->config_key("SPDP_RTPS_RELAY_ADDRESS") == sample.key()) {
        core.spdp_rtps_relay_address(config->spdp_rtps_relay_address());
        relay_spdp_task_->cancel();
        core.reset_relay_spdp_task_falloff();
        relay_spdp_task_->schedule(TimeDuration::zero_value);

        relay_stun_task_->cancel();
        core.reset_relay_stun_task_falloff();
        relay_stun_task_->schedule(TimeDuration::zero_value);
      } else if (config->config_key("SPDP_STUN_SERVER_ADDRESS") == sample.key()) {
        core.spdp_stun_server_address(config->spdp_stun_server_address());
      } else if (config->config_key("SEDP_RTPS_RELAY_ADDRESS") == sample.key()) {
        outer->sedp_->rtps_relay_address(config->sedp_rtps_relay_address());
      } else if (config->config_key("SEDP_STUN_SERVER_ADDRESS") == sample.key()) {
        outer->sedp_->stun_server_address(config->sedp_stun_server_address());
      }
#endif
    }
  }

  if (has_prefix) {
    core.reload(config_prefix);
  }
}


bool
Spdp::get_default_locators(const GUID_t& part_id, DCPS::LocatorSeq& target,
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
Spdp::get_last_recv_locator(const GUID_t& part_id, DCPS::LocatorSeq& target,
                            bool& inlineQos)
{
  DiscoveredParticipantIter pos = participants_.find(part_id);
  if (pos != participants_.end() && pos->second.last_recv_address_) {
    inlineQos = pos->second.pdata_.participantProxy.expectsInlineQos;
    target.length(1);
    DCPS::address_to_locator(target[0], pos->second.last_recv_address_.to_addr());
    return true;
  }
  return false;
}

bool
Spdp::associated() const
{
  return !participants_.empty();
}

bool
Spdp::has_discovered_participant(const DCPS::GUID_t& guid) const
{
  return participants_.find(guid) != participants_.end();
}

ACE_CDR::ULong Spdp::get_participant_flags(const DCPS::GUID_t& guid) const
{
  const DiscoveredParticipantMap::const_iterator iter = participants_.find(guid);
  if (iter == participants_.end()) {
    return PFLAGS_EMPTY;
  }
  return is_opendds(iter->second.pdata_.participantProxy)
    ? iter->second.pdata_.participantProxy.opendds_participant_flags.bits : PFLAGS_EMPTY;
}

void
Spdp::remove_lease_expiration_i(DiscoveredParticipantIter iter)
{
  for (std::pair<TimeQueue::iterator, TimeQueue::iterator> x = lease_expirations_.equal_range(iter->second.lease_expiration_);
       x.first != x.second; ++x.first) {
    if (x.first->second == iter->first) {
      lease_expirations_.erase(x.first);
      break;
    }
  }
}

void
Spdp::update_lease_expiration_i(DiscoveredParticipantIter iter,
                                const DCPS::MonotonicTimePoint& now)
{
  remove_lease_expiration_i(iter);

  // Compute new expiration.
  const DCPS::TimeDuration d =
    rtps_duration_to_time_duration(iter->second.pdata_.leaseDuration,
                                   iter->second.pdata_.participantProxy.protocolVersion,
                                   iter->second.pdata_.participantProxy.vendorId);

  iter->second.lease_expiration_ = now + d + lease_extension_;

  // Insert.
  const bool cancel = !lease_expirations_.empty() && iter->second.lease_expiration_ < lease_expirations_.begin()->first;
  const bool schedule = lease_expirations_.empty() || iter->second.lease_expiration_ < lease_expirations_.begin()->first;

  lease_expirations_.insert(std::make_pair(iter->second.lease_expiration_, iter->first));

  if (cancel) {
    tport_->lease_expiration_task_->cancel();
  }
  if (schedule) {
    tport_->lease_expiration_task_->schedule(d);
  }
}

void
Spdp::process_lease_expirations(const DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD (ACE_Thread_Mutex, g, lock_);

  for (TimeQueue::iterator pos = lease_expirations_.begin(), limit = lease_expirations_.end();
       pos != limit && pos->first <= now;) {
    DiscoveredParticipantIter part = participants_.find(pos->second);
    // Pre-emptively erase so purge_discovered_participant will not modify lease_expirations_.
    lease_expirations_.erase(pos++);

    if (part == participants_.end()) {
      continue;
    }

    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) Spdp::process_lease_expirations() - ")
                 ACE_TEXT("participant %C exceeded lease duration, removing\n"),
                 DCPS::LogGuid(part->first).c_str()));
    }

#ifdef OPENDDS_SECURITY
    DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint = sedp_->get_ice_endpoint();
    if (sedp_endpoint) {
      stop_ice(sedp_endpoint, part->first, part->second.pdata_.participantProxy.availableBuiltinEndpoints,
               part->second.pdata_.participantProxy.availableExtendedBuiltinEndpoints);
    }
    DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint = tport_->get_ice_endpoint();
    if (spdp_endpoint) {
      ice_agent_->stop_ice(spdp_endpoint, guid_, part->first);
    }
    purge_handshake_deadlines(part);
#endif
    purge_discovered_participant(part);
    participants_.erase(part);
  }

  if (!lease_expirations_.empty()) {
    tport_->lease_expiration_task_->schedule(lease_expirations_.begin()->first - now);
  }
}

#ifdef OPENDDS_SECURITY
Spdp::ParticipantCryptoInfoPair
Spdp::lookup_participant_crypto_info(const DCPS::GUID_t& id) const
{
  ParticipantCryptoInfoPair result = ParticipantCryptoInfoPair(DDS::HANDLE_NIL, DDS::Security::SharedSecretHandle_var());

  DiscoveredParticipantConstIter pi = participants_.find(id);
  if (pi != participants_.end()) {
    result.first = sedp_->get_handle_registry()->get_remote_participant_crypto_handle(id);
    result.second = pi->second.shared_secret_handle_;
  }
  return result;
}

void
Spdp::send_participant_crypto_tokens(const DCPS::GUID_t& id)
{
  const DCPS::GUID_t peer = make_id(id, ENTITYID_PARTICIPANT);
  const DiscoveredParticipantIter iter = participants_.find(peer);
  if (iter == participants_.end()) {
    if (DCPS::DCPS_debug_level > 0) {
      const DCPS::LogGuid logger(peer);
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::send_participant_crypto_tokens() - ")
                ACE_TEXT("Discovered participant %C not found.\n"),
                logger.c_str()));
    }
    return;
  }

  const DDS::Security::ParticipantCryptoTokenSeq& pcts = iter->second.crypto_tokens_;

  if (pcts.length() != 0) {
    const DCPS::GUID_t writer = make_id(guid_, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER);

    const DCPS::GUID_t reader = make_id(peer, ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER);

    DDS::Security::ParticipantVolatileMessageSecure msg;
    msg.message_identity.source_guid = writer;
    msg.related_message_identity.source_guid = GUID_UNKNOWN;
    msg.message_class_id = DDS::Security::GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS;
    msg.destination_participant_guid = peer;
    msg.destination_endpoint_guid = GUID_UNKNOWN; // unknown = whole participant
    msg.source_endpoint_guid = GUID_UNKNOWN;
    msg.message_data = reinterpret_cast<const DDS::Security::DataHolderSeq&>(pcts);

    if (sedp_->write_volatile_message(msg, reader) != DDS::RETCODE_OK) {
      if (DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::send_participant_crypto_tokens() - ")
          ACE_TEXT("Unable to write volatile message.\n")));
      }
    }
  }

  iter->second.participant_tokens_sent_ = true;
}

DDS::Security::PermissionsHandle
Spdp::lookup_participant_permissions(const DCPS::GUID_t& id) const
{
  DDS::Security::PermissionsHandle result = DDS::HANDLE_NIL;

  ACE_Guard<ACE_Thread_Mutex> g(lock_, false);
  DiscoveredParticipantConstIter pi = participants_.find(id);
  if (pi != participants_.end()) {
    result = pi->second.permissions_handle_;
  }
  return result;
}

AuthState Spdp::lookup_participant_auth_state(const GUID_t& id) const
{
  ACE_Guard<ACE_Thread_Mutex> g(lock_, false);
  DiscoveredParticipantConstIter pi = participants_.find(id);
  if (pi != participants_.end()) {
    return pi->second.auth_state_;
  }
  return AUTH_STATE_HANDSHAKE;
}
#endif

#ifdef OPENDDS_SECURITY
void Spdp::start_ice(DCPS::WeakRcHandle<ICE::Endpoint> endpoint, GUID_t r, BuiltinEndpointSet_t avail,
                     DDS::Security::ExtendedBuiltinEndpointSet_t extended_avail,
                     const ICE::AgentInfo& agent_info)
{
  GUID_t l = guid_;

  // See RTPS v2.1 section 8.5.5.1
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_WRITER) {
    l.entityId = ENTITYID_TL_SVC_REQ_READER;
    r.entityId = ENTITYID_TL_SVC_REQ_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_READER) {
    l.entityId = ENTITYID_TL_SVC_REQ_WRITER;
    r.entityId = ENTITYID_TL_SVC_REQ_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_WRITER) {
    l.entityId = ENTITYID_TL_SVC_REPLY_READER;
    r.entityId = ENTITYID_TL_SVC_REPLY_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER) {
    l.entityId = ENTITYID_TL_SVC_REPLY_WRITER;
    r.entityId = ENTITYID_TL_SVC_REPLY_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }

  using namespace DDS::Security;
  // See DDS-Security v1.1 section 7.3.7.1
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) {
    l.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    r.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) {
    l.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    r.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (extended_avail & TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE) {
    l.entityId = ENTITYID_TL_SVC_REQ_READER_SECURE;
    r.entityId = ENTITYID_TL_SVC_REQ_WRITER_SECURE;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (extended_avail & TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE) {
    l.entityId = ENTITYID_TL_SVC_REQ_WRITER_SECURE;
    r.entityId = ENTITYID_TL_SVC_REQ_READER_SECURE;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (extended_avail & TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE) {
    l.entityId = ENTITYID_TL_SVC_REPLY_READER_SECURE;
    r.entityId = ENTITYID_TL_SVC_REPLY_WRITER_SECURE;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
  if (extended_avail & TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE) {
    l.entityId = ENTITYID_TL_SVC_REPLY_WRITER_SECURE;
    r.entityId = ENTITYID_TL_SVC_REPLY_READER_SECURE;
    ice_agent_->start_ice(endpoint, l, r, agent_info);
  }
}

void Spdp::stop_ice(DCPS::WeakRcHandle<ICE::Endpoint> endpoint, DCPS::GUID_t r, BuiltinEndpointSet_t avail,
                    DDS::Security::ExtendedBuiltinEndpointSet_t extended_avail) {
  GUID_t l = guid_;

  // See RTPS v2.1 section 8.5.5.1
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_WRITER) {
    l.entityId = ENTITYID_TL_SVC_REQ_READER;
    r.entityId = ENTITYID_TL_SVC_REQ_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_READER) {
    l.entityId = ENTITYID_TL_SVC_REQ_WRITER;
    r.entityId = ENTITYID_TL_SVC_REQ_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_WRITER) {
    l.entityId = ENTITYID_TL_SVC_REPLY_READER;
    r.entityId = ENTITYID_TL_SVC_REPLY_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER) {
    l.entityId = ENTITYID_TL_SVC_REPLY_WRITER;
    r.entityId = ENTITYID_TL_SVC_REPLY_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }

  using namespace DDS::Security;
  // See DDS-Security v1.1 section 7.3.7.1
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & SEDP_BUILTIN_PUBLICATIONS_SECURE_READER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER) {
    l.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
    r.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
    l.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
    r.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER) {
    l.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    r.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (avail & SPDP_BUILTIN_PARTICIPANT_SECURE_READER) {
    l.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER;
    r.entityId = ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_READER;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (extended_avail & TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE) {
    l.entityId = ENTITYID_TL_SVC_REQ_READER_SECURE;
    r.entityId = ENTITYID_TL_SVC_REQ_WRITER_SECURE;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (extended_avail & TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE) {
    l.entityId = ENTITYID_TL_SVC_REQ_WRITER_SECURE;
    r.entityId = ENTITYID_TL_SVC_REQ_READER_SECURE;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (extended_avail & TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE) {
    l.entityId = ENTITYID_TL_SVC_REPLY_READER_SECURE;
    r.entityId = ENTITYID_TL_SVC_REPLY_WRITER_SECURE;
    ice_agent_->stop_ice(endpoint, l, r);
  }
  if (extended_avail & TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE) {
    l.entityId = ENTITYID_TL_SVC_REPLY_WRITER_SECURE;
    r.entityId = ENTITYID_TL_SVC_REPLY_READER_SECURE;
    ice_agent_->stop_ice(endpoint, l, r);
  }
}

DDS::Security::ParticipantCryptoHandle
Spdp::remote_crypto_handle(const DCPS::GUID_t& remote_participant) const
{
  return sedp_->get_handle_registry()->get_remote_participant_crypto_handle(remote_participant);
}

// Request and maintain a server-reflexive address.
void Spdp::SpdpTransport::relay_stun_task(const MonotonicTimePoint& /*now*/)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  if (outer->sedp_->core().use_rtps_relay() || outer->sedp_->core().rtps_relay_only()) {
    const DCPS::NetworkAddress relay_address = outer->sedp_->core().spdp_rtps_relay_address();
    if (relay_address) {
      process_relay_sra(relay_srsm_.send(relay_address.to_addr(), ICE::Configuration::instance()->server_reflexive_indication_count(), outer->guid_.guidPrefix));
      send(relay_address.to_addr(), relay_srsm_.message());
      relay_stun_task_->schedule(outer->sedp_->core().advance_relay_stun_task_falloff());
    }
  }
}

void Spdp::SpdpTransport::process_relay_sra(ICE::ServerReflexiveStateMachine::StateChange sc)
{
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  DCPS::ConnectionRecord connection_record;
  std::memset(connection_record.guid, 0, sizeof(connection_record.guid));
  connection_record.protocol = DCPS::RTPS_RELAY_STUN_PROTOCOL;
  connection_record.latency = DCPS::TimeDuration::zero_value.to_dds_duration();

  switch (sc) {
  case ICE::ServerReflexiveStateMachine::SRSM_None:
    if (relay_srsm_.latency_available()) {
      connection_record.address = DCPS::LogAddr(relay_srsm_.stun_server_address()).c_str();
      connection_record.latency = relay_srsm_.latency().to_dds_duration();
      relay_srsm_.latency_available(false);
      outer->sedp_->job_queue()->enqueue(DCPS::make_rch<DCPS::WriteConnectionRecords>(outer->bit_subscriber_, true, connection_record));
    }
    break;
  case ICE::ServerReflexiveStateMachine::SRSM_Set:
  case ICE::ServerReflexiveStateMachine::SRSM_Change:
    // Lengthen to normal period.
    outer->sedp_->core().set_relay_stun_task_falloff();
    connection_record.address = DCPS::LogAddr(relay_srsm_.stun_server_address()).c_str();
    connection_record.latency = relay_srsm_.latency().to_dds_duration();
    relay_srsm_.latency_available(false);
    outer->sedp_->job_queue()->enqueue(DCPS::make_rch<DCPS::WriteConnectionRecords>(outer->bit_subscriber_, true, connection_record));
    break;
  case ICE::ServerReflexiveStateMachine::SRSM_Unset:
    connection_record.address = DCPS::LogAddr(relay_srsm_.unset_stun_server_address()).c_str();
    outer->sedp_->job_queue()->enqueue(DCPS::make_rch<DCPS::WriteConnectionRecords>(outer->bit_subscriber_, false, connection_record));
    break;
  }
#else
  ACE_UNUSED_ARG(sc);
#endif
}

void Spdp::SpdpTransport::disable_relay_stun_task()
{
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  relay_stun_task_->cancel();

  DCPS::ConnectionRecord connection_record;
  std::memset(connection_record.guid, 0, sizeof(connection_record.guid));
  connection_record.protocol = DCPS::RTPS_RELAY_STUN_PROTOCOL;
  connection_record.latency = DCPS::TimeDuration::zero_value.to_dds_duration();

  if (relay_srsm_.stun_server_address() != ACE_INET_Addr())  {
    connection_record.address = DCPS::LogAddr(relay_srsm_.stun_server_address()).c_str();
    outer->sedp_->job_queue()->enqueue(DCPS::make_rch<DCPS::WriteConnectionRecords>(outer->bit_subscriber_, false, connection_record));
  }

  relay_srsm_ = ICE::ServerReflexiveStateMachine();
#endif
}

void Spdp::SpdpTransport::send_relay(const DCPS::MonotonicTimePoint& /*now*/)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  if (outer->sedp_->core().use_rtps_relay() || outer->sedp_->core().rtps_relay_only()) {
    const DCPS::NetworkAddress relay_address = outer->sedp_->core().spdp_rtps_relay_address();
    if (relay_address) {
      write(SEND_RELAY);
      relay_spdp_task_->schedule(outer->sedp_->core().advance_relay_spdp_task_falloff());
    }
  }
}
#endif

void Spdp::SpdpTransport::send_local(const DCPS::MonotonicTimePoint& /*now*/)
{
  write(SEND_MULTICAST);
}

void Spdp::SpdpTransport::send_directed(const DCPS::MonotonicTimePoint& /*now*/)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  ACE_GUARD(ACE_Thread_Mutex, g, outer->lock_);

  while (!directed_guids_.empty()) {
    const DCPS::GUID_t id = directed_guids_.front();
    directed_guids_.pop_front();

    DiscoveredParticipantConstIter pos = outer->participants_.find(id);
    if (pos == outer->participants_.end()) {
      continue;
    }

    write_i(id, pos->second.last_recv_address_, SEND_DIRECT | SEND_RELAY);
    directed_guids_.push_back(id);
    directed_send_task_->schedule(outer->resend_period_ * (1.0 / directed_guids_.size()));
    break;
  }
}

void
Spdp::SpdpTransport::process_lease_expirations(const DCPS::MonotonicTimePoint& now)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  outer->process_lease_expirations(now);
}

void Spdp::SpdpTransport::thread_status_task(const DCPS::MonotonicTimePoint& now)
{
  ACE_UNUSED_ARG(now);
#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  if (DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) Spdp::SpdpTransport::thread_status_task(): Updating internal thread status BIT.\n"));
  }

  ACE_GUARD(ACE_Thread_Mutex, g, outer->lock_);

  typedef DCPS::ThreadStatusManager::List List;
  List running;
  List removed;
  TheServiceParticipant->get_thread_status_manager().harvest(last_thread_status_harvest_, running, removed);
  last_thread_status_harvest_ = now;
  for (List::const_iterator i = removed.begin(); i != removed.end(); ++i) {
    DCPS::InternalThreadBuiltinTopicData data;
    data.thread_id = i->bit_key().c_str();
    outer->bit_subscriber_->remove_thread_status(data);
  }
  for (List::const_iterator i = running.begin(); i != running.end(); ++i) {
    DCPS::InternalThreadBuiltinTopicData data;
    data.thread_id = i->bit_key().c_str();
    data.utilization = i->utilization(now);
    outer->bit_subscriber_->add_thread_status(data, DDS::NEW_VIEW_STATE, i->timestamp());
  }

#endif /* DDS_HAS_MINIMUM_BIT */
}

#ifdef OPENDDS_SECURITY
void Spdp::SpdpTransport::process_handshake_deadlines(const DCPS::MonotonicTimePoint& now)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  outer->process_handshake_deadlines(now);
}

void Spdp::SpdpTransport::process_handshake_resends(const DCPS::MonotonicTimePoint& now)
{
  DCPS::RcHandle<Spdp> outer = outer_.lock();
  if (!outer) return;

  outer->process_handshake_resends(now);
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
  iter->second.handshake_resend_falloff_.set(auth_resend_period_);

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
                                   const DCPS::GUID_t& guid)
{
  ICE::AgentInfoMap ai_map;
  if (!ParameterListConverter::from_param_list(plist, ai_map)) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::process_participant_ice - ")
                ACE_TEXT("failed to convert from ParameterList to ")
                ACE_TEXT("ICE::AgentInfo\n")));
    }
    return;
  }
  ICE::AgentInfoMap::const_iterator sedp_pos = ai_map.find(SEDP_AGENT_INFO_KEY);
  ICE::AgentInfoMap::const_iterator spdp_pos = ai_map.find(SPDP_AGENT_INFO_KEY);

  DCPS::WeakRcHandle<ICE::Endpoint> sedp_endpoint;
  DCPS::WeakRcHandle<ICE::Endpoint> spdp_endpoint;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    if (!initialized_flag_ || shutdown_flag_) {
      return;
    }
    if (sedp_) {
      sedp_endpoint = sedp_->get_ice_endpoint();
    }
    if (tport_) {
      spdp_endpoint = tport_->get_ice_endpoint();
    }
    DiscoveredParticipantIter iter = participants_.find(guid);
    if (iter != participants_.end()) {
      if (sedp_pos != ai_map.end()) {
        iter->second.have_sedp_info_ = true;
        iter->second.sedp_info_ = sedp_pos->second;
      } else {
        iter->second.have_sedp_info_ = false;
      }

      if (spdp_pos != ai_map.end()) {
        iter->second.have_spdp_info_ = true;
        iter->second.spdp_info_ = spdp_pos->second;
      } else {
        iter->second.have_spdp_info_ = false;
      }
    }
  }

  if (sedp_endpoint) {
    if (sedp_pos != ai_map.end()) {
      start_ice(sedp_endpoint, guid, pdata.participantProxy.availableBuiltinEndpoints,
                pdata.participantProxy.availableExtendedBuiltinEndpoints, sedp_pos->second);
    } else {
      stop_ice(sedp_endpoint, guid, pdata.participantProxy.availableBuiltinEndpoints,
               pdata.participantProxy.availableExtendedBuiltinEndpoints);
    }
  }

  if (spdp_endpoint) {
    if (spdp_pos != ai_map.end()) {
      ice_agent_->start_ice(spdp_endpoint, guid_, guid, spdp_pos->second);
    } else {
      ice_agent_->stop_ice(spdp_endpoint, guid_, guid);
#ifndef DDS_HAS_MINIMUM_BIT
      ACE_GUARD(ACE_Thread_Mutex, g, lock_);
      DiscoveredParticipantIter iter = participants_.find(guid);
      if (iter != participants_.end()) {
        enqueue_location_update_i(iter, DCPS::LOCATION_ICE, DCPS::NetworkAddress(), "stop ice");
        process_location_updates_i(iter, "stop ice");
      }
#endif
    }
  }
}

#endif

const ParticipantData_t& Spdp::get_participant_data(const DCPS::GUID_t& guid) const
{
  DiscoveredParticipantConstIter iter = participants_.find(make_part_guid(guid));
  if (iter != participants_.end()) {
    return iter->second.pdata_;
  }

  static const ParticipantData_t pdata = {};
  return pdata;
}

ParticipantData_t& Spdp::get_participant_data(const DCPS::GUID_t& guid)
{
  DiscoveredParticipantIter iter = participants_.find(make_part_guid(guid));
  if (iter != participants_.end()) {
    return iter->second.pdata_;
  }

  static ParticipantData_t pdata;
  return pdata;
}

DCPS::MonotonicTime_t Spdp::get_participant_discovered_at() const
{
  return participant_discovered_at_;
}

DCPS::MonotonicTime_t Spdp::get_participant_discovered_at(const DCPS::GUID_t& guid) const
{
  const DiscoveredParticipantConstIter iter = participants_.find(make_part_guid(guid));
  if (iter != participants_.end()) {
    return iter->second.discovered_at_.to_monotonic_time();
  }

  return DCPS::MonotonicTime_t();
}

bool Spdp::secure_part_user_data() const
{
#ifdef OPENDDS_SECURITY
  return security_enabled_ && secure_participant_user_data_;
#else
  return false;
#endif
}

DDS::ParticipantBuiltinTopicData Spdp::get_part_bit_data(bool secure) const
{
  bool include_user_data = true;
#ifdef OPENDDS_SECURITY
  if (secure_part_user_data()) {
    include_user_data = secure;
  }
#else
  ACE_UNUSED_ARG(secure);
#endif
  DDS::ParticipantBuiltinTopicData bit_data;
  bit_data.key = DDS::BuiltinTopicKey_t();
  bit_data.user_data = include_user_data ? qos_.user_data : DDS::UserDataQosPolicy();
  return bit_data;
}

void Spdp::ignore_domain_participant(const GUID_t& ignoreId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  endpoint_manager().ignore(ignoreId);

  DiscoveredParticipantIter iter = participants_.find(ignoreId);
  if (iter != participants_.end()) {
    purge_discovered_participant(iter);
    participants_.erase(iter);
  }
}

void Spdp::remove_domain_participant(const GUID_t& removeId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  DiscoveredParticipantIter iter = participants_.find(removeId);
  if (iter != participants_.end()) {
    purge_discovered_participant(iter);
    participants_.erase(iter);
  }
}

bool Spdp::update_domain_participant_qos(const DDS::DomainParticipantQos& qos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  qos_ = qos;
  return announce_domain_participant_qos();
}

bool Spdp::has_domain_participant(const GUID_t& remote) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  return has_discovered_participant(remote);
}

DCPS::TopicStatus Spdp::assert_topic(GUID_t& topicId, const char* topicName,
  const char* dataTypeName, const DDS::TopicQos& qos,
  bool hasDcpsKey, DCPS::TopicCallbacks* topic_callbacks)
{
  if (std::strlen(topicName) > 256 || std::strlen(dataTypeName) > 256) {
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR LocalParticipant::assert_topic() - ")
                 ACE_TEXT("topic or type name length limit (256) exceeded\n")));
    }
    return DCPS::PRECONDITION_NOT_MET;
  }

  return endpoint_manager().assert_topic(topicId, topicName, dataTypeName, qos, hasDcpsKey, topic_callbacks);
}

void Spdp::purge_discovered_participant(const DiscoveredParticipantIter& iter)
{
  if (iter == participants_.end()) {
    return;
  }
  endpoint_manager().disassociate(iter->second);
  bit_subscriber_->remove_participant(iter->second.bit_ih_, iter->second.location_ih_);
  if (DCPS_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) LocalParticipant::purge_discovered_participant: "
               "erasing %C (%B)\n",
               DCPS::LogGuid(iter->first).c_str(), participants_.size()));
  }

  remove_lease_expiration_i(iter);

#ifdef OPENDDS_SECURITY
  if (security_config_) {
    DDS::Security::SecurityException se = {"", 0, 0};
    DDS::Security::Authentication_var auth = security_config_->get_authentication();
    DDS::Security::AccessControl_var access = security_config_->get_access_control();

    DDS::Security::ParticipantCryptoHandle pch =
      sedp_->get_handle_registry()->get_remote_participant_crypto_handle(iter->first);
    if (!security_config_->get_crypto_key_factory()->unregister_participant(pch, se)) {
      if (DCPS::security_debug.auth_warn) {
        ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                   ACE_TEXT("Spdp::purge_discovered_participant() - ")
                   ACE_TEXT("Unable to return crypto handle. ")
                   ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                   se.code, se.minor_code, se.message.in()));
      }
    }
    sedp_->get_handle_registry()->erase_remote_participant_crypto_handle(iter->first);
    sedp_->get_handle_registry()->erase_remote_participant_permissions_handle(iter->first);

    if (iter->second.identity_handle_ != DDS::HANDLE_NIL) {
      if (!auth->return_identity_handle(iter->second.identity_handle_, se)) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                     ACE_TEXT("Spdp::purge_discovered_participant() - ")
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
                     ACE_TEXT("Spdp::purge_discovered_participant() - ")
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
                     ACE_TEXT("Spdp::purge_discovered_participant() - ")
                     ACE_TEXT("Unable to return sharedsecret handle. ")
                     ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                     se.code, se.minor_code, se.message.in()));
        }
      }
    }

    if (iter->second.permissions_handle_ != DDS::HANDLE_NIL) {
      if (!access->return_permissions_handle(iter->second.permissions_handle_, se)) {
        if (DCPS::security_debug.auth_warn) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) {auth_warn} ")
                     ACE_TEXT("Spdp::purge_discovered_participant() - ")
                     ACE_TEXT("Unable to return permissions handle. ")
                     ACE_TEXT("Security Exception[%d.%d]: %C\n"),
                     se.code, se.minor_code, se.message.in()));
        }
      }
    }
  }

  if (iter->second.auth_state_ == AUTH_STATE_HANDSHAKE) {
    --n_participants_in_authentication_;
  }
#endif
}

#ifdef OPENDDS_SECURITY
void Spdp::set_auth_state(DiscoveredParticipant& dp, AuthState new_state)
{
  if (dp.auth_state_ == AUTH_STATE_HANDSHAKE &&
      new_state != AUTH_STATE_HANDSHAKE) {
    --n_participants_in_authentication_;
  }
  dp.auth_state_ = new_state;
}
#endif

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
