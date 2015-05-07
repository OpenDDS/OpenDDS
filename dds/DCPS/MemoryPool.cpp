#include "MemoryPool.h"
#include "PoolAllocator.h"
#include "ace/Log_Msg.h"
#include <stdexcept>
#include <climits>
#include <map>

#define TEST_CHECK(COND) \
  if (!( COND )) { \
    char msg[1024]; \
    snprintf(msg, 1024, "%s: FAILED at %s:%d", #COND, __FILE__, __LINE__); \
    printf("%s\n", msg); \
    throw std::runtime_error(msg); \
    return; \
  }

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

FreeIndex::FreeIndex(FreeHeader*& largest_free)
: size_(0)
, largest_free_(largest_free)
{
}

void
FreeIndex::add(FreeHeader* freed)
{
  FreeIndexNode* node = nodes_;
  do {
    // If the freed block is of the size this node manages
    if (node->contains(freed->size())) {
      if ((node->ptr() == NULL) || (node->ptr()->size() >= freed->size())) {
        node->set_ptr(freed);
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

  // Larger or equal to size
  FreeHeader* result = NULL;
  if (largest_free_ && (largest_free_->size() >= size)) {
    result = largest_free_;

    // Look from largest to smallest
    while (index >= nodes_) {
      if (index->ptr() && index->ptr()->size() >= size) {
        result = index->ptr();
      }

      if (index->size() < size) {
        // No more will be large enough
        break;
      }
      --index;
    }
  }

  // Now traverse, searching for smaller than result
  while (result) {
    FreeHeader* smaller = result->smaller_free(pool_base);
    if (smaller && smaller->size() >= size) {
      result = smaller;
    } else {
      break;
    }
  }

  return result;
}

void
FreeIndex::validate_index(FreeIndex& index, unsigned char* base, bool log)
{
  if (log) {
    FreeIndexNode* node = index.nodes_;
    while (node < index.nodes_ + index.size_) {
      if (node->ptr()) {
        printf("  IND[%4d] -> %4d\n", node->size(), node->ptr()->size());
      } else {
        printf("  IND[%4d] -> NULL\n", node->size());
      }
      ++node;
    };
  }

  for (size_t size = 8; size <= 4096; size *= 2) {
    // Find size or larger
    FreeHeader* first = index.find(size, base);
    if (first) {
      TEST_CHECK(first->size() >= size);

      FreeHeader* next_free = first;
      while (FreeHeader* smaller = next_free->smaller_free(base)) {
        TEST_CHECK(smaller->size() < first->size());
        TEST_CHECK(smaller->size() <= next_free->size());
        // Anything smaller should be too small
        TEST_CHECK(smaller->size() < size);
        next_free = smaller;
      }
    }
  }
}

MemoryPool::MemoryPool(unsigned int pool_size, size_t granularity)
: granularity_((granularity + 8 - 1) / 8 * 8)
, header_size_(sizeof(AllocHeader))
, min_free_size_(sizeof(FreeHeader))
, min_alloc_size_(align(min_free_size_ - header_size_))
, pool_size_(align(pool_size))
, pool_ptr_(new unsigned char[pool_size_])
, largest_free_(NULL)
, free_index_(largest_free_)
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
      printf("WARN: LWM under 10k\n");
    }
  }

  //printf("After alloc of %zu\n", size);
  //validate_pool(*this, debug_log_);

  if (!block) {
    printf("WARN: Alloc of %zu returning NULL\n", size);
  }
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

  //printf("After free of %x\n", ptr);
  //validate_pool(*this, debug_log_);
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
  // Find free alloc freed size or larger
  FreeHeader* alloc = free_index_.find(freed->size(), pool_ptr_);
  // If found
  if (alloc) {
    TEST_CHECK(alloc != freed);
      FreeHeader* smaller = alloc->smaller_free(pool_ptr_);

      // Insert into list
      freed->set_larger_free(alloc, pool_ptr_);
      alloc->set_smaller_free(freed, pool_ptr_);
      if (smaller) {
        smaller->set_larger_free(freed, pool_ptr_);
        freed->set_smaller_free(smaller, pool_ptr_);
      }
  // Else freed the largest alloc
  } else {
    if (freed != largest_free_) {
      freed->set_smaller_free(largest_free_, pool_ptr_);
      if (largest_free_) {
        largest_free_->set_larger_free(freed, pool_ptr_);
      }
      largest_free_ = freed;
    }
  }

  // Insert and replace alloc if necessary
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

void
MemoryPool::validate_pool(MemoryPool& pool, bool log) {
  AllocHeader* prev = 0;
  size_t allocated_bytes = 0;
  size_t free_bytes = 0;
  size_t oh_bytes = 0;
  size_t free_count = 0;
  unsigned char* pool_end = pool.pool_ptr_ + pool.pool_size_;
  bool prev_was_free;
  size_t index = 0;

  typedef std::map<FreeHeader*, int> FreeMap;
  FreeMap free_map;
  // Gather all free indices
  AllocHeader* alloc = reinterpret_cast<AllocHeader*>(pool.pool_ptr_);
  while (pool.includes(alloc)) {
    FreeHeader* free_header = alloc->is_free() ?
          reinterpret_cast<FreeHeader*>(alloc) : NULL;
    if (free_header) {
      free_map[free_header] = index;
    }
    alloc = alloc->next_adjacent();
    ++index;
  }

  index = 0;
  if (log) {
    printf("Pool ptr %zx end %zx\n", (unsigned long)pool.pool_ptr_,
           (unsigned long)pool_end);
   }

  // Check all allocs in positional order and not overlapping
  alloc = reinterpret_cast<AllocHeader*>(pool.pool_ptr_);
  while (pool.includes(alloc)) {
    if (log) {

      int smlr_index = -1;
      int lrgr_index = -1;
      char lrgr_buff[32];
      char smlr_buff[32];

      FreeHeader* free_header = alloc->is_free() ?
            reinterpret_cast<FreeHeader*>(alloc) : NULL;
      if (free_header) {
        FreeMap::const_iterator found;
        found = free_map.find(free_header->smaller_free(pool.pool_ptr_));
        if (found != free_map.end()) {
          smlr_index = found->second;
          sprintf(smlr_buff, "[%2d]", smlr_index);
        }
        found = free_map.find(free_header->larger_free(pool.pool_ptr_));
        if (found != free_map.end()) {
          lrgr_index = found->second;
          sprintf(lrgr_buff, "[%2d]", lrgr_index);
        }
      }
      printf(
        "Alloc[%zu] %s at %zx ptr %zx lg %s sm %s size %d psize %d\n",
        index++,
        alloc->is_free() ?
        (alloc == pool.largest_free_ ? "FREE!" : "free ")  : "     ",
        (unsigned long)alloc,
        (unsigned long)alloc->ptr(),
        lrgr_index >= 0 ? lrgr_buff : "[  ]",
        smlr_index >= 0 ? smlr_buff : "[  ]",
        alloc->size(),
        alloc->prev_size()
      );
    }

    TEST_CHECK(alloc->size());
    if (prev) {
      TEST_CHECK(prev->next_adjacent() == alloc);
      TEST_CHECK(alloc->prev_adjacent() == prev);
      // Validate  these are not consecutive free blocks
      TEST_CHECK(!(prev_was_free && alloc->is_free()));
    }

    if (!alloc->is_free()) {
      allocated_bytes += alloc->size();
      prev_was_free = false;
    } else {
      free_bytes += alloc->size();
      prev_was_free = true;
    }
    oh_bytes += sizeof(AllocHeader);
    prev = alloc;
    alloc = alloc->next_adjacent();
  }
  TEST_CHECK((unsigned char*)alloc == pool_end);

  TEST_CHECK(allocated_bytes + free_bytes + oh_bytes == pool.pool_size_);

  FreeIndex::validate_index(pool.free_index_, pool.pool_ptr_, log);

  size_t prev_size = 0;
  size_t free_bytes_in_list = 0;
  FreeHeader* free_alloc = NULL;
  FreeHeader* prev_free = NULL;

  // Check all free blocks in size order
  for (free_alloc = pool.largest_free_;
       free_alloc;
       free_alloc = free_alloc->smaller_free(pool.pool_ptr_)) {
    // Should be marked free
    TEST_CHECK(free_alloc->is_free());
    // Check for infinite loop
    TEST_CHECK(++free_count < 10000);

    // Sum bytes found
    free_bytes_in_list += free_alloc->size();

    // If not the first alloc
    if (prev_size) {
      TEST_CHECK(free_alloc->size() <= prev_size);
      TEST_CHECK(free_alloc->size() > 0);
    }
    prev_size = free_alloc->size();
    prev_free = free_alloc;
  }

  TEST_CHECK(free_bytes == free_bytes_in_list);

  // Try again from smallest to largest
  if (prev_free) {
    free_bytes_in_list = 0;

    for (free_alloc = prev_free;
         free_alloc;
         free_alloc = free_alloc->larger_free(pool.pool_ptr_)) {
      // Should be marked free
      TEST_CHECK(free_alloc->is_free());

      // Sum bytes found
      free_bytes_in_list += free_alloc->size();

      // If not the first alloc
      if (free_alloc != prev_free) {
        TEST_CHECK(free_alloc->size() >= prev_size);
        TEST_CHECK(free_alloc->size() > 0);
      }
      prev_size = free_alloc->size();
    }
    TEST_CHECK(free_bytes == free_bytes_in_list);
  }
}

}}

