/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BASICQUEUELINKCHUNK_T_H
#define OPENDDS_DCPS_BASICQUEUELINKCHUNK_T_H

#include "BasicQueueLink_T.h"

#include "ace/OS_NS_stdlib.h"
#include "dds/DCPS/PoolAllocationBase.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
struct BasicQueueLinkChunk : public PoolAllocationBase {
  typedef BasicQueueLink<T> LinkType;

  BasicQueueLinkChunk(size_t chunk_size)
    : next_(0)
  {
    links_ = new LinkType[chunk_size];
  }

  ~BasicQueueLinkChunk() {
    delete [] links_;
  }

  /// The array of LinkType objects in this chunk.
  LinkType* links_;

  /// The next chunk (or 0 if this is the last chunk).
  BasicQueueLinkChunk<T>* next_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_BASICQUEUELINKCHUNK_T_H */
