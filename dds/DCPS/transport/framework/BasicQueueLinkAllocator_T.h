/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BASICQUEUELINKALLOCATOR_T_H
#define OPENDDS_DCPS_BASICQUEUELINKALLOCATOR_T_H

#include "BasicQueueLink_T.h"
#include "BasicQueueLinkChunk_T.h"
#include "ace/Malloc_T.h"
#include "ace/Null_Mutex.h"
#include "dds/DCPS/PoolAllocationBase.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
class BasicQueueLinkAllocator : public ACE_New_Allocator, public PoolAllocationBase {
private:

  typedef BasicQueueLink<T>                              LinkType;
  typedef BasicQueueLinkChunk<T>                         ChunkType;
  typedef ACE_Cached_Mem_Pool_Node<LinkType >            NodeType;
  typedef ACE_Locked_Free_List<NodeType, ACE_Null_Mutex> FreeListType;

public:
  BasicQueueLinkAllocator(size_t chunk_size, size_t initial_chunks)
    : chunk_size_(chunk_size),
      head_chunk_(0),
      tail_chunk_(0),
      free_list_(ACE_PURE_FREE_LIST) {
    for (size_t i = 0; i < initial_chunks; i++) {
      this->grow();
    }
  }

  virtual ~BasicQueueLinkAllocator() {
    ChunkType* chunk = this->head_chunk_;

    while (chunk != 0) {
      ChunkType* next_chunk = chunk->next_;
      delete chunk;
      chunk = next_chunk;
    }
  }

  /// malloc implementation.
  void* malloc(size_t nbytes = sizeof(LinkType)) {
    // Check if size requested fits within pre-determined size.
    if (nbytes != sizeof(LinkType)) {
      return 0;
    }

    // addr() call is really not absolutely necessary because
    // of the way ACE_Cached_Mem_Pool_Node's internal
    // structure is arranged.
    void* ptr = this->free_list_.remove()->addr();

    if (ptr == 0) {
      this->grow();
      ptr = this->free_list_.remove()->addr();
    }

    return ptr;
  }

  /// calloc implementation.
  virtual void* calloc(size_t nbytes = sizeof(LinkType),
                       char initial_value = '\0') {
    // Check if size requested fits the pre-determined size.
    if (nbytes != sizeof(LinkType)) {
      return 0;
    }

    // addr() call is really not absolutely necessary because
    // of the way ACE_Cached_Mem_Pool_Node's internal
    // structure is arranged.
    void* ptr = this->free_list_.remove()->addr();

    if (ptr == 0) {
      this->grow();
      ptr = this->free_list_.remove()->addr();
    }

    ACE_OS::memset(ptr, initial_value, sizeof(LinkType));

    return ptr;
  }

  /// This interface not supported.
  virtual void* calloc(size_t n_elem,
                       size_t elem_size,
                       char   initial_value = '\0') {
    ACE_UNUSED_ARG(n_elem);
    ACE_UNUSED_ARG(elem_size);
    ACE_UNUSED_ARG(initial_value);
    ACE_NOTSUP_RETURN(0);
  }

  /// free implementation.
  void free(void* ptr) {
    this->free_list_.add((NodeType*)ptr);
  }

private:

  /// Grow by another chunk.
  void grow() {
    ChunkType* chunk;
    ACE_NEW(chunk, ChunkType(this->chunk_size_));

    for (size_t i = 0; i < this->chunk_size_; ++i) {
      void* placement = &(chunk->links_[i]);
      this->free_list_.add(new(placement) NodeType);
    }

    // Add the chunk to the "chunk list".
    if (head_chunk_ == 0) {
      this->head_chunk_ = this->tail_chunk_ = chunk;

    } else {
      this->tail_chunk_->next_ = chunk;
      this->tail_chunk_ = chunk;
    }
  }

  /// Number of links to allocate for each chunk.
  size_t chunk_size_;

  /// The first chunk
  ChunkType* head_chunk_;

  /// The last chunk
  ChunkType* tail_chunk_;

  /// Maintain a cached memory free list.
  FreeListType free_list_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_BASICQUEUELINKALLOCATOR_T_H */
