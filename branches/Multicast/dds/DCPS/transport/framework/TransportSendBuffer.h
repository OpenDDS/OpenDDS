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

#include "TransportSendStrategy.h"
#include "TransportSendStrategy_rch.h"

#include "ace/Message_Block.h"

#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/RcObject_T.h"

#include <map>
#include <set>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportSendBuffer
  : public RcObject<ACE_SYNCH_MUTEX> {
public:
  typedef std::pair<TransportSendStrategy::QueueType*, ACE_Message_Block*> buffer_type;

  explicit TransportSendBuffer(size_t capacity,
                               size_t max_samples_per_packet);
  ~TransportSendBuffer();

  void bind(TransportSendStrategy* strategy);

  void release_all();
  void release(buffer_type& buffer);

  void insert(SequenceNumber sequence, const buffer_type& value);

  void retain(RepoId pub_id);

  bool resend(const DisjointSequence::range_type& range,
              DisjointSequence& missing);
  void resend(buffer_type& buffer);

private:
  size_t capacity_;
  
  TransportRetainedElementAllocator sample_allocator_;
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
