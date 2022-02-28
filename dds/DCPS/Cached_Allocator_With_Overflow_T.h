/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CACHED_ALLOCATOR_WITH_OVERFLOW_T_H
#define OPENDDS_DCPS_CACHED_ALLOCATOR_WITH_OVERFLOW_T_H

#include "debug.h"
#include "SafetyProfilePool.h"
#include "PoolAllocationBase.h"

#include <ace/Atomic_Op.h>
#include <ace/Free_List.h>
#include <ace/Guard_T.h>
#include <ace/Malloc_Allocator.h>
#include <ace/Malloc_T.h>
#include <ace/Message_Block.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
* @class Cached_Allocator_With_Overflow
*
* @brief A fixed-size allocator that caches items for quicker access
*        but if the pool is exhausted it will use the heap.
*
* This class enables caching of dynamically allocated,
* fixed-sized classes.  Notice that the <code>sizeof (TYPE)</code>
* must be greater than or equal to <code> sizeof (void*) </code> for
* this to work properly.
* If the free list is empty then memory is allocated from the heap.
* This way the allocations will not fail but may be slower.
*
*/
template <class T, class ACE_LOCK>
class Cached_Allocator_With_Overflow : public ACE_New_Allocator, public PoolAllocationBase {
public:
  /// Create a cached memory pool with @a n_chunks chunks
  /// each with sizeof (TYPE) size.
  explicit Cached_Allocator_With_Overflow(size_t n_chunks)
    : free_list_(ACE_PURE_FREE_LIST)
    , n_chunks_(n_chunks)
  {
    // To maintain alignment requirements, make sure that each element
    // inserted into the free list is aligned properly for the platform.
    // Since the memory is allocated as a char[], the compiler won't help.
    // To make sure enough room is allocated, round up the size so that
    // each element starts aligned.
    //
    // NOTE - this would probably be easier by defining begin_ as a pointer
    // to T and allocating an array of them (the compiler would probably
    // take care of the alignment for us), but then the ACE_NEW below would
    // require a default constructor on T - a requirement that is not in
    // previous versions of ACE
    size_t chunk_size = sizeof(T);
    chunk_size = ACE_MALLOC_ROUNDUP(chunk_size, ACE_MALLOC_ALIGN);
    begin_ = static_cast<unsigned char*> (ACE_Allocator::instance()->malloc(n_chunks * chunk_size));

    // Remember end of the pool.
    end_ = begin_ + n_chunks * chunk_size;

    // Put into free list using placement contructor, no real memory
    // allocation in the <new> below.
    for (size_t c = 0; c < n_chunks; c++) {
      void* placement = begin_ + c * chunk_size;
      this->free_list_.add(new(placement) ACE_Cached_Mem_Pool_Node<T>);
    }
  }

  /// Clear things up.
  ~Cached_Allocator_With_Overflow()
  {
    ACE_Allocator::instance()->free(begin_);
  }
  /**
  * Get a chunk of memory from free list cache.  Note that @a nbytes is
  * only checked to make sure that it's less or equal to sizeof T, and is
  * otherwise ignored since @c malloc() always returns a pointer to an
  * item of sizeof (T).
  */
  void* malloc(size_t nbytes = sizeof(T))
  {
    // Check if size requested fits within pre-determined size.
    if (nbytes > sizeof(T))
      return 0;

    // addr() call is really not absolutely necessary because of the way
    // ACE_Cached_Mem_Pool_Node's internal structure arranged.
    void* rtn =  this->free_list_.remove()->addr();
    if (0 == rtn) {
      return ACE_Allocator::instance()->malloc(sizeof(T));
    }

    if (DCPS_debug_level >= 6 && this->available() % 512 == 0) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Cached_Allocator_With_Overflow::malloc %@"
                 " %Lu available from pool\n", this, this->available()));
    }
    return rtn;
  }

  /**
  * Get a chunk of memory from free list cache, giving them
  * @a initial_value.  Note that @a nbytes is only checked to make sure
  * that it's less or equal to sizeof T, and is otherwise ignored since
  * calloc() always returns a pointer to an item of sizeof (T).
  */
  virtual void *calloc(size_t /* nbytes */,
                       char /* initial_value */ = '\0')
  {
    ACE_NOTSUP_RETURN(0);
  }

  /// This method is a no-op and just returns 0 since the free list
  /// only works with fixed sized entities.
  virtual void *calloc(size_t /* n_elem */,
                       size_t /* elem_size */,
                       char /* initial_value */ = '\0')
  {
    ACE_NOTSUP_RETURN(0);
  }

  /// Return a chunk of memory back to free list cache.
  void free(void* ptr)
  {
    unsigned char* tmp = static_cast<unsigned char*>(ptr);

    if (tmp < begin_ || tmp >= end_) {
      ACE_Allocator::instance()->free(tmp);
    } else if (ptr != 0) {
      this->free_list_.add((ACE_Cached_Mem_Pool_Node<T> *) ptr) ;

      if (DCPS_debug_level >= 6 && this->available() % 512 == 0) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) Cached_Allocator_With_Overflow::free %@"
                   " %Lu available from pool\n", this, this->available()));
      }
    }
  }

  // -- for debug

  /** How many chunks are available at this time.
  */
  size_t available() { return free_list_.size(); }

  size_t n_chunks() const { return n_chunks_; }

private:
  /// Remember how we allocate the memory in the first place so
  /// we can clear things up later.
  unsigned char* begin_;
  /// The end of the pool.
  unsigned char* end_;

  /// Maintain a cached memory free list.
  ACE_Locked_Free_List<ACE_Cached_Mem_Pool_Node<T>, ACE_LOCK> free_list_;

  const size_t n_chunks_;
};

typedef Cached_Allocator_With_Overflow<ACE_Message_Block, ACE_Thread_Mutex> MessageBlockAllocator;
typedef Cached_Allocator_With_Overflow<ACE_Data_Block, ACE_Thread_Mutex> DataBlockAllocator;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_CACHED_ALLOCATOR_WITH_OVERFLOW_T_H */
