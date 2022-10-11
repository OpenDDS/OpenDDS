/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/transport/rtps_udp/MetaSubmessage.h>

#include <dds/DCPS/RTPS/BaseMessageUtils.h>
#include <dds/DCPS/RTPS/MessageTypes.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

namespace
{

MetaSubmessage create_heartbeat(const RepoId& from, const RepoId& dst, ACE_INT64 first, ACE_INT64 last, ACE_INT32 count, bool ignore)
{
  MetaSubmessage meta_submessage;

  const HeartBeatSubmessage heartbeat =
  {
    {
      HEARTBEAT,
      FLAG_E,
      HEARTBEAT_SZ
    },
    dst.entityId,
    from.entityId,
    to_rtps_seqnum(SequenceNumber(first)),
    to_rtps_seqnum(SequenceNumber(last)),
    { count }
  };

  meta_submessage.src_guid_ = from;
  meta_submessage.dst_guid_ = dst;
  meta_submessage.sm_.heartbeat_sm(heartbeat);
  meta_submessage.ignore_ = ignore;
  return meta_submessage;
}

MetaSubmessage create_acknack(const RepoId& from, const RepoId& dst, ACE_INT64 base, ACE_INT32 count, bool ignore)
{
  MetaSubmessage meta_submessage;

  LongSeq8 bitmap;

  const AckNackSubmessage acknack =
  {
    {
      ACKNACK,
      FLAG_E,
      0 /*length*/
    },
    from.entityId,
    dst.entityId,
    {
      to_rtps_seqnum(base),
      0 /* num_bits */,
      bitmap
    },
    {
      count
    }
  };

  meta_submessage.src_guid_ = from;
  meta_submessage.dst_guid_ = dst;
  meta_submessage.sm_.acknack_sm(acknack);
  meta_submessage.ignore_ = ignore;
  return meta_submessage;
}

MetaSubmessage create_gap(const RepoId& from, const RepoId& dst, ACE_INT64 start, ACE_INT64 base)
{
  MetaSubmessage meta_submessage;

  LongSeq8 bitmap;

  const GapSubmessage gap =
  {
    {
      GAP,
      FLAG_E,
      0 /*length*/
    },
    from.entityId,
    dst.entityId,
    to_rtps_seqnum(start),
    {
      to_rtps_seqnum(base),
      0 /* num_bits */,
      bitmap
    }
  };

  meta_submessage.src_guid_ = from;
  meta_submessage.dst_guid_ = dst;
  meta_submessage.sm_.gap_sm(gap);
  return meta_submessage;
}

const RepoId w1 = { { 0x01 }, { { 0x00, 0x00, 0x00 }, 0x02 } };
const RepoId w2 = { { 0x01 }, { { 0x01, 0x00, 0x00 }, 0x02 } };
const RepoId r1 = { { 0x01 }, { { 0x00, 0x00, 0x00 }, 0x07 } };
const RepoId r2 = { { 0x01 }, { { 0x01, 0x00, 0x00 }, 0x07 } };

}

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
  EXPECT_EQ(actual, expected);
}
