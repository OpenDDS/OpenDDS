/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "LeaseQueue.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

DCPS::MonotonicTimePoint LeaseQueue::queue_time_for(const DCPS::MonotonicTimePoint& expiration,
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

bool LeaseQueue::renew(Entry& entry, DCPS::GUID_t id,
                       DCPS::MonotonicTimePoint expiration,
                       DCPS::MonotonicTimePoint now)
{
  entry.expiration_ = expiration;
  if (entry.queued_ && entry.pos_->first <= expiration) {
    return false;
  }
  remove(entry);
  insert(entry, id, queue_time_for(expiration, now));
  return true;
}

bool LeaseQueue::requeue_if_renewed(Entry& entry, DCPS::GUID_t id,
                                    DCPS::MonotonicTimePoint now)
{
  remove(entry);
  if (now < entry.expiration_) {
    insert(entry, id, queue_time_for(entry.expiration_, now));
    return true;
  }
  return false;
}

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
