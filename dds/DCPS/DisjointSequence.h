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

#include <set>
#include <vector>

namespace OpenDDS {
namespace DCPS {

/// Data structure for a set of SequenceNumbers.
/// Sequence numbers can be inserted as single numbers, ranges,
/// or RTPS-style bitmaps.  The DisjointSequence can then be queried for
/// contiguous ranges and internal gaps.
class OpenDDS_Dcps_Export DisjointSequence {
public:

  DisjointSequence();
  void reset();

  bool empty() const;

  /// Lowest SequenceNumber in the set.
  /// Precondition: !empty()
  SequenceNumber low() const;

  /// Highest SequenceNumber in the set.
  /// Precondition: !empty()
  SequenceNumber high() const;

  /// Gets the high end of the lowest contiguous range.
  /// Named after the use case of tracking received messages (which may be
  /// received out-of-order) and then determining the largest value that the
  /// receiver is allowed to acknowledge (under a cumulative-acking protocol).
  /// If empty(), returns SEQUENCENUMBER_UNKNOWN.
  SequenceNumber cumulative_ack() const;

  /// Objects with the disjoint() property have an internal gap in the inserted
  /// SequenceNumbers.
  bool disjoint() const;

  /// All insert() methods return true upon modifying the set and false if
  /// the set already contained the SequenceNumber(s) that were to be inserted.
  /// This is the general form of insert() whereby the caller receives a list of
  /// sub-ranges of 'range' that were not already in the DisjointSequence.
  /// For example, given a DisjointSequence 'seq' containing (1, 2, 5, 9),
  /// calling seq.insert(SequenceRange(4, 12), v) returns true
  /// and yields v = [(4, 4), (6, 8), (10, 12)] and seq = (1, 2, 4, ..., 12).
  bool insert(const SequenceRange& range, std::vector<SequenceRange>& added);

  /// Insert all numbers between range.first and range.second (both inclusive).
  bool insert(const SequenceRange& range);

  /// Shorthand for "insert(SequenceRange(value, value))"
  bool insert(SequenceNumber value);

  /// Insert using the RTPS compact representation of a set.  The three
  /// parameters, taken together, describe a set with each 1 bit starting
  /// at the lsb of bits[0] and extending through num_bits indicating presence
  /// of the number (value + bit_index) in the set.  bit_index is 0-based.
  /// Precondition: the array 'bits' has at least ceil(num_bits / 32) entries.
  bool insert(SequenceNumber value,
              CORBA::ULong num_bits,
              const CORBA::Long bits[]);

  /// Returns missing ranges of SequenceNumbers (internal gaps in the sequence)
  std::vector<SequenceRange> missing_sequence_ranges() const;

  /// Returns a representation of the members of the sequence as a list of
  /// contiguous ranges (each Range is inclusive on both sides).
  std::vector<SequenceRange> present_sequence_ranges() const;

  void dump() const;

private:

  bool insert_i(const SequenceRange& range,
                std::vector<SequenceRange>* gaps = 0);

  static void validate(const SequenceRange& range);

  static bool SequenceRange_LessThan(const SequenceRange& lhs,
                                     const SequenceRange& rhs)
  {
    return lhs.second < rhs.second;
  }

  typedef bool (*SRCompare)(const SequenceRange&, const SequenceRange&);

  typedef std::set<SequenceRange, SRCompare> RangeSet;
  RangeSet sequences_;
};


} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#endif  /* DCPS_DISJOINTSEQUENCE_H */
