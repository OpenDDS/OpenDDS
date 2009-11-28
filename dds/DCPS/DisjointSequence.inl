/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
DisjointSequence::range_iterator::range_iterator(set_type::iterator pos)
  : pos_(pos)
{
  // N.B. range_iterators always look ahead; the iterator must be
  // incremented to properly initialize state. This is safe as we
  // are guaranteed to have at least one element in the set:
  ++*this;
}

ACE_INLINE
DisjointSequence::range_iterator::range_iterator(const range_iterator& it)
{
  this->pos_ = it.pos_;
  this->value_ = it.value_;
}

ACE_INLINE DisjointSequence::range_iterator&
DisjointSequence::range_iterator::operator++()
{
  set_type::iterator prev(this->pos_++);
  this->value_ = range_type(SequenceNumber(prev->value_ + 1),
                            SequenceNumber(this->pos_->value_ - 1));
  return *this;
}

ACE_INLINE DisjointSequence::range_iterator
DisjointSequence::range_iterator::operator++(int)
{
  range_iterator prev(*this);
  ++*this;
  return prev;
}

ACE_INLINE bool
DisjointSequence::range_iterator::operator==(const range_iterator& rhs)
{
  return this->pos_ == rhs.pos_;
}

ACE_INLINE bool
DisjointSequence::range_iterator::operator!=(const range_iterator& rhs)
{
  return !(*this == rhs);
}

ACE_INLINE DisjointSequence::range_type&
DisjointSequence::range_iterator::operator*()
{
  return this->value_;
}

ACE_INLINE DisjointSequence::range_type*
DisjointSequence::range_iterator::operator->()
{
  return &this->value_;
}

//

ACE_INLINE DisjointSequence::range_iterator
DisjointSequence::range_begin()
{
  return range_iterator(this->values_.begin());
}

ACE_INLINE DisjointSequence::range_iterator
DisjointSequence::range_end()
{
  return range_iterator(this->values_.end());
}

ACE_INLINE SequenceNumber
DisjointSequence::low() const
{
  return *(this->values_.begin());
}

ACE_INLINE SequenceNumber
DisjointSequence::high() const
{
  return *(this->values_.rbegin());
}

ACE_INLINE size_t
DisjointSequence::depth() const
{
  return high() - low();
}

ACE_INLINE bool
DisjointSequence::disjoint() const
{
  return this->values_.size() > 1;
}

ACE_INLINE
DisjointSequence::operator SequenceNumber() const
{
  // Always return low water mark; this value/ represents the
  // maximum contiguous value seen.
  return low();
}

} // namespace DCPS
} // namespace OpenDDS
