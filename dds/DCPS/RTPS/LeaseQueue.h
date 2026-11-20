/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef OPENDDS_DCPS_RTPS_LEASE_QUEUE_H
#define OPENDDS_DCPS_RTPS_LEASE_QUEUE_H

#include <dds/Versioned_Namespace.h>

#include <dds/DdsDcpsGuidC.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/TimeTypes.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

/**
 * Time-ordered queue of participant lease expirations.
 *
 * Expirations are tracked lazily.  Each participant has at most one queued
 * entry, queued at a time no later than its actual expiration.  Renewing a
 * lease normally only updates the participant's Entry and leaves the queued
 * entry where it is.  When a queued entry comes due, the owner calls
 * requeue_if_renewed(): a renewed lease is queued again, otherwise the
 * participant has expired.
 *
 * Queue times are rounded down to multiples of the granularity, so entries
 * for leases that expire close together come due together and are handled in
 * one pass.  When rounding down gives a time that is not after now, the entry
 * is in its final bucket and is queued at its exact expiration instead.  So
 * an expiration is detected neither early nor late.
 *
 * Not thread-safe: the caller provides locking.
 */
class LeaseQueue {
public:
  typedef OPENDDS_MULTIMAP(DCPS::MonotonicTimePoint, DCPS::GUID_t) Map;
  typedef Map::iterator iterator;

  /// Default granularity of queue times.
  static const ACE_UINT64 default_granularity_usec = 1000000;

  /// Per-participant lease state, stored with the participant.
  class Entry {
  public:
    Entry()
      : queued_(false)
    {}

    // pos_ is copied only when it is valid.  Copying is allowed so that the
    // owning struct stays copyable, but only one copy of a queued Entry may be
    // used with the queue.
    Entry(const Entry& other)
      : expiration_(other.expiration_)
      , queued_(other.queued_)
    {
      if (queued_) {
        pos_ = other.pos_;
      }
    }

    Entry& operator=(const Entry& other)
    {
      expiration_ = other.expiration_;
      queued_ = other.queued_;
      if (queued_) {
        pos_ = other.pos_;
      }
      return *this;
    }

    /// When the lease actually expires.
    const DCPS::MonotonicTimePoint& expiration() const { return expiration_; }

    /// Whether the entry is in the queue.
    bool queued() const { return queued_; }

    /// The time the entry is queued under.  Only valid if queued().
    const DCPS::MonotonicTimePoint& queue_time() const { return pos_->first; }

  private:
    friend class LeaseQueue;
    DCPS::MonotonicTimePoint expiration_;
    bool queued_;
    iterator pos_;
  };

  LeaseQueue()
    : granularity_usec_(default_granularity_usec)
  {}

  /// A zero granularity queues every entry at its exact expiration.
  explicit LeaseQueue(const DCPS::TimeDuration& granularity)
    : granularity_usec_(to_usec(granularity))
  {}

  /// The time a lease that expires at `expiration` is queued under.
  DCPS::MonotonicTimePoint queue_time_for(const DCPS::MonotonicTimePoint& expiration,
                                          const DCPS::MonotonicTimePoint& now) const
  {
    if (granularity_usec_ == 0) {
      return expiration;
    }
    ACE_UINT64 usec = 0;
    expiration.value().to_usec(usec);
    usec -= usec % granularity_usec_;
    const DCPS::MonotonicTimePoint rounded(
      ACE_Time_Value(static_cast<time_t>(usec / 1000000), static_cast<suseconds_t>(usec % 1000000)));
    return now < rounded ? rounded : expiration;
  }

  /**
   * Record that the lease of participant `id` now expires at `expiration`.
   * Returns true if the entry was (re)queued: the participant is new, or its
   * lease got shorter.  The caller must then make sure its timer fires no
   * later than entry.queue_time().
   */
  bool renew(Entry& entry, const DCPS::GUID_t& id,
             const DCPS::MonotonicTimePoint& expiration,
             const DCPS::MonotonicTimePoint& now)
  {
    entry.expiration_ = expiration;
    if (entry.queued_ && entry.pos_->first <= expiration) {
      return false;
    }
    remove(entry);
    insert(entry, id, queue_time_for(expiration, now));
    return true;
  }

  /// Remove the entry from the queue, if it is queued.  O(1).
  void remove(Entry& entry)
  {
    if (entry.queued_) {
      map_.erase(entry.pos_);
      entry.queued_ = false;
    }
  }

  /// The earliest queued entry if it is due at `now`, otherwise end().
  iterator due(const DCPS::MonotonicTimePoint& now)
  {
    const iterator pos = map_.begin();
    return (pos != map_.end() && pos->first <= now) ? pos : map_.end();
  }

  /// Whether `pos` is the queued entry of `entry`.
  bool is_current(const Entry& entry, iterator pos) const
  {
    return entry.queued_ && entry.pos_ == pos;
  }

  /// Drop a queued entry that no Entry owns.
  void erase(iterator pos)
  {
    map_.erase(pos);
  }

  /**
   * For an entry that came due: if the lease was renewed after the entry was
   * queued, queue it again and return true.  Otherwise remove it from the
   * queue and return false: the lease has expired.
   */
  bool requeue_if_renewed(Entry& entry, const DCPS::GUID_t& id,
                          const DCPS::MonotonicTimePoint& now)
  {
    remove(entry);
    if (now < entry.expiration_) {
      insert(entry, id, queue_time_for(entry.expiration_, now));
      return true;
    }
    return false;
  }

  iterator end() { return map_.end(); }
  bool empty() const { return map_.empty(); }
  size_t size() const { return map_.size(); }

  /// The time of the earliest queued entry.  The queue must not be empty.
  const DCPS::MonotonicTimePoint& next_due() const { return map_.begin()->first; }

private:
  void insert(Entry& entry, const DCPS::GUID_t& id, const DCPS::MonotonicTimePoint& when)
  {
    entry.pos_ = map_.insert(std::make_pair(when, id));
    entry.queued_ = true;
  }

  static ACE_UINT64 to_usec(const DCPS::TimeDuration& duration)
  {
    ACE_UINT64 usec = 0;
    duration.value().to_usec(usec);
    return usec;
  }

  ACE_UINT64 granularity_usec_;
  Map map_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_RTPS_LEASE_QUEUE_H
