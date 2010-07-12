/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
DisjointSequence::range_iterator::range_iterator()
{
}

ACE_INLINE
DisjointSequence::range_iterator::range_iterator(SequenceSet::iterator begin,
                                                 SequenceSet::iterator end)
  : pos_(begin),
    end_(end)
{
  // N.B. range_iterators always look ahead; the iterator must be
  // incremented to properly initialize state:
  ++*this;
}

ACE_INLINE DisjointSequence::range_iterator
DisjointSequence::range_iterator::operator++(int)
{
  range_iterator prev(*this);
  ++*this;
  return prev;
}

ACE_INLINE bool
DisjointSequence::range_iterator::operator==(const range_iterator& rhs) const
{
  return this->pos_ == rhs.pos_ &&
         this->end_ == rhs.end_;
}

ACE_INLINE bool
DisjointSequence::range_iterator::operator!=(const range_iterator& rhs) const
{
  return !(*this == rhs);
}

ACE_INLINE SequenceRange&
DisjointSequence::range_iterator::operator*()
{
  return this->value_;
}

ACE_INLINE SequenceRange*
DisjointSequence::range_iterator::operator->()
{
  return &this->value_;
}

//

ACE_INLINE SequenceNumber
DisjointSequence::low() const
{
  return *(this->sequences_.begin());
}

ACE_INLINE SequenceNumber
DisjointSequence::high() const
{
  return *(this->sequences_.rbegin());
}

ACE_INLINE size_t
DisjointSequence::depth() const
{
  ACE_UINT16 u_low(low());
  ACE_UINT16 u_high(high());

  return std::max(u_low, u_high) - std::min(u_low, u_high);
}

ACE_INLINE bool
DisjointSequence::disjoint() const
{
  return this->sequences_.size() > 1;
}

ACE_INLINE bool
DisjointSequence::seen(SequenceNumber value) const
{
  return value <= low();
}

ACE_INLINE DisjointSequence::range_iterator
DisjointSequence::range_begin()
{
  return range_iterator(this->sequences_.begin(),
                        this->sequences_.end());
}

ACE_INLINE DisjointSequence::range_iterator
DisjointSequence::range_end()
{
  return range_iterator(this->sequences_.end(),
                        this->sequences_.end());
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
