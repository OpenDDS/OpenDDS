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

//============================================================================
//============================================================================
//============================================================================


//============================================================================
//============================================================================
//============================================================================

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::ZeroCopyDataSeq(
    const size_t max_len = 0,
    const size_t init_size = ZCS_DEFAULT_SIZE,
    ACE_Allocator* alloc = 0) 
    : ZeroCopySeqBase(max_len)
    , is_zero_copy_(max_len == 0)
    , loaner_(0)
    , ptrs_(max_len > init_size ? max_len : init_size, alloc ? alloc : &defaultAllocator_)
    , samples_(max_len > 0 ? this->ptrs_.max_size() : 0)
{
}


//======== DDS specification inspired methods =====

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
CORBA::ULong 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::length() const {
    return this->length_;
}

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
void 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::length(CORBA::ULong length) {
    // Support spec saying:
    // To avoid potential memory leaks, the implementation of the Data and SampleInfo 
    // collections should disallow changing the length of a collection for which owns==FALSE.
    ACE_ASSERT(this->owns() == true);
    ACE_ASSERT(length <= this->max_slots());

    this->set_length(length);
}

//======== CORBA sequence like methods ======

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
Sample_T const & 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::operator[](CORBA::ULong i) const {
    // TBD? should we make it a violation to reference when max_len=0?
    if (is_zero_copy_) 
        return *((Sample_T*)this->ptrs_[i]->registered_data_);
    else
        return this->samples_[i];
}

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
Sample_T & 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::operator[](CORBA::ULong i) {
    // TBD? should we make it a violation to reference when max_len=0?
    if (is_zero_copy_) 
        return *((Sample_T*)this->ptrs_[i]->registered_data_);
    else
        return this->samples_[i];
}

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
CORBA::ULong 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::maximum() const {
    return this->max_len();
}

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
CORBA::ULong 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::max_slots() const {
    if (this->is_zero_copy_ == true)
    {
        return this->ptrs_.max_size();
    }
    else
    {
        return this->samples_.max_size();
    } 
}


// ==== OpenDDS unique methods =====

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
void 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::assign_ptr(::CORBA::ULong ii, 
                                                                     ::TAO::DCPS::ReceivedDataElement* item,
                                                                     ::TAO::DCPS::DataReaderImpl* loaner) 
{
    ACE_ASSERT(this->is_zero_copy_ == true);
    item->inc_ref();
    item->zero_copy_cnt_++;
    loaner_ = loaner; // remember which DataReadr contains this data
    ptrs_[ii] = item;
}

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
TAO::DCPS::ReceivedDataElement* 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::getPtr(::CORBA::ULong ii) const 
{
    ACE_ASSERT(this->is_zero_copy_ == true);
    return ptrs_[ii];
}
template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
void 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::assign_sample(::CORBA::ULong ii, const Sample_T& sample) 
{
    ACE_ASSERT(this->is_zero_copy_ == false);
    samples_[ii] = sample;
}

template <class Sample_T, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
void 
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::set_length(CORBA::ULong length) 
{
    ACE_ASSERT(length <= this->max_slots());

    /* this is too complicated since we are not supporting resizing

    // NOTE: +20 helps avoid copying every time the Seq length is incremented
    // and we know that they are pointers so 20 more is no big deal
    CORBA::ULong nextLen = length+20;
    if (this->max_len_)
    {
    if (nextLen > this->max_len_) {
    nextLen = length;
    }
    if (nextLen > this->max_len_) {
    ACE_ERROR((LM_ERROR,"ZeroCopySeq::length(%d) setting length > max_len %d -- no allowed.\n", 
    length, this->max_len_));
    // TBD - it would be nice to tell the calling code that this did not work.
    return;  
    }
    }
    if (this->length_ > ptrs_.max_size()) {

    ACE_ERROR((LM_ERROR,"ZeroCopySeq::length(%d) setting length > max/initial length %d -- no allowed.\n", 
    length, ptrs_.max_size()));
    ACE_ASSERT(this->length_ <= ptrs_.max_size()); // make it easier to debug
    return;
    // TBD allow resizing? - must copy the pointers
    //     I do not think it is required per the spec.
    // Note - resize does not copy the existing values.
    //this->ptrs_.resize(nextLen, (TAO::DCPS::ReceivedDataElement*)0); 
    }
    // only need to resize the samples if we are using them.
    if (this->max_len_ && this->length_ > samples_.max_size()) {

    // the above check should handle this but better safe than sorry.
    ACE_ERROR((LM_ERROR,"ZeroCopySeq::length(%d) setting length > max/initial length %d -- no allowed.\n", 
    length, ptrs_.max_size()));
    ACE_ASSERT(this->length_ <= samples_.max_size()); // make it easier to debug
    return;
    // TBD allow resizing? - must copy the pointers
    //     I do not think it is required per the spec.
    // Note - resize does not copy the existing values.
    this->samples_.resize(nextLen, Sample_T()); 
    }
    */
    this->length_ = length;
};

//============================================================================
//============================================================================
//============================================================================

template <size_t ZCS_DEFAULT_SIZE> ACE_INLINE
ZeroCopyInfoSeq<ZCS_DEFAULT_SIZE>::ZeroCopyInfoSeq(
    const size_t max_len = 0,
    const size_t init_size = ZCS_DEFAULT_SIZE,
    ACE_Allocator* alloc = 0) 
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

