/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsSendQueue.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsSendQueue::RtpsSendQueue()
: enabled_(true)
, heartbeats_need_merge_(false)
, acknacks_need_merge_(false)
{
}

bool RtpsSendQueue::push_back(const MetaSubmessage& ms)
{
  bool result = false;
  switch (ms.sm_._d()) {
    case RTPS::HEARTBEAT: {
      const std::pair<RepoId, RepoId> key = std::make_pair(ms.src_guid_, ms.dst_guid_);
      const MapType::iterator pos = heartbeat_map_.find(key);
      if (pos == heartbeat_map_.end()) {
        heartbeat_map_.insert(std::make_pair(key, ms));
        result = true;
        heartbeats_need_merge_ = true;
      } else if (pos->second.redundant_ || pos->second.sm_.heartbeat_sm().count.value < ms.sm_.heartbeat_sm().count.value) {
        pos->second = ms;
        result = true;
        heartbeats_need_merge_ = true;
      }
      break;
    }
    case RTPS::ACKNACK: {
      const std::pair<RepoId, RepoId> key = std::make_pair(ms.src_guid_, ms.dst_guid_);
      const MapType::iterator pos = acknack_map_.find(key);
      if (pos == acknack_map_.end()) {
        acknack_map_.insert(std::make_pair(key, ms));
        result = true;
        acknacks_need_merge_ = true;
      } else if (pos->second.redundant_ || pos->second.sm_.acknack_sm().count.value < ms.sm_.acknack_sm().count.value) {
        pos->second = ms;
        result = true;
        acknacks_need_merge_ = true;
      }
      break;
    }
    default: {
      queue_.push_back(ms);
      result = true;
      break;
    }
  };
  return result;
}

bool RtpsSendQueue::merge(RtpsSendQueue& from)
{
  bool result = !from.queue_.empty();
  queue_.insert(queue_.end(), from.queue_.begin(), from.queue_.end());
  from.queue_.clear();
  if (from.heartbeats_need_merge_) {
    from.heartbeats_need_merge_ = false;
    const MapType::iterator limit = from.heartbeat_map_.end();
    for (MapType::iterator it = from.heartbeat_map_.begin(); it != limit; ++it) {
      if (!it->second.redundant_) {
        const MapType::iterator pos = heartbeat_map_.find(it->first);
        if (pos == heartbeat_map_.end()) {
          heartbeat_map_.insert(std::make_pair(it->first, it->second));
          result = true;
          heartbeats_need_merge_ = true;
        } else if (pos->second.redundant_ || pos->second.sm_.heartbeat_sm().count.value < it->second.sm_.heartbeat_sm().count.value) {
          pos->second = it->second;
          result = true;
          heartbeats_need_merge_ = true;
        }
        it->second.redundant_ = true;
      }
    }
  }
  if (from.acknacks_need_merge_) {
    from.acknacks_need_merge_ = false;
    const MapType::iterator limit = from.acknack_map_.end();
    for (MapType::iterator it = from.acknack_map_.begin(); it != limit; ++it) {
      if (!it->second.redundant_) {
        const MapType::iterator pos = acknack_map_.find(it->first);
        if (pos == acknack_map_.end()) {
          acknack_map_.insert(std::make_pair(it->first, it->second));
          result = true;
          acknacks_need_merge_ = true;
        } else if (pos->second.redundant_ || pos->second.sm_.acknack_sm().count.value < it->second.sm_.acknack_sm().count.value) {
          pos->second = it->second;
          result = true;
          acknacks_need_merge_ = true;
        }
        it->second.redundant_ = true;
      }
    }
  }
  return result;
}

void RtpsSendQueue::condense_and_swap(MetaSubmessageVec& vec)
{
  if (heartbeats_need_merge_) {
    heartbeats_need_merge_ = false;
    const MapType::iterator limit = heartbeat_map_.end();
    for (MapType::iterator it = heartbeat_map_.begin(); it != limit; ++it) {
      if (!it->second.redundant_) {
        queue_.push_back(it->second);
        it->second.redundant_ = true;
      }
    }
  }
  if (acknacks_need_merge_) {
    acknacks_need_merge_ = false;
    const MapType::iterator limit = acknack_map_.end();
    for (MapType::iterator it = acknack_map_.begin(); it != limit; ++it) {
      if (!it->second.redundant_) {
        queue_.push_back(it->second);
        it->second.redundant_ = true;
      }
    }
  }
  queue_.swap(vec);
}

void RtpsSendQueue::purge_remote(const RepoId& id)
{
  for (MapType::iterator it = heartbeat_map_.begin(), limit = heartbeat_map_.end(); it != limit;) {
    if (it->first.second == id) {
      heartbeat_map_.erase(it++);
    } else {
      ++it;
    }
  }
  for (MapType::iterator it = acknack_map_.begin(), limit = acknack_map_.end(); it != limit;) {
    if (it->first.second == id) {
      acknack_map_.erase(it++);
    } else {
      ++it;
    }
  }
}

void RtpsSendQueue::purge_local(const RepoId& id)
{
  for (MapType::iterator it = heartbeat_map_.begin(), limit = heartbeat_map_.end(); it != limit;) {
    if (it->first.first == id) {
      heartbeat_map_.erase(it++);
    } else {
      ++it;
    }
  }
  for (MapType::iterator it = acknack_map_.begin(), limit = acknack_map_.end(); it != limit;) {
    if (it->first.first == id) {
      acknack_map_.erase(it++);
    } else {
      ++it;
    }
  }
}

void RtpsSendQueue::enabled(bool enabled)
{
  enabled_ = enabled;
}

bool RtpsSendQueue::enabled() const
{
  return enabled_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
