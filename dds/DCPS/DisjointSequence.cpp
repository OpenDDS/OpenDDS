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
  OPENDDS_ASSERT(range.first <= range.second);

  typedef RangeSet::Container::iterator iter_t;

  iter_t range_above = sequences_.ranges_.lower_bound(range);
  if (range_above != sequences_.ranges_.end()
      && range_above->first <= range.first) {
    return false; // already have this range, nothing to insert
  }

  SequenceRange newRange = range;
  if (range_above != sequences_.ranges_.end()
      && ++SequenceNumber(newRange.second) >= range_above->first) {
    // newRange overlaps range_above, replace range_above with modified newRange
    newRange.second = range_above->second;
    // move to just past this iterator for the erase
    ++range_above;
  }

  const SequenceNumber::Value previous = range.first.getValue() - 1;
  // find the lower_bound for the SequenceNumber just before this range
  // to see if any ranges need to combine
  const iter_t range_below =
    sequences_.ranges_.lower_bound(SequenceRange(1 /*ignored*/,
                                                 (previous > 0) ? previous
                                                 : SequenceNumber::ZERO()));
  if (range_below != sequences_.ranges_.end()) {
    // if low end falls inside of the range_below range
    // then combine
    if (newRange.first > range_below->first) {
      newRange.first = range_below->first;
    }

    if (gaps) {
      iter_t gap_iter = range_below;
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

    sequences_.ranges_.erase(range_below, range_above);
  }

  sequences_.ranges_.insert(newRange);
  return true;
}

bool
DisjointSequence::insert(SequenceNumber value, ACE_CDR::ULong num_bits,
                         const ACE_CDR::Long bits[])
{
  bool inserted = false;
  RangeSet::Container::iterator iter = sequences_.ranges_.end();
  bool range_start_is_valid = false;
  SequenceNumber::Value range_start = 0;
  const SequenceNumber::Value val = value.getValue();

  // See RTPS v2.1 section 9.4.2.6 SequenceNumberSet
  for (ACE_CDR::ULong i = 0, x = 0, bit = 0; i < num_bits; ++i, ++bit) {

    if (bit == 32) bit = 0;

    if (bit == 0) {
      x = static_cast<ACE_CDR::ULong>(bits[i / 32]);
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
      if (!range_start_is_valid) {
        range_start = val + i;
        range_start_is_valid = true;
      }
    } else if (range_start_is_valid) {
      // this is a "0" bit and we've previously seen a "1": insert a range
      const SequenceNumber::Value to_insert = val + i - 1;
      if (insert_bitmap_range(iter, SequenceRange(range_start, to_insert))) {
        inserted = true;
      }
      range_start = 0;
      range_start_is_valid = false;

      if (iter != sequences_.ranges_.end() && iter->second.getValue() != to_insert) {
        // skip ahead: next gap in sequence must be past iter->second
        ACE_CDR::ULong next_i = ACE_CDR::ULong(iter->second.getValue() - val);
        bit = next_i % 32;
        if (next_i / 32 != i / 32 && next_i < num_bits) {
          x = static_cast<ACE_CDR::ULong>(bits[next_i / 32]);
        }
        i = next_i;
      }
    }
  }

  if (range_start_is_valid) {
    // iteration finished before we saw a "0" (inside a range)
    SequenceNumber range_end = (value + num_bits).previous();
    if (insert_bitmap_range(iter, SequenceRange(range_start, range_end))) {
      return true;
    }
  }
  return inserted;
}

bool
DisjointSequence::insert_bitmap_range(RangeSet::Container::iterator& iter,
                                      const SequenceRange& range)
{
  // This is similar to insert_i(), except it doesn't need an O(log(n)) search
  // of sequences_ every time to find the starting point, and it doesn't
  // compute the 'gaps'.

  const SequenceNumber::Value previous = range.first.getValue() - 1,
    next = range.second.getValue() + 1;

  if (!sequences_.empty()) {
    if (iter == sequences_.ranges_.end()) {
      iter = sequences_.ranges_.lower_bound(SequenceRange(0 /*ignored*/, previous));
    } else {
      // start where we left off last time and get the lower_bound(previous)
      for (; iter != sequences_.ranges_.end() && iter->second < previous; ++iter) ;
    }
  }

  if (iter == sequences_.ranges_.end() || iter->first > next) {
    // can't combine on either side, insert a new range
    iter = sequences_.ranges_.insert(iter, range);
    return true;
  }

  if (iter->first <= range.first && iter->second >= range.second) {
    // range is already covered by this DisjointSet
    return false;
  }

  // find the right-most (highest) range we can use
  RangeSet::Container::iterator right = iter;
  for (; right != sequences_.ranges_.end() && right->second < next; ++right) ;

  SequenceNumber high = range.second;
  if (right != sequences_.ranges_.end()
      && right->first <= next && right->first > range.first) {
    high = right->second;
    ++right;
  }

  const SequenceNumber low = std::min(iter->first, range.first);
  sequences_.ranges_.erase(iter, right);

  iter = sequences_.ranges_.insert(SequenceRange(low, high)).first;
  return true;
}

bool
DisjointSequence::to_bitmap(ACE_CDR::Long bitmap[], ACE_CDR::ULong length,
                            ACE_CDR::ULong& num_bits, ACE_CDR::ULong& cumulative_bits_added, bool invert) const
{
  // num_bits will be 1 more than the index of the last bit we wrote
  num_bits = 0;
  if (!disjoint()) {
    return true;
  }

  const SequenceNumber base = ++SequenceNumber(cumulative_ack());

  for (RangeSet::const_iterator iter = sequences_.begin(), prev = iter++;
       iter != sequences_.end(); ++iter, ++prev) {

    ACE_CDR::ULong low = 0, high = 0;

    if (invert) {
      low = ACE_CDR::ULong(prev->second.getValue() + 1 - base.getValue());
      high = ACE_CDR::ULong(iter->first.getValue() - 1 - base.getValue());

    } else {
      low = ACE_CDR::ULong(iter->first.getValue() - base.getValue());
      high = ACE_CDR::ULong(iter->second.getValue() - base.getValue());
    }

    if (!fill_bitmap_range(low, high, bitmap, length, num_bits, cumulative_bits_added)) {
      return false;
    }
  }

  return true;
}

bool
DisjointSequence::fill_bitmap_range(ACE_CDR::ULong low, ACE_CDR::ULong high,
                                    ACE_CDR::Long bitmap[], ACE_CDR::ULong length,
                                    ACE_CDR::ULong& num_bits, ACE_CDR::ULong& cumulative_bits_added)
{
  bool clamped = false;
  if ((low / 32) >= length) {
    return false;
  }
  if ((high / 32) >= length) {
    high = length * 32 - 1;
    clamped = true;
  }

  const ACE_CDR::ULong idx_nb = num_bits / 32, bit_nb = num_bits % 32,
                     idx_low = low / 32, bit_low = low % 32,
                     idx_high = high / 32, bit_high = high % 32;

  // handle idx_nb zeros
  if (bit_nb) {
    bitmap[idx_nb] &= ~((0x1u << (32 - bit_nb)) - 1);
  } else {
    bitmap[idx_nb] = 0;
  }

  // handle zeros between idx_nb and idx_low (if gap exists)
  for (ACE_CDR::ULong i = idx_nb + 1; i < idx_low; ++i) {
    bitmap[i] = 0;
  }

  // handle idx_nb ones
  if (bit_low) {
    if (idx_low > idx_nb) {
      bitmap[idx_low] = (0x1u << (32 - bit_low)) - 1;
    } else {
      bitmap[idx_low] |= (0x1u << (32 - bit_low)) - 1;
    }
  } else {
    bitmap[idx_low] = 0xFFFFFFFF;
  }

  // handle ones between idx_low and idx_high (if gap exists)
  for (ACE_CDR::ULong i = idx_low + 1; i < idx_high; ++i) {
    bitmap[i] = 0xFFFFFFFF;
  }

  // handle idx_high
  if (idx_high > idx_low) {
    bitmap[idx_high] = ~((0x1u << (31 - bit_high)) - 1);
  } else if (bit_high < 31) {
    bitmap[idx_high] &= ~((0x1u << (31 - bit_high)) - 1);
  }

  num_bits = high + 1;
  cumulative_bits_added += high - low + 1;
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

ACE_CDR::ULong
DisjointSequence::bitmap_num_longs(const SequenceNumber& low, const SequenceNumber& high)
{
  return high < low ? 0u : std::min(8u, unsigned((high.getValue() - low.getValue() + 32) / 32));
}

void
DisjointSequence::erase(const SequenceNumber value)
{
  RangeSet::Container::iterator iter =
    sequences_.ranges_.lower_bound(SequenceRange(0 /*ignored*/, value));
  if (iter != sequences_.ranges_.end()) {
    if (iter->first == value &&
        iter->second == value) {
      sequences_.ranges_.erase(iter);
    } else if (iter->first == value) {
      SequenceRange x(value + 1, iter->second);
      sequences_.ranges_.erase(iter);
      sequences_.ranges_.insert(x);
    } else if (iter->second == value) {
      SequenceRange x(iter->first, value.previous());
      sequences_.ranges_.erase(iter);
      sequences_.ranges_.insert(x);
    } else {
      SequenceRange x(iter->first, value.previous());
      SequenceRange y(value + 1, iter->second);
      sequences_.ranges_.erase(iter);
      sequences_.ranges_.insert(x);
      sequences_.ranges_.insert(y);
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
