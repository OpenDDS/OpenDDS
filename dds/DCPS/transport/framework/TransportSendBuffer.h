/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TRANSPORTSENDBUFFER_H
#define DCPS_TRANSPORTSENDBUFFER_H

#include "dds/DCPS/dcps_export.h"

#include "BasicQueue_T.h"
#include "TransportQueueElement.h"
#include "TransportRetainedElement.h"
#include "TransportReplacedElement.h"
#include "TransportSendStrategy_rch.h"

#include "ace/Message_Block.h"

#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/RcObject_T.h"

#include <map>
#include <set>
#include <utility>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportSendBuffer
  : public RcObject<ACE_SYNCH_MUTEX> {
public:
  typedef BasicQueue<TransportQueueElement> queue_type;
  typedef std::pair<queue_type*, ACE_Message_Block*> buffer_type;

  explicit TransportSendBuffer(size_t capacity,
                               size_t max_samples_per_packet);
  ~TransportSendBuffer();

  size_t capacity() const;
  size_t n_chunks() const;

  void bind(TransportSendStrategy* strategy);

  void release_all();
  void release(buffer_type& buffer);

  void retain_all(RepoId pub_id);

  void insert(SequenceNumber sequence, const buffer_type& value);

  bool resend(const DisjointSequence::range_type& range,
              DisjointSequence& missing);
  void resend(buffer_type& buffer);

private:
  size_t capacity_;
  size_t n_chunks_;

  TransportRetainedElementAllocator retained_allocator_;
  TransportReplacedElementAllocator replaced_allocator_;

  TransportSendStrategy_rch strategy_;

  typedef std::map<SequenceNumber, buffer_type> BufferMap;
  BufferMap buffers_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "TransportSendBuffer.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_TRANSPORTSENDBUFFER_H */
