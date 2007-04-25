// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopySeq_T.inl
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

template <size_t ZCS_DEFAULT_SIZE> ACE_INLINE
ZeroCopyInfoSeq<ZCS_DEFAULT_SIZE>::ZeroCopyInfoSeq(
    const size_t max_len,
    const size_t init_size,
    ACE_Allocator* alloc) 
    : ZeroCopySeqBase(max_len)
    , info_(max_len > init_size ? max_len : init_size, alloc ? alloc : &defaultAllocator_)
{};

//======== CORBA sequence like methods ======

template <size_t ZCS_DEFAULT_SIZE> ACE_INLINE
::DDS::SampleInfo const & 
ZeroCopyInfoSeq<ZCS_DEFAULT_SIZE>::operator[](CORBA::ULong i) const 
{
    return this->info_[i];
};

template <size_t ZCS_DEFAULT_SIZE> ACE_INLINE
::DDS::SampleInfo & 
ZeroCopyInfoSeq<ZCS_DEFAULT_SIZE>::operator[](CORBA::ULong i) 
{
    return this->info_[i];
};

template <size_t ZCS_DEFAULT_SIZE> ACE_INLINE
CORBA::ULong 
ZeroCopyInfoSeq<ZCS_DEFAULT_SIZE>::length() const 
{
    return this->length_;
};

template <size_t ZCS_DEFAULT_SIZE> ACE_INLINE
void 
ZeroCopyInfoSeq<ZCS_DEFAULT_SIZE>::length(CORBA::ULong length) 
{
    // TBD - support resizing.
    this->length_ = length;
};


template <size_t ZCS_DEFAULT_SIZE> ACE_INLINE
CORBA::ULong 
ZeroCopyInfoSeq<ZCS_DEFAULT_SIZE>::maximum() const 
{
    return this->max_len();
};

template <size_t ZCS_DEFAULT_SIZE> ACE_INLINE
CORBA::ULong 
ZeroCopyInfoSeq<ZCS_DEFAULT_SIZE>::max_slots() const 
{
    return this->info_.max_size();
};


    } // namespace  ::DDS
} // namespace TAO

