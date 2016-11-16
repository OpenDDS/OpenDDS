/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H
#define OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H

#include "BasicQueueLink_T.h"
#include "BasicQueueLinkAllocator_T.h"
#include "ace/Malloc_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
class BasicQueueLinkPool {
private:

  typedef BasicQueueLinkAllocator<T> AllocatorType;

public:

  typedef BasicQueueLink<T> LinkType;

  // TMB: links_per_pool == CHUNK_SIZE in Mike's code
  //      num_pools      == 1 in Mike's code.
  BasicQueueLinkPool(size_t links_per_pool, size_t num_pools) {
    this->allocator_ = new AllocatorType(links_per_pool, num_pools);
  }

  ~BasicQueueLinkPool() {
    delete this->allocator_;
  }

  LinkType* obtain(T* elem) {
    LinkType* link;

    ACE_NEW_MALLOC_RETURN(
      link,
      (LinkType*)this->allocator_->malloc(sizeof(LinkType)),
      LinkType(elem),
      0);

    return link;
  }

  void release(LinkType* link) {
    link->reset();
    this->allocator_->free(link);
  }

private:

  ACE_Allocator* allocator_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_BASICQUEUELINKPOOL_T_H */
