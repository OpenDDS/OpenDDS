/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/transport/rtps_udp/ThreadedRtpsSendQueue.h>

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
  bitmap.length(0);

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

TEST(dds_DCPS_ThreadedRtpsSendQueue, DefaultConstructor)
{
  ThreadedRtpsSendQueue sq;

  MetaSubmessageVec vec;
  sq.condense_and_swap(vec);

  EXPECT_EQ(vec.size(), 0u);
  vec.clear();
}

TEST(dds_DCPS_ThreadedRtpsSendQueue, EnableDisable)
{
  ThreadedRtpsSendQueue sq;

  sq.enable_thread_queue();
  EXPECT_FALSE(sq.disable_thread_queue());
  sq.enable_thread_queue();

  const ACE_INT64 first = 3;
  const ACE_INT64 last = 5;
  ACE_INT32 hb_count = 0;

  const ACE_INT64 base = 3;
  ACE_INT32 an_count = 0;

  MetaSubmessageVec vec;
  vec.push_back(create_heartbeat(w1, r2, first, last, hb_count++));
  vec.push_back(create_acknack(r1, w2, base, an_count++));
  sq.enqueue(vec);

  sq.enqueue(create_heartbeat(w1, GUID_UNKNOWN, first, last, hb_count++));

  vec.clear();
  sq.condense_and_swap(vec);

  EXPECT_EQ(vec.size(), 0u);
  vec.clear();

  EXPECT_TRUE(sq.disable_thread_queue());
  sq.condense_and_swap(vec);

  EXPECT_EQ(vec.size(), 3u);
  vec.clear();
}

TEST(dds_DCPS_ThreadedRtpsSendQueue, PurgeLocal)
{
  ThreadedRtpsSendQueue sq;

  sq.enable_thread_queue();
  EXPECT_FALSE(sq.disable_thread_queue());
  sq.enable_thread_queue();

  const ACE_INT64 first = 3;
  const ACE_INT64 last = 5;
  ACE_INT32 hb_count = 0;

  const ACE_INT64 base = 3;
  ACE_INT32 an_count = 0;

  MetaSubmessageVec vec;
  vec.push_back(create_heartbeat(w1, r2, first, last, hb_count++));
  vec.push_back(create_acknack(r1, w2, base, an_count++));
  sq.enqueue(vec);

  vec.clear();
  sq.condense_and_swap(vec);

  EXPECT_EQ(vec.size(), 0u);
  vec.clear();

  sq.purge_local(w1);

  EXPECT_TRUE(sq.disable_thread_queue());
  sq.condense_and_swap(vec);

  EXPECT_EQ(vec.size(), 1u);
  EXPECT_EQ(vec[0].sm_._d(), ACKNACK);
  vec.clear();
}

TEST(dds_DCPS_ThreadedRtpsSendQueue, PurgeRemote)
{
  ThreadedRtpsSendQueue sq;

  sq.enable_thread_queue();
  EXPECT_FALSE(sq.disable_thread_queue());
  sq.enable_thread_queue();

  const ACE_INT64 first = 3;
  const ACE_INT64 last = 5;
  ACE_INT32 hb_count = 0;

  const ACE_INT64 base = 3;
  ACE_INT32 an_count = 0;

  MetaSubmessageVec vec;
  vec.push_back(create_heartbeat(w1, r2, first, last, hb_count++));
  vec.push_back(create_acknack(r1, w2, base, an_count++));
  sq.enqueue(vec);

  vec.clear();
  sq.condense_and_swap(vec);

  EXPECT_EQ(vec.size(), 0u);
  vec.clear();

  EXPECT_TRUE(sq.disable_thread_queue());

  sq.purge_remote(w2);

  sq.condense_and_swap(vec);

  EXPECT_EQ(vec.size(), 1u);
  EXPECT_EQ(vec[0].sm_._d(), HEARTBEAT);
  vec.clear();
}
