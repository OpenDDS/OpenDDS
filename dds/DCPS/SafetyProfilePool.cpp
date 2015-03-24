#include "SafetyProfilePool.h"

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

void*
PoolAllocation::allocate(size_t alloc_size, PoolAllocation* target)
{
  // Divide my alloc into remainder and target
  size_t remainder = size_ - alloc_size;
  char* next_buff = ptr_ + remainder;
  target->set(next_buff, alloc_size);
  target->free_ = false;
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
{
  first_free_ = allocs_;
  first_free_->set(pool_ptr_, pool_size_);
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

    // While there are more small enough blocks remaining
    while (block_to_alloc->next_free_ && block_to_alloc->size_ >= alloc_size)
    {
      prev_block = block_to_alloc;
      block_to_alloc = block_to_alloc->next_free_;
    }

    block = allocate_block(block_to_alloc, prev_block, alloc_size);
  }

  return block;
}

void
Pool::pool_free(void* ptr)
{
}

// Can prev_block be null?

char* 
Pool::allocate_block(PoolAllocation* from_block,
                     PoolAllocation* prev_block,
                     size_t alloc_size)
{
  char* buffer;

  if (from_block->size() == alloc_size) {
    // Going to use all of free block, keep size same 
    buffer = from_block->ptr();
    // Take out of free list
// will crash if prev_block is null
    prev_block->next_free_ = from_block->next_free_;
    from_block->next_free_ = NULL;
  } else if (from_block->size() > alloc_size) {
    int index = from_block - allocs_;
    // Second clause should always be true, 
    // was written to show it has been considered
    if ((allocs_in_use_ < max_allocs_) && (index < max_allocs_ - 1)) {
      PoolAllocation* next_block = from_block->next_free_;
      // Slide alocations down to maintain buffer order
      PoolAllocation* target = make_room_for_allocation(index);

      // Allocate a buffer using taegetand put remainder in from_block
      buffer = static_cast<char*>(from_block->allocate(alloc_size, target));
      ++allocs_in_use_;

      // from_block is now smaller
      if (next_block && from_block->size_ < next_block->size_) {
        reorder_block(from_block, prev_block);
      }

      // If no next block, the list is in the right order still
    }

    // Going to split free block, need next one available
  } else {
    // Should not happen
  }

  return buffer;
}

PoolAllocation* 
Pool::make_room_for_allocation(int index)
{
  PoolAllocation* src = allocs_ + index;
  PoolAllocation* dest = src + 1;

  if (index < allocs_in_use_ - 1) {
    // After the move, free list pointers will be off by one PoolAllocation.
    // Fix that now
    for (PoolAllocation* iter = first_free_;
         iter != NULL;
         iter = iter->next_free_)
    {
      if (iter->next_free_ >= src) {
        // Impacted by move
        iter->next_free_ = iter->next_free_ + 1;
      }
    }

    // Move the memory
    memmove(dest, &allocs_[index], 
            sizeof(PoolAllocation) * (allocs_in_use_ - index));
  }

  return dest;
}

void
Pool::reorder_block(PoolAllocation* resized_block, PoolAllocation* prev_block)
{
  PoolAllocation* next_block = resized_block->next_free_;

  while (next_block && resized_block->size_ < next_block->size_) {
    // may be first iteration, and first blok allocated, so no prev
    if (prev_block) {
      prev_block->next_free_ = next_block;
    } else {
      first_free_ = next_block;
    }
    prev_block = next_block;
  }

  // prev_block can't now be null, because resized block getting smaller than
  // next was a precondition for entering
  prev_block->next_free_ = resized_block;
  
  // next_block is smaller than resized block, or null if none smaller
  resized_block = next_block;
}

}}

