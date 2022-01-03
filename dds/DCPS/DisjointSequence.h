/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DISJOINTSEQUENCE_H
#define OPENDDS_DCPS_DISJOINTSEQUENCE_H

#include "dcps_export.h"
#include "Definitions.h"
#include "SequenceNumber.h"
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

  bool contains_any(const SequenceRange& range) const;

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

  void erase(SequenceNumber value);

  /// Insert using the RTPS compact representation of a set.  The three
  /// parameters, taken together, describe a set with each 1 bit starting
  /// at the msb of bits[0] and extending through num_bits (which are located at
  /// the next least significant bits of bits[0] followed by the msb of bits[1],
  /// etc.) indicating presence of the number (value + bit_index) in the set.
  /// bit_index is 0-based.
  /// Precondition: the array 'bits' has at least ceil(num_bits / 32) entries.
  bool insert(SequenceNumber value,
              ACE_CDR::ULong num_bits,
              const ACE_CDR::Long bits[]);

  /// Insert the intersection of range and filter
  bool insert_filtered(const SequenceRange& range, const DisjointSequence& filter);

  /// Inverse of insert(value, num_bits, bits).  Populates array of
  /// bitmap[length] with the bitmap of ranges above the cumulative_ack() value.
  /// Sets the number of significant (used) bits in num_bits.  The 'base' of the
  /// bitmap is one larger than cumulative_ack().  Returns true if the entire
  /// DisjointSequence was able to fit in bitmap[].  Returning false is not an
  /// error, it's just that the higher-valued ranges didn't fit.  If invert is
  /// true, the 1's in the bitmap represent the missing_sequence_ranges()
  /// instead of the present_sequence_ranges().
  /// Precondition: the array 'bits' has 'length' entries allocated.
  bool to_bitmap(ACE_CDR::Long bitmap[],
                 ACE_CDR::ULong length,
                 ACE_CDR::ULong& num_bits,
                 ACE_CDR::ULong& cumulative_bits_added,
                 bool invert = false) const;

  /// Returns missing ranges of SequenceNumbers (internal gaps in the sequence)
  OPENDDS_VECTOR(SequenceRange) missing_sequence_ranges() const;

  /// Returns a representation of the members of the sequence as a list of
  /// contiguous ranges (each Range is inclusive on both sides).
  OPENDDS_VECTOR(SequenceRange) present_sequence_ranges() const;

  void dump() const;

  /// Core data structure of DisjointSequence:
  /// Use a balanced binary tree (std::set) to store a list of ranges (std::pair of T).
  /// Maintain invariants (in addition to those from std::set):
  /// - For any element x of the set, x.second >= x.first
  /// - No adjacent or overlapping ranges.  Given two elements ordered "x before y", y.first > x.second + 1
  /// Common non-mutating operations on the underlying set are public members of this class.
  /// Note that due to this design, size() is the number of contiguous ranges, not individual values.
  /// Some mutating operations on the underlying set that can't violate the invariants are also provided (like clear).
  /// Type T needs to support value-initialization, construction from int, copying,
  /// addition, subtraction, and comparison using == and <.
  template <typename T>
  class OrderedRanges {
  public:
    typedef std::pair<T, T> TPair;
    typedef bool (*Compare)(const TPair&, const TPair&);
    typedef OPENDDS_SET_CMP(TPair, Compare) Container;
    typedef typename Container::size_type size_type;
    typedef typename Container::const_iterator const_iterator;
    typedef const_iterator iterator;
    typedef typename Container::const_reverse_iterator const_reverse_iterator;
    typedef const_reverse_iterator reverse_iterator;

    static bool range_less(const TPair& lhs, const TPair& rhs)
    {
      return lhs.second < rhs.second;
    }

    OrderedRanges()
      : ranges_(range_less)
    {}

    const_iterator begin() const { return ranges_.begin(); }
    const_iterator cbegin() const { return ranges_.begin(); }

    const_iterator end() const { return ranges_.end(); }
    const_iterator cend() const { return ranges_.end(); }

    const_reverse_iterator rbegin() const { return ranges_.rbegin(); }
    const_reverse_iterator crbegin() const { return ranges_.rbegin(); }

    const_reverse_iterator rend() const { return ranges_.rend(); }
    const_reverse_iterator crend() const { return ranges_.rend(); }

    bool empty() const { return ranges_.empty(); }
    size_type size() const { return ranges_.size(); }
    void clear() { ranges_.clear(); }

    void add(T lower, T upper)
    {
      OPENDDS_ASSERT(upper >= lower);
      if (has(lower, upper)) {
        return;
      }

      const T above = upper + 1;
      for (typename Container::iterator i = lower_bound_i(lower - 1);
          i != ranges_.end() && (i->first <= above || i->second <= above);
          /* iterate in loop because of removal */) {
        lower = (std::min)(lower, i->first);
        upper = (std::max)(upper, i->second);
        ranges_.erase(i++);
      }

      ranges_.insert(TPair(lower, upper));
    }

    void add(T value)
    {
      add(value, value);
    }

    void remove(T value)
    {
      const typename Container::iterator iter = lower_bound_i(value);
      if (iter == end() || value < iter->first) {
        return;
      }
      remove_i(iter, value);
    }

    T pop_front()
    {
      const T value = begin()->first;
      remove_i(ranges_.begin(), value);
      return value;
    }

    bool has(T lower, T upper) const
    {
      const const_iterator iter = lower_bound(upper);
      return iter != end() && !(lower < iter->first);
    }

    bool has(const TPair& range) const
    {
      return has(range.first, range.second);
    }

    bool has(T value) const
    {
      return has(value, value);
    }

    bool has_any(T lower, T upper) const
    {
      const const_iterator iter = lower_bound(lower);
      return iter != end() && !(upper < iter->first);
    }

    bool has_any(const TPair& range) const
    {
      return has_any(range.first, range.second);
    }

    template <typename Type> friend
    bool operator==(const OrderedRanges<Type>& a, const OrderedRanges<Type>& b);

  private:
    const_iterator lower_bound(const TPair& p) const { return ranges_.lower_bound(p); }

    const_iterator lower_bound(T t) const
    {
      return ranges_.lower_bound(TPair(T() /*ignored*/, t));
    }

    // explicitly get a non-const iterator for use with methods like erase()
    typename Container::iterator lower_bound_i(T t)
    {
      return ranges_.lower_bound(TPair(T() /*ignored*/, t));
    }

    const_iterator upper_bound(const TPair& p) const { return ranges_.upper_bound(p); }

    const_iterator upper_bound(T t) const
    {
      return ranges_.upper_bound(TPair(T() /*ignored*/, t));
    }

    // 'iter' must be a valid iterator to a range that contains 'value'
    void remove_i(typename Container::iterator iter, T value)
    {
      const TPair orig = *iter;
      ranges_.erase(iter);
      if (value == orig.first) {
        if (value < orig.second) {
          ranges_.insert(TPair(value + T(1), orig.second));
        }
      } else if (value == orig.second) {
        ranges_.insert(TPair(orig.first, value - T(1)));
      } else {
        ranges_.insert(TPair(orig.first, value - T(1)));
        ranges_.insert(TPair(value + T(1), orig.second));
      }
    }

    friend class DisjointSequence;
    Container ranges_;
  };

private:
  typedef OrderedRanges<SequenceNumber> RangeSet;
  RangeSet sequences_;

  // helper methods:

  bool insert_i(const SequenceRange& range,
                OPENDDS_VECTOR(SequenceRange)* gaps = 0);

  bool insert_bitmap_range(RangeSet::Container::iterator& iter, const SequenceRange& sr);

public:
  /// Set the bits in range [low, high] in the bitmap, updating num_bits.
  static bool fill_bitmap_range(ACE_CDR::ULong low, ACE_CDR::ULong high,
                                ACE_CDR::Long bitmap[], ACE_CDR::ULong length,
                                ACE_CDR::ULong& num_bits, ACE_CDR::ULong& cumulative_bits_added);

  /// Return the number of CORBA::Longs required for the bitmap representation of
  /// sequence numbers between low and high, inclusive (maximum 8 longs).
  static ACE_CDR::ULong bitmap_num_longs(const SequenceNumber& low, const SequenceNumber& high);
};

template <typename T>
bool operator==(
  const DisjointSequence::OrderedRanges<T>& a, const DisjointSequence::OrderedRanges<T>& b)
{
  return a.ranges_ == b.ranges_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#endif  /* DCPS_DISJOINTSEQUENCE_H */
