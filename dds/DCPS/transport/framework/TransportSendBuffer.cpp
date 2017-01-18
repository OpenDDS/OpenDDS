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
TransportSendBuffer::resend_one(const BufferType& buffer)
{
  int bp = 0;
  this->strategy_->do_send_packet(buffer.second, bp);
}


// class SingleSendBuffer

const size_t SingleSendBuffer::UNLIMITED = 0;

SingleSendBuffer::SingleSendBuffer(size_t capacity,
                                   size_t max_samples_per_packet)
  : TransportSendBuffer(capacity),
    n_chunks_(capacity * max_samples_per_packet),
    retained_allocator_(this->n_chunks_),
    retained_mb_allocator_(this->n_chunks_ * 2),
    retained_db_allocator_(this->n_chunks_ * 2),
    replaced_allocator_(this->n_chunks_),
    replaced_mb_allocator_(this->n_chunks_ * 2),
    replaced_db_allocator_(this->n_chunks_ * 2)
{
}

SingleSendBuffer::~SingleSendBuffer()
{
  release_all();
}

void
SingleSendBuffer::release_all()
{
  for (BufferMap::iterator it(this->buffers_.begin());
       it != this->buffers_.end();) {
    release(it++);
  }
}

void
SingleSendBuffer::release_acked(SequenceNumber seq) {
  BufferMap::iterator buffer_iter = buffers_.begin();
  BufferType& buffer(buffer_iter->second);

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::release_acked() - ")
      ACE_TEXT("releasing buffer at: (0x%@,0x%@)\n"),
      buffer.first, buffer.second
    ));
  }
  while (buffer_iter != buffers_.end()) {
    if (buffer_iter->first == seq) {
      release(buffer_iter);
      return;
    }
    ++buffer_iter;
  }
}

void
SingleSendBuffer::release(BufferMap::iterator buffer_iter)
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
SingleSendBuffer::retain_all(RepoId pub_id)
{
  if (Transport_debug_level > 5) {
    GuidConverter converter(pub_id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::retain_all() - ")
      ACE_TEXT("copying out blocks for publication: %C\n"),
      OPENDDS_STRING(converter).c_str()
    ));
  }
  for (BufferMap::iterator it(this->buffers_.begin());
       it != this->buffers_.end();) {
    if (it->second.first && it->second.second) {
      if (retain_buffer(pub_id, it->second) == REMOVE_ERROR) {
        GuidConverter converter(pub_id);
        ACE_ERROR((LM_WARNING,
                   ACE_TEXT("(%P|%t) WARNING: ")
                   ACE_TEXT("SingleSendBuffer::retain_all: ")
                   ACE_TEXT("failed to retain data from publication: %C!\n"),
                   OPENDDS_STRING(converter).c_str()));
        release(it++);
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
            release(bm_it++);
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
                              this->replaced_allocator_,
                              this->replaced_mb_allocator_,
                              this->replaced_db_allocator_);

  buffer.first->accept_replace_visitor(visitor);
  return visitor.status();
}

void
SingleSendBuffer::insert(SequenceNumber sequence,
                         TransportSendStrategy::QueueType* queue,
                         ACE_Message_Block* chain)
{
  check_capacity();

  BufferType& buffer = this->buffers_[sequence];
  insert_buffer(buffer, queue, chain);

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::insert() - ")
      ACE_TEXT("saved PDU: %q as buffer(0x%@,0x%@)\n"),
      sequence.getValue(),
      buffer.first, buffer.second
    ));
  }
}

void
SingleSendBuffer::insert_buffer(BufferType& buffer,
                                TransportSendStrategy::QueueType* queue,
                                ACE_Message_Block* chain)
{
  // Copy sample's TransportQueueElements:
  TransportSendStrategy::QueueType*& elems = buffer.first;
  ACE_NEW(elems, TransportSendStrategy::QueueType(queue->size(), 1));

  CopyChainVisitor visitor(*elems,
                           &this->retained_allocator_,
                           &this->retained_mb_allocator_,
                           &this->retained_db_allocator_);
  queue->accept_visitor(visitor);

  // Copy sample's message/data block descriptors:
  ACE_Message_Block*& data = buffer.second;
  data = TransportQueueElement::clone_mb(chain,
                                         &this->retained_mb_allocator_,
                                         &this->retained_db_allocator_);
}

void
SingleSendBuffer::insert_fragment(SequenceNumber sequence,
                                  SequenceNumber fragment,
                                  TransportSendStrategy::QueueType* queue,
                                  ACE_Message_Block* chain)
{
  check_capacity();

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
}

void
SingleSendBuffer::check_capacity()
{
  if (this->capacity_ == SingleSendBuffer::UNLIMITED) {
    return;
  }
  // Age off oldest sample if we are at capacity:
  if (this->buffers_.size() == this->capacity_) {
    BufferMap::iterator it(this->buffers_.begin());
    if (it == this->buffers_.end()) return;

    if (Transport_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) SingleSendBuffer::check_capacity() - ")
        ACE_TEXT("aging off PDU: %q as buffer(0x%@,0x%@)\n"),
        it->first.getValue(),
        it->second.first, it->second.second
      ));
    }

    release(it);
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
  //Special case, nak to make sure it has all history
  const SequenceNumber lowForAllResent = range.first == SequenceNumber() ? low() : range.first;

  for (SequenceNumber sequence(range.first);
       sequence <= range.second; ++sequence) {
    // Re-send requested sample if still buffered; missing samples
    // will be scored against the given DisjointSequence:
    BufferMap::iterator it(this->buffers_.find(sequence));
    if (it == this->buffers_.end()) {
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
  const BufferMap& buffers = fragments_[seq];
  const OPENDDS_VECTOR(SequenceRange) psr =
    requested_frags.present_sequence_ranges();
  SequenceNumber sent = SequenceNumber::ZERO();
  for (size_t i = 0; i < psr.size(); ++i) {
    BufferMap::const_iterator it = buffers.lower_bound(psr[i].first);
    if (it == buffers.end()) {
      return;
    }
    BufferMap::const_iterator it2 = buffers.lower_bound(psr[i].second);
    while (true) {
      if (sent < it->first) {
        resend_one(it->second);
        sent = it->first;
      }
      if (it == it2) {
        break;
      }
      ++it;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
