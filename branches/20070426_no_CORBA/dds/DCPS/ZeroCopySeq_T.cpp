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
  if (loaner_) 
    {
      loaner_->auto_return_loan(this);
      loaner_ = 0;
    }
}

template <class Sample_T, size_t ZCS_DEFAULT_SIZE>
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE> &
ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE>::operator= (const ZeroCopyDataSeq<Sample_T, ZCS_DEFAULT_SIZE> &frm)
{
  if (this != &frm)
    {
      if (loaner_) 
        {
          loaner_->auto_return_loan(this);
        }
      this->ZeroCopySeqBase::operator=(frm);
      this->is_zero_copy_ = frm.is_zero_copy_;
      this->loaner_       = frm.loaner_;
      if (this->is_zero_copy_)
        {
          this->ptrs_    = frm.ptrs_;
          // increment reference counts.
          for (size_t /* Ptr_Seq_Type::size_type */ ii = 0; ii < this->length_; ii++)
            {
              ptrs_[ii]->inc_ref();
              ptrs_[ii]->zero_copy_cnt_++;
            }
        }
      else
        {
          this->samples_ = frm.samples_;
        }
    }
  return *this;
}

    } // namespace  ::DDS
} // namespace TAO


#endif /* ZEROCOPYSEQ_H  */



