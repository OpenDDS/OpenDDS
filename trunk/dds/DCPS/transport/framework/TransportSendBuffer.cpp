/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
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
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

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
    replaced_allocator_(this->n_chunks_)
{
}

TransportSendBuffer::~TransportSendBuffer()
{
  release_all();
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
  RemoveAllVisitor visitor;
  buffer.first->accept_remove_visitor(visitor);
  delete buffer.first;

  buffer.second->release();
  buffer.second = 0;
}

void
TransportSendBuffer::retain_all(RepoId pub_id)
{
  for (BufferMap::iterator it(this->buffers_.begin());
       it != this->buffers_.end(); ++it) {

    buffer_type& buffer(it->second);

    TransportRetainedElement sample(0, pub_id);
    PacketRemoveVisitor visitor(sample,
                                buffer.second,
                                buffer.second,
                                this->replaced_allocator_);

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

    release(it->second);
    this->buffers_.erase(it);
  }

  std::pair<BufferMap::iterator, bool> pair =
    this->buffers_.insert(BufferMap::value_type(sequence, buffer_type()));
  if (pair.first == this->buffers_.end()) return;

  buffer_type& buffer(pair.first->second);

  // Copy sample's TransportQueueElements:
  queue_type*& elems = buffer.first;
  ACE_NEW(elems, queue_type(value.first->size(), 1));

  CopyChainVisitor visitor(*elems, &this->retained_allocator_);
  value.first->accept_visitor(visitor);

  // Copy sample's message/data block descriptors:
  ACE_Message_Block*& data = buffer.second;
  data = value.second->duplicate();
}

bool
TransportSendBuffer::resend(const DisjointSequence::range_type& range,
                            DisjointSequence& missing)
{
  // Set bounds on given DisjointSequence:
  missing.skip(range.first.value_ - 1);     // low
  missing.update(range.second.value_ + 1);  // high

  for (SequenceNumber sequence(range.first);
       sequence <= range.second; ++sequence) {
    // Re-send requested sample if still buffered; missing samples
    // will be scored against the given DisjointSequence:
    BufferMap::iterator it(this->buffers_.find(sequence));
    if (it != this->buffers_.end()) {
      resend(it->second);
      missing.update(sequence);
    }
  }

  return !missing.disjoint();
}

void
TransportSendBuffer::resend(buffer_type& buffer)
{
  ACE_GUARD(ACE_SYNCH_MUTEX,
            guard,
            this->strategy_->lock());

  int bp = 0;
  this->strategy_->do_send_packet(buffer.second, bp);
}

} // namespace DCPS
} // namespace OpenDDS
