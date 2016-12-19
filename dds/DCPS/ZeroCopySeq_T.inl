/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/ReceivedDataElementList.h"
#include "dds/DCPS/Loaner.h"
#include "ace/Truncate.h"

#include <utility>
#include <algorithm>

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

namespace TAO {
namespace DCPS {

//ZeroCopyVector implementation

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
ZeroCopyDataSeq<Sample_T, DEF_MAX>::ZeroCopyVector::ZeroCopyVector(
  const size_t init_size,
  ACE_Allocator* alloc)
  : ACE_Vector<OpenDDS::DCPS::ReceivedDataElement*, DEF_MAX> (init_size, alloc)
{
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::ZeroCopyVector::swap(ZeroCopyVector& rhs)
{
  //Later versions of ACE do have a working ACE_Vector<T,MAX>::swap so we must
  //delegate up to ACE_Array<T> to get consistent swap behavior.
  ACE_Array<OpenDDS::DCPS::ReceivedDataElement*>::swap(rhs);
  std::swap(this->length_, rhs.length_);
  std::swap(this->curr_max_size_, rhs.curr_max_size_);
}

//ZeroCopyDataSeq implementation

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
ZeroCopyDataSeq<Sample_T, DEF_MAX>::ZeroCopyDataSeq(
  CORBA::ULong maximum /* = 0 */,
  CORBA::ULong init_size /* = DEF_MAX */,
  ACE_Allocator* alloc /* = 0 */)
  : loaner_(0)
  , ptrs_((maximum == 0) ? init_size : 0
            , alloc ? alloc : &default_allocator_)
  , sc_maximum_(maximum)
  , sc_length_(0)
  , sc_buffer_(sc_maximum_ ? allocbuf(sc_maximum_) : 0)
  , sc_release_(sc_maximum_)
{
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
ZeroCopyDataSeq<Sample_T, DEF_MAX>::ZeroCopyDataSeq(
  CORBA::ULong maximum,
  CORBA::ULong length,
  Sample_T* buffer,
  CORBA::Boolean release /* = false */)
  : loaner_(0)
  , ptrs_(0)
  , sc_maximum_(maximum)
  , sc_length_(length)
  , sc_buffer_(buffer)
  , sc_release_(release)
{
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
ZeroCopyDataSeq<Sample_T, DEF_MAX>&
ZeroCopyDataSeq<Sample_T, DEF_MAX>::operator=(
  const ZeroCopyDataSeq& frm)
{
  if (this != &frm) {
    ZeroCopyDataSeq<Sample_T, DEF_MAX> temp(frm);
    swap(temp);
  }

  return *this;
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::swap(ZeroCopyDataSeq& frm)
{
  bool thisUsedDefAlloc = ptrs_.allocator_ == &default_allocator_;
  bool thisUsedLocalBuffer = ptrs_.array_ == default_allocator_.pool();
  bool frmUsedDefAlloc = frm.ptrs_.allocator_ == &frm.default_allocator_;
  bool frmUsedLocalBuffer = frm.ptrs_.array_ == frm.default_allocator_.pool();

  std::swap(loaner_, frm.loaner_);
  std::swap(default_allocator_, frm.default_allocator_);
  ptrs_.swap(frm.ptrs_);
  std::swap(sc_maximum_, frm.sc_maximum_);
  std::swap(sc_length_, frm.sc_length_);
  std::swap(sc_buffer_, frm.sc_buffer_);
  std::swap(sc_release_, frm.sc_release_);

  if (thisUsedDefAlloc) frm.ptrs_.allocator_ = &frm.default_allocator_;

  if (thisUsedLocalBuffer) frm.ptrs_.array_ = frm.default_allocator_.pool();

  if (frmUsedDefAlloc) ptrs_.allocator_ = &default_allocator_;

  if (frmUsedLocalBuffer) ptrs_.array_ = default_allocator_.pool();
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
ZeroCopyDataSeq<Sample_T, DEF_MAX>::~ZeroCopyDataSeq()
{
  if (loaner_) loaner_->auto_return_loan(this);

  if (sc_release_ && sc_buffer_) freebuf(sc_buffer_);
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
bool
ZeroCopyDataSeq<Sample_T, DEF_MAX>::is_zero_copy() const
{
  return sc_maximum_ == 0;
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
CORBA::ULong
ZeroCopyDataSeq<Sample_T, DEF_MAX>::maximum() const
{
  return sc_maximum_;
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
CORBA::ULong
ZeroCopyDataSeq<Sample_T, DEF_MAX>::max_slots() const
{
  return is_zero_copy() ? static_cast<CORBA::ULong>(ptrs_.max_size())
    : sc_maximum_;
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
CORBA::ULong
ZeroCopyDataSeq<Sample_T, DEF_MAX>::length() const
{
  return is_zero_copy() ? static_cast<CORBA::ULong>(ptrs_.size()) : sc_length_;
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
const Sample_T&
ZeroCopyDataSeq<Sample_T, DEF_MAX>::operator[](CORBA::ULong i) const
{
  if (is_zero_copy()) {
    if (ptrs_[i]->registered_data_) {
      return *static_cast<const Sample_T*>(ptrs_[i]->registered_data_);
    }
    return default_;

  } else {
    return sc_buffer_[i];
  }
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
Sample_T&
ZeroCopyDataSeq<Sample_T, DEF_MAX>::operator[](CORBA::ULong i)
{
  if (is_zero_copy()) {
    if (ptrs_[i]->registered_data_) {
      return *static_cast<Sample_T*>(ptrs_[i]->registered_data_);
    }
    return default_;

  } else {
    return sc_buffer_[i];
  }
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
CORBA::Boolean
ZeroCopyDataSeq<Sample_T, DEF_MAX>::release() const
{
  return sc_release_; //will always be false in zero-copy mode
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::replace(
  CORBA::ULong maximum,
  CORBA::ULong length,
  Sample_T* buffer,
  CORBA::Boolean release /* = false */)
{
  ZeroCopyDataSeq<Sample_T, DEF_MAX> newOne(maximum, length, buffer, release);
  swap(newOne);
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::make_single_copy(CORBA::ULong maximum)
{
  CORBA::ULong currentSize(static_cast<CORBA::ULong>(ptrs_.size()));
  ZeroCopyDataSeq<Sample_T, DEF_MAX> sc((std::max)(maximum, currentSize));
  sc.length(currentSize);

  for (CORBA::ULong i(0); i < ptrs_.size(); ++i) {
    sc[i] = (*this)[i];
  }

  swap(sc);
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
const Sample_T*
ZeroCopyDataSeq<Sample_T, DEF_MAX>::get_buffer() const
{
  //If we're currently zero-copy we must become single copy in order to return
  //a contiguous buffer.  The only way to do this and meet the CORBA/C++ spec
  //interface is to cast-away the constness.
  if (is_zero_copy())
    const_cast<ZeroCopyDataSeq*>(this)->make_single_copy(max_slots());

  if (!sc_buffer_) {
    sc_buffer_ = allocbuf(sc_maximum_);
    sc_release_ = true;
  }

  return sc_buffer_;
}

/*static*/
template <class Sample_T, size_t DEF_MAX> ACE_INLINE
Sample_T*
ZeroCopyDataSeq<Sample_T, DEF_MAX>::allocbuf(CORBA::ULong nelems)
{
  return new Sample_T[nelems];
}

/*static*/
template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::freebuf(Sample_T* buffer)
{
  delete[] buffer;
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::internal_set_length(CORBA::ULong len)
{
  if (!is_zero_copy() || len < ptrs_.size()) {
    length(len);

  } else if (len > ptrs_.size()) {
    //We need the vector to grow efficiently (not reallocate on each call)...
    ptrs_.resize((std::max)(len, CORBA::ULong(ptrs_.size()) * 2), 0);
    //...but maintain the invariant that the size of ptrs_ is our length
    ptrs_.resize(len, 0);
  }
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::set_loaner(
  OpenDDS::DCPS::Loaner* loaner)
{
  loaner_ = loaner;
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::assign_ptr(
  CORBA::ULong ii,
  OpenDDS::DCPS::ReceivedDataElement* item)
{
  ACE_ASSERT(is_zero_copy());
  item->inc_ref();
  ++item->zero_copy_cnt_;
  ptrs_[ii] = item;
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
OpenDDS::DCPS::ReceivedDataElement*
ZeroCopyDataSeq<Sample_T, DEF_MAX>::get_ptr(CORBA::ULong ii) const
{
  ACE_ASSERT(is_zero_copy());
  return ptrs_[ii];
}

template <class Sample_T, size_t DEF_MAX> ACE_INLINE
void
ZeroCopyDataSeq<Sample_T, DEF_MAX>::assign_sample(
  CORBA::ULong ii, const Sample_T& sample)
{
  ACE_ASSERT(!is_zero_copy());
  sc_buffer_[ii] = sample;
}

} // namespace DCPS
} // namespace TAO

TAO_END_VERSIONED_NAMESPACE_DECL
