// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopySeqBase.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "dds/DCPS/ZeroCopySeqBase.h"

#if !defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopySeqBase.inl"
#endif /* __//ACE_INLINE__ */

namespace TAO
{
    namespace DCPS
    {

ZeroCopySeqBase::ZeroCopySeqBase(const size_t max_len)
    : length_(0)
    , max_len_(max_len)
    , owns_(max_len ? true : false)
{
}


    } // namespace  ::DDS
} // namespace TAO

