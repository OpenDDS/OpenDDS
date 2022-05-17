/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/transport/rtps_udp/RtpsSendQueue.h>

#include <dds/DCPS/RTPS/BaseMessageUtils.h>
#include <dds/DCPS/RTPS/MessageTypes.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

namespace
{

MetaSubmessage create_heartbeat(const RepoId& from, const RepoId& dst, ACE_INT64 first, ACE_INT64 last, ACE_INT32 count)
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
  return meta_submessage;
}

MetaSubmessage create_acknack(const RepoId& from, const RepoId& dst, ACE_INT64 base, ACE_INT32 count)
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
  return meta_submessage;
}

const RepoId w1 = { { 0x01 }, { { 0x00, 0x00, 0x00 }, 0x02 } };
const RepoId w2 = { { 0x01 }, { { 0x01, 0x00, 0x00 }, 0x02 } };
const RepoId r1 = { { 0x01 }, { { 0x00, 0x00, 0x00 }, 0x07 } };
const RepoId r2 = { { 0x01 }, { { 0x01, 0x00, 0x00 }, 0x07 } };

}

TEST(dds_DCPS_RtpsSendQueue, DefaultConstructor)
{
  RtpsSendQueue sq;

  EXPECT_EQ(sq.enabled(), true);

  sq.enabled(false);
  EXPECT_EQ(sq.enabled(), false);

  sq.enabled(true);

  MetaSubmessageVec vec;
  sq.condense_and_swap(vec);

  EXPECT_EQ(vec.size(), 0u);
  vec.clear();
}

TEST(dds_DCPS_RtpsSendQueue, HandlingDuplicateHeartbeats)
{
  RtpsSendQueue sq;
  MetaSubmessageVec vec;

  const ACE_INT64 first = 3;
  const ACE_INT64 last = 5;
  ACE_INT32 count = 0;

  sq.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w1, r1, first, last, count++));
  sq.push_back(create_heartbeat(w1, r2, first, last, count++));

  sq.push_back(create_heartbeat(w2, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w2, r1, first, last, count++));
  sq.push_back(create_heartbeat(w2, r2, first, last, count++));

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 6u);
  vec.clear();

  sq.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w1, r1, first, last, count++));
  sq.push_back(create_heartbeat(w1, r2, first, last, count++));

  sq.push_back(create_heartbeat(w2, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w2, r1, first, last, count++));
  sq.push_back(create_heartbeat(w2, r2, first, last, count++));

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 6u);
  vec.clear();

  sq.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w1, r1, first, last, count++));
  sq.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w1, r1, first, last, count++));
  sq.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, count++));

  sq.push_back(create_heartbeat(w2, r1, first, last, count++));
  sq.push_back(create_heartbeat(w2, r2, first, last, count++));
  sq.push_back(create_heartbeat(w2, r1, first, last, count++));
  sq.push_back(create_heartbeat(w2, r2, first, last, count++));
  sq.push_back(create_heartbeat(w2, r1, first, last, count++));
  sq.push_back(create_heartbeat(w2, r2, first, last, count++));

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 4u);
  vec.clear();
}

TEST(dds_DCPS_RtpsSendQueue, HandlingDuplicateAcknacks)
{
  RtpsSendQueue sq;
  MetaSubmessageVec vec;

  const ACE_INT64 base = 3;
  ACE_INT32 count = 0;

  sq.push_back(create_acknack(r1, w1, base, count++));
  sq.push_back(create_acknack(r1, w2, base, count++));

  sq.push_back(create_acknack(r2, w1, base, count++));
  sq.push_back(create_acknack(r2, w2, base, count++));

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 4u);
  vec.clear();

  sq.push_back(create_acknack(r1, w1, base, count++));
  sq.push_back(create_acknack(r1, w2, base, count++));

  sq.push_back(create_acknack(r2, w1, base, count++));
  sq.push_back(create_acknack(r2, w2, base, count++));

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 4u);
  vec.clear();

  sq.push_back(create_acknack(r1, w1, base, count++));
  sq.push_back(create_acknack(r1, w1, base, count++));

  sq.push_back(create_acknack(r2, w1, base, count++));
  sq.push_back(create_acknack(r2, w2, base, count++));
  sq.push_back(create_acknack(r2, w1, base, count++));
  sq.push_back(create_acknack(r2, w2, base, count++));
  sq.push_back(create_acknack(r2, w1, base, count++));
  sq.push_back(create_acknack(r2, w2, base, count++));

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 3u);
  vec.clear();
}

TEST(dds_DCPS_RtpsSendQueue, Merging)
{
  RtpsSendQueue sq1;
  RtpsSendQueue sq2;
  MetaSubmessageVec vec;

  const ACE_INT64 first = 3;
  const ACE_INT64 last = 15;
  ACE_INT32 hb_count = 0;

  const ACE_INT64 base = 3;
  ACE_INT32 an_count = 0;

  sq1.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w1, r1, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w1, r1, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, hb_count++));

  sq1.push_back(create_heartbeat(w2, r1, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w2, r2, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w2, r1, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w2, r2, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w2, r1, first, last, hb_count++));
  sq1.push_back(create_heartbeat(w2, r2, first, last, hb_count++));

  sq2.push_back(create_acknack(r1, w1, base, an_count++));
  sq2.push_back(create_acknack(r1, w1, base, an_count++));

  sq2.push_back(create_acknack(r2, w1, base, an_count++));
  sq2.push_back(create_acknack(r2, w2, base, an_count++));
  sq2.push_back(create_acknack(r2, w1, base, an_count++));
  sq2.push_back(create_acknack(r2, w2, base, an_count++));
  sq2.push_back(create_acknack(r2, w1, base, an_count++));
  sq2.push_back(create_acknack(r2, w2, base, an_count++));

  sq1.merge(sq2);
  sq1.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 7u);
  vec.clear();

  sq2.merge(sq1);
  sq2.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 0u);
  vec.clear();

  sq1.push_back(create_acknack(r1, w1, base, an_count++));
  sq1.push_back(create_heartbeat(w2, r1, first, last, hb_count++));

  sq2.merge(sq1);
  sq2.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 2u);
  vec.clear();
}

TEST(dds_DCPS_RtpsSendQueue, Purging)
{
  RtpsSendQueue sq;
  MetaSubmessageVec vec;

  const ACE_INT64 first = 4;
  const ACE_INT64 last = 9;

  const ACE_INT64 base = 7;

  ACE_INT32 count = 4;

  sq.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w1, r1, first, last, count++));
  sq.push_back(create_heartbeat(w1, r2, first, last, count++));

  sq.push_back(create_heartbeat(w2, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w2, r1, first, last, count++));
  sq.push_back(create_heartbeat(w2, r2, first, last, count++));

  sq.purge_remote(r2);

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 4u);
  vec.clear();

  sq.push_back(create_acknack(r1, w1, base, count++));
  sq.push_back(create_acknack(r1, w2, base, count++));

  sq.push_back(create_acknack(r2, w1, base, count++));
  sq.push_back(create_acknack(r2, w2, base, count++));

  sq.purge_local(r1);

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 2u);
  vec.clear();

  sq.push_back(create_heartbeat(w1, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w1, r1, first, last, count++));
  sq.push_back(create_heartbeat(w1, r2, first, last, count++));

  sq.push_back(create_heartbeat(w2, GUID_UNKNOWN, first, last, count++));
  sq.push_back(create_heartbeat(w2, r1, first, last, count++));
  sq.push_back(create_heartbeat(w2, r2, first, last, count++));

  sq.push_back(create_acknack(r1, w1, base, count++));
  sq.push_back(create_acknack(r1, w2, base, count++));

  sq.push_back(create_acknack(r2, w1, base, count++));
  sq.push_back(create_acknack(r2, w2, base, count++));

  sq.purge_remote(r1);
  sq.purge_local(w1);
  sq.purge_remote(w2);
  sq.purge_remote(w2);
  sq.purge_remote(GUID_UNKNOWN);
  sq.purge_local(r1);
  sq.purge_local(r1);
  sq.purge_remote(w1);
  sq.purge_remote(r1);
  sq.purge_local(w1);
  sq.purge_local(w2);
  sq.purge_remote(w1);
  sq.purge_remote(r2);
  sq.purge_remote(r2);
  sq.purge_local(r2);
  sq.purge_local(r2);
  sq.purge_local(w2);

  sq.condense_and_swap(vec);
  EXPECT_EQ(vec.size(), 0u);
  vec.clear();
}
