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

#ifndef __ACE_INLINE__
# include "TransportSendBuffer.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportSendBuffer::TransportSendBuffer(size_t capacity,
                                         size_t max_samples_per_packet)
  : capacity_(capacity),
    sample_allocator_(capacity * max_samples_per_packet),
    replaced_allocator_(capacity * max_samples_per_packet)
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

  buffer_type& buffer(pair.first->second);

  // Copy the sample's TransportQueueElements:
  TransportSendStrategy::QueueType*& samples(buffer.first);
  ACE_NEW(samples, TransportSendStrategy::QueueType(value.first->size(), 1));

  CopyChainVisitor visitor(*samples, &this->sample_allocator_);
  value.first->accept_visitor(visitor);

  // Copy the sample's message/data block descriptors:
  ACE_Message_Block*& data(buffer.second);
  data = value.second->duplicate();
}

void
TransportSendBuffer::retain(RepoId pub_id)
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
                 ACE_TEXT("TransportSendBuffer::retain_samples: ")
                 ACE_TEXT("failed to retain samples from publication: %C!\n"),
                 std::string(converter).c_str()));
      release(buffer);
    }
  }
}

bool
TransportSendBuffer::resend(const DisjointSequence::range_type& range,
                            DisjointSequence& missing)
{
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
  int bp = 0;
  this->strategy_->do_send_packet(buffer.second, bp);
}

} // namespace DCPS
} // namespace OpenDDS
