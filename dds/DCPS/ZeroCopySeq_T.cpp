/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ZEROCOPYSEQ_T_CPP
#define ZEROCOPYSEQ_T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/ZeroCopySeq_T.h"

#if !defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopySeq_T.inl"
#endif /* __//ACE_INLINE__ */

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

namespace TAO {
namespace DCPS {

template <class Sample_T, size_t DEF_MAX>
Sample_T ZeroCopyDataSeq<Sample_T, DEF_MAX>::default_;

template <class Sample_T, size_t DEF_MAX>
ZeroCopyDataSeq<Sample_T, DEF_MAX>::ZeroCopyDataSeq(
  const ZeroCopyDataSeq& frm)
  : loaner_(frm.loaner_)
  , ptrs_(frm.ptrs_.size(),
            (frm.ptrs_.allocator_ ==
             static_cast<const ACE_Allocator*>(&frm.default_allocator_))
            ? &default_allocator_
            : const_cast<ACE_Allocator*>(frm.ptrs_.allocator_))
    //The constructor of ptrs_ requires a non-const ptr to ACE_Alloc.
  , sc_maximum_(frm.sc_maximum_)
  , sc_length_(0) //initialized below
  , sc_buffer_(frm.sc_maximum_ ? allocbuf(frm.sc_maximum_) : 0)
  , sc_release_(frm.sc_maximum_)
{
  if (frm.is_zero_copy()) {
    ptrs_ = frm.ptrs_;

    //ptrs_ doesn't manage the ref count for its elements
    for (size_t ii = 0; ii < frm.ptrs_.size(); ++ii) {
      ptrs_[ii]->inc_ref();
      ++ptrs_[ii]->zero_copy_cnt_;
    }

  } else {
    for (CORBA::ULong i = 0; i < frm.sc_length_; ++i) {
      sc_buffer_[i] = frm.sc_buffer_[i];
      ++sc_length_;
    }
  }
}

#if defined (_MSC_VER)
#pragma warning (disable:4996) //std::copy OK here
#endif
template <class Sample_T, size_t DEF_MAX>
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::length(CORBA::ULong length)
{
  using std::fill;
  using std::max;
  using std::copy;

  if (length == this->length()) {
    return;
  }

  if (is_zero_copy()) {
    if (length < ptrs_.size()) {
      if (!loaner_) {
        make_single_copy(length);
        this->length(length);
        return;
      }

      for (size_t i(length); i < ptrs_.size(); ++i) {
        --ptrs_[i]->zero_copy_cnt_;
        loaner_->dec_ref_data_element(ptrs_[i]);
      }

      ptrs_.resize(length, 0);
      // At this point, there is no longer a loan
      this->set_loaner(0);

    } else {
      //There's no way we can expand the size (logical) of the zero-copy
      //array and have the user do any meaningful operations on the new
      //elements.  The fact that they're pointers to ReceivedDataElement
      //is hidden from the user.  Thus we need to make the sequence
      //single-copy at this point.
      make_single_copy(length);
      sc_length_ = length;
    }

  } else {
    if (length < sc_length_) { //shrink
      sc_length_ = length;

    } else if (length <= sc_maximum_) { //grow within buffer
      fill(&sc_buffer_[sc_length_], &sc_buffer_[length], Sample_T());
      sc_length_ = length;

    } else { //grow to larger buffer
      ZeroCopyDataSeq<Sample_T, DEF_MAX> grow(max(length, sc_maximum_*2));
      copy(sc_buffer_, &sc_buffer_[sc_length_], grow.sc_buffer_);
      fill(&grow.sc_buffer_[sc_length_], &grow.sc_buffer_[length],
           Sample_T());
      swap(grow);
    }
  }
}
#if defined (_MSC_VER)
#pragma warning (default:4996)
#endif

template <class Sample_T, size_t DEF_MAX>
Sample_T*
ZeroCopyDataSeq<Sample_T, DEF_MAX>::get_buffer(
  CORBA::Boolean orphan /* = false */)
{
  //Case 1: I can't give away what's not mine
  //  (includes zero-copy since sc_release_ is always false for zero-copy
  if (orphan && !sc_release_) return 0;

  // (preparation for cases 2-3)
  if (is_zero_copy()) make_single_copy(max_slots());

  if (!sc_buffer_) {
    sc_buffer_ = allocbuf(sc_maximum_);

    if (!orphan) sc_release_ = true;
  }

  //Case 2: Keeping the buffer but letting client use it too
  if (!orphan) return sc_buffer_;

  //Case 3: Orphaning the buffer to the client, leaves "this" in the
  //  default-constructed state (which in our case is ZC-enabled)
  ZeroCopyDataSeq<Sample_T, DEF_MAX> yours;
  swap(yours);
  yours.sc_release_ = false; //don't freebuf in dtor
  return yours.sc_buffer_;
}

#if defined (_MSC_VER)
#pragma warning (default:4996)
#endif

template <class Sample_T, size_t DEF_MAX>
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::increment_references(void){
  if (is_zero_copy()) {
    for (size_t ii = 0; ii < ptrs_.size(); ++ii) {
      ptrs_[ii]->inc_ref();
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

TAO_END_VERSIONED_NAMESPACE_DECL

#endif /* ZEROCOPYSEQ_H  */
