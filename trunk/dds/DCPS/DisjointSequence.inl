/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <cassert>

namespace OpenDDS {
namespace DCPS {

ACE_INLINE SequenceNumber
DisjointSequence::low() const
{
  assert(!this->values_.empty());
  return *(this->values_.begin());
}

ACE_INLINE SequenceNumber
DisjointSequence::high() const
{
  assert(!this->values_.empty());
  return *(this->values_.rbegin());
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
