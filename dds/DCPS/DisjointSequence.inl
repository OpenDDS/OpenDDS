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
DisjointSequence::low()
{
  return this->low_;
}

ACE_INLINE SequenceNumber
DisjointSequence::high()
{
  return this->high_;
}

ACE_INLINE bool
DisjointSequence::disjoint()
{
  return this->low_ != this->high_;
}

ACE_INLINE
DisjointSequence::operator SequenceNumber()
{
  // Always return low water mark; this value
  // represents the maximum contiguous value.
  return this->low_;
}

} // namespace DCPS
} // namespace OpenDDS
