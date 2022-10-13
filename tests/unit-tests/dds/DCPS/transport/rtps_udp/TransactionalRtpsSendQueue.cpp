/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/transport/rtps_udp/TransactionalRtpsSendQueue.h>

#include "util.h"

using namespace OpenDDS::DCPS;
using namespace test;

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, enqueue)
{
  TransactionalRtpsSendQueue sq;

  EXPECT_TRUE(sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false)));
  EXPECT_FALSE(sq.enqueue(create_heartbeat(w1, r2, 1, 2, 300, false)));

  MetaSubmessageVec actual;
  sq.begin_transaction();
  sq.ready_to_send();
  sq.end_transaction(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, false));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_TRUE(meta_submessage_vec_equal(actual, expected));
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, enqueue_vector)
{
  TransactionalRtpsSendQueue sq;

  MetaSubmessageVec vec;
  vec.push_back(create_heartbeat(w1, r1, 1, 2, 300, false));
  vec.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));
  EXPECT_TRUE(sq.enqueue(vec));
  vec.clear();
  EXPECT_FALSE(sq.enqueue(vec));

  MetaSubmessageVec actual;
  sq.begin_transaction();
  sq.ready_to_send();
  sq.end_transaction(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, false));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_TRUE(meta_submessage_vec_equal(actual, expected));
}

TEST(dds_DCPS_TransactionalRtpsSendQueue, begin_and_end_transaction)
{
  TransactionalRtpsSendQueue sq;

  sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false));
  sq.enqueue(create_heartbeat(w1, r2, 1, 2, 300, false));

  MetaSubmessageVec actual;
  sq.begin_transaction();
  sq.begin_transaction();
  sq.ready_to_send();
  sq.end_transaction(actual);
  EXPECT_TRUE(actual.empty());
  sq.end_transaction(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, false));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_TRUE(meta_submessage_vec_equal(actual, expected));
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, purge)
{
  TransactionalRtpsSendQueue sq;

  sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false));
  sq.enqueue(create_heartbeat(w1, r2, 1, 2, 300, false));

  sq.ignore(w1, r1);

  MetaSubmessageVec actual;
  sq.begin_transaction();
  sq.ready_to_send();
  sq.end_transaction(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, true));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_TRUE(meta_submessage_vec_equal(actual, expected));
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, purge_remote)
{
  TransactionalRtpsSendQueue sq;

  sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false));
  sq.enqueue(create_heartbeat(w1, r2, 1, 2, 300, false));

  sq.ignore_remote(r1);

  MetaSubmessageVec actual;
  sq.begin_transaction();
  sq.ready_to_send();
  sq.end_transaction(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, true));
  expected.push_back(create_heartbeat(w1, r2, 1, 2, 300, false));

  EXPECT_TRUE(meta_submessage_vec_equal(actual, expected));
}

TEST(dds_DCPS_transport_rtps_udp_TransactionalRtpsSendQueue, purge_local)
{
  TransactionalRtpsSendQueue sq;

  sq.enqueue(create_heartbeat(w1, r1, 1, 2, 300, false));
  sq.enqueue(create_heartbeat(w2, r2, 1, 2, 300, false));

  sq.ignore_local(w1);

  MetaSubmessageVec actual;
  sq.begin_transaction();
  sq.ready_to_send();
  sq.end_transaction(actual);

  MetaSubmessageVec expected;
  expected.push_back(create_heartbeat(w1, r1, 1, 2, 300, true));
  expected.push_back(create_heartbeat(w2, r2, 1, 2, 300, false));

  EXPECT_TRUE(meta_submessage_vec_equal(actual, expected));
}
