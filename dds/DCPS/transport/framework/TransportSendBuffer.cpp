/*
 * $Id$
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

namespace OpenDDS {
namespace DCPS {

TransportSendBuffer::~TransportSendBuffer()
{
  //release_all();
}

void
TransportSendBuffer::resend_one(const BufferType& buffer)
{
  int bp = 0;
  this->strategy_->do_send_packet(buffer.second, bp);
}


// class SingleSendBuffer

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

void
SingleSendBuffer::release_all()
{
  for (BufferMap::iterator it(this->buffers_.begin());
       it != this->buffers_.end(); ++it) {
    release(it->second);
  }
  this->buffers_.clear();
}

void
SingleSendBuffer::release(BufferType& buffer)
{
  if (Transport_debug_level >= 10) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::release() - ")
      ACE_TEXT("releasing buffer at: (0x%x,0x%x)\n"),
      buffer.first, buffer.second
    ));
  }
  RemoveAllVisitor visitor;
  buffer.first->accept_remove_visitor(visitor);
  delete buffer.first;

  buffer.second->release();
  buffer.second = 0;
}

void
SingleSendBuffer::retain_all(RepoId pub_id)
{
  if (Transport_debug_level >= 4) {
    GuidConverter converter(pub_id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::retain_all() - ")
      ACE_TEXT("copying out blocks for publication: %C\n"),
      std::string(converter).c_str()
    ));
  }
  for (BufferMap::iterator it(this->buffers_.begin());
       it != this->buffers_.end(); ++it) {

    BufferType& buffer(it->second);

    TransportQueueElement::MatchOnPubId match(pub_id);
    PacketRemoveVisitor visitor(match,
                                buffer.second,
                                buffer.second,
                                this->replaced_allocator_,
                                this->replaced_mb_allocator_,
                                this->replaced_db_allocator_);

    buffer.first->accept_replace_visitor(visitor);
    if (visitor.status() == REMOVE_ERROR) {
      GuidConverter converter(pub_id);
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("SingleSendBuffer::retain_all: ")
                 ACE_TEXT("failed to retain data from publication: %C!\n"),
                 std::string(converter).c_str()));
      release(buffer);
    }
  }
}

void
SingleSendBuffer::insert(SequenceNumber sequence,
                         TransportSendStrategy::QueueType* queue,
                         ACE_Message_Block* chain)
{

  // Age off oldest sample if we are at capacity:
  if (this->buffers_.size() == this->capacity_) {
    BufferMap::iterator it(this->buffers_.begin());
    if (it == this->buffers_.end()) return;

    if (Transport_debug_level >= 10) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) SingleSendBuffer::insert() - ")
        ACE_TEXT("aging off PDU: %q as buffer(%q,%q)\n"),
        it->first.getValue(),
        it->second.first, it->second.second
      ));
    }

    release(it->second);
    this->buffers_.erase(it);
  }

  BufferType& buffer = this->buffers_[sequence];

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

  if (Transport_debug_level >= 10) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) SingleSendBuffer::insert() - ")
      ACE_TEXT("saved PDU: 0x%x as buffer(0x%x,0x%x)\n"),
      sequence.getValue(),
      buffer.first, buffer.second
    ));
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
      if (Transport_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) SingleSendBuffer::resend() - ")
                   ACE_TEXT("resending PDU: 0x%x, (0x%x,0x%x)\n"),
                   sequence.getValue(),
                   it->second.first,
                   it->second.second));
      }
      resend_one(it->second);
    }
  }

  // Have we resent all requested data?
  return range.first >= low() && range.second <= high();
}


} // namespace DCPS
} // namespace OpenDDS
