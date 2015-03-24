#ifndef OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
#define OPENDDS_DCPS_SAFETY_PROFILE_POOL_H

#ifdef OPENDDS_SAFETY_PROFILE
#include "ace/Malloc_Base.h"
#include "ace/Atomic_Op.h"
#include "ace/Singleton.h"
#include "dcps_export.h"

class PoolAllocationTest;
class PoolTest;
class SafetyProfilePoolTest;

namespace OpenDDS {
namespace DCPS {

// An Allocated segment of the pool
class OpenDDS_Dcps_Export PoolAllocation {
  friend class PoolAllocationTest;
public:
  
  PoolAllocation();
  char* ptr() { return ptr_; }
  size_t size() { return size_; }

  void set(void* ptr, size_t size);
  // Allocate some of my bytes into target, leaving remainder
  void* allocate(size_t size, PoolAllocation* target);

  char* ptr_;
  size_t size_;
  PoolAllocation* next_free_;  // Next smallest free block
  bool free_;
};

class OpenDDS_Dcps_Export Pool {
  friend class ::PoolTest;
public:
  Pool(size_t size, size_t max_allocs);
  ~Pool();
  bool include(void* ptr) { return (pool_ptr_ <= ptr) && (ptr < pool_end_); }
  char* pool_alloc(size_t size);
  void pool_free(void* ptr);

private:
  size_t pool_size_;
  char* pool_ptr_;
  char* pool_end_;          // Past the end of pool
  size_t max_allocs_;
  size_t allocs_in_use_;
  PoolAllocation* allocs_;
  PoolAllocation* first_free_;  // Largest free block

  char* allocate_block(PoolAllocation* from_block,
                       PoolAllocation* prev_block,
                       size_t alloc_size);

  // Slide array members down
  PoolAllocation* make_room_for_allocation(int index);

  void reorder_block(PoolAllocation* resized_block, PoolAllocation* prev_block);
};

/// Memory pool for use when the Safety Profeile is enabled.
///
/// Saftey Profile disallows std::free() and the delete operators
/// See PoolAllocator.h for a class that allows STL containers to use an
/// instance of SafetyProfilePool managed by our Service_Participant singleton.
///
/// This class currently allocates from a single array and doesn't attempt to
/// reuse a free()'d block.  That's expected to change as Safety Profile
/// work is completed and becomes ready for production use.
class SafetyProfilePool : public ACE_Allocator
{
  friend class SafetyProfilePoolTest;
public:
  SafetyProfilePool()
  : init_pool_(new Pool(1024, 128))
  , main_pool_(0)
  {
  }

  ~SafetyProfilePool()
  {
#ifndef OPENDDS_SAFETY_PROFILE
    delete init_pool_;
    delete main_pool_;
#endif
  }

  void* malloc(std::size_t size)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, lock, lock_, 0);
    if (main_pool_) {
      return main_pool_->pool_alloc(size);
    } else {
      return init_pool_->pool_alloc(size);
    }
  }

  void free(void*)
  {
    ACE_GUARD(ACE_Thread_Mutex, lock, lock_);
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
  static SafetyProfilePool* instance() {
    return ACE_Singleton<SafetyProfilePool, ACE_SYNCH_MUTEX>::instance();
  }

private:
  SafetyProfilePool(const SafetyProfilePool&);
  SafetyProfilePool& operator=(const SafetyProfilePool&);

  Pool* init_pool_;
  Pool* main_pool_;
  ACE_Thread_Mutex lock_;
  //ACE_Atomic_Op<ACE_Thread_Mutex, unsigned long> idx_;
};

}}
#endif // OPENDDS_SAFETY_PROFILE
#endif // OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
