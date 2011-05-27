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

template<class T, std::size_t N> ACE_INLINE
FirstTimeFastAllocator<T, N>::FirstTimeFastAllocator()
  : firstTime_(true)
{
}

template<class T, std::size_t N> ACE_INLINE
void *
FirstTimeFastAllocator<T, N>::malloc(size_t nbytes)
{
  if (firstTime_ && nbytes <= N * sizeof(T)) {
    firstTime_ = false;
    return (void*) pool_;

  } else {
    return ACE_OS::malloc(nbytes);
  }
}

template<class T, std::size_t N> ACE_INLINE
void
FirstTimeFastAllocator<T, N>::free(void *ptr)
{
  if (ptr != (void*) pool_) {
    ACE_OS::free(ptr);
  }
}

} // namespace  DDS
} // namespace OpenDDS
