/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, this->n_chunks_);
  return this->n_chunks_;
}

ACE_INLINE void
SingleSendBuffer::pre_insert(SequenceNumber sequence)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  pre_seq_.insert(sequence);
}

ACE_INLINE size_t
SingleSendBuffer::size() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
  return retained_mb_allocator_.bytes_heap_allocated()
    + retained_db_allocator_.bytes_heap_allocated()
    + replaced_mb_allocator_.bytes_heap_allocated()
    + replaced_db_allocator_.bytes_heap_allocated()
    + buffers_.size()
    + fragments_.size()
    + destinations_.size()
    + pre_seq_.size();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
