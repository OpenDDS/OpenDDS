/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#if defined (_MSC_VER) && (_MSC_VER >= 1400)
# pragma warning (push)
# pragma warning (disable:4351)
#endif

template<class T, std::size_t N> ACE_INLINE
FirstTimeFastAllocator<T, N>::FirstTimeFastAllocator()
  : firstTime_(true)
  , pool_()
{
}
#if defined (_MSC_VER) && (_MSC_VER >= 1400)
# pragma warning (pop)
#endif

template<class T, std::size_t N> ACE_INLINE
void *
FirstTimeFastAllocator<T, N>::malloc(size_t nbytes)
{
  if (firstTime_ && nbytes <= N * sizeof(T)) {
    firstTime_ = false;
    return (void*) pool_;

  } else {
#if defined (ACE_HAS_ALLOC_HOOKS)
    return ACE_Allocator::instance()->malloc(nbytes);
#else
    return ACE_OS::malloc(nbytes);
#endif /* ACE_HAS_ALLOC_HOOKS */
  }
}

template<class T, std::size_t N> ACE_INLINE
void
FirstTimeFastAllocator<T, N>::free(void *ptr)
{
  if (ptr != (void*) pool_) {
#if defined (ACE_HAS_ALLOC_HOOKS)
     ACE_Allocator::instance()->free(ptr);
#else
    ACE_OS::free(ptr);
#endif /* ACE_HAS_ALLOC_HOOKS */
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
