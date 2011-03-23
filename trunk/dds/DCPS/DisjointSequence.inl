/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE SequenceNumber
DisjointSequence::low() const
{
  // only the high end of the range matters for the 
  // low sequence since all numbers less than it are 
  // also in the sequence
  return this->sequences_.begin()->second;
}

ACE_INLINE SequenceNumber
DisjointSequence::high() const
{
  return this->sequences_.rbegin()->second;
}

ACE_INLINE bool
DisjointSequence::disjoint() const
{
  return this->sequences_.size() > 1;
}

} // namespace DCPS
} // namespace OpenDDS
