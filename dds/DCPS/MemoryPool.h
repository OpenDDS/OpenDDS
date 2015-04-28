#ifndef OPENDDS_MEMORY_POOL_H
#define OPENDDS_MEMORY_POOL_H

#ifdef OPENDDS_SAFETY_PROFILE
#include "dcps_export.h"

class MemoryPoolTest;

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export AllocHeader {
public:

  unsigned int size() const { return is_free() ? -alloc_size_ : alloc_size_; }
  unsigned int prev_size() const { return prev_size_; }
  bool is_free() const { return alloc_size_ < 0; }

  // Get pointer to start of my buffer
  unsigned char* ptr() const;

  // Go to next header
  AllocHeader* next_adjacent();

  // Go to prev header
  AllocHeader* prev_adjacent();

  // Allocate from this block, return remainder size, or zero if not
  void allocate(size_t size);

  // Set the values stored in this header 
  void set_size(size_t size);
  void set_prev_size(int size) { prev_size_ = size; }
  void set_alloc() { if (alloc_size_ < 0) alloc_size_ = - alloc_size_; }

  // Is this node joinable with an adjacent node
  bool joinable_next();
  bool joinable_prev();

  // Join with the next adjacent block (if both indeed free)
  void join_next();

protected:
  // Sizes are those of buffers, does not include size of headers
  int alloc_size_; // Size of my buffer, negative if free, positive if alloc
  int prev_size_;  // Size of previous buffer, or 0 if first
};

class OpenDDS_Dcps_Export FreeHeader : public AllocHeader {
public:
  // Initialize the first AllocHeader with its size
  void init_free_block(unsigned int size);

  void set_free();

  FreeHeader* smaller_free(unsigned char* pool_base) const;
  FreeHeader* larger_free(unsigned char* pool_base) const;

  void set_smaller_free(FreeHeader* next, unsigned char* pool_base);
  void set_larger_free(FreeHeader* prev, unsigned char* pool_base);

private:
  size_t offset_smaller_free_; // Offset to next free block in size order
  size_t offset_larger_free_; // Offset to prev free block in size order
};

class FreeIndexNode {
public:
  FreeIndexNode();
  void set_ptr(FreeHeader* ptr) { ptr_ = ptr; }
  void set_sizes(size_t size, size_t limit);

  bool contains(size_t size) { return ((size >= size_) && (size < limit_)); }

  FreeHeader* ptr() { return ptr_; }
  unsigned int size() const { return size_; }

private:
  size_t size_;       // size of buffer
  size_t limit_;      // upper_limit of buffer size (one too large)
  FreeHeader* ptr_;   // points to smallest free alloc of size_ or larger
};

class OpenDDS_Dcps_Export FreeIndex {
  friend class ::MemoryPoolTest;
public:
  FreeIndex();
  void init(FreeHeader* init_free_block);

  void add(FreeHeader* free_block);
  void remove(FreeHeader* free_block, FreeHeader* next_largest);

  // Find size or larger
  FreeHeader* find(size_t size, unsigned char* base);

private:
  size_t size_;
  FreeIndexNode nodes_[20];
};

// MemoryPool tracks free list, in size order (next meaning largest to smallest)
// and an index into the list at various sizes, which point to the smallest 
// free allocation of that size or larger (but not larger than the next size).
// Allocations can be done by checking the index for the needed size, and going
// to the first free block.
class OpenDDS_Dcps_Export MemoryPool {
  friend class ::MemoryPoolTest;
public:
  MemoryPool(unsigned int pool_size, size_t align_size = 8);
  ~MemoryPool();

  size_t align(size_t size) {
     return (size + align_size_ - 1) / align_size_ * align_size_; }
  bool includes(void* ptr) {
     return (pool_ptr_ <= ptr) && (ptr < pool_ptr_ + pool_size_); }

  char* pool_alloc(size_t size);
  void pool_free(void* ptr);

private:
  const size_t align_size_;      // Alignment size configured
  const size_t header_size_;     // Aligned header size
  const size_t min_free_size_;   // Aligned free header size
  const size_t min_alloc_size_;  // Aligned minimum allocation size
  const size_t pool_size_;       // Configured pool size
  unsigned char* pool_ptr_;

  FreeIndex free_index_;
  FreeHeader* largest_free_;

  bool debug_log_;

  // Helpers
  FreeHeader* find_free_block(size_t req_size);
  void remove_free_alloc(FreeHeader* block_to_alloc);
  void insert_free_alloc(FreeHeader* block_freed);
  void join_free_allocs(FreeHeader* block_freed);
  unsigned char* allocate(FreeHeader* free_block, size_t alloc_size);

  void log_allocs();
  void validate();
};

}} // end namespaces

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_MEMORY_POOL_H
