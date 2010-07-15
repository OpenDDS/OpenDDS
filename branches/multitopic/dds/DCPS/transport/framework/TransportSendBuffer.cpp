/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TransportSendBuffer.h"
#include "CopyChainVisitor.h"
#include "PacketRemoveVisitor.h"
#include "RemoveAllVisitor.h"

#include "ace/Log_Msg.h"

#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/RepoIdConverter.h"

#ifndef __ACE_INLINE__
# include "TransportSendBuffer.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportSendBuffer::TransportSendBuffer(size_t capacity,
                                         size_t max_samples_per_packet)
  : capacity_(capacity),
    n_chunks_(capacity * max_samples_per_packet),
    retained_allocator_(this->n_chunks_),
    retained_mb_allocator_(this->n_chunks_ * 2),
    retained_db_allocator_(this->n_chunks_ * 2),
    replaced_allocator_(this->n_chunks_),
    replaced_mb_allocator_(this->n_chunks_ * 2),
    replaced_db_allocator_(this->n_chunks_ * 2)
{
}

TransportSendBuffer::~TransportSendBuffer()
{
  //release_all();
}

void
TransportSendBuffer::release_all()
{
  for (BufferMap::iterator it(this->buffers_.begin());
       it != this->buffers_.end(); ++it) {
    release(it->second);
  }
  this->buffers_.clear();
}

void
TransportSendBuffer::release(buffer_type& buffer)
{
  if ( OpenDDS::DCPS::Transport_debug_level >= 10) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) TransportSendBuffer::release() - ")
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
TransportSendBuffer::retain_all(RepoId pub_id)
{
  if ( OpenDDS::DCPS::Transport_debug_level >= 4) {
    RepoIdConverter converter(pub_id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) TransportSendBuffer::retain_all() - ")
      ACE_TEXT("copying out blocks for publication: %C\n"),
      std::string(converter).c_str()
    ));
  }
  for (BufferMap::iterator it(this->buffers_.begin());
       it != this->buffers_.end(); ++it) {

    buffer_type& buffer(it->second);

    TransportRetainedElement sample(0, pub_id);
    PacketRemoveVisitor visitor(sample,
                                buffer.second,
                                buffer.second,
                                this->replaced_allocator_,
                                this->replaced_mb_allocator_,
                                this->replaced_db_allocator_);

    buffer.first->accept_replace_visitor(visitor);
    if (visitor.status() < 0) {
      RepoIdConverter converter(pub_id);
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("TransportSendBuffer::retain_all: ")
                 ACE_TEXT("failed to retain data from publication: %C!\n"),
                 std::string(converter).c_str()));
      release(buffer);
    }
  }
}

void
TransportSendBuffer::insert(SequenceNumber sequence, const buffer_type& value)
{

  // Age off oldest sample if we are at capacity:
  if (this->buffers_.size() == this->capacity_) {
    BufferMap::iterator it(this->buffers_.begin());
    if (it == this->buffers_.end()) return;

    if ( OpenDDS::DCPS::Transport_debug_level >= 10) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) TransportSendBuffer::insert() - ")
        ACE_TEXT("aging off PDU: 0x%x as buffer(0x%x,0x%x)\n"),
        it->first.getValue(),
        it->second.first, it->second.second
      ));
    }

    release(it->second);
    this->buffers_.erase(it);
  }

  std::pair<BufferMap::iterator, bool> pair =
    this->buffers_.insert(BufferMap::value_type(sequence, buffer_type()));
  if (pair.first == this->buffers_.end()) return;

  buffer_type& buffer(pair.first->second);

  // Copy sample's TransportQueueElements:
  TransportSendStrategy::QueueType*& elems = buffer.first;
  ACE_NEW(elems, TransportSendStrategy::QueueType(value.first->size(), 1));

  CopyChainVisitor visitor(*elems, 
                           &this->retained_allocator_, 
                           &this->retained_mb_allocator_, 
                           &this->retained_db_allocator_);
  value.first->accept_visitor(visitor);

  // Copy sample's message/data block descriptors:
  ACE_Message_Block*& data = buffer.second;
  data = TransportQueueElement::clone(value.second, 
                                      &this->retained_mb_allocator_, 
                                      &this->retained_db_allocator_);

  if ( OpenDDS::DCPS::Transport_debug_level >= 10) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) TransportSendBuffer::insert() - ")
      ACE_TEXT("saved PDU: 0x%x as buffer(0x%x,0x%x)\n"),
      sequence.getValue(),
      buffer.first, buffer.second
    ));
  }
}

bool
TransportSendBuffer::resend(const SequenceRange& range)
{
  ACE_GUARD_RETURN( TransportSendStrategy::LockType,
                    guard,
                    this->strategy_->lock_,
                    false);

  for (SequenceNumber sequence(range.first);
       sequence <= range.second; ++sequence) {
    // Re-send requested sample if still buffered; missing samples
    // will be scored against the given DisjointSequence:
    BufferMap::iterator it(this->buffers_.find(sequence));
    if (it != this->buffers_.end()) {
      if (OpenDDS::DCPS::Transport_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) TransportSendBuffer::resend() - ")
                   ACE_TEXT("resending PDU: 0x%x, (0x%x,0x%x)\n"),
                   sequence.getValue(),
                   it->second.first,
                   it->second.second));
      }
      resend(it->second);
    }
  }

  // Have we resent all requested data?
  return range.first >= low() && range.second <= high();
}

void
TransportSendBuffer::resend(buffer_type& buffer)
{
  int bp = 0;
  this->strategy_->do_send_packet(buffer.second, bp);
}

} // namespace DCPS
} // namespace OpenDDS
