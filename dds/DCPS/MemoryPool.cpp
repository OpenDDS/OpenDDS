/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"  ////Only the _pch include should start with DCPS/
#include "MemoryPool.h"
#include "PoolAllocator.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdio.h"
#include <stdexcept>
#include <limits>
#include <map>
#include <cstring>

#if defined(WITH_VALGRIND)
#include "valgrind/memcheck.h"
#endif

#define TEST_CHECK(COND) \
  if (!( COND )) { \
    char msg[1024]; \
    ACE_OS::snprintf(msg, 1024, "%s: FAILED at %s:%d", #COND, __FILE__, __LINE__); \
    ACE_OS::printf("%s\n", msg); \
    throw std::runtime_error(msg); \
    return; \
  }

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {  namespace DCPS {

AllocHeader::AllocHeader()
: alloc_size_(0)
, prev_size_(0)
{
}

unsigned char*
AllocHeader::ptr() const
{
  const unsigned char* buff = reinterpret_cast<const unsigned char*>(this + 1);
  return const_cast<unsigned char*>(buff);
}

AllocHeader*
AllocHeader::next_adjacent() {
  unsigned char* past_buffer_end = ptr() + size();
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
  set_allocated();
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

void
AllocHeader::join_next() {
  // All unsigned, set_size will make negative if free (it is)
  size_t next_size = this->next_adjacent()->size();
  size_t joined_size = this->size() + next_size + sizeof(AllocHeader);
  this->set_size(joined_size);
}

void
FreeHeader::init_free_block(unsigned int pool_size)
{
  alloc_size_ = static_cast<int>(pool_size - sizeof(AllocHeader));
  prev_size_ = 0;
  set_free();
}

void
FreeHeader::set_free()
{
  // If this is newly freed
  if (!is_free()) {
    alloc_size_ *= -1;
    set_smaller_free(NULL, NULL);
    set_larger_free(NULL, NULL);
  }
}
FreeHeader*
FreeHeader::smaller_free(unsigned char* pool_base) const
{
  FreeHeader* result = NULL;
  if (offset_smaller_free_ != std::numeric_limits<size_t>::max()) {
    result = reinterpret_cast<FreeHeader*>(pool_base + offset_smaller_free_);
  }
  return result;
}

FreeHeader*
FreeHeader::larger_free(unsigned char* pool_base) const
{
  FreeHeader* result = NULL;
  if (offset_larger_free_ != std::numeric_limits<size_t>::max()) {
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
    offset_smaller_free_ = std::numeric_limits<size_t>::max();
  }
}

void
FreeHeader::set_larger_free(FreeHeader* prev, unsigned char* pool_base)
{
  if (prev) {
    offset_larger_free_ = reinterpret_cast<unsigned char*>(prev) - pool_base;
  } else {
    offset_larger_free_ = std::numeric_limits<size_t>::max();
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
  unsigned int index = node_index(freed->size());
  FreeIndexNode* node = nodes_ + index;

  // If the node is empty, or if freed is smaller or equal to the node's alloc
  if ((node->ptr() == NULL) || (node->ptr()->size() >= freed->size())) {
    // Use this alloc in the index
    node->set_ptr(freed);
  }
}

void
FreeIndex::remove(FreeHeader* free_block, FreeHeader* larger)
{
  unsigned int index = node_index(free_block->size());
  FreeIndexNode* node = nodes_ + index;

  // If the node points to the free block
  if (node->ptr() == free_block) {
    // If the larger can be used by this node
    if (larger && node->contains(larger->size())) {
      node->set_ptr(larger);
    } else {
      node->set_ptr(NULL);
    }
  }
}

void
FreeIndex::init(FreeHeader* init_free_block)
{
  size_t max = std::numeric_limits<size_t>::max();
  for (size_t size = min_index; size <= max_index; size *= 2) {
    nodes_[size_].set_sizes(size, (size == max_index) ? max  :  size*2);
    ++size_;
  }
  add(init_free_block);
}

FreeHeader*
FreeIndex::find(size_t search_size, unsigned char* pool_base)
{
  unsigned int index = node_index(search_size);
  FreeIndexNode* index_node = nodes_ + index;

  // Larger or equal to search_size
  FreeHeader* result = NULL;
  if (largest_free_ && (largest_free_->size() >= search_size)) {
    result = largest_free_;

    // Look from here and larger
    while (index_node < nodes_ + size_) {
      if (index_node->ptr() && index_node->ptr()->size() >= search_size) {
        result = index_node->ptr();
        break;
      }
      ++index_node;
    }
  }

  // Now traverse, searching for smaller than result
  while (result) {
    FreeHeader* smaller = result->smaller_free(pool_base);
    if (smaller && smaller->size() >= search_size) {
      result = smaller;
    } else {
      break;
    }
  }

  return result;
}

unsigned int
FreeIndex::node_index(size_t size)
{
  // Use shifting to perform log base 2 of size
  //   start by using min + 1 (+1 because min is a power of 2 whch is already
  //   one bit)
  size_t size_copy = size >> (min_index_pow + 1);
  unsigned int index = 0;
  unsigned int max_idx = max_index_pow - min_index_pow;
  while (size_copy && (index < max_idx)) {
    ++index;
    size_copy = size_copy >> 1;
  }
  return index;
}

#ifdef VALIDATE_MEMORY_POOL
void
FreeIndex::validate_index(FreeIndex& index, unsigned char* base, bool log)
{
  if (log) {
    FreeIndexNode* node = index.nodes_;
    while (node < index.nodes_ + index.size_) {
      if (node->ptr()) {
        ACE_OS::printf("  IND[%4d] -> %4d\n", node->size(), node->ptr()->size());
      } else {
        ACE_OS::printf("  IND[%4d] -> NULL\n", node->size());
      }
      ++node;
    }
  }

  // Validate searches of each size
  for (size_t size = min_index; size <= max_index; size *= 2) {
    // Find size or larger
    FreeHeader* size_or_larger = index.find(size, base);
    if (size_or_larger) {
      TEST_CHECK(size_or_larger->size() >= size);
    }
  }

  // Validate each node points to a free block of the proper size;
  for (FreeIndexNode* node = index.nodes_; node < index.nodes_ + index.size_; ++node) {
    FreeHeader* block = node->ptr();
    if (block) {
      // node should point to a free block of the proper size;
      TEST_CHECK(node->contains(block->size()));

      FreeHeader* smaller = block;
      while ((smaller = smaller->smaller_free(base))) {
        // Anything smaller should be too small for this node
        TEST_CHECK(smaller->size() < node->size());
      }
    }
  }
}
#endif

MemoryPool::MemoryPool(unsigned int pool_size, size_t granularity)
: granularity_(align(granularity, 8))
, min_alloc_size_(align(min_free_size - sizeof(AllocHeader), granularity_))
, pool_size_(align(pool_size, granularity_))
, pool_ptr_(new unsigned char[pool_size_])
, largest_free_(NULL)
, free_index_(largest_free_)
{
  AllocHeader* the_pool = new (pool_ptr_) AllocHeader();
  FreeHeader* first_free = reinterpret_cast<FreeHeader*>(the_pool);
  first_free->init_free_block(static_cast<unsigned int>(pool_size_));
  largest_free_ = first_free;
  free_index_.init(first_free);
  lwm_free_bytes_ = largest_free_->size();
#if defined(WITH_VALGRIND)
  VALGRIND_MAKE_MEM_NOACCESS(pool_ptr_, pool_size_);
  VALGRIND_CREATE_MEMPOOL(pool_ptr_, 0, false);
#endif
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
#if defined(WITH_VALGRIND)
  VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(pool_ptr_, pool_size_);
#endif

  // Pointer to return
  unsigned char* block = NULL;

  // Round up to 8-byte boundary
  size_t aligned_size = align(size, granularity_);

  if (aligned_size < min_alloc_size_) {
    aligned_size = min_alloc_size_;
  }

  // The block to allocate from
  FreeHeader* block_to_alloc = free_index_.find(aligned_size, pool_ptr_);

  if (block_to_alloc) {
    block = allocate(block_to_alloc, aligned_size);
  }

  // Update lwm
  size_t largest_free_bytes = largest_free_ ? largest_free_->size() : 0;
  if (largest_free_bytes < lwm_free_bytes_) {
    lwm_free_bytes_ = largest_free_bytes;
  }

#ifdef VALIDATE_MEMORY_POOL
  validate_pool(*this, false);
#endif

#if defined(WITH_VALGRIND)
  VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(pool_ptr_, pool_size_);
  VALGRIND_MEMPOOL_ALLOC(pool_ptr_, block, size);
#endif

  return block;
}

bool
MemoryPool::pool_free(void* ptr)
{
  bool freed = false;
  if (ptr && includes(ptr)) {
#if defined(WITH_VALGRIND)
    VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(pool_ptr_, pool_size_);
#endif

    FreeHeader* header = reinterpret_cast<FreeHeader*>(
        reinterpret_cast<AllocHeader*>(ptr) - 1);

    // Free header
    header->set_free();

    join_free_allocs(header);

#ifdef VALIDATE_MEMORY_POOL
    validate_pool(*this, false);
#endif

    freed = true;

#if defined(WITH_VALGRIND)
    VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(pool_ptr_, pool_size_);
    VALGRIND_MEMPOOL_FREE(pool_ptr_, ptr);
#endif
  }

  return freed;
}

void
MemoryPool::join_free_allocs(FreeHeader* freed)
{
  // Check adjacent
  if (joinable_next(freed)) {
    FreeHeader* next_free = reinterpret_cast<FreeHeader*>(freed->next_adjacent());
    remove_free_alloc(next_free);
    freed->join_next();
    // Adjust psize of adjacent
    AllocHeader* next = freed->next_adjacent();
    if (includes(next)) {
      next->set_prev_size(freed->size());
    }
  }
  if (joinable_prev(freed)) {
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

unsigned char*
MemoryPool::allocate(FreeHeader* free_block, size_t alloc_size)
{
  size_t free_block_size = free_block->size();
  size_t remainder = free_block_size - alloc_size;

  // May not be enough room for another allocation
  if (remainder < min_free_size) {
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
      next_adjacent->set_prev_size(static_cast<int>(alloc_size));
    }

    // Always remove, resize, and reinsert to make sure free list and free
    // index are in sync
    remove_free_alloc(free_block);
    free_block->set_size(remainder);
    insert_free_alloc(free_block);

    // After resize, can use next_adjacent() to safely get to the end of the
    // resized block.
    // Taking free memory and allocating, so invoke constructor
    AllocHeader* alloc_block = new(free_block->next_adjacent()) AllocHeader();

    // Allocate adjacent block (at end of existing block)
    alloc_block->set_size(alloc_size);
    alloc_block->set_allocated();
    alloc_block->set_prev_size(static_cast<int>(remainder));
    return alloc_block->ptr();
  // Else we ARE allocating the whole block
  } else {
    free_block->set_allocated();
    // remove free_block from free list
    remove_free_alloc(free_block);
    return free_block->ptr();
  }
}

bool
MemoryPool::joinable_next(FreeHeader* freed)
{
  AllocHeader* next_alloc = freed->next_adjacent();
  return freed->is_free() &&
         includes(next_alloc) &&
         next_alloc->is_free();
}

bool
MemoryPool::joinable_prev(FreeHeader* freed)
{
  AllocHeader* prev_alloc = freed->prev_adjacent();
  return freed->is_free() &&
         includes(prev_alloc) &&
         prev_alloc->is_free();
}

#ifdef VALIDATE_MEMORY_POOL
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
    ACE_OS::printf("Pool ptr %zx end %zx\n", (unsigned long)pool.pool_ptr_,
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
          snprintf(smlr_buff, 32, "[%2d]", smlr_index); // preprocessed out
        }
        found = free_map.find(free_header->larger_free(pool.pool_ptr_));
        if (found != free_map.end()) {
          lrgr_index = found->second;
          snprintf(lrgr_buff, 32, "[%2d]", lrgr_index); // preprocessed out
        }
      }
      ACE_OS::printf(
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
#endif

}}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
