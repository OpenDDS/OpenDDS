/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <dds/DCPS/RTPS/LocalEntities.h>

#include <dds/OpenDDSConfigWrapper.h>

TEST(dds_DCPS_RTPS_DiscoveredEntities, LocalEntity_ctor)
{
  OpenDDS::RTPS::LocalEntity uut;
  EXPECT_EQ(uut.topic_id_, OpenDDS::DCPS::GUID_UNKNOWN);
  EXPECT_EQ(uut.participant_discovered_at_, OpenDDS::DCPS::monotonic_time_zero());
  EXPECT_EQ(uut.transport_context_, 0u);
  EXPECT_EQ(uut.sequence_, OpenDDS::DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN());
#if OPENDDS_CONFIG_SECURITY
  EXPECT_EQ(uut.have_ice_agent_info, false);
  EXPECT_EQ(uut.security_attribs_.base.is_read_protected, false);
  EXPECT_EQ(uut.security_attribs_.base.is_write_protected, false);
  EXPECT_EQ(uut.security_attribs_.base.is_discovery_protected, false);
  EXPECT_EQ(uut.security_attribs_.base.is_liveliness_protected, false);
  EXPECT_EQ(uut.security_attribs_.is_submessage_protected, false);
  EXPECT_EQ(uut.security_attribs_.is_payload_protected, false);
  EXPECT_EQ(uut.security_attribs_.is_key_protected, false);
  EXPECT_EQ(uut.security_attribs_.plugin_endpoint_attributes, 0u);
#endif
}
