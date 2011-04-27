/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_DISJOINTSEQUENCE_H
#define DCPS_DISJOINTSEQUENCE_H

#include "dcps_export.h"
#include "Definitions.h"

#include <cstdlib>
#include <iterator>
#include <set>
#include <vector>

namespace OpenDDS {
namespace DCPS {

struct SequenceRange_LessThan {
  bool operator()(const SequenceRange& lhs, const SequenceRange& rhs) const {
    return lhs.second < rhs.second;
  }
};

class OpenDDS_Dcps_Export DisjointSequence {
public:
  typedef std::set<SequenceRange, SequenceRange_LessThan> RangeSet;

  explicit DisjointSequence(SequenceNumber value = SequenceNumber());

  SequenceNumber low() const;
  SequenceNumber high() const;

  bool disjoint() const;

  void reset(SequenceNumber value = SequenceNumber());

  // indicates the new lowest valid SequenceNumber,
  // if there are SequenceNumbers missing before
  // this value, they will be treated as received
  // and the low water mark will be moved to at least
  // the SequenceNumber prior to value (if value itself
  // has already been received, then low water mark will
  // be moved to just prior to the next missing Sequence
  // Number).  lowest_valid will return true if invalid
  // values have been dropped.
  bool lowest_valid(SequenceNumber value);

  // add the value or range of values to the set of seen
  // values
  bool update(SequenceNumber value);
  bool update(const SequenceRange& range);

  // returns missing ranges of SequenceNumbers
  std::vector<SequenceRange> missing_sequence_ranges() const;

  void dump() const;

private:
  void validate(const SequenceRange& range) const;

  RangeSet sequences_;
};


} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#endif  /* DCPS_DISJOINTSEQUENCE_H */
