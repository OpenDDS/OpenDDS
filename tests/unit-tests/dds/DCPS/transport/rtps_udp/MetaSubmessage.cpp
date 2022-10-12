/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/transport/rtps_udp/MetaSubmessage.h>

#include "util.h"

using namespace OpenDDS::DCPS;
using namespace test;

TEST(dds_DCPS_transport_rtps_udp_MetaSubmessage, DefaultConstructor)
{
  MetaSubmessage m;

  EXPECT_EQ(m.src_guid_, GUID_UNKNOWN);
  EXPECT_EQ(m.dst_guid_, GUID_UNKNOWN);
  EXPECT_FALSE(m.ignore_);
}

TEST(dds_DCPS_transport_rtps_udp_MetaSubmessage, Constructor)
{
  MetaSubmessage m(w1, w2);

  EXPECT_EQ(m.src_guid_, w1);
  EXPECT_EQ(m.dst_guid_, w2);
  EXPECT_FALSE(m.ignore_);
}

TEST(dds_DCPS_transport_rtps_udp_MetaSubmessage, reset_destination)
{
  MetaSubmessage m(w1, w2);
  m.reset_destination();

  EXPECT_EQ(m.src_guid_, w1);
  EXPECT_EQ(m.dst_guid_, GUID_UNKNOWN);
  EXPECT_FALSE(m.ignore_);
}

TEST(dds_DCPS_transport_rtps_udp_MetaSubmessage, dedup_empty)
{
  MetaSubmessageVec vec;
  const size_t marked = dedup(vec);
  EXPECT_TRUE(vec.empty());
  EXPECT_EQ(marked, 0u);
}

TEST(dds_DCPS_transport_rtps_udp_MetaSubmessage, Merging)
{
  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, GUID_UNKNOWN, 2, 11, 3, false));
  expected.push_back(create_heartbeat(w1, GUID_UNKNOWN, 2, 10, 2, true));
  expected.push_back(create_heartbeat(w1, GUID_UNKNOWN, 1, 10, 1, true));
  expected.push_back(create_gap(w1, GUID_UNKNOWN, 1, 1));
  expected.push_back(create_heartbeat(w1, r1, 2, 11, 3, false));
  expected.push_back(create_heartbeat(w1, r1, 2, 10, 2, true));
  expected.push_back(create_heartbeat(w1, r1, 1, 10, 1, true));
  expected.push_back(create_gap(w1, r1, 1, 1));
  expected.push_back(create_acknack(r1, w1, 8, 3, false));
  expected.push_back(create_acknack(r1, w1, 8, 2, true));
  expected.push_back(create_acknack(r1, w1, 8, 1, true));
  expected.push_back(create_heartbeat(w2, r2, 2, 11, 3, false));
  expected.push_back(create_heartbeat(w2, r2, 2, 10, 2, true));
  expected.push_back(create_heartbeat(w2, r2, 1, 10, 1, true));
  expected.push_back(create_gap(w2, r2, 1, 1));
  expected.push_back(create_acknack(r2, w2, 8, 3, false));
  expected.push_back(create_acknack(r2, w2, 8, 2, true));
  expected.push_back(create_acknack(r2, w2, 8, 1, true));

  MetaSubmessageVec actual;

  actual.push_back(create_gap(w1, GUID_UNKNOWN, 1, 1));

  actual.push_back(create_heartbeat(w1, GUID_UNKNOWN, 2, 10, 2, false));
  actual.push_back(create_heartbeat(w1, GUID_UNKNOWN, 1, 10, 1, false));
  actual.push_back(create_heartbeat(w1, GUID_UNKNOWN, 2, 11, 3, false));
  actual.push_back(create_heartbeat(w1, r1, 2, 10, 2, false));
  actual.push_back(create_heartbeat(w1, r1, 1, 10, 1, false));
  actual.push_back(create_heartbeat(w1, r1, 2, 11, 3, false));
  actual.push_back(create_heartbeat(w2, r2, 2, 10, 2, false));
  actual.push_back(create_heartbeat(w2, r2, 1, 10, 1, false));
  actual.push_back(create_heartbeat(w2, r2, 2, 11, 3, false));

  actual.push_back(create_gap(w2, r2, 1, 1));

  actual.push_back(create_acknack(r1, w1, 8, 2, false));
  actual.push_back(create_acknack(r1, w1, 8, 1, false));
  actual.push_back(create_acknack(r1, w1, 8, 3, false));
  actual.push_back(create_acknack(r2, w2, 8, 2, false));
  actual.push_back(create_acknack(r2, w2, 8, 3, false));
  actual.push_back(create_acknack(r2, w2, 8, 1, false));

  actual.push_back(create_gap(w1, r1, 1, 1));

  const size_t marked = dedup(actual);
  EXPECT_EQ(marked, 10u);
  EXPECT_TRUE(meta_submessage_vec_equal(actual, expected));
}
