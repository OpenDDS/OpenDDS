#ifndef OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
#define OPENDDS_DCPS_SAFETY_PROFILE_POOL_H

#include "ace/Malloc_Base.h"

#ifdef OPENDDS_SAFETY_PROFILE
#include "ace/Atomic_Op.h"
#include "ace/Singleton.h"
#include "dcps_export.h"
#include "MemoryPool.h"

class SafetyProfilePoolTest;

namespace OpenDDS {
namespace DCPS {

/// Memory pool for use when the Safety Profile is enabled.
///
/// Saftey Profile disallows std::free() and the delete operators
/// See PoolAllocator.h for a class that allows STL containers to use an
/// instance of SafetyProfilePool managed by our Service_Participant singleton.
///
/// This class currently allocates from a single array and doesn't attempt to
/// reuse a free()'d block.  That's expected to change as Safety Profile
/// work is completed and becomes ready for production use.
class OpenDDS_Dcps_Export SafetyProfilePool : public ACE_Allocator
{
  friend class SafetyProfilePoolTest;
public:
  SafetyProfilePool();
  ~SafetyProfilePool();

  void configure_pool(size_t size, size_t granularity);

  void* malloc(std::size_t size)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, lock, lock_, 0);
    if (main_pool_) {
      return main_pool_->pool_alloc(size);
    } else {
      return init_pool_->pool_alloc(size);
    }
  }

  void free(void* ptr)
  {
    ACE_GUARD(ACE_Thread_Mutex, lock, lock_);
    if (main_pool_ && main_pool_->includes(ptr)) {
      return main_pool_->pool_free(ptr);
    } else if (init_pool_->includes(ptr)) {
      // If this is uncommented, crash
      return init_pool_->pool_free(ptr);
    }
  }

  void* calloc(std::size_t, char = '\0') { return 0; }
  void* calloc(std::size_t, std::size_t, char = '\0') { return 0; }
  int remove() { return -1; }
  int bind(const char*, void*, int = 0) { return -1; }
  int trybind(const char*, void*&) { return -1; }
  int find(const char*, void*&) { return -1; }
  int find(const char*) { return -1; }
  int unbind(const char*, void*&) { return -1; }
  int unbind(const char*) { return -1; }
  int sync(ssize_t = -1, int = MS_SYNC) { return -1; }
  int sync(void*, size_t, int = MS_SYNC) { return -1; }
  int protect(ssize_t = -1, int = PROT_RDWR) { return -1; }
  int protect(void*, size_t, int = PROT_RDWR) { return -1; }
  void dump() const {}

  /// Return a singleton instance of this class.
  static SafetyProfilePool* instance();

private:
  SafetyProfilePool(const SafetyProfilePool&);
  SafetyProfilePool& operator=(const SafetyProfilePool&);

  MemoryPool* init_pool_;
  MemoryPool* main_pool_;
  ACE_Thread_Mutex lock_;
};

}}

#else // ! OPENDDS_SAFETY_PROFILE

namespace OpenDDS {
namespace DCPS {
typedef ACE_Allocator SafetyProfilePool;
}
}

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
