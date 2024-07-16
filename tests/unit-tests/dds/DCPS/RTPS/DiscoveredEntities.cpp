/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <dds/DCPS/Time_Helper.h>

#include <dds/DCPS/RTPS/DiscoveredEntities.h>

#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY
#  include <dds/DCPS/RTPS/RtpsSecurityC.h>
#endif

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

TEST(dds_DCPS_RTPS_DiscoveredEntities, DiscoveredParticipant_ctor)
{
  {
    DiscoveredParticipant uut;

    EXPECT_EQ(uut.location_ih_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.bit_ih_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.seq_reset_count_, 0);
#if OPENDDS_CONFIG_SECURITY
    EXPECT_EQ(uut.have_spdp_info_, false);
    EXPECT_EQ(uut.have_sedp_info_, false);
    EXPECT_EQ(uut.have_auth_req_msg_, false);
    EXPECT_EQ(uut.have_handshake_msg_, false);
    EXPECT_EQ(uut.handshake_resend_falloff_.get(), TimeDuration::zero_value);
    EXPECT_EQ(uut.auth_state_, AUTH_STATE_HANDSHAKE);
    EXPECT_EQ(uut.handshake_state_, HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST);
    EXPECT_EQ(uut.is_requester_, false);
    EXPECT_EQ(uut.auth_req_sequence_number_, 0);
    EXPECT_EQ(uut.handshake_sequence_number_, 0);
    EXPECT_EQ(uut.identity_handle_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.handshake_handle_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.permissions_handle_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.extended_builtin_endpoints_, 0u);
    EXPECT_EQ(uut.participant_tokens_sent_, false);
    EXPECT_EQ(uut.security_info_.participant_security_attributes, 0u);
    EXPECT_EQ(uut.security_info_.plugin_participant_security_attributes, 0u);
#endif
  }

  {
    ParticipantData_t p;
    std::memset(p.participantProxy.guidPrefix, 0, sizeof(p.participantProxy.guidPrefix));
    p.participantProxy.guidPrefix[0] = 83;
    SequenceNumber seq(84);
    TimeDuration resend_period(85);
    DiscoveredParticipant uut(p, seq, resend_period);

    // Can't compared IDL defined type.
    //EXPECT_EQ(uut.pdata_, p);
    EXPECT_EQ(uut.location_ih_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.bit_ih_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.max_seq_, seq);
    EXPECT_EQ(uut.seq_reset_count_, 0);
    GUID_t guid;
    std::memcpy(&guid, uut.location_data_.guid, sizeof(guid));
    EXPECT_EQ(guid, make_part_guid(p.participantProxy.guidPrefix));

    EXPECT_EQ(uut.location_data_.location, 0u);
    EXPECT_EQ(uut.location_data_.change_mask, 0u);
    EXPECT_EQ(uut.location_data_.local_timestamp.sec, 0);
    EXPECT_EQ(uut.location_data_.local_timestamp.nanosec, 0u);
    EXPECT_EQ(uut.location_data_.ice_timestamp.sec, 0);
    EXPECT_EQ(uut.location_data_.ice_timestamp.nanosec, 0u);
    EXPECT_EQ(uut.location_data_.relay_timestamp.sec, 0);
    EXPECT_EQ(uut.location_data_.relay_timestamp.nanosec, 0u);
    EXPECT_EQ(uut.location_data_.local6_timestamp.sec, 0);
    EXPECT_EQ(uut.location_data_.local6_timestamp.nanosec, 0u);
    EXPECT_EQ(uut.location_data_.ice6_timestamp.sec, 0);
    EXPECT_EQ(uut.location_data_.ice6_timestamp.nanosec, 0u);
    EXPECT_EQ(uut.location_data_.relay6_timestamp.sec, 0);
    EXPECT_EQ(uut.location_data_.relay6_timestamp.nanosec, 0u);
    EXPECT_EQ(uut.location_data_.lease_duration.sec, 0);
    EXPECT_EQ(uut.location_data_.lease_duration.nanosec, 0u);

#if OPENDDS_CONFIG_SECURITY
    EXPECT_EQ(uut.have_spdp_info_, false);
    EXPECT_EQ(uut.have_sedp_info_, false);
    EXPECT_EQ(uut.have_auth_req_msg_, false);
    EXPECT_EQ(uut.have_handshake_msg_, false);
    EXPECT_EQ(uut.handshake_resend_falloff_.get(), resend_period);
    EXPECT_EQ(uut.auth_state_, AUTH_STATE_HANDSHAKE);
    EXPECT_EQ(uut.handshake_state_, HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST);
    EXPECT_EQ(uut.is_requester_, false);
    EXPECT_EQ(uut.auth_req_sequence_number_, 0);
    EXPECT_EQ(uut.handshake_sequence_number_, 0);
    EXPECT_EQ(uut.identity_handle_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.handshake_handle_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.permissions_handle_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.extended_builtin_endpoints_, 0u);
    EXPECT_EQ(uut.participant_tokens_sent_, false);
    EXPECT_EQ(uut.security_info_.participant_security_attributes, 0u);
    EXPECT_EQ(uut.security_info_.plugin_participant_security_attributes, 0u);
#endif
  }
}

#if OPENDDS_CONFIG_SECURITY
TEST(dds_DCPS_RTPS_DiscoveredEntities, DiscoveredParticipant_has_security_data)
{
  DiscoveredParticipant uut;
  EXPECT_FALSE(uut.has_security_data());
  uut.pdata_.dataKind = OpenDDS::Security::DPDK_ENHANCED;
  EXPECT_TRUE(uut.has_security_data());
  uut.pdata_.dataKind = OpenDDS::Security::DPDK_SECURE;
  EXPECT_TRUE(uut.has_security_data());
}
#endif

TEST(dds_DCPS_RTPS_DiscoveredEntities, DiscoveredSubscription_ctor)
{
  {
    DiscoveredSubscription uut;

    EXPECT_EQ(uut.bit_ih_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.participant_discovered_at_, monotonic_time_zero());
    EXPECT_EQ(uut.transport_context_, 0u);
#if OPENDDS_CONFIG_SECURITY
    EXPECT_EQ(uut.have_ice_agent_info_, false);
    // Can't compare IDL defined type.
    //EXPECT_EQ(uut.security_attribs_.base, DDS::Security::TopicSecurityAttributes());
    EXPECT_EQ(uut.security_attribs_.is_key_protected, 0);
    EXPECT_EQ(uut.security_attribs_.is_payload_protected, 0);
    EXPECT_EQ(uut.security_attribs_.is_submessage_protected, 0);
    EXPECT_EQ(uut.security_attribs_.plugin_endpoint_attributes, 0u);
#endif
    EXPECT_STREQ(uut.get_topic_name(), "");
    EXPECT_STREQ(uut.get_type_name(), "");
  }

  {
    DiscoveredReaderData r;
    r.ddsSubscriptionData.topic_name = "a topic";
    r.ddsSubscriptionData.type_name = "a type";
    DiscoveredSubscription uut(r);

    // Can't compare IDL defined type.
    //EXPECT_EQ(uut.reader_data_, r);
    EXPECT_EQ(uut.bit_ih_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.participant_discovered_at_, monotonic_time_zero());
    EXPECT_EQ(uut.transport_context_, 0u);
#if OPENDDS_CONFIG_SECURITY
    // Can't compare IDL defined type.
    //EXPECT_EQ(uut.security_attribs_, DDS::Security::EndpointSecurityAttributes());
    EXPECT_EQ(uut.have_ice_agent_info_, false);
    // Can't compare IDL defined type.
    //EXPECT_EQ(uut.security_attribs_.base, DDS::Security::TopicSecurityAttributes());
    EXPECT_EQ(uut.security_attribs_.is_key_protected, 0);
    EXPECT_EQ(uut.security_attribs_.is_payload_protected, 0);
    EXPECT_EQ(uut.security_attribs_.is_submessage_protected, 0);
    EXPECT_EQ(uut.security_attribs_.plugin_endpoint_attributes, 0u);
#endif
    EXPECT_STREQ(uut.get_topic_name(), "a topic");
    EXPECT_STREQ(uut.get_type_name(), "a type");
  }
}

TEST(dds_DCPS_RTPS_DiscoveredEntities, DiscoveredPublication_ctor)
{
  {
    DiscoveredPublication uut;

    EXPECT_EQ(uut.bit_ih_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.participant_discovered_at_, monotonic_time_zero());
    EXPECT_EQ(uut.transport_context_, 0u);
#if OPENDDS_CONFIG_SECURITY
    EXPECT_EQ(uut.have_ice_agent_info_, false);
    // Can't compare IDL defined type.
    //EXPECT_EQ(uut.security_attribs_.base, DDS::Security::TopicSecurityAttributes());
    EXPECT_EQ(uut.security_attribs_.is_key_protected, 0);
    EXPECT_EQ(uut.security_attribs_.is_payload_protected, 0);
    EXPECT_EQ(uut.security_attribs_.is_submessage_protected, 0);
    EXPECT_EQ(uut.security_attribs_.plugin_endpoint_attributes, 0u);
#endif
    EXPECT_STREQ(uut.get_topic_name(), "");
    EXPECT_STREQ(uut.get_type_name(), "");
  }

  {
    DiscoveredWriterData w;
    w.ddsPublicationData.topic_name = "a topic";
    w.ddsPublicationData.type_name = "a type";
    DiscoveredPublication uut(w);

    // Can't compare IDL defined type.
    //EXPECT_EQ(uut.reader_data_, r);
    EXPECT_EQ(uut.bit_ih_, DDS::HANDLE_NIL);
    EXPECT_EQ(uut.participant_discovered_at_, monotonic_time_zero());
    EXPECT_EQ(uut.transport_context_, 0u);
#if OPENDDS_CONFIG_SECURITY
    // Can't compare IDL defined type.
    //EXPECT_EQ(uut.security_attribs_, DDS::Security::EndpointSecurityAttributes());
    EXPECT_EQ(uut.have_ice_agent_info_, false);
    // Can't compare IDL defined type.
    //EXPECT_EQ(uut.security_attribs_.base, DDS::Security::TopicSecurityAttributes());
    EXPECT_EQ(uut.security_attribs_.is_key_protected, 0);
    EXPECT_EQ(uut.security_attribs_.is_payload_protected, 0);
    EXPECT_EQ(uut.security_attribs_.is_submessage_protected, 0);
    EXPECT_EQ(uut.security_attribs_.plugin_endpoint_attributes, 0u);
#endif
    EXPECT_STREQ(uut.get_topic_name(), "a topic");
    EXPECT_STREQ(uut.get_type_name(), "a type");
  }
}
