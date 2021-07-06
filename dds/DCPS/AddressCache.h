/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ADDRESSCACHE_H
#define OPENDDS_DCPS_ADDRESSCACHE_H

#include "dcps_export.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Definitions.h"
#include "PoolAllocator.h"
#include "TimeTypes.h"
#include "GuidUtils.h"

#include "ace/INET_Addr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

typedef OPENDDS_SET(ACE_INET_Addr) AddrSet;

struct AddressCacheEntry {
  AddressCacheEntry() : addrs_(), expires_(MonotonicTimePoint::max_value) {}
  AddressCacheEntry(const AddrSet& addrs, const MonotonicTimePoint& expires) : addrs_(addrs), expires_(expires) {}

  AddrSet addrs_;
  MonotonicTimePoint expires_;
};

template <typename Key>
class AddressCache {
public:

  typedef OPENDDS_MAP(Key, AddressCacheEntry) MapType;

  AddressCache() {}
  virtual ~AddressCache() {}

  struct ScopedAccess
  {
    ScopedAccess(AddressCache& cache, const Key& key)
      : guard_(cache.mutex_)
      , find_result_(cache.map_.find(key))
      , value_(find_result_ == cache.map_.end() ? cache.map_[key] : find_result_->second)
      , previous_expired_(value_.expires_ < MonotonicTimePoint::now())
      , is_new_(find_result_ == cache.map_.end() || previous_expired_)
    {
      if (previous_expired_) {
        value_.addrs_.clear();
        value_.expires_ = MonotonicTimePoint::max_value;
      }
    }

    ACE_Guard<ACE_Thread_Mutex> guard_;
    typename MapType::iterator find_result_;
    AddressCacheEntry& value_;
    bool previous_expired_;
    bool is_new_;

  private:
    ScopedAccess();
    ScopedAccess(const ScopedAccess&);
    ScopedAccess& operator=(const ScopedAccess&);
  };

  bool load(const Key& key, AddrSet& addrs) const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    typename MapType::const_iterator pos = map_.find(key);
    if (pos != map_.end()) {
      if (MonotonicTimePoint::now() < pos->second.expires_) {
        const AddrSet& as = pos->second.addrs_;
        for (AddrSet::const_iterator it = as.begin(); it != as.end(); ++it) {
          addrs.insert(*it);
        }
        return true;
      }
    }
    return false;
  }

  void store(const Key& key, const AddrSet& addrs, const MonotonicTimePoint& expires = MonotonicTimePoint::max_value)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    std::pair<typename MapType::iterator, bool> ins = map_.insert(typename MapType::value_type(key, AddressCacheEntry(addrs, expires)));
    if (!ins.second) {
      ins.first->second.addrs_ = addrs;
      ins.first->second.expires_ = expires;
    }
  }

  bool remove(const Key& key)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return map_.erase(key) != 0;
  }

  void remove(const Key& start_key, const Key& end_key)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    typename MapType::iterator start = map_.lower_bound(start_key);
    if (start != map_.end()) {
      typename MapType::iterator end = map_.upper_bound(end_key);
      while (start != end) {
        map_.erase(start++);
      }
    }
  }

  template <typename T>
  void remove_contains(const T& val)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    for (typename MapType::iterator it = map_.begin(); it != map_.end(); /* inc in loop */) {
      if (it->first.contains(val)) {
        map_.erase(it++);
      } else {
        ++it;
      }
    }
  }

private:

  mutable ACE_Thread_Mutex mutex_;
  MapType map_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_RADDRESSCACHE_H */
