/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_MEMORY_POOL_H
#define OPENDDS_MEMORY_POOL_H

#include "dcps_export.h"

class MemoryPoolTest;
class FreeIndexTest;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Header of all allocations - found at beginning of allocation inside pool.
/// Extra room must be added to each allocation for this header.
class OpenDDS_Dcps_Export AllocHeader {
public:
  /** Construct */
  AllocHeader();

  /** Get alloc size */
  unsigned int size() const { return is_free() ? -alloc_size_ : alloc_size_; }
  /** Get prev alloc size */
  unsigned int prev_size() const { return prev_size_; }
  /** Is this alloc free */
  bool is_free() const { return alloc_size_ < 0; }

  /** Get pointer to start of my buffer */
  unsigned char* ptr() const;

  /** Go to next header */
  AllocHeader* next_adjacent();

  /** Go to prev header */
  AllocHeader* prev_adjacent();

  /** Allocate from this block: change size, and mark as allocated */
  void allocate(size_t size);

  /** Set the size value stored in this header */
  void set_size(size_t size);
  /** Set the prev size value stored in this header */
  void set_prev_size(int size) { prev_size_ = size; }
  /** Mark this block as allocated */
  void set_allocated() { if (alloc_size_ < 0) alloc_size_ = - alloc_size_; }

  /** Join with the next adjacent block */
  void join_next();

protected:
  /** Sizes are those of buffers, does not include size of headers */
  int alloc_size_; ///< Size of my buffer, negative if free, positive if alloc
  int prev_size_;  ///< Size of previous buffer, or 0 if first, never negative
};

/// Header of free allocations - found at beginning of allocation inside pool.
/// This forces a minimum allocation size, so that, when freed, header can
/// be cast to this type.
class OpenDDS_Dcps_Export FreeHeader : public AllocHeader {
public:
  /** Initialize the first AllocHeader with its size */
  void init_free_block(unsigned int pool_size);

  /** Mark as free */
  void set_free();

  /** Get next equal or smaller free block in size order */
  FreeHeader* smaller_free(unsigned char* pool_base) const;
  /** Get next equal or larger free block in size order */
  FreeHeader* larger_free(unsigned char* pool_base) const;

  /** Set the next equal or smaller free block in size order */
  void set_smaller_free(FreeHeader* next, unsigned char* pool_base);
  /** Set the next equal or larger free block in size order */
  void set_larger_free(FreeHeader* prev, unsigned char* pool_base);

private:
  size_t offset_smaller_free_; ///< Offset to smaller free block in size order
  size_t offset_larger_free_;  ///< Offset to larger free block in size order
};

/// Node of free index.  Should point to smallest free block of the range
/// from size (inclusive) to limit (non inclusive).  If multiple free
/// allocations of the smallest size in range exist, should point to the one
/// which is the "smallest" (in free list order).
class FreeIndexNode {
public:
  FreeIndexNode();
  /** Set the free alloc this node points to */
  void set_ptr(FreeHeader* ptr) { ptr_ = ptr; }
  /** Set the sizes for this node */
  void set_sizes(size_t size, size_t limit);

  /** Does this node contain a given size */
  bool contains(size_t size) { return ((size >= size_) && (size < limit_)); }

  /** Get this node's free block */
  FreeHeader* ptr() { return ptr_; }
  /** Get this node's minimum size */
  unsigned int size() const { return static_cast<unsigned int>(size_); }

private:
  size_t size_;       ///< size of buffer
  size_t limit_;      ///< upper_limit of buffer size (one too large)
  FreeHeader* ptr_;   ///< points to smallest free alloc of size_ or larger
};

/// Index of free nodes in memory pool.
/// Allows for a faster search of free nodes
class OpenDDS_Dcps_Export FreeIndex {
  friend class ::MemoryPoolTest;
  friend class ::FreeIndexTest;
public:
  explicit FreeIndex(FreeHeader*& largest_free);
  /** Initialize index with initial free block */
  void init(FreeHeader* init_free_block);

  /** Add free block to index */
  void add(FreeHeader* free_block);
  /** Remove free block from index */
  void remove(FreeHeader* free_block, FreeHeader* next_largest);

  /** Find smallest free block of size or larger */
  FreeHeader* find(size_t size, unsigned char* base);

  /** Calculate index of node corresponding to a size */
  static unsigned int node_index(size_t size);

#ifdef VALIDATE_MEMORY_POOL
  static void validate_index(FreeIndex& index,
                             unsigned char* base,
                             bool log = false);
#endif
private:
  enum {
    min_index_pow = 3,
    max_index_pow = 12
  };
  enum {
    min_index = 8,    // 2^^3
    max_index = 4096  // 2^^12
  };

  size_t size_;               ///< Number of index nodes
  FreeHeader*& largest_free_; ///< Memory pool's pointer to largest free block
  FreeIndexNode nodes_[max_index_pow - min_index_pow + 1];   ///< Index nodes
};

// MemoryPool tracks free list, in size order (next meaning largest to smallest)
// and an index into the list at various sizes, which point to the smallest
// free allocation of that size or larger (but not larger than the next size).
// Allocations can be done by checking the index for the needed size, and going
// to the first free block.
class OpenDDS_Dcps_Export MemoryPool {
  friend class ::MemoryPoolTest;
public:
  explicit MemoryPool(unsigned int pool_size, size_t granularity = 8);
  ~MemoryPool();

  /** Does the pool include a given pointer */
  bool includes(void* ptr) const {
     return (pool_ptr_ <= ptr) && (ptr < pool_ptr_ + pool_size_); }

  /** Allocate size bytes from the pool **/
  void* pool_alloc(size_t size);

  /** Attempt to free an allocation.  Return true if allocation is managed by
      this pool (and thus was freed).
   */
  bool pool_free(void* ptr);

  /** Low water mark of maximum available bytes for an allocation */
  size_t lwm_free_bytes() const;

  /** Calculate aligned size of allocation */
  static size_t align(size_t size, size_t granularity) {
     return (size + granularity - 1) / granularity * granularity; }

  size_t size () const { return pool_size_; }

private:
  const size_t granularity_;     ///< Configured granularity
  const size_t min_alloc_size_;  ///< Aligned minimum allocation size
  const size_t pool_size_;       ///< Configured pool size
  size_t lwm_free_bytes_;        ///< Low water mark of available bytes
  unsigned char* pool_ptr_;      ///< Pointer to pool

  FreeHeader* largest_free_;     ///< Pointer to largest free index
  FreeIndex free_index_;         ///< Index of free nodex

  enum {
    min_free_size = sizeof(FreeHeader)
  };

  // Helpers
  void remove_free_alloc(FreeHeader* block_to_alloc);
  void insert_free_alloc(FreeHeader* block_freed);
  void join_free_allocs(FreeHeader* block_freed);
  unsigned char* allocate(FreeHeader* free_block, size_t alloc_size);

  bool joinable_next(FreeHeader* freed);
  bool joinable_prev(FreeHeader* freed);

#ifdef VALIDATE_MEMORY_POOL
  static void validate_pool(MemoryPool& pool, bool log = false);
#endif
};

}} // end namespaces

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_MEMORY_POOL_H
