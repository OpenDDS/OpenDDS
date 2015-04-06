#ifndef OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
#define OPENDDS_DCPS_SAFETY_PROFILE_POOL_H

#include "ace/Malloc_Base.h"

#ifdef OPENDDS_SAFETY_PROFILE
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

  // Set the size and pointer
  void set(void* ptr, size_t size);
  void set(void* ptr, size_t size, bool free);
  // Allocate some of my bytes into target, leaving remainder
  void* allocate(size_t size, PoolAllocation* target);
  // Join this freed alloc with the one next to it.  Return true if joined
  bool join_freed(PoolAllocation* next);

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
  bool includes(void* ptr) { return (pool_ptr_ <= ptr) && (ptr < pool_end_); }
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
  bool debug_log_;

  ////////////////////////
  // Related to allocation
  ////////////////////////

  // Allocate a block of memory from a free block
  char* allocate_block(PoolAllocation* from_block,
                       PoolAllocation* prev_block,
                       size_t alloc_size);

  // Slide array members down at index, and adjust impacted free pointers
  PoolAllocation* make_room_for_allocation(unsigned int index);

  // When a free block is partially allocated, and gets smaller than the next
  // free block, it must move ahead in the free list
  void move_free_block_ahead(PoolAllocation* resized_block,
                             PoolAllocation* prev_block);

  //////////////////
  // Related to free
  //////////////////

  // Find an allocation for freeing
  PoolAllocation* find_alloc(void* ptr);

  // Join this newly freed alloc with either adjacent free block, shift
  // allocs and adjust impacted free pointers
  void join_free_allocs(PoolAllocation* alloc);

  // Slide array members up to index, by count number of slots
  void recover_unused_allocation(unsigned int index, unsigned int count);

  // Account for shifted memory and size changes in free list
  void adjust_free_list_after_joins(PoolAllocation* src,
                                    unsigned int join_count,
                                    PoolAllocation* new_or_grown,
                                    PoolAllocation* to_remove);

  void log_allocs();
  void validate();
};

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

  void configure_pool(size_t size, size_t allocs);

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
    } else {
      // If this is uncommented, crash
      // return init_pool_->pool_free(ptr);
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

  Pool* init_pool_;
  Pool* main_pool_;
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
