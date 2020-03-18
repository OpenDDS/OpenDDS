/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TRANSPORTSENDBUFFER_H
#define DCPS_TRANSPORTSENDBUFFER_H

#include "dds/DCPS/dcps_export.h"

#include "TransportRetainedElement.h"
#include "TransportReplacedElement.h"
#include "TransportSendStrategy.h"

#include "dds/DCPS/Definitions.h"

#include "dds/DCPS/PoolAllocator.h"
#include "ace/Synch_Traits.h"
#include <utility>

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DisjointSequence;

/// Abstract base class that forms the interface for TransportSendStrategy
/// to store data for potential retransmission.  Derived classes actually
/// store the data and can utilize TransportSendBuffer's friendship in
/// TransportSendStrategy to retransmit (see method "resend_one").
class OpenDDS_Dcps_Export TransportSendBuffer {
public:
  size_t capacity() const;
  void bind(TransportSendStrategy* strategy);

  virtual void retain_all(const RepoId& pub_id);
  virtual void insert(SequenceNumber sequence,
                      TransportSendStrategy::QueueType* queue,
                      ACE_Message_Block* chain) = 0;

  typedef TransportSendStrategy::LockType LockType;
  LockType& strategy_lock() { return this->strategy_->lock_; }

protected:
  explicit TransportSendBuffer(size_t capacity)
    : strategy_(0), capacity_(capacity) {}
  virtual ~TransportSendBuffer();

  typedef TransportSendStrategy::QueueType QueueType;
  typedef std::pair<QueueType*, ACE_Message_Block*> BufferType;

  void resend_one(const BufferType& buffer);

  TransportSendStrategy* strategy_;
  const size_t capacity_;

private:
  TransportSendBuffer(const TransportSendBuffer&); // unimplemented
  TransportSendBuffer& operator=(const TransportSendBuffer&); // unimplemented
};

/// Implementation of TransportSendBuffer that manages data for a single
/// domain of SequenceNumbers -- for a given SingleSendBuffer object, the
/// sequence numbers passed to insert() must be generated from the same place.
class OpenDDS_Dcps_Export SingleSendBuffer
  : public TransportSendBuffer, public RcObject {
public:

  static const size_t UNLIMITED;

  void release_all();
  typedef OPENDDS_VECTOR(BufferType) BufferVec;
  typedef OPENDDS_MAP(SequenceNumber, BufferType) BufferMap;
  void release_acked(SequenceNumber seq);
  void remove_acked(SequenceNumber seq, BufferVec& removed);
  size_t n_chunks() const;

  SingleSendBuffer(size_t capacity, size_t max_samples_per_packet);
  ~SingleSendBuffer();

  bool resend(const SequenceRange& range, DisjointSequence* gaps = 0);

  // caller must already have the send strategy lock
  bool resend_i(const SequenceRange& range, DisjointSequence* gaps = 0);
  bool resend_i(const SequenceRange& range, DisjointSequence* gaps,
                const RepoId& destination);

  void resend_fragments_i(const SequenceNumber& sequence,
                          const DisjointSequence& fragments);

  SequenceNumber low() const;
  SequenceNumber high() const;
  bool empty() const;
  bool contains(const SequenceNumber& seq) const;

  void retain_all(const RepoId& pub_id);
  void insert(SequenceNumber sequence,
              TransportSendStrategy::QueueType* queue,
              ACE_Message_Block* chain);
  void insert_fragment(SequenceNumber sequence,
                       SequenceNumber fragment,
                       TransportSendStrategy::QueueType* queue,
                       ACE_Message_Block* chain);

private:
  void check_capacity_i(BufferVec& removed);
  void release_i(BufferMap::iterator buffer_iter);
  void remove_i(BufferMap::iterator buffer_iter, BufferVec& removed);

  RemoveResult retain_buffer(const RepoId& pub_id, BufferType& buffer);
  void insert_buffer(BufferType& buffer,
                     TransportSendStrategy::QueueType* queue,
                     ACE_Message_Block* chain);

  size_t n_chunks_;

  MessageBlockAllocator retained_mb_allocator_;
  DataBlockAllocator retained_db_allocator_;
  MessageBlockAllocator replaced_mb_allocator_;
  DataBlockAllocator replaced_db_allocator_;

  BufferMap buffers_;

  typedef OPENDDS_MAP(SequenceNumber, BufferMap) FragmentMap;
  FragmentMap fragments_;

  typedef OPENDDS_MAP(SequenceNumber, RepoId) DestinationMap;
  DestinationMap destinations_;

  ACE_Thread_Mutex mutex_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "TransportSendBuffer.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_TRANSPORTSENDBUFFER_H */
