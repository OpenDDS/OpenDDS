/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/RTPS/LeaseQueue.h>

#include <dds/DCPS/GuidUtils.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

using namespace OpenDDS::RTPS;
using namespace OpenDDS::DCPS;

namespace {

// All times are milliseconds after a whole second, so bucket boundaries fall
// on multiples of 1000.
const time_t base_sec = 1000;

MonotonicTimePoint at_ms(ACE_UINT64 ms)
{
  return MonotonicTimePoint(ACE_Time_Value(static_cast<time_t>(base_sec + ms / 1000),
                                           static_cast<suseconds_t>((ms % 1000) * 1000)));
}

const TimeDuration one_second(1);

GUID_t make_guid(unsigned index)
{
  GUID_t guid = GUID_UNKNOWN;
  guid.guidPrefix[8] = static_cast<CORBA::Octet>(index >> 24);
  guid.guidPrefix[9] = static_cast<CORBA::Octet>(index >> 16);
  guid.guidPrefix[10] = static_cast<CORBA::Octet>(index >> 8);
  guid.guidPrefix[11] = static_cast<CORBA::Octet>(index);
  guid.entityId = ENTITYID_PARTICIPANT;
  return guid;
}

unsigned guid_index(const GUID_t& guid)
{
  return (static_cast<unsigned>(guid.guidPrefix[8]) << 24) |
    (static_cast<unsigned>(guid.guidPrefix[9]) << 16) |
    (static_cast<unsigned>(guid.guidPrefix[10]) << 8) |
    static_cast<unsigned>(guid.guidPrefix[11]);
}

typedef std::map<GUID_t, LeaseQueue::Entry, GUID_tKeyLessThan> Participants;

}

// queue_time_for

TEST(dds_DCPS_RTPS_LeaseQueue, queue_time_for_rounds_down)
{
  const LeaseQueue queue(one_second);
  EXPECT_EQ(queue.queue_time_for(at_ms(5300), at_ms(0)), at_ms(5000));
  EXPECT_EQ(queue.queue_time_for(at_ms(5999), at_ms(0)), at_ms(5000));
}

TEST(dds_DCPS_RTPS_LeaseQueue, queue_time_for_on_boundary)
{
  const LeaseQueue queue(one_second);
  EXPECT_EQ(queue.queue_time_for(at_ms(5000), at_ms(0)), at_ms(5000));
}

TEST(dds_DCPS_RTPS_LeaseQueue, queue_time_for_final_bucket_is_exact)
{
  const LeaseQueue queue(one_second);
  // Rounding down gives a time that is not after now.
  EXPECT_EQ(queue.queue_time_for(at_ms(5300), at_ms(5000)), at_ms(5300));
  EXPECT_EQ(queue.queue_time_for(at_ms(5300), at_ms(5100)), at_ms(5300));
  // Just before the bucket starts, rounding still applies.
  EXPECT_EQ(queue.queue_time_for(at_ms(5300), at_ms(4999)), at_ms(5000));
}

TEST(dds_DCPS_RTPS_LeaseQueue, queue_time_for_zero_granularity_is_exact)
{
  const LeaseQueue queue(TimeDuration::zero_value);
  EXPECT_EQ(queue.queue_time_for(at_ms(5300), at_ms(0)), at_ms(5300));
}

TEST(dds_DCPS_RTPS_LeaseQueue, default_granularity_is_one_second)
{
  const LeaseQueue queue;
  EXPECT_EQ(queue.queue_time_for(at_ms(5300), at_ms(0)), at_ms(5000));
}

// renew

TEST(dds_DCPS_RTPS_LeaseQueue, renew_new_entry)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  EXPECT_FALSE(entry.queued());

  EXPECT_TRUE(queue.renew(entry, make_guid(1), at_ms(5300), at_ms(0)));
  EXPECT_TRUE(entry.queued());
  EXPECT_EQ(entry.expiration(), at_ms(5300));
  EXPECT_EQ(entry.queue_time(), at_ms(5000));
  EXPECT_EQ(queue.size(), 1u);
  EXPECT_EQ(queue.next_due(), at_ms(5000));
}

TEST(dds_DCPS_RTPS_LeaseQueue, renew_longer_lease_does_not_move_entry)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  queue.renew(entry, make_guid(1), at_ms(5300), at_ms(0));

  EXPECT_FALSE(queue.renew(entry, make_guid(1), at_ms(7300), at_ms(2000)));
  EXPECT_EQ(entry.expiration(), at_ms(7300));
  EXPECT_EQ(entry.queue_time(), at_ms(5000));
  EXPECT_EQ(queue.size(), 1u);
}

TEST(dds_DCPS_RTPS_LeaseQueue, renew_shorter_lease_after_queue_time_does_not_move_entry)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  queue.renew(entry, make_guid(1), at_ms(5300), at_ms(0));

  EXPECT_FALSE(queue.renew(entry, make_guid(1), at_ms(5100), at_ms(0)));
  EXPECT_EQ(entry.expiration(), at_ms(5100));
  EXPECT_EQ(entry.queue_time(), at_ms(5000));
  EXPECT_EQ(queue.size(), 1u);
}

TEST(dds_DCPS_RTPS_LeaseQueue, renew_shorter_lease_before_queue_time_moves_entry)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  queue.renew(entry, make_guid(1), at_ms(5300), at_ms(0));

  EXPECT_TRUE(queue.renew(entry, make_guid(1), at_ms(3200), at_ms(0)));
  EXPECT_EQ(entry.expiration(), at_ms(3200));
  EXPECT_EQ(entry.queue_time(), at_ms(3000));
  EXPECT_EQ(queue.size(), 1u);
  EXPECT_EQ(queue.next_due(), at_ms(3000));
}

TEST(dds_DCPS_RTPS_LeaseQueue, queue_time_never_after_expiration)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  const ACE_UINT64 expirations[] = {5300, 9000, 4100, 4100, 12999, 3000, 3001};
  for (size_t i = 0; i < sizeof(expirations) / sizeof(expirations[0]); ++i) {
    queue.renew(entry, make_guid(1), at_ms(expirations[i]), at_ms(0));
    EXPECT_LE(entry.queue_time(), entry.expiration());
    EXPECT_EQ(queue.size(), 1u);
  }
}

// remove

TEST(dds_DCPS_RTPS_LeaseQueue, remove_unqueued_is_noop)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry queued;
  queue.renew(queued, make_guid(1), at_ms(5300), at_ms(0));

  LeaseQueue::Entry unqueued;
  queue.remove(unqueued);
  EXPECT_FALSE(unqueued.queued());
  EXPECT_EQ(queue.size(), 1u);

  queue.remove(queued);
  EXPECT_FALSE(queued.queued());
  EXPECT_TRUE(queue.empty());
  queue.remove(queued);
  EXPECT_TRUE(queue.empty());
}

TEST(dds_DCPS_RTPS_LeaseQueue, remove_from_shared_bucket)
{
  LeaseQueue queue(one_second);
  static const unsigned count = 1000;
  std::vector<LeaseQueue::Entry> entries(count);
  for (unsigned i = 0; i < count; ++i) {
    queue.renew(entries[i], make_guid(i), at_ms(5000 + i), at_ms(0));
    EXPECT_EQ(entries[i].queue_time(), at_ms(5000));
  }
  EXPECT_EQ(queue.size(), size_t(count));

  queue.remove(entries[500]);
  EXPECT_FALSE(entries[500].queued());
  EXPECT_EQ(queue.size(), size_t(count - 1));

  // Every other entry still refers to its own element: removing each one
  // removes exactly one element.
  size_t expected_size = count - 1;
  for (unsigned i = 0; i < count; ++i) {
    if (i == 500) {
      continue;
    }
    EXPECT_TRUE(entries[i].queued());
    queue.remove(entries[i]);
    EXPECT_EQ(queue.size(), --expected_size);
  }
  EXPECT_TRUE(queue.empty());
}

// due

TEST(dds_DCPS_RTPS_LeaseQueue, due)
{
  LeaseQueue queue(one_second);
  EXPECT_TRUE(queue.due(at_ms(10000)) == queue.end());

  LeaseQueue::Entry entry;
  const GUID_t id = make_guid(7);
  queue.renew(entry, id, at_ms(5300), at_ms(0));

  EXPECT_TRUE(queue.due(at_ms(4999)) == queue.end());

  const LeaseQueue::iterator pos = queue.due(at_ms(5000));
  ASSERT_TRUE(pos != queue.end());
  EXPECT_EQ(pos->first, at_ms(5000));
  EXPECT_EQ(pos->second, id);
  EXPECT_TRUE(queue.is_current(entry, pos));
}

// requeue_if_renewed

TEST(dds_DCPS_RTPS_LeaseQueue, requeue_if_renewed_renewed)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  const GUID_t id = make_guid(1);
  queue.renew(entry, id, at_ms(5300), at_ms(0));
  queue.renew(entry, id, at_ms(12300), at_ms(2000));

  ASSERT_TRUE(queue.due(at_ms(5000)) != queue.end());
  EXPECT_TRUE(queue.requeue_if_renewed(entry, id, at_ms(5000)));
  EXPECT_TRUE(entry.queued());
  EXPECT_EQ(entry.queue_time(), at_ms(12000));
  EXPECT_EQ(queue.size(), 1u);
}

TEST(dds_DCPS_RTPS_LeaseQueue, requeue_if_renewed_expired)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  const GUID_t id = make_guid(1);
  queue.renew(entry, id, at_ms(5000), at_ms(0));
  EXPECT_EQ(entry.queue_time(), at_ms(5000));

  // Expires at exactly its expiration.
  EXPECT_FALSE(queue.requeue_if_renewed(entry, id, at_ms(5000)));
  EXPECT_FALSE(entry.queued());
  EXPECT_TRUE(queue.empty());
}

TEST(dds_DCPS_RTPS_LeaseQueue, expiry_is_exact_with_buckets)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  const GUID_t id = make_guid(1);
  queue.renew(entry, id, at_ms(5300), at_ms(0));

  // Comes due at the start of its final bucket and is queued at its exact
  // expiration.
  EXPECT_TRUE(queue.due(at_ms(4999)) == queue.end());
  ASSERT_TRUE(queue.due(at_ms(5000)) != queue.end());
  EXPECT_TRUE(queue.requeue_if_renewed(entry, id, at_ms(5000)));
  EXPECT_EQ(entry.queue_time(), at_ms(5300));

  // Not early.
  EXPECT_TRUE(queue.due(at_ms(5299)) == queue.end());
  // Not late.
  ASSERT_TRUE(queue.due(at_ms(5300)) != queue.end());
  EXPECT_FALSE(queue.requeue_if_renewed(entry, id, at_ms(5300)));
  EXPECT_TRUE(queue.empty());
}

TEST(dds_DCPS_RTPS_LeaseQueue, zero_granularity_queues_exact_expiration)
{
  LeaseQueue queue(TimeDuration::zero_value);
  LeaseQueue::Entry entry;
  const GUID_t id = make_guid(1);
  queue.renew(entry, id, at_ms(5300), at_ms(0));
  EXPECT_EQ(entry.queue_time(), at_ms(5300));

  queue.renew(entry, id, at_ms(8300), at_ms(3000));
  EXPECT_EQ(entry.queue_time(), at_ms(5300));
  ASSERT_TRUE(queue.due(at_ms(5300)) != queue.end());
  EXPECT_TRUE(queue.requeue_if_renewed(entry, id, at_ms(5300)));
  EXPECT_EQ(entry.queue_time(), at_ms(8300));
}

// Stale entries and copies

TEST(dds_DCPS_RTPS_LeaseQueue, stale_entry_of_missing_participant)
{
  LeaseQueue queue(one_second);
  Participants participants;
  const GUID_t id = make_guid(1);
  queue.renew(participants[id], id, at_ms(5300), at_ms(0));

  // The participant goes away without removing its entry.
  participants.erase(id);

  const LeaseQueue::iterator pos = queue.due(at_ms(5000));
  ASSERT_TRUE(pos != queue.end());
  EXPECT_TRUE(participants.find(pos->second) == participants.end());
  queue.erase(pos);
  EXPECT_TRUE(queue.empty());
}

TEST(dds_DCPS_RTPS_LeaseQueue, stale_entry_of_recreated_participant)
{
  LeaseQueue queue(one_second);
  Participants participants;
  const GUID_t id = make_guid(1);
  queue.renew(participants[id], id, at_ms(5300), at_ms(0));

  // The participant is replaced by a new one with the same GUID, leaving the
  // old element in the queue.
  participants[id] = LeaseQueue::Entry();
  EXPECT_FALSE(participants[id].queued());
  queue.renew(participants[id], id, at_ms(5600), at_ms(100));
  EXPECT_EQ(queue.size(), 2u);

  // Both elements are queued at 5000; only one is current.
  size_t stale = 0;
  size_t current = 0;
  // Bounded so that a broken is_current fails instead of looping.
  for (int passes = 0; passes < 10; ++passes) {
    const LeaseQueue::iterator pos = queue.due(at_ms(5000));
    if (pos == queue.end()) {
      break;
    }
    LeaseQueue::Entry& entry = participants[pos->second];
    if (queue.is_current(entry, pos)) {
      ++current;
      EXPECT_TRUE(queue.requeue_if_renewed(entry, pos->second, at_ms(5000)));
    } else {
      ++stale;
      queue.erase(pos);
    }
  }
  EXPECT_EQ(stale, 1u);
  EXPECT_EQ(current, 1u);
  EXPECT_EQ(queue.size(), 1u);
  EXPECT_EQ(participants[id].queue_time(), at_ms(5600));
}

TEST(dds_DCPS_RTPS_LeaseQueue, copy_entry)
{
  LeaseQueue queue(one_second);
  LeaseQueue::Entry entry;
  queue.renew(entry, make_guid(1), at_ms(5300), at_ms(0));

  const LeaseQueue::Entry copy(entry);
  EXPECT_TRUE(copy.queued());
  EXPECT_EQ(copy.expiration(), at_ms(5300));
  EXPECT_EQ(copy.queue_time(), at_ms(5000));
  EXPECT_TRUE(queue.is_current(copy, queue.due(at_ms(5000))));

  LeaseQueue::Entry assigned;
  assigned = entry;
  EXPECT_TRUE(queue.is_current(assigned, queue.due(at_ms(5000))));

  const LeaseQueue::Entry unqueued;
  const LeaseQueue::Entry unqueued_copy(unqueued);
  EXPECT_FALSE(unqueued_copy.queued());
}

// Simulation of Spdp's use of LeaseQueue.

namespace {

/**
 * Drives a LeaseQueue the way Spdp does, on a simulated clock.
 *
 * renew() mirrors Spdp::update_lease_expiration_i and process() mirrors
 * Spdp::process_lease_expirations; keep them in sync.  The timer models
 * SporadicEvent, which only ever moves an armed event earlier.
 */
class LeaseSimulation {
public:
  struct Result {
    Result()
      : wakeups(0)
      , wakeups_before_end(0)
      , removals(0)
      , early_removals(0)
      , max_lateness_ms(0)
      , size_mismatches(0)
    {}

    size_t wakeups;
    size_t wakeups_before_end;
    size_t removals;
    size_t early_removals;
    ACE_UINT64 max_lateness_ms;
    size_t size_mismatches;
  };

  LeaseSimulation(const TimeDuration& granularity, ACE_UINT64 lease_ms)
    : queue_(granularity)
    , lease_ms_(lease_ms)
    , timer_armed_(false)
  {}

  void add_renewal(ACE_UINT64 ms, unsigned id)
  {
    renewals_.push_back(Renewal(ms, id));
  }

  /// Run until every participant has expired.  Renewals are handed out in
  /// time order; wake-ups before end_ms are counted separately.
  Result run(ACE_UINT64 end_ms)
  {
    std::stable_sort(renewals_.begin(), renewals_.end());
    result_ = Result();
    size_t next = 0;
    while (next < renewals_.size() || timer_armed_) {
      if (next < renewals_.size() &&
          (!timer_armed_ || at_ms(renewals_[next].ms) <= timer_time_)) {
        renew(renewals_[next].ms, renewals_[next].id);
        ++next;
      } else {
        timer_armed_ = false;
        process(timer_time_);
        if (timer_time_ < at_ms(end_ms)) {
          ++result_.wakeups_before_end;
        }
      }
      if (queue_.size() != participants_.size()) {
        ++result_.size_mismatches;
      }
    }
    EXPECT_TRUE(participants_.empty());
    EXPECT_TRUE(queue_.empty());
    return result_;
  }

private:
  struct Renewal {
    Renewal(ACE_UINT64 m, unsigned i) : ms(m), id(i) {}
    bool operator<(const Renewal& other) const { return ms < other.ms; }
    ACE_UINT64 ms;
    unsigned id;
  };

  void schedule(const MonotonicTimePoint& now, const TimeDuration& delay)
  {
    const MonotonicTimePoint when = now + delay;
    if (!timer_armed_ || when < timer_time_) {
      timer_time_ = when;
      timer_armed_ = true;
    }
  }

  void renew(ACE_UINT64 ms, unsigned id)
  {
    const MonotonicTimePoint now = at_ms(ms);
    const GUID_t guid = make_guid(id);
    LeaseQueue::Entry& lease = participants_[guid];
    last_renewal_ms_[id] = ms;
    if (queue_.renew(lease, guid, now + TimeDuration::from_msec(lease_ms_), now)) {
      schedule(now, lease.queue_time() - now);
    }
  }

  void process(const MonotonicTimePoint& now)
  {
    ++result_.wakeups;
    static const size_t max_lease_entries_per_pass = 256;
    for (size_t i = 0; i < max_lease_entries_per_pass; ++i) {
      const LeaseQueue::iterator pos = queue_.due(now);
      if (pos == queue_.end()) {
        break;
      }

      Participants::iterator part = participants_.find(pos->second);
      if (part == participants_.end() || !queue_.is_current(part->second, pos)) {
        queue_.erase(pos);
        continue;
      }

      if (queue_.requeue_if_renewed(part->second, part->first, now)) {
        continue;
      }

      record_removal(guid_index(part->first), now);
      participants_.erase(part);
      break;
    }

    if (!queue_.empty()) {
      // Processing takes no simulated time, so now stands in for
      // MonotonicTimePoint::now().
      schedule(now, std::max(queue_.next_due() - now, TimeDuration::from_msec(1)));
    }
  }

  void record_removal(unsigned id, const MonotonicTimePoint& now)
  {
    ++result_.removals;
    const MonotonicTimePoint expiration = at_ms(last_renewal_ms_[id] + lease_ms_);
    if (now < expiration) {
      ++result_.early_removals;
      return;
    }
    const ACE_UINT64 lateness_ms = static_cast<ACE_UINT64>((now - expiration).value().msec());
    result_.max_lateness_ms = std::max(result_.max_lateness_ms, lateness_ms);
  }

  LeaseQueue queue_;
  const ACE_UINT64 lease_ms_;
  Participants participants_;
  std::vector<Renewal> renewals_;
  std::map<unsigned, ACE_UINT64> last_renewal_ms_;
  bool timer_armed_;
  MonotonicTimePoint timer_time_;
  Result result_;
};

/**
 * 1000 participants join 7 ms apart and renew every 3 s with a 10 s lease.
 * Every tenth one stops renewing after 6 s; the rest stop at 60 s.  All
 * expirations are at distinct times.
 */
LeaseSimulation::Result run_steady(const TimeDuration& granularity)
{
  static const unsigned count = 1000;
  static const ACE_UINT64 lease_ms = 10000;
  static const ACE_UINT64 renew_ms = 3000;
  static const ACE_UINT64 end_ms = 60000;
  LeaseSimulation sim(granularity, lease_ms);
  for (unsigned i = 0; i < count; ++i) {
    const ACE_UINT64 join_ms = 7 * i;
    const ACE_UINT64 last_ms = (i % 10 == 0) ? join_ms + 2 * renew_ms : end_ms;
    for (ACE_UINT64 ms = join_ms; ms <= last_ms; ms += renew_ms) {
      sim.add_renewal(ms, i);
    }
  }
  return sim.run(end_ms);
}

/**
 * 10000 participants join 1 ms apart, as after a relay restart, and renew
 * every 20 s with a 65 s lease until 200 s.
 */
LeaseSimulation::Result run_burst(const TimeDuration& granularity)
{
  static const unsigned count = 10000;
  static const ACE_UINT64 lease_ms = 65000;
  static const ACE_UINT64 renew_ms = 20000;
  static const ACE_UINT64 end_ms = 200000;
  LeaseSimulation sim(granularity, lease_ms);
  for (unsigned i = 0; i < count; ++i) {
    for (ACE_UINT64 ms = i; ms <= end_ms; ms += renew_ms) {
      sim.add_renewal(ms, i);
    }
  }
  return sim.run(end_ms);
}

void expect_correct(const LeaseSimulation::Result& result, size_t count)
{
  EXPECT_EQ(result.removals, count);
  EXPECT_EQ(result.early_removals, 0u);
  EXPECT_EQ(result.size_mismatches, 0u);
}

}

TEST(dds_DCPS_RTPS_LeaseQueue, simulation_steady)
{
  const LeaseSimulation::Result bucketed = run_steady(one_second);
  expect_correct(bucketed, 1000);
  EXPECT_LE(bucketed.max_lateness_ms, 1u);

  const LeaseSimulation::Result exact = run_steady(TimeDuration::zero_value);
  expect_correct(exact, 1000);
  EXPECT_LE(exact.max_lateness_ms, 1u);
}

TEST(dds_DCPS_RTPS_LeaseQueue, simulation_burst)
{
  const LeaseSimulation::Result bucketed = run_burst(one_second);
  expect_correct(bucketed, 10000);
  // Everyone stops renewing at once and expirations are 1 ms apart.  Passes
  // that re-queue a full bucket delay the expirations behind them slightly.
  EXPECT_LE(bucketed.max_lateness_ms, 5u);

  const LeaseSimulation::Result exact = run_burst(TimeDuration::zero_value);
  expect_correct(exact, 10000);
  EXPECT_LE(exact.max_lateness_ms, 1u);

  // Buckets let one wake-up handle many entries.
  EXPECT_LT(bucketed.wakeups_before_end * 20, exact.wakeups_before_end);

  std::cout << "burst wake-ups before end: bucketed " << bucketed.wakeups_before_end
            << ", exact " << exact.wakeups_before_end
            << "; max lateness ms: bucketed " << bucketed.max_lateness_ms
            << ", exact " << exact.max_lateness_ms << std::endl;
}
