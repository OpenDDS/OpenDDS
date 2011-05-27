// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopySeq_T.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef ZEROCOPYSEQ_T_CPP
#define ZEROCOPYSEQ_T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/ZeroCopySeq_T.h"
#include "dds/DCPS/DataReaderImpl.h" // needed for gcc

#if !defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopySeq_T.inl"
#endif /* __//ACE_INLINE__ */

namespace TAO
{
    namespace DCPS
    {




template <class Sample_T, size_t ZCS_DEFAULT_SIZE>
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::~ZeroCopyDataSeq()
{
    if (loaner_) {
        loaner_->auto_return_loan(this);
        loaner_ = 0;
    }
}

    } // namespace  ::DDS
} // namespace TAO


#endif /* ZEROCOPYSEQ_H  */



