// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopyAllocator_T.inl
 *
 *  $Id$
 *
 *
 */
// ============================================================================


namespace TAO
{
    namespace DCPS
    {

template<class T, std::size_t N> ACE_INLINE
FirstTimeFastAllocator<T, N>::FirstTimeFastAllocator() 
: firstTime_(1) 
{
};

template<class T, std::size_t N> ACE_INLINE
void *
FirstTimeFastAllocator<T, N>::malloc (size_t nbytes) { 
    if (firstTime_ && nbytes <= N * sizeof(T)) {
        firstTime_ = 0;
        return (void*) pool_;
    }
    else {
        return ACE_OS::malloc(nbytes);
    }
};

template<class T, std::size_t N> ACE_INLINE
void
FirstTimeFastAllocator<T, N>::free (void *ptr) {
    if (ptr != (void*) pool_) {
        ACE_OS::free(ptr);
    }
};


    } // namespace  ::DDS
} // namespace TAO

