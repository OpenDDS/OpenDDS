#ifdef OPENDDS_HAS_CXX11

#include <dds/rtpsrelaylib/Utility.h>

#include <gtest/gtest.h>

using namespace RtpsRelay;

TEST(tools_dds_rtpsrelaylib_Utility, guid_to_string)
{
  OpenDDS::DCPS::GUID_t guid = OpenDDS::DCPS::GUID_UNKNOWN;
  EXPECT_EQ(guid_to_string(guid), "00000000.00000000.00000000.00000000");
}

TEST(tools_dds_rtpsrelaylib_Utility, AddrPort_default_ctor)
{
  AddrPort ap;
  EXPECT_EQ(ap.addr, ACE_INET_Addr());
  EXPECT_EQ(ap.port, SPDP);
}

TEST(tools_dds_rtpsrelaylib_Utility, AddrPort_ctor)
{
  ACE_INET_Addr addr("127.0.0.1");
  AddrPort ap(addr, DATA);
  EXPECT_EQ(ap.addr, addr);
  EXPECT_EQ(ap.port, DATA);
}

TEST(tools_dds_rtpsrelaylib_Utility, AddrPort_equal)
{
  ACE_INET_Addr addr("127.0.0.1:1234");
  AddrPort ap1(addr, DATA);
  AddrPort ap2(addr, DATA);
  EXPECT_EQ(ap1, ap2);
}

TEST(tools_dds_rtpsrelaylib_Utility, AddrPort_not_equal)
{
  ACE_INET_Addr addr1("192.168.1.1:1234");
  ACE_INET_Addr addr2("192.168.1.2:1234");
  AddrPort ap1(addr1, DATA);
  AddrPort ap2(addr1, SPDP);
  AddrPort ap3(addr2, DATA);
  AddrPort ap4(addr2, SPDP);
  EXPECT_NE(ap1, ap2);
  EXPECT_NE(ap1, ap3);
  EXPECT_NE(ap1, ap4);
  EXPECT_NE(ap2, ap3);
  EXPECT_NE(ap2, ap4);
  EXPECT_NE(ap3, ap4);
}

TEST(tools_dds_rtpsrelaylib_Utility, AddrPort_less_than)
{
  ACE_INET_Addr addr1("192.168.1.1:1234");
  ACE_INET_Addr addr2("192.168.1.2:1234");
  AddrPort ap1(addr1, SPDP);
  AddrPort ap2(addr1, DATA);
  AddrPort ap3(addr2, SPDP);
  AddrPort ap4(addr2, DATA);
  EXPECT_LT(ap1, ap2);
  EXPECT_LT(ap2, ap3);
  EXPECT_LT(ap3, ap4);
}

TEST(tools_dds_rtpsrelaylib_Utility, GuidAddr_ctor)
{
  OpenDDS::DCPS::GUID_t guid;
  ACE_INET_Addr addr("127.0.0.1");
  AddrPort ap(addr, SPDP);
  GuidAddr ga(guid, ap);
  EXPECT_EQ(ga.guid, guid);
  EXPECT_EQ(ga.address, ap);
}

TEST(tools_dds_rtpsrelaylib_Utility, GuidAddr_equal)
{
  OpenDDS::DCPS::GUID_t guid;
  ACE_INET_Addr addr("127.0.0.1:1234");
  AddrPort ap(addr, SPDP);
  GuidAddr ga1(guid, ap);
  GuidAddr ga2(guid, ap);
  EXPECT_EQ(ga1, ga2);
}

TEST(tools_dds_rtpsrelaylib_Utility, GuidAddr_not_equal)
{
  OpenDDS::DCPS::GUID_t guid1 = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::GUID_t guid2 = guid1;
  ++guid2.guidPrefix[0];
  ACE_INET_Addr addr("192.168.1.1:1234");
  AddrPort ap1(addr, SPDP);
  AddrPort ap2(addr, SEDP);

  GuidAddr ga1(guid1, ap1);
  GuidAddr ga2(guid1, ap2);
  GuidAddr ga3(guid2, ap1);
  GuidAddr ga4(guid2, ap2);
  EXPECT_NE(ga1, ga2);
  EXPECT_NE(ga1, ga3);
  EXPECT_NE(ga1, ga4);
  EXPECT_NE(ga2, ga3);
  EXPECT_NE(ga2, ga4);
  EXPECT_NE(ga3, ga4);
}

TEST(tools_dds_rtpsrelaylib_Utility, GuidAddr_less_than)
{
  OpenDDS::DCPS::GUID_t guid1 = OpenDDS::DCPS::GUID_UNKNOWN;
  OpenDDS::DCPS::GUID_t guid2 = guid1;
  ++guid2.guidPrefix[0];
  ACE_INET_Addr addr("192.168.1.1:1234");
  AddrPort ap1(addr, SPDP);
  AddrPort ap2(addr, SEDP);

  GuidAddr ga1(guid1, ap1);
  GuidAddr ga2(guid1, ap2);
  GuidAddr ga3(guid2, ap1);
  GuidAddr ga4(guid2, ap2);
  EXPECT_LT(ga1, ga2);
  EXPECT_LT(ga2, ga3);
  EXPECT_LT(ga3, ga4);
}

TEST(tools_dds_rtpsrelaylib_Utility, entity_id_equal)
{
  const EntityId_t eid1 = { { 0, 1, 2}, 3 };
  const EntityId_t eid2 = { { 0, 1, 2}, 4 };

  EXPECT_EQ(eid1, eid1);
  EXPECT_EQ(eid2, eid2);
  EXPECT_NE(eid1, eid2);
}

TEST(tools_dds_rtpsrelaylib_Utility, assign_entity_ids)
{
  const OpenDDS::DCPS::EntityId_t eid = { { 0, 1, 2}, 3 };
  const EntityId_t expected_eid = { { 0, 1, 2}, 3 };

  EntityId_t actual_eid;
  assign(actual_eid, eid);

  EXPECT_EQ(actual_eid, expected_eid);
}

TEST(tools_dds_rtpsrelaylib_Utility, guid_equal)
{
  const GUID_t guid1 = { {0, 1, 2}, {{3, 4, 5}, 6} };
  const GUID_t guid2 = { {0, 1, 2}, {{3, 4, 5}, 7} };

  EXPECT_EQ(guid1, guid1);
  EXPECT_EQ(guid2, guid2);
  EXPECT_NE(guid1, guid2);
}

TEST(tools_dds_rtpsrelaylib_Utility, assign_guids)
{
  const OpenDDS::DCPS::GUID_t guid = { {0, 1, 2}, {{3, 4, 5}, 6} };
  const GUID_t expected_guid = { {0, 1, 2}, {{3, 4, 5}, 6} };

  GUID_t actual_guid;
  assign(actual_guid, guid);

  EXPECT_EQ(actual_guid, expected_guid);
}

TEST(tools_dds_rtpsrelaylib_Utility, duration_equal)
{
  const Duration_t duration1(1,2);
  const Duration_t duration2(3,4);

  EXPECT_EQ(duration1, duration1);
  EXPECT_EQ(duration2, duration2);
  EXPECT_NE(duration1, duration2);
}

TEST(tools_dds_rtpsrelaylib_Utility, duration_less_than)
{
  const Duration_t duration1(1,2);
  const Duration_t duration2(3,4);
  const Duration_t duration3(1,3);

  EXPECT_LT(duration1, duration2);
  EXPECT_LT(duration1, duration3);
}

TEST(tools_dds_rtpsrelaylib_Utility, time_diff_to_duration)
{
  const OpenDDS::DCPS::TimeDuration duration(1,200000);
  const Duration_t expected_duration(1,200000000);

  Duration_t actual_duration;
  actual_duration = time_diff_to_duration(duration);

  EXPECT_EQ(actual_duration, expected_duration);
}

TEST(tools_dds_rtpsrelaylib_Utility, relay_guid_to_rtps_guid)
{
  const GUID_t guid = { {0, 1, 2}, {{3, 4, 5}, 6} };
  const OpenDDS::DCPS::GUID_t expected_guid = { {0, 1, 2}, {{3, 4, 5}, 6} };

  const OpenDDS::DCPS::GUID_t actual_guid = relay_guid_to_rtps_guid(guid);

  EXPECT_EQ(actual_guid, expected_guid);
}

TEST(tools_dds_rtpsrelaylib_Utility, rtps_guid_to_relay_guid)
{
  const OpenDDS::DCPS::GUID_t guid = { {0, 1, 2}, {{3, 4, 5}, 6} };
  const GUID_t expected_guid = { {0, 1, 2}, {{3, 4, 5}, 6} };

  const GUID_t actual_guid = rtps_guid_to_relay_guid(guid);

  EXPECT_EQ(actual_guid, expected_guid);
}

TEST(tools_dds_rtpsrelaylib_Utility, GuidHash)
{
  const OpenDDS::DCPS::GUID_t guid = { {0, 1, 2}, {{3, 4, 5}, 6} };
  const size_t expected = 4;
  const size_t actual = GuidHash()(guid);

  EXPECT_EQ(expected, actual);
}

#endif
