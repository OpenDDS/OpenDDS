#include "MemoryPool.h"
#include <stdexcept>
#include <climits>

namespace OpenDDS {  namespace DCPS {

void*
AllocHeader::ptr() const
{
  const char* self = reinterpret_cast<const char*>(this);
  const char* buffer_start = self + sizeof(AllocHeader);
  return (void*)buffer_start;
}

AllocHeader*
AllocHeader::next_adjacent() {
  char* self = reinterpret_cast<char*>(this);
  char* buffer_start = self + sizeof(AllocHeader);
  char* past_buffer_end = buffer_start + size();
  return reinterpret_cast<AllocHeader*>(past_buffer_end);
}

AllocHeader*
AllocHeader::prev_adjacent() {
  AllocHeader* result = NULL;
  if (prev_size_) {
    char* self = reinterpret_cast<char*>(this);
    char* prev_buffer_start = self - prev_size_;
    char* past_alloc = prev_buffer_start - sizeof(AllocHeader);
    result = reinterpret_cast<AllocHeader*>(past_alloc);
  }
  return result;
}

void
AllocHeader::allocate(size_t size) {
  set_alloc();
  set_size(size);
}

void
AllocHeader::set_size(size_t size)
{
  if (is_free()) {
    size *= -1;
  }
  alloc_size_ = size;
}

bool
AllocHeader::joinable_next() {
  return (next_adjacent() && is_free() && next_adjacent()->is_free());
}

bool
AllocHeader::joinable_prev() {
  return (prev_adjacent() && is_free() && prev_adjacent()->is_free());
}

void
AllocHeader::join_next() {
  // All unsigned, set_size will make negative if free (it is)
  size_t next_size = this->next_adjacent()->size();
  size_t joined_size = this->size() + next_size + sizeof(AllocHeader);
  this->set_size(joined_size);
}

void
AllocHeader::join_prev() {
  prev_adjacent()->join_next();
}

void
FreeHeader::init_free_block(unsigned int free_size)
{
  alloc_size_ = (static_cast<int>((free_size - sizeof(AllocHeader)) * -1));
  prev_size_ = 0;
  set_smaller_free(NULL, NULL);
  set_larger_free(NULL, NULL);
}

FreeHeader*
FreeHeader::smaller_free(char* buffer) const
{
  FreeHeader* result = NULL;
  if (offset_smaller_free_ != ULONG_MAX) {
    result = reinterpret_cast<FreeHeader*>(buffer + offset_smaller_free_);
  }
  return result;
}

FreeHeader*
FreeHeader::larger_free(char* buffer) const
{
  FreeHeader* result = NULL;
  if (offset_larger_free_ != ULONG_MAX) {
    result = reinterpret_cast<FreeHeader*>(buffer + offset_larger_free_);
  }
  return result;
}

void
FreeHeader::set_smaller_free(FreeHeader* next, char* buffer)
{
  if (next) {
    offset_smaller_free_ = reinterpret_cast<char*>(next) - buffer;
  } else {
    offset_smaller_free_ = ULONG_MAX;
  }
}

void
FreeHeader::set_larger_free(FreeHeader* prev, char* buffer)
{
  if (prev) {
    offset_larger_free_ = reinterpret_cast<char*>(prev) - buffer;
  } else {
    offset_larger_free_ = ULONG_MAX;
  }
}

FreeIndex::FreeIndex()
: size_(0)
, ptr_(0)
{
}

MemoryPool::MemoryPool(unsigned int pool_size, size_t alignment)
: align_size_(alignment)
, header_size_(align(sizeof(AllocHeader)))
, min_free_size_(align(sizeof(FreeHeader)))
, pool_size_(align(pool_size))
, pool_ptr_(new char[pool_size_])
, free_index_size_(0)
, debug_log_(false)
{
  FreeHeader* first_free = reinterpret_cast<FreeHeader*>(pool_ptr_);
  first_free->init_free_block(pool_size_);
  largest_free_ = first_free;

  init_index(first_free);
}

MemoryPool::~MemoryPool()
{
#ifndef OPENDDS_SAFETY_PROFILE
  delete [] pool_ptr_;
#endif
}

/*
FreeHeader*
MemoryPool::smaller_free(const FreeHeader* node)
{
  FreeHeader* result = NULL;
  unsigned long offset = node->next_free_offset();
  if (offset != ULONG_MAX) {
    result = reinterpret_cast<FreeHeader*>(pool_ptr_ + offset);
  }
  return result;
}

FreeHeader*
MemoryPool::larger_free(const FreeHeader* node)
{
  FreeHeader* result = NULL;
  unsigned long offset = node->prev_free_offset();
  if (offset != ULONG_MAX) {
    result = reinterpret_cast<FreeHeader*>(pool_ptr_ + offset);
  }
  return result;
}
*/

char*
MemoryPool::pool_alloc(size_t size)
{
  // Pointer to return
  char* block = NULL;

  // Round up to 8-byte boundary
  size_t aligned_size = align(size);

  // The block to allocate from
  FreeHeader* block_to_alloc = find_free_block(aligned_size);

  if (block_to_alloc) {
    block = allocate(block_to_alloc, aligned_size);
/*
    size_t free_block_size = block_to_alloc->size();

    size_t remainder = free_block_size - aligned_size;

    // May not be enough room for another allocation
    if (remainder < min_free_size_) {
      aligned_size = free_block_size; // use it all
      remainder = 0;
    }

    // Change size and mark allocated
    block_to_alloc->allocate(aligned_size);

    // If free size is changing
    if (remainder) {
      block_to_alloc->set_alloc();
      FreeHeader* next = reinterpret_cast<FreeHeader*>(block_to_alloc->next_adjacent());
      next->set_size(remainder);
      next->set_free();
      next->set_prev_size(aligned_size);
      next->next_adjacent()->set_prev_size(remainder);

      // Maintain size order
      remove_free_alloc(block_to_alloc);
      insert_free_alloc(next);
    // else free block is removed
    } else {
      remove_free_alloc(block_to_alloc);
    }
*/
  }

  //if (debug_log_) log_allocs();
  //validate();

  return block;
}

void
MemoryPool::pool_free(void* ptr)
{
  FreeHeader* header = reinterpret_cast<FreeHeader*>(
      reinterpret_cast<AllocHeader*>(ptr) - 1);

  // Free header
  header->set_free();

  join_free_allocs(header);

  //validate();
  //if (debug_log_) log_allocs();
}

void
MemoryPool::join_free_allocs(FreeHeader* freed)
{
  // Check adjacent
  if (freed->joinable_next()) {
    FreeHeader* next_free = reinterpret_cast<FreeHeader*>(freed->next_adjacent());
    remove_free_alloc(next_free);
    freed->join_next();
  }
  if (freed->joinable_prev()) {
    FreeHeader* prev_free = reinterpret_cast<FreeHeader*>(freed->prev_adjacent());
    remove_free_alloc(prev_free);
    // Join prev with freed
    freed->join_prev();
    insert_free_alloc(prev_free);
  } else {
    insert_free_alloc(freed);
  }
}

void
MemoryPool::remove_free_alloc(FreeHeader* block_to_alloc)
{
  FreeHeader* smaller = block_to_alloc->smaller_free(pool_ptr_);
  FreeHeader* larger = block_to_alloc->larger_free(pool_ptr_);

  // If this was the largest free alloc
  if (block_to_alloc == largest_free_) {
    // It no longer is
    largest_free_ = smaller;
  }

  if (larger) {
    larger->set_smaller_free(smaller, pool_ptr_);
  } else {
    // Should be largest
    largest_free_ = smaller;
  }

  if (smaller) {
    smaller->set_larger_free(larger, pool_ptr_);
  }

  // Remove from free index
  FreeIndex* index = find_insert_index(block_to_alloc->size());
  if (index->ptr() == block_to_alloc) {
    index->set_ptr(larger);
  }
}

void
MemoryPool::insert_free_alloc(FreeHeader* freed)
{
  FreeIndex* insert_index = find_insert_index(freed->size());
  // Won't be null
  FreeHeader* alloc = insert_index->ptr();
  // traverse to find correct point
  while (alloc && freed->size() > alloc->size()) {
    alloc = alloc->larger_free(pool_ptr_);
  }

  if (alloc) {
    // Alloc points to larger or equal size
    FreeHeader* smaller = alloc->smaller_free(pool_ptr_);
    alloc->set_smaller_free(freed, pool_ptr_);
    freed->set_larger_free(alloc, pool_ptr_);
    freed->set_smaller_free(alloc, pool_ptr_);
    if (smaller) {
      smaller->set_larger_free(freed, pool_ptr_);
    }
  } else {
    // nothing larger
    if (largest_free_) {
      largest_free_->set_larger_free(freed, pool_ptr_);
    }
    freed->set_smaller_free(largest_free_, pool_ptr_);
    largest_free_ = freed;
  }
}

FreeIndex*
MemoryPool::find_insert_index(size_t size) {
  // I have a free node of a given size to insert
  FreeIndex* index = free_index_ + (free_index_size_ - 1);
  // Linear search
  while (index >= free_index_) {
    if (size >= index->size()) {
      return index;
    }
    --index;
  }

  return NULL;
}

unsigned long
MemoryPool::offset_of(const AllocHeader* alloc)
{
  char* char_alloc = (char*)alloc;
  return (char_alloc - pool_ptr_);
}

FreeHeader*
MemoryPool::find_free_block(size_t req_size)
{
  // If larger than final index
  if (req_size > free_index_[free_index_size_ - 1].size()) {
    // Does not fit in index, start at end and move larger
    FreeHeader* free_block = free_index_[free_index_size_ - 1].ptr();
    while (free_block) {
      if (free_block->size() > req_size) {
        return free_block;
      } else {
        free_block = free_block->larger_free(pool_ptr_);
      }
    }
  } else {
    unsigned int i = 0;
    // Search index
    FreeIndex* index = free_index_;
    for (i = 0; i < free_index_size_; ++i) {
      if (index->ptr() && index->size() >= req_size) {
        // Found
        return free_index_[i].ptr();
      }
      ++index;
    }
  }
  // Too large
  return NULL;
}

void
MemoryPool::init_index(FreeHeader* init_free)
{
  int init_index = 0;
  unsigned int size = 8;
  while (size <= 4096) {
    free_index_[free_index_size_].set_size(size);
    if (init_free->size() >= size) {
      init_index = free_index_size_;
    }
    ++free_index_size_;
    size *= 2;
  }
  free_index_[init_index].set_ptr(init_free);
}

char*
MemoryPool::allocate(FreeHeader* free_block, size_t alloc_size)
{
  size_t free_block_size = free_block->size();
  size_t remainder = free_block_size - alloc_size;

  // May not be enough room for another allocation
  if (remainder < min_free_size_) {
    alloc_size = free_block_size; // use it all
    remainder = 0;
  }

  // If we are NOT allocating the whole block
  if (remainder) {
    // Account for header here - won't overflow due to check, above
    remainder -= align(sizeof(AllocHeader));

    // Adjust current adjacent block (after free block)
    AllocHeader* next_adjacent = free_block->next_adjacent();
    if (includes(next_adjacent)) {
      next_adjacent->set_prev_size(alloc_size);
    }

    // If there is a free block smaller than the one we are allocating from
    if (FreeHeader* smaller = free_block->smaller_free(pool_ptr_)) {
      // If next smaller is now larger
      if (remainder < smaller->size()) {
        // Move the free block ahead in the free list

        // Remove from index
        remove_free_alloc(free_block);
        // Change size
        free_block->set_size(remainder);
        // Insert back into index
        insert_free_alloc(free_block);
      // Else there is smaller, but free_block is still larger
      } else {
        // Change size
        free_block->set_size(remainder);
      }
    // Else allocating from smallest block
    } else {
      // Change size
      free_block->set_size(remainder);
    }

    // After resize, can use next_adjacent() to safely get to the end of the
    // resized block
    AllocHeader* alloc_block = free_block->next_adjacent();
    // Allocate adjacent block (at end of existing block)
    alloc_block->set_size(alloc_size);
    alloc_block->set_alloc();
    alloc_block->set_prev_size(remainder);
    return (char*)alloc_block->ptr();
  // Else we ARE allocating the whole block
  } else {
    free_block->set_alloc();
    // remove free_block from free list
    remove_free_alloc(free_block);
    return (char*)free_block->ptr();
  }
}

#define ASSERT(expr, msg) \
  if (!(expr)) throw std::runtime_error(msg);

void
MemoryPool::validate() {
/*
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
*/
}

}}

