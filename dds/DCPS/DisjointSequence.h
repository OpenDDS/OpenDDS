/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_DISJOINTSEQUENCE_H
#define DCPS_DISJOINTSEQUENCE_H

#include "dcps_export.h"
#include "Definitions.h"

#include "PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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

  /// Gets the low end of the highest contiguous range, may be thought of as
  /// the inverse of cumulative_ack().
  /// If empty(), returns SEQUENCENUMBER_UNKNOWN.
  SequenceNumber last_ack() const;

  /// Objects with the disjoint() property have an internal gap in the inserted
  /// SequenceNumbers.
  bool disjoint() const;

  bool contains(SequenceNumber value) const;

  /// All insert() methods return true upon modifying the set and false if
  /// the set already contained the SequenceNumber(s) that were to be inserted.
  /// This is the general form of insert() whereby the caller receives a list of
  /// sub-ranges of 'range' that were not already in the DisjointSequence.
  /// For example, given a DisjointSequence 'seq' containing (1, 2, 5, 9),
  /// calling seq.insert(SequenceRange(4, 12), v) returns true
  /// and yields v = [(4, 4), (6, 8), (10, 12)] and seq = (1, 2, 4, ..., 12).
  bool insert(const SequenceRange& range, OPENDDS_VECTOR(SequenceRange)& added);

  /// Insert all numbers between range.first and range.second (both inclusive).
  bool insert(const SequenceRange& range);

  /// Shorthand for "insert(SequenceRange(value, value))"
  bool insert(SequenceNumber value);

  /// Insert using the RTPS compact representation of a set.  The three
  /// parameters, taken together, describe a set with each 1 bit starting
  /// at the msb of bits[0] and extending through num_bits (which are located at
  /// the next least significant bits of bits[0] followed by the msb of bits[1],
  /// etc.) indicating presence of the number (value + bit_index) in the set.
  /// bit_index is 0-based.
  /// Precondition: the array 'bits' has at least ceil(num_bits / 32) entries.
  bool insert(SequenceNumber value,
              CORBA::ULong num_bits,
              const CORBA::Long bits[]);

  /// Inverse of insert(value, num_bits, bits).  Populates array of
  /// bitmap[length] with the bitmap of ranges above the cumulative_ack() value.
  /// Sets the number of significant (used) bits in num_bits.  The 'base' of the
  /// bitmap is one larger than cumulative_ack().  Returns true if the entire
  /// DisjointSequence was able to fit in bitmap[].  Returning false is not an
  /// error, it's just that the higher-valued ranges didn't fit.  If invert is
  /// true, the 1's in the bitmap represent the missing_sequence_ranges()
  /// instead of the present_sequence_ranges().
  /// Precondition: the array 'bits' has 'length' entries allocated.
  bool to_bitmap(CORBA::Long bitmap[],
                 CORBA::ULong length,
                 CORBA::ULong& num_bits,
                 bool invert = false) const;

  /// Returns missing ranges of SequenceNumbers (internal gaps in the sequence)
  OPENDDS_VECTOR(SequenceRange) missing_sequence_ranges() const;

  /// Returns a representation of the members of the sequence as a list of
  /// contiguous ranges (each Range is inclusive on both sides).
  OPENDDS_VECTOR(SequenceRange) present_sequence_ranges() const;

  void dump() const;

private:
  static void validate(const SequenceRange& range);

  static bool SequenceRange_LessThan(const SequenceRange& lhs,
                                     const SequenceRange& rhs)
  {
    return lhs.second < rhs.second;
  }

  typedef bool (*SRCompare)(const SequenceRange&, const SequenceRange&);
  typedef OPENDDS_SET_CMP(SequenceRange, SRCompare) RangeSet;
  RangeSet sequences_;


  // helper methods:

  bool insert_i(const SequenceRange& range,
                OPENDDS_VECTOR(SequenceRange)* gaps = 0);

  bool insert_bitmap_range(RangeSet::iterator& iter, const SequenceRange& sr);

public:
  /// Set the bits in range [low, high] in the bitmap, updating num_bits.
  static bool fill_bitmap_range(CORBA::ULong low, CORBA::ULong high,
                                CORBA::Long bitmap[], CORBA::ULong length,
                                CORBA::ULong& num_bits);
};


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#endif  /* DCPS_DISJOINTSEQUENCE_H */
