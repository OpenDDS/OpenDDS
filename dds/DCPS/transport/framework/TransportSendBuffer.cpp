/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TransportSendBuffer.h"
#include "CopyChainVisitor.h"
#include "PacketRemoveVisitor.h"
#include "RemoveAllVisitor.h"

#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/DisjointSequence.h"

#include "ace/Log_Msg.h"

#include "dds/DCPS/GuidConverter.h"

#ifndef __ACE_INLINE__
# include "TransportSendBuffer.inl"
#endif  /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


TransportSendBuffer::~TransportSendBuffer()
{
}

void
TransportSendBuffer::retain_all(const RepoId&)
{
}

void
TransportSendBuffer::resend_one(const BufferType& buffer)
{
  int bp = 0;
  strategy_->do_send_packet(buffer.second, bp);
}


// class SingleSendBuffer

const size_t SingleSendBuffer::UNLIMITED = 0;

SingleSendBuffer::SingleSendBuffer(size_t capacity,
                                   size_t max_samples_per_packet)
  : TransportSendBuffer(capacity),
    n_chunks_(capacity * max_samples_per_packet),
    retained_mb_allocator_(n_chunks_ * 2),
    retained_db_allocator_(n_chunks_ * 2),
    replaced_mb_allocator_(n_chunks_ * 2),
    replaced_db_allocator_(n_chunks_ * 2)
{
}

SingleSendBuffer::~SingleSendBuffer()
{
  release_all();
}

void
SingleSendBuffer::release_all()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  for (BufferMap::iterator it = buffers_.begin();
       it != buffers_.end();) {
    release_i(it++);
  }
}

void
SingleSendBuffer::release_acked(SequenceNumber seq) {
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  BufferMap::iterator buffer_iter = buffers_.find(seq);
  if (buffer_iter != buffers_.end()) {
    release_i(buffer_iter);
  }
}

void
SingleSendBuffer::remove_acked(SequenceNumber seq, BufferVec& removed) {
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  BufferMap::iterator buffer_iter = buffers_.find(seq);
  if (buffer_iter != buffers_.end()) {
    remove_i(buffer_iter, removed);
  }
}

void
SingleSendBuffer::release_i(BufferMap::iterator buffer_iter)
{
  BufferType& buffer(buffer_iter->second);
  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::release() - ")
      ACE_TEXT("releasing buffer at: (0x%@,0x%@)\n"),
      buffer.first, buffer.second
    ));
  }

  if (buffer.first && buffer.second) {
    // not a fragment
    RemoveAllVisitor visitor;
    buffer.first->accept_remove_visitor(visitor);
    delete buffer.first;

    buffer.second->release();
    buffer.second = 0;

  } else {
    // data actually stored in fragments_
    const FragmentMap::iterator fm_it = fragments_.find(buffer_iter->first);
    if (fm_it != fragments_.end()) {
      for (BufferMap::iterator bm_it = fm_it->second.begin();
           bm_it != fm_it->second.end(); ++bm_it) {
        RemoveAllVisitor visitor;
        bm_it->second.first->accept_remove_visitor(visitor);
        delete bm_it->second.first;

        bm_it->second.second->release();
        bm_it->second.second = 0;
      }
      fragments_.erase(fm_it);
    }
  }

  buffers_.erase(buffer_iter);
}

void
SingleSendBuffer::remove_i(BufferMap::iterator buffer_iter, BufferVec& removed)
{
  BufferType& buffer(buffer_iter->second);
  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::release() - ")
      ACE_TEXT("releasing buffer at: (0x%@,0x%@)\n"),
      buffer.first, buffer.second
    ));
  }

  if (buffer.first && buffer.second) {
    // not a fragment
    removed.push_back(buffer);
  } else {
    // data actually stored in fragments_
    const FragmentMap::iterator fm_it = fragments_.find(buffer_iter->first);
    if (fm_it != fragments_.end()) {
      for (BufferMap::iterator bm_it = fm_it->second.begin();
           bm_it != fm_it->second.end(); ++bm_it) {
        removed.push_back(bm_it->second);
      }
      fragments_.erase(fm_it);
    }
  }

  buffers_.erase(buffer_iter);
}

void
SingleSendBuffer::retain_all(const RepoId& pub_id)
{
  if (Transport_debug_level > 5) {
    GuidConverter converter(pub_id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::retain_all() - ")
      ACE_TEXT("copying out blocks for publication: %C\n"),
      OPENDDS_STRING(converter).c_str()
    ));
  }
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  for (BufferMap::iterator it(buffers_.begin());
       it != buffers_.end();) {
    if (it->second.first && it->second.second) {
      if (retain_buffer(pub_id, it->second) == REMOVE_ERROR) {
        GuidConverter converter(pub_id);
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ")
                   ACE_TEXT("SingleSendBuffer::retain_all: ")
                   ACE_TEXT("failed to retain data from publication: %C!\n"),
                   OPENDDS_STRING(converter).c_str()));
        release_i(it++);
      } else {
        ++it;
      }

    } else {
      const FragmentMap::iterator fm_it = fragments_.find(it->first);
      if (fm_it != fragments_.end()) {
        for (BufferMap::iterator bm_it = fm_it->second.begin();
             bm_it != fm_it->second.end();) {
          if (retain_buffer(pub_id, bm_it->second) == REMOVE_ERROR) {
            GuidConverter converter(pub_id);
            ACE_ERROR((LM_WARNING,
                       ACE_TEXT("(%P|%t) WARNING: ")
                       ACE_TEXT("SingleSendBuffer::retain_all: failed to ")
                       ACE_TEXT("retain fragment data from publication: %C!\n"),
                       OPENDDS_STRING(converter).c_str()));
            release_i(bm_it++);
          } else {
            ++bm_it;
          }
        }
      }
      ++it;
    }
  }
}

RemoveResult
SingleSendBuffer::retain_buffer(const RepoId& pub_id, BufferType& buffer)
{
  TransportQueueElement::MatchOnPubId match(pub_id);
  PacketRemoveVisitor visitor(match,
                              buffer.second,
                              buffer.second,
                              replaced_mb_allocator_,
                              replaced_db_allocator_);

  buffer.first->accept_replace_visitor(visitor);
  return visitor.status();
}

void
SingleSendBuffer::insert(SequenceNumber sequence,
                         TransportSendStrategy::QueueType* queue,
                         ACE_Message_Block* chain)
{
  BufferVec removed;
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  check_capacity_i(removed);

  BufferType& buffer = buffers_[sequence];
  insert_buffer(buffer, queue, chain);

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::insert() - ")
      ACE_TEXT("saved PDU: %q as buffer(0x%@,0x%@)\n"),
      sequence.getValue(),
      buffer.first, buffer.second
    ));
  }

  if (queue && queue->size() == 1) {
    const TransportQueueElement* elt = queue->peek();
    const RepoId subId = elt->subscription_id();
    const ACE_Message_Block* msg = elt->msg();
    if (msg && subId != GUID_UNKNOWN &&
        !DataSampleHeader::test_flag(HISTORIC_SAMPLE_FLAG, msg)) {
      destinations_[sequence] = subId;
    }
  }
  g.release();
  for (size_t i = 0; i < removed.size(); ++i) {
    RemoveAllVisitor visitor;
    removed[i].first->accept_remove_visitor(visitor);
    delete removed[i].first;
    removed[i].second->release();
  }
}

void
SingleSendBuffer::insert_buffer(BufferType& buffer,
                                TransportSendStrategy::QueueType* queue,
                                ACE_Message_Block* chain)
{
  // Copy sample's TransportQueueElements:
  TransportSendStrategy::QueueType*& elems = buffer.first;
  ACE_NEW(elems, TransportSendStrategy::QueueType());

  CopyChainVisitor visitor(*elems,
                           &retained_mb_allocator_,
                           &retained_db_allocator_);
  queue->accept_visitor(visitor);

  // Copy sample's message/data block descriptors:
  ACE_Message_Block*& data = buffer.second;
  data = TransportQueueElement::clone_mb(chain,
                                         &retained_mb_allocator_,
                                         &retained_db_allocator_);
}

void
SingleSendBuffer::insert_fragment(SequenceNumber sequence,
                                  SequenceNumber fragment,
                                  TransportSendStrategy::QueueType* queue,
                                  ACE_Message_Block* chain)
{
  BufferVec removed;
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  check_capacity_i(removed);

  // Insert into buffers_ so that the overall capacity is maintained
  // The entry in buffers_ with two null pointers indicates that the
  // actual data is stored in fragments_[sequence].
  buffers_[sequence] = std::make_pair(static_cast<QueueType*>(0),
                                      static_cast<ACE_Message_Block*>(0));

  BufferType& buffer = fragments_[sequence][fragment];
  insert_buffer(buffer, queue, chain);

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::insert_fragment() - ")
      ACE_TEXT("saved PDU: %q,%q as buffer(0x%@,0x%@)\n"),
      sequence.getValue(), fragment.getValue(),
      buffer.first, buffer.second
    ));
  }
  g.release();
  for (size_t i = 0; i < removed.size(); ++i) {
    RemoveAllVisitor visitor;
    removed[i].first->accept_remove_visitor(visitor);
    delete removed[i].first;
    removed[i].second->release();
  }
}

void
SingleSendBuffer::check_capacity_i(BufferVec& removed)
{
  if (capacity_ == SingleSendBuffer::UNLIMITED) {
    return;
  }
  // Age off oldest sample if we are at capacity:
  if (buffers_.size() == capacity_) {
    BufferMap::iterator it(buffers_.begin());
    if (it == buffers_.end()) return;

    if (Transport_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) SingleSendBuffer::check_capacity() - ")
        ACE_TEXT("aging off PDU: %q as buffer(0x%@,0x%@)\n"),
        it->first.getValue(),
        it->second.first, it->second.second
      ));
    }

    destinations_.erase(it->first);
    remove_i(it, removed);
  }
}

bool
SingleSendBuffer::resend(const SequenceRange& range, DisjointSequence* gaps)
{
  ACE_GUARD_RETURN(LockType, guard, strategy_lock(), false);
  return resend_i(range, gaps);
}

bool
SingleSendBuffer::resend_i(const SequenceRange& range, DisjointSequence* gaps)
{
  return resend_i(range, gaps, GUID_UNKNOWN);
}

bool
SingleSendBuffer::resend_i(const SequenceRange& range, DisjointSequence* gaps,
                           const RepoId& destination)
{
  //Special case, nak to make sure it has all history
  const SequenceNumber lowForAllResent = range.first == SequenceNumber() ? low() : range.first;
  const bool has_dest = destination != GUID_UNKNOWN;

  for (SequenceNumber sequence(range.first);
       sequence <= range.second; ++sequence) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
    // Re-send requested sample if still buffered; missing samples
    // will be scored against the given DisjointSequence:
    BufferMap::iterator it(buffers_.find(sequence));
    DestinationMap::iterator dest_data;
    if (has_dest) {
      dest_data = destinations_.find(sequence);
    }
    if (it == buffers_.end() || (has_dest && (dest_data == destinations_.end() ||
                                              dest_data->second != destination))) {
      if (gaps) {
        gaps->insert(sequence);
      }
    } else {
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) SingleSendBuffer::resend() - ")
                   ACE_TEXT("resending PDU: %q, (0x%@,0x%@)\n"),
                   sequence.getValue(),
                   it->second.first,
                   it->second.second));
      }
      if (it->second.first && it->second.second) {
        resend_one(it->second);
      } else {
        const FragmentMap::iterator fm_it = fragments_.find(it->first);
        if (fm_it != fragments_.end()) {
          for (BufferMap::iterator bm_it = fm_it->second.begin();
                bm_it != fm_it->second.end(); ++bm_it) {
            resend_one(bm_it->second);
          }
        }
      }
    }
  }
  // Have we resent all requested data?
  return lowForAllResent >= low() && range.second <= high();
}

void
SingleSendBuffer::resend_fragments_i(const SequenceNumber& seq,
                                     const DisjointSequence& requested_frags)
{
  if (fragments_.empty() || requested_frags.empty()) {
    return;
  }
  const FragmentMap::const_iterator fm_it = fragments_.find(seq);
  if (fm_it == fragments_.end()) {
    return;
  }
  const BufferMap& buffers = fm_it->second;
  const OPENDDS_VECTOR(SequenceRange)& psr = requested_frags.present_sequence_ranges();

  BufferMap::const_iterator it = buffers.lower_bound(psr.front().first);
  BufferMap::const_iterator end = buffers.lower_bound(psr.back().second);
  if (end != buffers.end()) {
    ++end;
  }

  SequenceNumber frag_min;
  size_t i = 0;

  // Iterate over both containers simultaneously
  while (i < psr.size() && it != end) {
    if (psr[i].second < frag_min) {
      ++i;
    } else {
      // Once the range max is over our fragment minimum, we either
      // expect overlap (resend fragment) or the range is too high (skip fragment)
      // Either way, we will increment the fragment now to avoid duplicate resends
      if (it->first >= psr[i].first) {
        resend_one(it->second); // overlap - resend fragment buffer
      }
      frag_min = it->first + 1; // increment fragment buffer
      ++it;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
