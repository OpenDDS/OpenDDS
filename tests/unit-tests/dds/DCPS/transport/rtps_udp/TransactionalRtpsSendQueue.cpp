/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/transport/rtps_udp/TransactionalRtpsSendQueue.h>

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

const RepoId w1 = { { 0x01 }, { { 0x00, 0x00, 0x00 }, 0x02 } };
const RepoId w2 = { { 0x01 }, { { 0x01, 0x00, 0x00 }, 0x02 } };
const RepoId r1 = { { 0x01 }, { { 0x00, 0x00, 0x00 }, 0x07 } };
const RepoId r2 = { { 0x01 }, { { 0x01, 0x00, 0x00 }, 0x07 } };

}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, DefaultConstructor)
{
  ThreadStatusManager tsm;
  TransactionalRtpsSendQueue sq(tsm);

  MetaSubmessageVec vec;
  sq.swap(vec);

  EXPECT_EQ(vec.size(), 0u);
  vec.clear();
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, enqueue)
{
  ThreadStatusManager tsm;
  TransactionalRtpsSendQueue sq(tsm);

  EXPECT_TRUE(sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false)));
  EXPECT_FALSE(sq.enqueue(create_heartbeat(w1, r2, 1, 2, 300, false)));

  MetaSubmessageVec actual;
  sq.swap(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, false));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_EQ(actual, expected);
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, enqueue_vector)
{
  ThreadStatusManager tsm;
  TransactionalRtpsSendQueue sq(tsm);

  MetaSubmessageVec vec;
  vec.push_back(create_heartbeat(w1, r1, 1, 2, 300, false));
  vec.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));
  EXPECT_TRUE(sq.enqueue(vec));
  vec.clear();
  EXPECT_FALSE(sq.enqueue(vec));

  MetaSubmessageVec actual;
  sq.swap(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, false));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_EQ(actual, expected);
}

TEST(dds_DCPS_TransactionalRtpsSendQueue, begin_and_end_transaction)
{
  ThreadStatusManager tsm;
  TransactionalRtpsSendQueue sq(tsm);

  EXPECT_EQ(sq.active_transaction_count(), 0u);
  sq.begin_transaction();
  EXPECT_EQ(sq.active_transaction_count(), 1u);
  sq.begin_transaction();
  EXPECT_EQ(sq.active_transaction_count(), 2u);
  sq.end_transaction();
  EXPECT_EQ(sq.active_transaction_count(), 1u);
  sq.end_transaction();
  EXPECT_EQ(sq.active_transaction_count(), 0u);
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, purge)
{
  ThreadStatusManager tsm;
  TransactionalRtpsSendQueue sq(tsm);

  sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false));
  sq.enqueue(create_heartbeat(w1, r2, 1, 2, 300, false));

  sq.purge(w1, r1);

  MetaSubmessageVec actual;
  sq.swap(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, true));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_EQ(actual, expected);
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, purge_remote)
{
  ThreadStatusManager tsm;
  TransactionalRtpsSendQueue sq(tsm);

  sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false));
  sq.enqueue(create_heartbeat(w1, r2, 1, 2, 300, false));

  sq.purge_remote(r1);

  MetaSubmessageVec actual;
  sq.swap(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, true));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_EQ(actual, expected);
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, purge_local)
{
  ThreadStatusManager tsm;
  TransactionalRtpsSendQueue sq(tsm);

  sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false));
  sq.enqueue(create_heartbeat(w2, r2, 1, 2, 300, false));

  sq.purge_local(w1);

  MetaSubmessageVec actual;
  sq.swap(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, true));
  expected.push_back(create_heartbeat(w2, r2, 1, 2, 300, false));

  EXPECT_EQ(actual, expected);
}
