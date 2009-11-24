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
DisjointSequence::const_iterator::const_iterator(const disjoint_set& values,
                                                 disjoint_set::const_iterator pos)
  : values_(values),
    pos_(pos)
{
}

ACE_INLINE
DisjointSequence::const_iterator::const_iterator(const const_iterator& it)
{
  this->values_ = it.values_;
  this->pos_ = it.pos_;
}

ACE_INLINE DisjointSequence::const_iterator&
DisjointSequence::const_iterator::operator++()
{
  return *this; // TODO
}

ACE_INLINE DisjointSequence::const_iterator&
DisjointSequence::const_iterator::operator++(int)
{
  return *this; // TODO
}

ACE_INLINE bool
DisjointSequence::const_iterator::operator==(const const_iterator& rhs)
{
  return this->values_ == rhs.values_ &&
         this->pos_ == rhs.pos_;
}

ACE_INLINE bool
DisjointSequence::const_iterator::operator!=(const const_iterator& rhs)
{
  return !(*this == rhs);
}

ACE_INLINE DisjointSequence::disjoint_range
DisjointSequence::iterator::operator*()
{
  return disjoint_range();  // TODO
}

//

ACE_INLINE DisjointSequence::iterator
DisjointSequence::begin()
{
  return iterator(this->values_,
                  this->values_.begin());
}

ACE_INLINE DisjointSequence::const_iterator
DisjointSequence::begin() const
{
  return const_iterator(this->values_,
                        this->values_.begin());
}

ACE_INLINE DisjointSequence::iterator
DisjointSequence::end()
{
  return iterator(this->values_,
                  this->values_.end());
}

ACE_INLINE DisjointSequence::const_iterator
DisjointSequence::end() const
{
  return const_iterator(this->values_,
                        this->values_.end());
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
  // Always return low water mark; this value
  // represents the max contiguous value seen.
  return low();
}

} // namespace DCPS
} // namespace OpenDDS
