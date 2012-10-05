/*
 * $Id$
 *
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

ACE_INLINE void
TransportSendBuffer::bind(TransportSendStrategy* strategy)
{
  this->strategy_ = strategy;
}


// class SingleSendBuffer

ACE_INLINE size_t
SingleSendBuffer::n_chunks() const
{
  return this->n_chunks_;
}

ACE_INLINE SequenceNumber
SingleSendBuffer::low() const
{
  if (this->buffers_.empty()) throw std::exception();
  return this->buffers_.begin()->first;
}

ACE_INLINE SequenceNumber
SingleSendBuffer::high() const
{
  if (this->buffers_.empty()) throw std::exception();
  return this->buffers_.rbegin()->first;
}

ACE_INLINE bool
SingleSendBuffer::empty() const
{
  return this->buffers_.empty();
}

ACE_INLINE bool
SingleSendBuffer::contains(const SequenceNumber& seq) const
{
  return this->buffers_.count(seq);
}

ACE_INLINE ACE_Message_Block*
SingleSendBuffer::msg(const SequenceNumber& seq) const
{
  BufferMap::const_iterator iter = this->buffers_.find(seq);
  if (iter == this->buffers_.end()) {
    return 0;
  }
  return iter->second.second;
}

} // namespace DCPS
} // namespace OpenDDS
