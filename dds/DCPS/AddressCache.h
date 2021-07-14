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
#include "RcObject.h"

#include "ace/INET_Addr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

typedef OPENDDS_SET(ACE_INET_Addr) AddrSet;

struct AddressCacheEntry : public virtual RcObject {

  AddressCacheEntry() : addrs_(), expires_(MonotonicTimePoint::max_value) {}
  AddressCacheEntry(const AddrSet& addrs, const MonotonicTimePoint& expires) : addrs_(addrs), expires_(expires) {}

  AddrSet addrs_;
  MonotonicTimePoint expires_;
};

struct AddressCacheEntryProxy {
  AddressCacheEntryProxy(RcHandle<AddressCacheEntry> rch) : entry_(rch) {}

  bool operator<(const AddressCacheEntryProxy& rhs) const {
    return (rhs.entry_ && (!entry_ || (entry_->addrs_ < rhs.entry_->addrs_)));
  }

  RcHandle<AddressCacheEntry> entry_;
};

template <typename Key>
class AddressCache {
public:

  typedef OPENDDS_MAP_T(Key, RcHandle<AddressCacheEntry>) MapType;

  AddressCache() {}
  virtual ~AddressCache() {}

  struct ScopedAccess {
    ScopedAccess(AddressCache& cache, const Key& key)
      : guard_(cache.mutex_)
      , rch_()
      , is_new_(false)
    {
      typename MapType::iterator pos = cache.map_.find(key);
      if (pos == cache.map_.end()) {
        rch_ = make_rch<AddressCacheEntry>();
        cache.map_[key] = rch_;
        is_new_ = true;
      } else {
        rch_ = pos->second;
      }

      if (rch_->expires_ < MonotonicTimePoint::now()) {
        rch_->addrs_.clear();
        rch_->expires_ = MonotonicTimePoint::max_value;
        is_new_ = true;
      }
    }

    inline AddressCacheEntry& value() {
      OPENDDS_ASSERT(rch_);
      return *rch_;
    }

    inline const AddressCacheEntry& value() const {
      OPENDDS_ASSERT(rch_);
      return *rch_;
    }

    ACE_Guard<ACE_Thread_Mutex> guard_;
    RcHandle<AddressCacheEntry> rch_;
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
      if (MonotonicTimePoint::now() < pos->second->expires_) {
        const AddrSet& as = pos->second->addrs_;
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
    RcHandle<AddressCacheEntry>& rch = map_[key];
    if (rch) {
      rch->addrs_ = addrs;
      rch->expires_ = expires;
    } else {
      rch = make_rch<AddressCacheEntry>(addrs, expires);
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
