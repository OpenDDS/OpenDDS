#include "SafetyProfilePool.h"
#include <stdexcept>

namespace OpenDDS {  namespace DCPS {

PoolAllocation::PoolAllocation()
: ptr_(0)
, size_(0)
, next_free_(0)
, free_(false)
{
}

void
PoolAllocation::set(void* ptr, size_t size)
{
  ptr_ = static_cast<char*>(ptr);
  size_ = size;
}

void
PoolAllocation::set(void* ptr, size_t size, bool free)
{
  set(ptr, size);
  free_ = free;
}

bool
PoolAllocation::join_freed(PoolAllocation* next_alloc)
{
  bool joined = false;
  // If both are free
  if (free_ && next_alloc->free_) {
    // If they are contiguous
    if ((ptr_ + size_) == next_alloc->ptr_) {
      size_ += next_alloc->size_;
      joined = true;

      // Clean up next_alloc, but leave size and next_free_ intact
      next_alloc->ptr_ = NULL;
    }
  }
  return joined;
}

void*
PoolAllocation::allocate(size_t alloc_size, PoolAllocation* target)
{
  // Divide my alloc into remainder and target
  size_t remainder = size_ - alloc_size;
  char* next_buff = ptr_ + remainder;
  target->set(next_buff, alloc_size, false);
  target->next_free_ = NULL;
  this->free_ = true;
  size_ = remainder;
  return next_buff;
}

Pool::Pool(size_t size, size_t max_allocs)
: pool_size_(size)
, pool_ptr_(new char[pool_size_])
, pool_end_(pool_ptr_ + pool_size_)
, max_allocs_(max_allocs * 2 + 1) // Allow for fragmentation
, allocs_in_use_(0)
, allocs_(new PoolAllocation[max_allocs_])
, debug_log_(false)
{
  first_free_ = allocs_;
  first_free_->set(pool_ptr_, pool_size_, true);
  allocs_in_use_ = 1;
}

Pool::~Pool()
{
#ifndef OPENDDS_SAFETY_PROFILE
  delete [] pool_ptr_;
  delete [] allocs_;
#endif
}

char*
Pool::pool_alloc(size_t size)
{
  // Pointer to return
  char* block = NULL;
  // The block to allocate from
  PoolAllocation* block_to_alloc = NULL;

  // prev_block is block pointing to the one to be split
  PoolAllocation* prev_block = NULL;

  // Round up to 8-byte boundary
  size_t alloc_size = (size + 7) / 8 * 8;

  // If there are free blocks large enough
  if (first_free_ && first_free_->size_ >= alloc_size) {

    // Find smallest block that fits
    block_to_alloc = first_free_;

    // While there are more "small enough" blocks remaining
    while (block_to_alloc->next_free_ &&
           block_to_alloc->next_free_->size_ >= alloc_size)
    {
      // Move ahead, tracking previous
      prev_block = block_to_alloc;
      block_to_alloc = block_to_alloc->next_free_;
    }

    block = allocate_block(block_to_alloc, prev_block, alloc_size);
    // prev_block may have moved, don't use
  } else {
    printf("first free block not large enough for %zu\n", alloc_size);
  }

  //if (debug_log_) log_allocs();
  //validate();

  return block;
}

void
Pool::pool_free(void* ptr)
{
  PoolAllocation* alloc = find_alloc(ptr);
  if (alloc) {
    debug_log_ = true;
    if (debug_log_) printf("freeing block of %zu at %zu\n",
                           alloc->size(), alloc - allocs_);
    alloc->free_ = true;
    // Check next and prev for combining
    join_free_allocs(alloc);
  }
  //validate();
  //if (debug_log_) log_allocs();
}

char*
Pool::allocate_block(PoolAllocation* from_block,
                     PoolAllocation* prev_block,
                     size_t alloc_size)
{
  char* buffer = 0;

  if (debug_log_) printf("allocating %zu\n", alloc_size);
  if (from_block->size() == alloc_size) {
    // Going to use all of free block, keep size same
    buffer = from_block->ptr();
    // Take out of free list
    if (prev_block) {
      prev_block->next_free_ = from_block->next_free_;
    } else {
      // head of list
      first_free_ = from_block->next_free_;
    }
    from_block->next_free_ = NULL;
    from_block->free_ = false;
    if (debug_log_) printf("allocating %zu from block %zu\n",
                           alloc_size, from_block - allocs_);
  } else if (from_block->size() > alloc_size) {
    unsigned int index = from_block - allocs_;
    // Second clause should always be true,
    // was written to show it has been considered
    if ((allocs_in_use_ < max_allocs_) && (index < max_allocs_ - 1)) {
      PoolAllocation* next_block = from_block->next_free_;
      // Slide alocations down to maintain buffer order
      PoolAllocation* target = make_room_for_allocation(index);
      // Adjust prev_block, if necessary
      if (prev_block >= target) {
        prev_block += 1;
      }
      if (next_block >= target) {
        next_block += 1;
      }

      if (debug_log_) printf("allocating %zu from inserted block %zu\n",
                           alloc_size, target - allocs_);
      // Allocate a buffer using taegetand put remainder in from_block
      buffer = static_cast<char*>(from_block->allocate(alloc_size, target));
      ++allocs_in_use_;

      if (next_block) printf("comparing from block size %zu to next block size %zu\n", from_block->size_, next_block->size_);

      // from_block is now smaller
      if (next_block && from_block->size_ < next_block->size_) {
        move_free_block_ahead(from_block, prev_block);
      }

      // If no next block, the list is in the right order still
    } else {
      printf("out of allocations current %zu\n", allocs_in_use_);
    }

    // Going to split free block, need next one available
  } else {
    // Should not happen
    printf("from block not large enough for %zu\n", alloc_size);
  }

  return buffer;
}

PoolAllocation*
Pool::make_room_for_allocation(unsigned int index)
{
  PoolAllocation* src = allocs_ + index;
  PoolAllocation* dest = src + 1;

  // If this not the last alloc
  if (index < allocs_in_use_ - 1) {
    // After the move, free list pointers will be off by one PoolAllocation.
    // Fix that now
    for (PoolAllocation* iter = first_free_; iter != NULL; )
         
    {
      // If impacted by move
      if (iter->next_free_ > src) {
        PoolAllocation* next = iter->next_free_; // Save value before change
        iter->next_free_ = iter->next_free_ + 1;
        iter = next;
      } else {
        iter = iter->next_free_; // Save value before change
      }
    }

    // Move first_free also
    if (first_free_ > src) {
      first_free_ += 1;
    }
    
    // Move the memory
    memmove(dest, src, sizeof(PoolAllocation) * (allocs_in_use_ - index));
  }

  return dest;
}

void
Pool::recover_unused_allocation(unsigned int index, unsigned int count)
{
  PoolAllocation* dest = allocs_ + index;
  PoolAllocation* src = dest + count;

  // If this not the last alloc
  if (index < allocs_in_use_ - count) {
    // Move the memory
    memmove(dest, src,
            sizeof(PoolAllocation) * (allocs_in_use_ - index - count));
  }

  // Fewer allocs in use
  allocs_in_use_ -= count;
}

PoolAllocation* adjust_for_join(PoolAllocation* free_list_ptr, 
                                PoolAllocation* first,
                                unsigned int join_count)
{
  if (join_count && (free_list_ptr >= first + join_count)) {
    return free_list_ptr - join_count;
  } else {
    return free_list_ptr;
  }
}

void
Pool::adjust_free_list_after_joins(PoolAllocation* first,
                                   unsigned int join_count,
                                   PoolAllocation* new_or_grown,
                                   PoolAllocation* to_remove)

{
  // Find to_remove and remove
  // Find grown insertion point and insert
  // Find grown and remove

  // At each node
    // Prev always points to alloc before iter
    //   when removing iter, point prev to iter's next and keep prev where it is
    // If to_remove, remove it
    // If grown and inserted, remove it
    // If grown and not inserted, leave it
    // If insertion point, insert
    // Remember prev, so you can remove next
  PoolAllocation* prev = NULL;
  PoolAllocation* after_grown = new_or_grown ? new_or_grown->next_free_ : NULL;
  PoolAllocation* smaller_than_new_or_grown = NULL;
  PoolAllocation* iter = first_free_;
  bool inserted = false;

  if (debug_log_) printf("Adjusting free list, count %d, new/grown %zx, to_remove %zx\n", join_count, (unsigned long)new_or_grown, (unsigned long)to_remove);

  while (iter) {
    if (debug_log_) printf("Visiting index %d\n", (int)(iter - allocs_));

    bool iter_removed = false;

    // If after inserting new grown, have reached original position of grown
    // remove grown
    if (inserted && iter == new_or_grown) {
      if (prev) { // should always be true
        if (debug_log_) printf("Removing old position of grown from list\n");

        prev->next_free_ = adjust_for_join(after_grown, first, join_count);
        iter_removed = true;
      }
    // Else if this is the alloc to remove, remove it
    } else if (iter == to_remove) {
      if (!prev) {
        if (debug_log_) printf("Removing to_remove from head\n");

        first_free_ = adjust_for_join(iter->next_free_, first, join_count);
        iter_removed = true;
      } else {
        if (debug_log_) printf("Removing to_remove from list\n");

        prev->next_free_ = adjust_for_join(iter->next_free_, first, join_count);
        iter_removed = true;
      }
    }

    // If iter is still in the list
    if (!iter_removed) {
      // If I have reached the new/grown alloc
      if (iter == new_or_grown && !inserted) {
        // Grown, but not enough to move earlier.  Leave alone
        inserted = true;
        if (debug_log_) printf("grown did not grow enough to change positions\n");

      // Else if I have found a smaller alloc than one to insert
      } else if ((!inserted) &&
                 new_or_grown && new_or_grown->size_ >= iter->size_) {
        if (!prev) {
          if (debug_log_) printf("Inserting new/grown at head\n");

          // First alloc in list, make pool point to it
          first_free_ = new_or_grown;

        } else {
          if (debug_log_) printf("Inserting new/grown inside list\n");

          prev->next_free_ = new_or_grown;
        }

        if (debug_log_) printf("remembering smaller than grown size %zu\n", iter->size());
        smaller_than_new_or_grown = iter;
        inserted = true;
      }

      // Remember prev for next iteration (only if iter is not removed)
      prev = iter;
    }

    // If iter's next pointer is now off
    PoolAllocation* next = iter->next_free_;
    iter->next_free_ = adjust_for_join(iter->next_free_, first, join_count);
    iter = next;
    
/*
    if (join_count && (iter->next_free_ >= first + join_count)) {
      // Save for after adjustment
      PoolAllocation* next = iter->next_free_;

      if (debug_log_) printf("adjusting for joins next free from index %u to %u\n", iter->next_free_ - allocs_, iter->next_free_ - allocs_ - join_count);
      // Adjust for later move
      iter->next_free_ = iter->next_free_ - join_count;

      // Prepare next iteration
      iter = next;
    } else {
      iter = iter->next_free_;
    }
*/
  }

  // May be smallest, and not inserted
  if (new_or_grown && !inserted) {
    if (debug_log_) printf("inserting at end\n");
    if (prev) {
      prev->next_free_ = new_or_grown;
    } else {
      // head of list
      first_free_ = new_or_grown;
    }
  }

  // Lastly, point new_or_grown to proper spot, to prevent a loop in the list
  if (smaller_than_new_or_grown) {
    new_or_grown->next_free_ = adjust_for_join(
      smaller_than_new_or_grown, first, join_count);
/*
    if (join_count && (smaller_than_new_or_grown >= first + join_count)) {
      // adjust for join
      new_or_grown->next_free_ = smaller_than_new_or_grown - join_count;
    } else {
      new_or_grown->next_free_ = smaller_than_new_or_grown;
    }
*/
  }

  if (debug_log_) printf("exiting\n");
}

PoolAllocation*
Pool::find_alloc(void* ptr)
{
  // Linear search, can be changed to binary search
  for (unsigned int i = 0; i < allocs_in_use_; ++i) {
    PoolAllocation* alloc = allocs_ + i;
    if (alloc->ptr() == ptr) {
      return alloc;
    }
  }
  return NULL;
}

void
Pool::move_free_block_ahead(PoolAllocation* resized_block, PoolAllocation* prev_block)
{
  PoolAllocation* next_block = resized_block->next_free_;

  printf("Moving free block ahead\n");

  // Remove resized_block from free list
  resized_block->next_free_ = NULL;
  if (prev_block) {
    // point it ahead
    printf("  Pointing prev to next %zu\n", next_block - allocs_);
    prev_block->next_free_ = next_block;
  } else if (first_free_ == resized_block) {
    // Point to new head of list
    printf("  Pointing head to next %zu\n", next_block - allocs_);
    first_free_ = next_block;
  }

  // Move ahead while the next block exists and is larger than the resized block
  while (next_block && resized_block->size_ < next_block->size_) {
    printf("  Skipping size %zu\n", next_block->size());
    prev_block = next_block;
    next_block = next_block->next_free_;
  }

  // prev_block can't now be null, because resized block getting smaller than
  // next was a precondition for entering
  printf("  Inserting resized block after %zu\n", prev_block - allocs_);
  prev_block->next_free_ = resized_block;

  // next_block is smaller than resized block, or null if none smaller
  resized_block->next_free_ = next_block;
  printf("  Pointing resized block to %zu\n", next_block - allocs_);
}

void
Pool::join_free_allocs(PoolAllocation* freed)
{
  // Four possibilities for next and prev allocs - freed is always free,
  // but is not yet in free list
  //    case  prev  freed   next
  //     I     A      F      A
  //     II    A      F<join>F
  //     III   F<join>F      A
  //     IV    F<join>F<join>F
  // * Case I: neither adjacent block is free.  No joining, no shifting, just
  //   insert freed into free list.
  // * Case II: next is free.  Join freed with next, shift by one, adjust free
  //   pointers, remove next, insert freed into free list.
  // * Case III: prev is free.  Join prev with freed, shift by one, adjust free
  //   pointers, move prev earlier in free list.
  // * Case IV: prev and next are free.  Join both (freed with next first),
  //   shift by two, adjust free pointers, remove next from free list, move
  //   prev earlier in free list.

  // Implementation:
  // * Find out number of joins (0, 1, or 2) and do joins.
  // * Iterate through the (now corrupted) free list, adjusting freed pointers
  //   after next by 1 or 2 PoolAllocs. (Freed is newly freed, so
  //   isn't  in any free lists yet.  Prev didn't move.)  This must be done
  //   to the entire list.
  // * While iterating, remove joined allocs from free list and insert alloc.
  // * Move memory to recover, and reduce allocs_in_use by number of joins.

  unsigned int joined_count = 0;

  // The first alloc which needs to be copied over
  PoolAllocation* first = NULL;
  PoolAllocation* new_or_grown = NULL;
  PoolAllocation* removed = NULL;

  // Try to join with next adjancent alloc if this is not the last alloc
  if (freed != (allocs_ + (allocs_in_use_ - 1))) {
    PoolAllocation* next_alloc = freed + 1;
    if (freed->join_freed(next_alloc)) {
      first = next_alloc;
      ++joined_count;
      removed = next_alloc;
      new_or_grown = freed;
    }
  }

  // Try joining with previous adjacent alloc if this is not the first alloc
  if (freed != allocs_) {
    PoolAllocation* prev_alloc = freed - 1;
    // join freed and prev_alloc
    if (prev_alloc->join_freed(freed)) {
      first = freed;
      ++joined_count;
      new_or_grown = prev_alloc;
    }
  }

  // Adjust free pointers by joined count
  if (joined_count) {
    // Slice up and manipulate free list pointers
    unsigned int index = first - allocs_;
    // Iterate, adjust pointers, sort_allocs
    adjust_free_list_after_joins(first, joined_count, new_or_grown, removed);
    // Move memory
    recover_unused_allocation(index, joined_count);
  } else {
    // Nothing joined, just insert newly freed alloc
    adjust_free_list_after_joins(NULL, 0, freed, NULL);
  }
}

void
Pool::log_allocs() {
  char* expected = 0;
  for (unsigned int i = 0; i < allocs_in_use_; ++i) {
    PoolAllocation* alloc = allocs_ + i;
    int next_index = -1;
    if (alloc->next_free_) {
      next_index = alloc->next_free_ - allocs_;
    }
    printf("[%u] %zx %s %s %zu %d\n",
           i,
           (unsigned long)alloc->ptr_,
           alloc->free_ ? "free " : "alloc",
           first_free_ == alloc ? "first" : "     ",
           alloc->size(),
           next_index);
    if (expected && (alloc->ptr_ != expected)) {
      printf("Alloc %u ptr not expected;\n", i);
    }
    expected = alloc->ptr_ + alloc->size();
  }
}

#define ASSERT(expr, msg) \
  if (!(expr)) throw std::runtime_error(msg);

void
Pool::validate() {
  unsigned int free_nodes = 0;
  char* prev = 0;
  char* prev_end = 0;
  size_t allocated_bytes = 0;
  size_t free_bytes = 0;
  bool prev_free;
  // Check all allocs in positional order and not overlapping
  for (unsigned int i = 0; i < allocs_in_use_; ++i) {
    PoolAllocation* alloc = allocs_ + i;
    if (debug_log_) {
      printf("Index %d: alloc %zx, ptr is %zx, %s size %zu %s nextind %d\n",
              i, (unsigned long)alloc, (unsigned long)(void*)alloc->ptr(),
              alloc->free_ ? "free " : "     ",
              alloc->size(),
              alloc == first_free_ ? "FIRST" : "     ",
              alloc->next_free_ ? int(alloc->next_free_ - allocs_) : -1);
    }
    ASSERT(prev < alloc->ptr(), "alloc pointers out of order");
    ASSERT(alloc->size(), "alloc zero sized");
    if (prev_end) {
      ASSERT(prev_end == alloc->ptr(), "prev end not matching alloc ptr");
      // Validate  these are not consecutive free blocks
      ASSERT(!(prev_free && alloc->free_), "adjacent free blocks");
    }

    prev_end = alloc->ptr() + alloc->size();
    prev_free = alloc->free_;

    if (!alloc->free_) {
      allocated_bytes += alloc->size();
    } else {
      free_bytes += alloc->size();
    }
    prev = alloc->ptr();
  }

  ASSERT(allocated_bytes + free_bytes == pool_size_, "bytes not adding up");

  size_t prev_size = 0;
  size_t free_bytes_in_list = 0;

  // Check all free blocks in size order
  for (PoolAllocation* free_alloc = first_free_;
       free_alloc;
       free_alloc = free_alloc->next_free_) {
    // Should be marked free
    ASSERT(free_alloc->free_, "free list node not marked as free");

    ASSERT(++free_nodes <= allocs_in_use_, "more free nodes than allocs_in_use_");
    // Sum bytes found
    free_bytes_in_list += free_alloc->size();

    // If not the first alloc
    if (free_alloc != first_free_) {
      ASSERT(free_alloc->size() <= prev_size, "free list out of order");
    }
    prev_size = free_alloc->size();
  }

printf("Free bytes %zu, in list %zu\n", free_bytes, free_bytes_in_list);
  ASSERT(free_bytes == free_bytes_in_list, "free bytes mismatch");
}

SafetyProfilePool:: SafetyProfilePool()
: init_pool_(new Pool(1024*1024, 256))
, main_pool_(0)
{
}

SafetyProfilePool::~SafetyProfilePool()
{
#ifndef OPENDDS_SAFETY_PROFILE
  delete init_pool_;
  delete main_pool_;
#endif
}

void
SafetyProfilePool::configure_pool(size_t size, size_t allocs)
{
  ACE_GUARD(ACE_Thread_Mutex, lock, lock_);

  if (main_pool_ == NULL) {
    main_pool_ = new Pool(size, allocs);
  }
}

SafetyProfilePool*
SafetyProfilePool::instance() {
  return ACE_Singleton<SafetyProfilePool, ACE_SYNCH_MUTEX>::instance();
}

}}

