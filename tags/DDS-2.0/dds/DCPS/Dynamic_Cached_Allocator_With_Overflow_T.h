/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DYNAMIC_CACHED_ALLOCATOR_WITH_OVERFLOW_T_H
#define DYNAMIC_CACHED_ALLOCATOR_WITH_OVERFLOW_T_H

#include "debug.h"
#include "ace/Malloc_Allocator.h"
#include "ace/Malloc_T.h"
#include "ace/Free_List.h"
#include "ace/Guard_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

/**
* @class Dynamic_Cached_Allocator_With_Overflow
*
* @brief A size-based allocator that caches blocks for quicker access,
*        but if the pool is exhausted it will use the heap.
*
* This class enables caching of dynamically allocated,
* fixed-size chunks.  Notice that the <code>chunk_size</code>
* must be greater than or equal to <code> sizeof (void*) </code> for
* this to work properly.
*
* This class can be configured flexibly with different types of
* ACE_LOCK strategies that support the @a ACE_Thread_Mutex and @a
* ACE_Process_Mutex constructor API.
*/
template <class ACE_LOCK>
class Dynamic_Cached_Allocator_With_Overflow : public ACE_New_Allocator {
public:
  /// Create a cached memory pool with @a n_chunks chunks
  /// each with @a chunk_size size.
  Dynamic_Cached_Allocator_With_Overflow(size_t n_chunks, size_t chunk_size)
  : allocs_from_heap_(0),
      allocs_from_pool_(0),
      frees_to_heap_(0),
      frees_to_pool_(0),
      pool_(0),
      free_list_(ACE_PURE_FREE_LIST),
      chunk_size_(chunk_size) {
    chunk_size_ = ACE_MALLOC_ROUNDUP(chunk_size, ACE_MALLOC_ALIGN);
    ACE_NEW(this->pool_, char[n_chunks * chunk_size_]);

    for (size_t c = 0;
         c < n_chunks;
         c++) {
      void* placement = this->pool_ + c * chunk_size_;

      this->free_list_.add(new(placement) ACE_Cached_Mem_Pool_Node<char>);
    }

    // Put into free list using placement contructor, no real memory
    // allocation in the above <new>.

    // Remember end of the pool.
    last_ = reinterpret_cast<char*>(this->pool_ + n_chunks * chunk_size_);
  }

  /// Clear things up.
  ~Dynamic_Cached_Allocator_With_Overflow() {
    delete [] this->pool_;
    this->pool_ = 0;
    chunk_size_ = 0;
  }

  /**
  * Get a chunk of memory from free list cache.  Note that @a nbytes is
  * only checked to make sure that it's less or equal to @a chunk_size,
  * and is otherwise ignored since malloc() always returns a pointer to an
  * item of @a chunk_size size.
  */
  void *malloc(size_t nbytes = 0) {
    // Check if size requested fits within pre-determined size.
    if (nbytes > chunk_size_)
      return 0;

    // addr() call is really not absolutely necessary because of the way
    // ACE_Cached_Mem_Pool_Node's internal structure arranged.
    void* rtn = this->free_list_.remove()->addr();

    if (0 == rtn) {
      rtn = ACE_reinterpret_cast(void*, new char[chunk_size_]);
      allocs_from_heap_++;

      if (DCPS_debug_level >= 2) {
        if (allocs_from_heap_ == 1 && DCPS_debug_level >= 2)
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) Dynamic_Cached_Allocator_With_Overflow::malloc %x"
                     " %d heap allocs with %d outstanding\n",
                     this, this->allocs_from_heap_,
                     this->allocs_from_heap_ - this->frees_to_heap_));

        if (DCPS_debug_level >= 6)
          if (allocs_from_heap_ % 500 == 0)
            ACE_DEBUG((LM_DEBUG,
                       "(%P|%t) Dynamic_Cached_Allocator_With_Overflow::malloc %x"
                       " %d heap allocs with %d outstanding\n",
                       this, this->allocs_from_heap_,
                       this->allocs_from_heap_ - this->frees_to_heap_));
      }

    } else {
      allocs_from_pool_++;

      if (DCPS_debug_level >= 6)
        if (allocs_from_pool_ % 500 == 0)
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) Dynamic_Cached_Allocator_With_Overflow::malloc %x"
                     " %d pool allocs %d pool free with %d available\n",
                     this, this->allocs_from_pool_,
                     this->frees_to_pool_,
                     this->available()));
    }

    return rtn;
  }

  /**
  * Get a chunk of memory from free list cache, giving them
  * @a initial_value.  Note that @a nbytes is only checked to make sure
  * that it's less or equal to @a chunk_size, and is otherwise ignored
  * since calloc() always returns a pointer to an item of @a chunk_size.
  */
  virtual void *calloc(size_t /* nbytes */,
                       char /* initial_value */ = '\0') {
    ACE_NOTSUP_RETURN(0);
  }

  /// This method is a no-op and just returns 0 since the free list
  /// only works with fixed sized entities.
  virtual void *calloc(size_t /* n_elem */,
                       size_t /* elem_size */,
                       char /* initial_value */ = '\0') {
    ACE_NOTSUP_RETURN(0);
  }

  /// Return a chunk of memory back to free list cache.
  void free(void * ptr) {
    if (ptr < ACE_reinterpret_cast(void*, pool_) ||
        ptr > ACE_reinterpret_cast(void*, last_)) {
      char* tmp = ACE_reinterpret_cast(char *, ptr);
      delete []tmp;
      frees_to_heap_ ++;

      if (frees_to_heap_ > allocs_from_heap_) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR Dynamic_Cached_Allocator_With_Overflow::free %x"
                   " more deletes %d than allocs %d to the heap\n",
                   this,
                   this->frees_to_heap_,
                   this->allocs_from_heap_));
      }

      if (DCPS_debug_level >= 6) {
        if (frees_to_heap_ % 500 == 0) {
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) Dynamic_Cached_Allocator_With_Overflow::free %x"
                     " %d heap allocs with %d oustanding\n",
                     this, this->allocs_from_heap_,
                     this->allocs_from_heap_ - this->frees_to_heap_));
        }
      }

      return;

    } else if (ptr != 0) {
      this->frees_to_pool_ ++;

      if (frees_to_pool_ > allocs_from_pool_) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR Dynamic_Cached_Allocator_With_Overflow::free %x"
                   " more deletes %d than allocs %d from the pool\n",
                   this,
                   this->frees_to_pool_,
                   this->allocs_from_pool_));
      }

      this->free_list_.add((ACE_Cached_Mem_Pool_Node<char> *) ptr) ;

      if (DCPS_debug_level >= 6)
        if (this->available() % 500 == 0)
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) Dynamic_Cached_Allocator_With_Overflow::malloc %x"
                     " %d pool allocs %d pool frees with %d available\n",
                     this, this->allocs_from_pool_, this->frees_to_pool_,
                     this->available()));
    }
  }

  /// Return the number of chunks available in the cache.
  size_t pool_depth() {
    return this->free_list_.size() ;
  }

  // -- for debug

  /** How many chunks are available at this time.
  */
  size_t available() {
    return free_list_.size();
  };

  /// number of allocations from the heap.
  u_long allocs_from_heap_;
  /// number of allocations from the pool.
  u_long allocs_from_pool_;
  /// number of frees returned to the heap
  u_long frees_to_heap_ ;
  /// number of frees returned to the pool
  u_long frees_to_pool_;
private:
  /// Remember how we allocate the memory in the first place so
  /// we can clear things up later.
  char *pool_;

  /// The end of the pool.
  char *last_;

  /// Maintain a cached memory free list. We use @c char as template
  /// parameter, although sizeof(char) is usually less than
  /// sizeof(void*). Really important is that @a chunk_size
  /// must be greater or equal to sizeof(void*).
  ACE_Locked_Free_List<ACE_Cached_Mem_Pool_Node<char>, ACE_LOCK> free_list_;

  /// Remember the size of our chunks.
  size_t chunk_size_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DYNAMIC_CACHED_ALLOCATOR_WITH_OVERFLOW_T_H */
