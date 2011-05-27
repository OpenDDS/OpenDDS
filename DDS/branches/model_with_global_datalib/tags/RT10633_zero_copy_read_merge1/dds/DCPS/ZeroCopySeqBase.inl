// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopySeqBase.inl
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



ACE_INLINE
CORBA::ULong 
ZeroCopySeqBase::max_len() const 
{
    return this->max_len_;
}

ACE_INLINE
void 
ZeroCopySeqBase::max_len(CORBA::ULong length) 
{
    this->max_len_ = length;
}


ACE_INLINE
bool 
ZeroCopySeqBase::owns() const 
{
    return this->owns_;
}

ACE_INLINE
void 
ZeroCopySeqBase::owns(bool owns) 
{
    this->owns_ = owns;
}

//======== CORBA sequence like methods ======

ACE_INLINE
CORBA::Boolean 
ZeroCopySeqBase::release() const 
{
    return this->owns_;
}

    } // namespace  ::DDS
} // namespace TAO


