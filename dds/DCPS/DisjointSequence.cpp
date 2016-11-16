/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "DisjointSequence.h"

#include "ace/Log_Msg.h"

#include <stdexcept>
#include <algorithm>
#include <iterator>

#ifndef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool
DisjointSequence::insert_i(const SequenceRange& range,
                           OPENDDS_VECTOR(SequenceRange)* gaps /* = 0 */)
{
  validate(range);

  RangeSet::iterator range_above = sequences_.lower_bound(range);
  if (range_above != sequences_.end()
      && range_above->first <= range.first) {
    return false; // already have this range, nothing to insert
  }

  SequenceRange newRange = range;
  if (range_above != sequences_.end()
      && ++SequenceNumber(newRange.second) >= range_above->first) {
    // newRange overlaps range_above, replace range_above with modified newRange
    newRange.second = range_above->second;
    // move to just past this iterator for the erase
    ++range_above;
  }

  const SequenceNumber::Value previous = range.first.getValue() - 1;
  // find the lower_bound for the SequenceNumber just before this range
  // to see if any ranges need to combine
  const RangeSet::iterator range_below =
    sequences_.lower_bound(SequenceRange(1 /*ignored*/,
                                         (previous > 0) ? previous
                                         : SequenceNumber::ZERO()));
  if (range_below != sequences_.end()) {
    // if low end falls inside of the range_below range
    // then combine
    if (newRange.first > range_below->first) {
      newRange.first = range_below->first;
    }

    if (gaps) {
      RangeSet::iterator gap_iter = range_below;
      if (range.first < gap_iter->second) {
        gaps->push_back(SequenceRange(range.first,
                                      gap_iter->second.previous()));
      }
      SequenceNumber last_gap = gap_iter++->second;
      for (; gap_iter != range_above; ++gap_iter) {
        const SequenceNumber in_range =
          std::min(gap_iter->first.previous().getValue(),
                   range.second.getValue());
        gaps->push_back(SequenceRange(++last_gap, in_range));
        last_gap = gap_iter->second;
      }
      if (last_gap < range.second) {
        gaps->push_back(SequenceRange(++last_gap, range.second));
      }
    }

    sequences_.erase(range_below, range_above);
  }

  sequences_.insert(newRange);
  return true;
}

bool
DisjointSequence::insert(SequenceNumber value, CORBA::ULong num_bits,
                         const CORBA::Long bits[])
{
  bool inserted = false;
  RangeSet::iterator iter = sequences_.end();
  SequenceNumber::Value range_start = 0;
  const SequenceNumber::Value val = value.getValue();

  // See RTPS v2.1 section 9.4.2.6 SequenceNumberSet
  for (CORBA::ULong i = 0, x = 0, bit = 0; i < num_bits; ++i, ++bit) {

    if (bit == 32) bit = 0;

    if (bit == 0) {
      x = static_cast<CORBA::ULong>(bits[i / 32]);
      if (x == 0) {
        // skip an entire Long if it's all 0's (adds 32 due to ++i)
        i += 31;
        bit = 31;
        //FUTURE: this could be generalized with something like the x86 "bsr"
        //        instruction using compiler intrinsics, VC++ _BitScanReverse()
        //        and GCC __builtin_clz()
        continue;
      }
    }

    if (x & (1 << (31 - bit))) {
      if (range_start == 0) {
        range_start = val + i;
      }

    } else if (range_start != 0) {
      // this is a "0" bit and we've previously seen a "1": insert a range
      const SequenceNumber::Value to_insert = val + i - 1;
      if (insert_bitmap_range(iter, SequenceRange(range_start, to_insert))) {
        inserted = true;
      }
      range_start = 0;

      if (iter != sequences_.end() && iter->second.getValue() != to_insert) {
        // skip ahead: next gap in sequence must be past iter->second
        CORBA::ULong next_i = CORBA::ULong(iter->second.getValue() - val);
        bit = next_i % 32;
        if (next_i / 32 != i / 32) {
          x = static_cast<CORBA::ULong>(bits[next_i / 32]);
        }
        i = next_i;
      }
    }
  }

  if (range_start != 0) {
    // iteration finished before we saw a "0" (inside a range)
    SequenceNumber range_end = (value + num_bits).previous();
    if (insert_bitmap_range(iter, SequenceRange(range_start, range_end))) {
      return true;
    }
  }
  return inserted;
}

bool
DisjointSequence::insert_bitmap_range(RangeSet::iterator& iter,
                                      const SequenceRange& range)
{
  // This is similar to insert_i(), except it doesn't need an O(log(n)) search
  // of sequences_ every time to find the starting point, and it doesn't
  // compute the 'gaps'.

  const SequenceNumber::Value previous = range.first.getValue() - 1,
    next = range.second.getValue() + 1;

  if (!sequences_.empty()) {
    if (iter == sequences_.end()) {
      iter = sequences_.lower_bound(SequenceRange(1 /*ignored*/, previous));
    } else {
      // start where we left off last time and get the lower_bound(previous)
      for (; iter != sequences_.end() && iter->second < previous; ++iter) ;
    }
  }

  if (iter == sequences_.end() || iter->first > next) {
    // can't combine on either side, insert a new range
    iter = sequences_.insert(iter, range);
    return true;
  }

  if (iter->first <= range.first && iter->second >= range.second) {
    // range is already covered by this DisjointSet
    return false;
  }

  // find the right-most (highest) range we can use
  RangeSet::iterator right = iter;
  for (; right != sequences_.end() && right->second < next; ++right) ;

  SequenceNumber high = range.second;
  if (right != sequences_.end()
      && right->first <= next && right->first > range.first) {
    high = right->second;
    ++right;
  }

  const SequenceNumber low = std::min(iter->first, range.first);
  sequences_.erase(iter, right);

  iter = sequences_.insert(SequenceRange(low, high)).first;
  return true;
}

bool
DisjointSequence::to_bitmap(CORBA::Long bitmap[], CORBA::ULong length,
                            CORBA::ULong& num_bits, bool invert) const
{
  // num_bits will be 1 more than the index of the last bit we wrote
  num_bits = 0;
  if (!disjoint()) {
    return true;
  }

  const SequenceNumber base = ++SequenceNumber(cumulative_ack());

  for (RangeSet::const_iterator iter = sequences_.begin(), prev = iter++;
       iter != sequences_.end(); ++iter, ++prev) {

    CORBA::ULong low = 0, high = 0;

    if (invert) {
      low = CORBA::ULong(prev->second.getValue() + 1 - base.getValue());
      high = CORBA::ULong(iter->first.getValue() - 1 - base.getValue());

    } else {
      low = CORBA::ULong(iter->first.getValue() - base.getValue());
      high = CORBA::ULong(iter->second.getValue() - base.getValue());
    }

    if (!fill_bitmap_range(low, high, bitmap, length, num_bits)) {
      return false;
    }
  }

  return true;
}

bool
DisjointSequence::fill_bitmap_range(CORBA::ULong low, CORBA::ULong high,
                                    CORBA::Long bitmap[], CORBA::ULong length,
                                    CORBA::ULong& num_bits)
{
  bool clamped = false;
  CORBA::ULong idx_low = low / 32, bit_low = low % 32,
               idx_high = high / 32, bit_high = high % 32;

  if (idx_low >= length) {
    return false;
  }
  if (idx_high >= length) {
    // clamp to largest number we can represent
    high = length * 32 - 1;
    idx_high = length - 1;
    bit_high = high % 32;
    clamped = true;
  }

  // clear any full Longs between the last bit we wrote and idx_low
  for (CORBA::ULong i = (num_bits + 31) / 32; i < idx_low; ++i) {
    bitmap[i] = 0;
  }

  // write the Long at idx_low, preserving bits that may already be there
  CORBA::ULong x = bitmap[idx_low]; // use unsigned for bitwise operators
  //    clear the bits in x in the range [bit_last, bit_low)
  if (num_bits > 0) {
    const size_t bit_last = ((num_bits - 1) / 32 == idx_low)
                            ? ((num_bits - 1) % 32 + 1) : 0;
    for (size_t m = bit_last; m < bit_low; ++m) {
      x &= ~(1 << (31 - m));
    }
  } else {
    x = 0;
  }
  //    set the bits in x in the range [bit_low, limit)
  const size_t limit = (idx_high == idx_low) ? bit_high : 31;
  for (size_t b = bit_low; b <= limit; ++b) {
    x |= (1 << (31 - b));
  }
  bitmap[idx_low] = x;

  // any full Longs inside the current range are set to all 1's
  for (CORBA::ULong elt = idx_low + 1; elt < idx_high; ++elt) {
    bitmap[elt] = 0xFFFFFFFF;
  }

  if (idx_high > idx_low) {
    // write the Long at idx_high, no need to preserve bits since this is
    // the first iteration that's writing it
    x = 0;
    for (size_t b = 0; b <= bit_high; ++b) {
      x |= (1 << (31 - b));
    }
    bitmap[idx_high] = x;
  }

  num_bits = high + 1;
  return !clamped;
}

OPENDDS_VECTOR(SequenceRange)
DisjointSequence::missing_sequence_ranges() const
{
  OPENDDS_VECTOR(SequenceRange) missing;
  if (!disjoint()) {
    return missing;
  }

  RangeSet::const_iterator secondIter = sequences_.begin();
  for (RangeSet::const_iterator firstIter = secondIter++;
       secondIter != sequences_.end(); ++firstIter, ++secondIter) {

    const SequenceNumber missingLow = ++SequenceNumber(firstIter->second),
                         missingHigh = secondIter->first.previous();

    if (missingLow <= missingHigh) {
      missing.push_back(SequenceRange(missingLow, missingHigh));
    }
  }

  return missing;
}

OPENDDS_VECTOR(SequenceRange)
DisjointSequence::present_sequence_ranges() const
{
  OPENDDS_VECTOR(SequenceRange) present;
  std::copy(sequences_.begin(), sequences_.end(), std::back_inserter(present));
  return present;
}

bool
DisjointSequence::contains(SequenceNumber value) const
{
  RangeSet::const_iterator iter =
    sequences_.lower_bound(SequenceRange(0 /*ignored*/, value));
  return iter != sequences_.end() && iter->first <= value;
}

void
DisjointSequence::validate(const SequenceRange& range)
{
  if (range.first > range.second) {
    throw std::runtime_error("SequenceNumber range invalid, range must "
                             "be ascending.");
  }
}

void
DisjointSequence::dump() const
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DisjointSequence[%X]::dump included ranges of "
                       "SequenceNumbers:\n", this));
  for (RangeSet::const_iterator iter = sequences_.begin();
       iter != sequences_.end(); ++iter) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DisjointSequence[%X]::dump\t%q-%q\n",
               this, iter->first.getValue(), iter->second.getValue()));
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
