/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE size_t
TransportSendBuffer::capacity() const
{
  return this->capacity_;
}

ACE_INLINE size_t
TransportSendBuffer::n_chunks() const
{
  return this->n_chunks_;
}

ACE_INLINE void
TransportSendBuffer::bind(TransportSendStrategy* strategy) {
  this->strategy_ = strategy;
}

ACE_INLINE SequenceNumber
TransportSendBuffer::low() const
{
  if (this->buffers_.empty()) throw std::exception();
  return this->buffers_.begin()->first;
}

ACE_INLINE SequenceNumber
TransportSendBuffer::high() const
{
  if (this->buffers_.empty()) throw std::exception();
  return this->buffers_.rbegin()->first;
}

} // namespace DCPS
} // namespace OpenDDS
