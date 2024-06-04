/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef OPENDDS_DCPS_RTPS_DISCOVERED_ENTITIES_H
#define OPENDDS_DCPS_RTPS_DISCOVERED_ENTITIES_H

#include <dds/Versioned_Namespace.h>

#include "AssociationRecord.h"
#include "RtpsCoreC.h"

#include "ICE/Ice.h"

#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/FibonacciSequence.h>
#include <dds/DCPS/PoolAllocationBase.h>
#include <dds/DCPS/SequenceNumber.h>

#if OPENDDS_CONFIG_SECURITY
#  include "RtpsSecurityC.h"
#endif

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

#if OPENDDS_CONFIG_SECURITY
enum AuthState {
  AUTH_STATE_HANDSHAKE,
  AUTH_STATE_AUTHENTICATED,
  AUTH_STATE_UNAUTHENTICATED
};

enum HandshakeState {
  HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST, //!< Requester should call begin_handshake_request
  HANDSHAKE_STATE_BEGIN_HANDSHAKE_REPLY, //!< Replier should call begin_handshake_reply
  HANDSHAKE_STATE_PROCESS_HANDSHAKE, //!< Requester and replier should call process handshake
  HANDSHAKE_STATE_DONE //!< Handshake concluded or timed out
};

typedef Security::SPDPdiscoveredParticipantData ParticipantData_t;
#else
typedef SPDPdiscoveredParticipantData ParticipantData_t;
#endif

struct DiscoveredParticipant {
  DiscoveredParticipant()
    : location_ih_(DDS::HANDLE_NIL)
    , bit_ih_(DDS::HANDLE_NIL)
    , seq_reset_count_(0)
#if OPENDDS_CONFIG_SECURITY
    , have_spdp_info_(false)
    , have_sedp_info_(false)
    , have_auth_req_msg_(false)
    , have_handshake_msg_(false)
    , handshake_resend_falloff_(DCPS::TimeDuration::zero_value)
    , auth_state_(AUTH_STATE_HANDSHAKE)
    , handshake_state_(HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST)
    , is_requester_(false)
    , auth_req_sequence_number_(0)
    , handshake_sequence_number_(0)
    , identity_handle_(DDS::HANDLE_NIL)
    , handshake_handle_(DDS::HANDLE_NIL)
    , permissions_handle_(DDS::HANDLE_NIL)
    , extended_builtin_endpoints_(0)
    , participant_tokens_sent_(false)
#endif
  {
#if OPENDDS_CONFIG_SECURITY
    pdata_.dataKind = Security::DPDK_NONE;
    security_info_.participant_security_attributes = 0;
    security_info_.plugin_participant_security_attributes = 0;
#endif
  }

  DiscoveredParticipant(const ParticipantData_t& p,
                        const DCPS::SequenceNumber& seq,
                        const DCPS::TimeDuration& resend_period)
    : pdata_(p)
    , location_ih_(DDS::HANDLE_NIL)
    , bit_ih_(DDS::HANDLE_NIL)
    , max_seq_(seq)
    , seq_reset_count_(0)
#if OPENDDS_CONFIG_SECURITY
    , have_spdp_info_(false)
    , have_sedp_info_(false)
    , have_auth_req_msg_(false)
    , have_handshake_msg_(false)
    , handshake_resend_falloff_(resend_period)
    , auth_state_(AUTH_STATE_HANDSHAKE)
    , handshake_state_(HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST)
    , is_requester_(false)
    , auth_req_sequence_number_(0)
    , handshake_sequence_number_(0)
    , identity_handle_(DDS::HANDLE_NIL)
    , handshake_handle_(DDS::HANDLE_NIL)
    , permissions_handle_(DDS::HANDLE_NIL)
    , extended_builtin_endpoints_(0)
    , participant_tokens_sent_(false)
#endif
  {
    const DCPS::GUID_t guid = DCPS::make_part_guid(p.participantProxy.guidPrefix);
    assign(location_data_.guid, guid);
    location_data_.location = 0;
    location_data_.change_mask = 0;
    location_data_.local_timestamp.sec = 0;
    location_data_.local_timestamp.nanosec = 0;
    location_data_.ice_timestamp.sec = 0;
    location_data_.ice_timestamp.nanosec = 0;
    location_data_.relay_timestamp.sec = 0;
    location_data_.relay_timestamp.nanosec = 0;
    location_data_.local6_timestamp.sec = 0;
    location_data_.local6_timestamp.nanosec = 0;
    location_data_.ice6_timestamp.sec = 0;
    location_data_.ice6_timestamp.nanosec = 0;
    location_data_.relay6_timestamp.sec = 0;
    location_data_.relay6_timestamp.nanosec = 0;
    location_data_.lease_duration.sec = 0;
    location_data_.lease_duration.nanosec = 0;

#if OPENDDS_CONFIG_SECURITY
    security_info_.participant_security_attributes = 0;
    security_info_.plugin_participant_security_attributes = 0;
#else
    ACE_UNUSED_ARG(resend_period);
#endif
  }

  ParticipantData_t pdata_;

  struct LocationUpdate {
    DCPS::ParticipantLocation mask_;
    DCPS::NetworkAddress from_;
    DCPS::SystemTimePoint timestamp_;
    LocationUpdate() {}
    LocationUpdate(DCPS::ParticipantLocation mask,
                   const DCPS::NetworkAddress& from,
                   const DCPS::SystemTimePoint& timestamp)
      : mask_(mask), from_(from), timestamp_(timestamp) {}
  };
  typedef OPENDDS_VECTOR(LocationUpdate) LocationUpdateList;
  LocationUpdateList location_updates_;
  DCPS::ParticipantLocationBuiltinTopicData location_data_;
  DDS::InstanceHandle_t location_ih_;

  DCPS::NetworkAddress last_recv_address_;
  DCPS::MonotonicTimePoint discovered_at_;
  DCPS::MonotonicTimePoint lease_expiration_;
  DDS::InstanceHandle_t bit_ih_;
  DCPS::SequenceNumber max_seq_;
  ACE_UINT16 seq_reset_count_;
  typedef OPENDDS_LIST(BuiltinAssociationRecord) BuiltinAssociationRecords;
  BuiltinAssociationRecords builtin_pending_records_;
  BuiltinAssociationRecords builtin_associated_records_;
  typedef OPENDDS_LIST(WriterAssociationRecord_rch) WriterAssociationRecords;
  WriterAssociationRecords writer_pending_records_;
  WriterAssociationRecords writer_associated_records_;
  typedef OPENDDS_LIST(ReaderAssociationRecord_rch) ReaderAssociationRecords;
  ReaderAssociationRecords reader_pending_records_;
  ReaderAssociationRecords reader_associated_records_;
#if OPENDDS_CONFIG_SECURITY
  bool have_spdp_info_;
  ICE::AgentInfo spdp_info_;
  bool have_sedp_info_;
  ICE::AgentInfo sedp_info_;
  bool have_auth_req_msg_;
  DDS::Security::ParticipantStatelessMessage auth_req_msg_;
  bool have_handshake_msg_;
  DDS::Security::ParticipantStatelessMessage handshake_msg_;
  DCPS::FibonacciSequence<DCPS::TimeDuration> handshake_resend_falloff_;
  DCPS::MonotonicTimePoint stateless_msg_deadline_;

  DCPS::MonotonicTimePoint handshake_deadline_;
  AuthState auth_state_;
  HandshakeState handshake_state_;
  bool is_requester_;
  CORBA::LongLong auth_req_sequence_number_;
  CORBA::LongLong handshake_sequence_number_;

  DDS::Security::IdentityToken identity_token_;
  DDS::Security::PermissionsToken permissions_token_;
  DDS::Security::PropertyQosPolicy property_qos_;
  DDS::Security::ParticipantSecurityInfo security_info_;
  DDS::Security::IdentityStatusToken identity_status_token_;
  DDS::Security::IdentityHandle identity_handle_;
  DDS::Security::HandshakeHandle handshake_handle_;
  DDS::Security::AuthRequestMessageToken local_auth_request_token_;
  DDS::Security::AuthRequestMessageToken remote_auth_request_token_;
  DDS::Security::AuthenticatedPeerCredentialToken authenticated_peer_credential_token_;
  DDS::Security::SharedSecretHandle_var shared_secret_handle_;
  DDS::Security::PermissionsHandle permissions_handle_;
  DDS::Security::ParticipantCryptoTokenSeq crypto_tokens_;
  DDS::Security::ExtendedBuiltinEndpointSet_t extended_builtin_endpoints_;
  bool participant_tokens_sent_;

  bool has_security_data() const
  {
    return pdata_.dataKind == Security::DPDK_ENHANCED || pdata_.dataKind == Security::DPDK_SECURE;
  }
#endif
};

struct DiscoveredSubscription : DCPS::PoolAllocationBase {
  DiscoveredSubscription()
    : bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
#if OPENDDS_CONFIG_SECURITY
    , have_ice_agent_info_(false)
#endif
  {
#if OPENDDS_CONFIG_SECURITY
    security_attribs_.base = DDS::Security::TopicSecurityAttributes();
    security_attribs_.is_key_protected = 0;
    security_attribs_.is_payload_protected = 0;
    security_attribs_.is_submessage_protected = 0;
    security_attribs_.plugin_endpoint_attributes = 0;
#endif
  }

  explicit DiscoveredSubscription(const DCPS::DiscoveredReaderData& r)
    : reader_data_(r)
    , bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
#if OPENDDS_CONFIG_SECURITY
    , security_attribs_(DDS::Security::EndpointSecurityAttributes())
    , have_ice_agent_info_(false)
#endif
  {
#if OPENDDS_CONFIG_SECURITY
    security_attribs_.base = DDS::Security::TopicSecurityAttributes();
    security_attribs_.is_key_protected = 0;
    security_attribs_.is_payload_protected = 0;
    security_attribs_.is_submessage_protected = 0;
    security_attribs_.plugin_endpoint_attributes = 0;
#endif
  }

  DCPS::RepoIdSet matched_endpoints_;
  DCPS::DiscoveredReaderData reader_data_;
  DDS::InstanceHandle_t bit_ih_;
  DCPS::MonotonicTime_t participant_discovered_at_;
  ACE_CDR::ULong transport_context_;
  XTypes::TypeInformation type_info_;

#if OPENDDS_CONFIG_SECURITY
  DDS::Security::EndpointSecurityAttributes security_attribs_;
  bool have_ice_agent_info_;
  ICE::AgentInfo ice_agent_info_;
#endif

  const char* get_topic_name() const
  {
    return reader_data_.ddsSubscriptionData.topic_name;
  }

  const char* get_type_name() const
  {
    return reader_data_.ddsSubscriptionData.type_name;
  }
};

struct DiscoveredPublication : DCPS::PoolAllocationBase {
  DiscoveredPublication()
    : bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
#if OPENDDS_CONFIG_SECURITY
    , have_ice_agent_info_(false)
#endif
  {
#if OPENDDS_CONFIG_SECURITY
    security_attribs_.base = DDS::Security::TopicSecurityAttributes();
    security_attribs_.is_key_protected = 0;
    security_attribs_.is_payload_protected = 0;
    security_attribs_.is_submessage_protected = 0;
    security_attribs_.plugin_endpoint_attributes = 0;
#endif
  }

  explicit DiscoveredPublication(const DCPS::DiscoveredWriterData& w)
    : writer_data_(w)
    , bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
#if OPENDDS_CONFIG_SECURITY
    , have_ice_agent_info_(false)
#endif
  {
#if OPENDDS_CONFIG_SECURITY
    security_attribs_.base = DDS::Security::TopicSecurityAttributes();
    security_attribs_.is_key_protected = 0;
    security_attribs_.is_payload_protected = 0;
    security_attribs_.is_submessage_protected = 0;
    security_attribs_.plugin_endpoint_attributes = 0;
#endif
  }

  DCPS::RepoIdSet matched_endpoints_;
  DCPS::DiscoveredWriterData writer_data_;
  DDS::InstanceHandle_t bit_ih_;
  DCPS::MonotonicTime_t participant_discovered_at_;
  ACE_CDR::ULong transport_context_;
  XTypes::TypeInformation type_info_;

#if OPENDDS_CONFIG_SECURITY
  DDS::Security::EndpointSecurityAttributes security_attribs_;
  bool have_ice_agent_info_;
  ICE::AgentInfo ice_agent_info_;
#endif

  const char* get_topic_name() const
  {
    return writer_data_.ddsPublicationData.topic_name;
  }

  const char* get_type_name() const
  {
    return writer_data_.ddsPublicationData.type_name;
  }
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_RTPS_DISCOVERED_ENTITIES_H
