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

#ifndef ZEROCOPYINFOSEQ_T_CPP
#define ZEROCOPYINFOSEQ_T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/ZeroCopyInfoSeq_T.h"

#if !defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopyInfoSeq_T.inl"
#endif /* __//ACE_INLINE__ */

namespace TAO
{
    namespace DCPS
    {
template <class InfoType, size_t ZCS_DEFAULT_SIZE> ACE_INLINE
ZeroCopyInfoSeq<InfoType, ZCS_DEFAULT_SIZE> &
ZeroCopyInfoSeq<InfoType, ZCS_DEFAULT_SIZE>::operator= (const ZeroCopyInfoSeq<InfoType, ZCS_DEFAULT_SIZE> &frm)
{
  if (this != &frm)
    {
      this->ZeroCopySeqBase::operator=(frm);
      this->info_ = frm.info_;
    }
  return *this;
}

    } // namespace  ::DDS
} // namespace TAO


#endif /* ZEROCOPYINFOSEQ_T_CPP  */



