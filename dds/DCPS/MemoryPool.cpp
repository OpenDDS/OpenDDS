#include "MemoryPool.h"
#include "ace/Log_Msg.h"
#include <stdexcept>
#include <climits>

namespace OpenDDS {  namespace DCPS {

unsigned char*
AllocHeader::ptr() const
{
  const unsigned char* self = reinterpret_cast<const unsigned char*>(this);
  const unsigned char* buffer_start = self + sizeof(AllocHeader);
  return const_cast<unsigned char*>(buffer_start);
}

AllocHeader*
AllocHeader::next_adjacent() {
  unsigned char* self = reinterpret_cast<unsigned char*>(this);
  unsigned char* buffer_start = self + sizeof(AllocHeader);
  unsigned char* past_buffer_end = buffer_start + size();
  return reinterpret_cast<AllocHeader*>(past_buffer_end);
}

AllocHeader*
AllocHeader::prev_adjacent() {
  AllocHeader* result = NULL;
  if (prev_size_) {
    unsigned char* self = reinterpret_cast<unsigned char*>(this);
    unsigned char* prev_buffer_start = self - prev_size_;
    unsigned char* past_alloc = prev_buffer_start - sizeof(AllocHeader);
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
  alloc_size_ = (int)size;
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
FreeHeader::init_free_block(unsigned int free_size)
{
  alloc_size_ = (static_cast<int>((free_size - sizeof(AllocHeader)) * -1));
  prev_size_ = 0;
  set_smaller_free(NULL, NULL);
  set_larger_free(NULL, NULL);
}

void
FreeHeader::set_free()
{
  // If this is newly freed
  if (!is_free()) {
    alloc_size_ = - alloc_size_;
    set_smaller_free(NULL, NULL);
    set_larger_free(NULL, NULL);
  }
}
FreeHeader*
FreeHeader::smaller_free(unsigned char* pool_base) const
{
  FreeHeader* result = NULL;
  if (offset_smaller_free_ != ULONG_MAX) {
    result = reinterpret_cast<FreeHeader*>(pool_base + offset_smaller_free_);
  }
  return result;
}

FreeHeader*
FreeHeader::larger_free(unsigned char* pool_base) const
{
  FreeHeader* result = NULL;
  if (offset_larger_free_ != ULONG_MAX) {
    result = reinterpret_cast<FreeHeader*>(pool_base + offset_larger_free_);
  }
  return result;
}

void
FreeHeader::set_smaller_free(FreeHeader* next, unsigned char* pool_base)
{
  if (next) {
    offset_smaller_free_ = reinterpret_cast<unsigned char*>(next) - pool_base;
  } else {
    offset_smaller_free_ = ULONG_MAX;
  }
}

void
FreeHeader::set_larger_free(FreeHeader* prev, unsigned char* pool_base)
{
  if (prev) {
    offset_larger_free_ = reinterpret_cast<unsigned char*>(prev) - pool_base;
  } else {
    offset_larger_free_ = ULONG_MAX;
  }
}

FreeIndexNode::FreeIndexNode()
: size_(0)
, limit_(0)
, ptr_(0)
{
}

void
FreeIndexNode::set_sizes(size_t size, size_t limit)
{
  size_ = size;
  limit_ = limit;
}

FreeIndex::FreeIndex()
: size_(0)
{
}

void
FreeIndex::add(FreeHeader* free_block)
{
  FreeIndexNode* node = nodes_;
  do {
    if (node->contains(free_block->size())) {
      if ((node->ptr() == NULL) || (node->ptr()->size() > free_block->size())) {
        node->set_ptr(free_block);
      }
      break;
    }
    ++node;
  } while (node < nodes_ + size_);
}

void
FreeIndex::remove(FreeHeader* free_block, FreeHeader* larger)
{
  FreeIndexNode* node = nodes_;
  do {
    if (node->contains(free_block->size())) {
      if (node->ptr() == free_block) {
        if (larger && node->contains(larger->size())) {
          node->set_ptr(larger);
        } else {
          node->set_ptr(NULL);
        }
      }
      break;
    }
    ++node;
  } while (node < nodes_ + size_);
}

void
FreeIndex::init(FreeHeader* init_free_block)
{
  int init_index = 0;
  unsigned int size = 8;
  while (size <= 4096) {
    nodes_[size_].set_sizes(size, (size == 4096) ? -1 :  size*2);
    if (init_free_block->size() >= size) {
      init_index = size_;
    }
    ++size_;
    size *= 2;
  }
  nodes_[init_index].set_ptr(init_free_block);
}

FreeHeader*
FreeIndex::find(size_t size, unsigned char* pool_base)
{    
  // Search index
  FreeIndexNode* index = nodes_ + size_ - 1;
  FreeHeader* larger = NULL;
  
  while (index >= nodes_) {
    if (index->ptr() && index->ptr()->size() >= size) {
      larger = index->ptr();
    }
    --index;
  }

  // Now search for smaller than larger
  while (larger) {
    FreeHeader* smaller = larger->smaller_free(pool_base);
    if (smaller && smaller->size() >= size) {
      larger = smaller;
    } else {
      break;
    }
  }

  return larger;
}

MemoryPool::MemoryPool(unsigned int pool_size, size_t alignment)
: align_size_(alignment)
, header_size_(sizeof(AllocHeader))
, min_free_size_(sizeof(FreeHeader))
, min_alloc_size_(align(min_free_size_ - header_size_))
, pool_size_(align(pool_size))
, pool_ptr_(new unsigned char[pool_size_])
, debug_log_(false)
{
  FreeHeader* first_free = reinterpret_cast<FreeHeader*>(pool_ptr_);
  first_free->init_free_block(pool_size_);
  largest_free_ = first_free;
  free_index_.init(first_free);
  lwm_free_bytes_ = largest_free_->size();
}

MemoryPool::~MemoryPool()
{
#ifndef OPENDDS_SAFETY_PROFILE
  delete [] pool_ptr_;
#endif
}

size_t
MemoryPool::lwm_free_bytes() const
{
  return lwm_free_bytes_;
}

void*
MemoryPool::pool_alloc(size_t size)
{
  // Pointer to return
  unsigned char* block = NULL;

  // Round up to 8-byte boundary
  size_t aligned_size = align(size);

  if (aligned_size < min_alloc_size_) {
    aligned_size = min_alloc_size_;
  }

  // The block to allocate from
  FreeHeader* block_to_alloc = find_free_block(aligned_size);

  if (block_to_alloc) {
    block = allocate(block_to_alloc, aligned_size);
  }

  // Update lwm
  size_t largest_free_bytes = largest_free_ ? largest_free_->size() : 0;
  if (largest_free_bytes < lwm_free_bytes_) {
    lwm_free_bytes_ = largest_free_bytes;
    if (lwm_free_bytes_ < 10000) {
      //ACE_DEBUG((LM_DEBUG, "LWM under 10k\n"));
    }
  }

  //if (debug_log_) log_allocs();
  //validate();

  return block;
}

void
MemoryPool::pool_free(void* ptr)
{
  if (!ptr) {
    return;
  }

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
    // Adjust psize of adjacent
    AllocHeader* next = freed->next_adjacent();
    if (includes(next)) {
      next->set_prev_size(freed->size());
    }
  }
  if (freed->joinable_prev()) {
    FreeHeader* prev_free = reinterpret_cast<FreeHeader*>(freed->prev_adjacent());
    remove_free_alloc(prev_free);
    // Join prev with freed
    prev_free->join_next();
    insert_free_alloc(prev_free);
    // Adjust psize of adjacent
    AllocHeader* next = prev_free->next_adjacent();
    if (includes(next)) {
      next->set_prev_size(prev_free->size());
    }
  } else {
    insert_free_alloc(freed);
  }
}

void
MemoryPool::remove_free_alloc(FreeHeader* block_to_alloc)
{
  FreeHeader* smaller = block_to_alloc->smaller_free(pool_ptr_);
  FreeHeader* larger = block_to_alloc->larger_free(pool_ptr_);

  block_to_alloc->set_smaller_free(NULL, NULL);
  block_to_alloc->set_larger_free(NULL, NULL);

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
  free_index_.remove(block_to_alloc, larger);
}

void
MemoryPool::insert_free_alloc(FreeHeader* freed)
{
  // Find free alloc this size or larger
  FreeHeader* alloc = free_index_.find(freed->size(), pool_ptr_);
  // If found
  if (alloc) {
    if (alloc != freed) {
      FreeHeader* smaller = alloc->smaller_free(pool_ptr_);

      freed->set_larger_free(alloc, pool_ptr_);
      alloc->set_smaller_free(freed, pool_ptr_);
      if (smaller) {
        smaller->set_larger_free(freed, pool_ptr_);
        freed->set_smaller_free(smaller, pool_ptr_);
      }
    }
  // Else this is the largest alloc
  } else {
    if (freed != largest_free_) {
      freed->set_smaller_free(largest_free_, pool_ptr_);
      if (largest_free_) {
        largest_free_->set_larger_free(freed, pool_ptr_);
      }
      largest_free_ = freed;
    }
  }
  
  free_index_.add(freed);
}

FreeHeader*
MemoryPool::find_free_block(size_t req_size)
{
  // If larger than final index
  if (largest_free_ && req_size >= largest_free_->size()) {
    FreeHeader* free_block = largest_free_;
    while (free_block) {
      if (free_block->size() >= req_size) {
        return free_block;
      } else {
        free_block = free_block->larger_free(pool_ptr_);
      }
    }
  } else {
    return free_index_.find(req_size, pool_ptr_);
  }
  // Too large
  return NULL;
}

unsigned char*
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
    remainder -= sizeof(AllocHeader);

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

        // Remove from free list and index
        remove_free_alloc(free_block);
        // Change size
        free_block->set_size(remainder);
        // Insert back into free list and index
        insert_free_alloc(free_block);
      // Else there is smaller, but free_block is still larger
      } else {
        FreeHeader* larger = free_block->larger_free(pool_ptr_);
        // Remove from free list
        free_index_.remove(free_block, larger);
        // Change size
        free_block->set_size(remainder);
        // Add back to free list
        free_index_.add(free_block);
      }
    // Else allocating from smallest block
    } else {
      FreeHeader* larger = free_block->larger_free(pool_ptr_);
      // Remove from free list
      free_index_.remove(free_block, larger);
      // Change size
      free_block->set_size(remainder);
      // Add back to free list
      free_index_.add(free_block);
    }

    // After resize, can use next_adjacent() to safely get to the end of the
    // resized block
    AllocHeader* alloc_block = free_block->next_adjacent();
    // Allocate adjacent block (at end of existing block)
    alloc_block->set_size(alloc_size);
    alloc_block->set_alloc();
    alloc_block->set_prev_size(remainder);
    return alloc_block->ptr();
  // Else we ARE allocating the whole block
  } else {
    free_block->set_alloc();
    // remove free_block from free list
    remove_free_alloc(free_block);
    return free_block->ptr();
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

