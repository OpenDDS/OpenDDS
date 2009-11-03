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
  return high() - low() + 1;
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
