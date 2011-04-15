/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "DisjointSequence.h"

#include "ace/Log_Msg.h"
#include <stdexcept>

#ifndef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#include <climits>

namespace OpenDDS {
namespace DCPS {

const size_t DisjointSequence::MAX_DEPTH(SHRT_MAX / 2 - 1);

DisjointSequence::DisjointSequence(SequenceNumber value)
{
  this->sequences_.insert(SequenceRange(value, value));
}

void
DisjointSequence::reset(SequenceNumber value)
{
  this->sequences_.clear();
  this->sequences_.insert(SequenceRange(value, value));
}

bool
DisjointSequence::lowest_valid(SequenceNumber value)
{
  validate(SequenceRange(value, value));
  if (++low() >= value) return false; // nothing to shift

  // if the current range goes negative, then assume we can go negative,
  // otherwise it is not important
  const SequenceNumber validLowWaterMark(value.previous());
  SequenceRange validLowWaterRange(validLowWaterMark, validLowWaterMark);

  RangeSet::iterator low_bound =
    this->sequences_.lower_bound(validLowWaterRange);
  // already account for low_bound == this->sequences_.begin() above
  if (low_bound != this->sequences_.end() && low_bound->first <= value) {
    // since this "seen" range encompasses the new low water mark,
    // we use the highest seen in the range
    validLowWaterRange = SequenceRange(low_bound->second, low_bound->second);
    // move past the low_bound, so that the original low_bound will be erased
    ++low_bound;
  }
  this->sequences_.erase(this->sequences_.begin(), low_bound);
  this->sequences_.insert(validLowWaterRange);
  return true;
}

bool
DisjointSequence::update(SequenceNumber value)
{
  return update(SequenceRange(value, value));
}

bool
DisjointSequence::update(const SequenceRange& range)
{
  validate(range);
  RangeSet::iterator range_above = this->sequences_.lower_bound(range);
  if (range_above != this->sequences_.end()
      && (range_above == this->sequences_.begin()
          || range_above->first <= range.first)) {
    return false; // already saw this range, nothing to update
  }

  SequenceRange newRange(range);
  if (range_above != this->sequences_.end()
      && ++SequenceNumber(newRange.second) >= range_above->first) {
    newRange.second = range_above->second;
    // move to just past this iterator for the erase
    ++range_above;
  }
  const SequenceNumber previous(range.first.previous());
  // find the lower_bound for the SequenceNumber just before this range
  // to see if any ranges need to combine
  RangeSet::iterator range_below =
    this->sequences_.lower_bound(SequenceRange(previous, previous));
  if (range_below != this->sequences_.end()) {
    if (range_below == this->sequences_.begin()) {
      // low just indicates the highest value
      // before the first unseen SequenceNumber
      newRange.first = newRange.second;
    } else {
      // if low end falls inside of the range_below range
      // then combine
      if (newRange.first > range_below->first) {
        newRange.first = range_below->first;
      }
    }
    this->sequences_.erase(range_below, range_above);
  }
  this->sequences_.insert(newRange);
  return true;
}

std::vector<SequenceRange>
DisjointSequence::missing_sequence_ranges() const {
  std::vector<SequenceRange> missing_sequence_ranges;
  RangeSet::const_iterator secondIter = this->sequences_.begin();
  if (secondIter == this->sequences_.end()) {
    // no missing Sequence numbers yet
    return missing_sequence_ranges;
  }
  RangeSet::const_iterator firstIter = secondIter++;
  for(; secondIter != this->sequences_.end(); ++firstIter, ++ secondIter) {
    const SequenceNumber missingLow(++SequenceNumber(firstIter->second)),
      missingHigh(secondIter->first.previous());
    if (missingLow <= missingHigh) {
      missing_sequence_ranges.push_back(SequenceRange(missingLow, missingHigh));
    }
  }
  return missing_sequence_ranges;
}

void
DisjointSequence::validate(const SequenceRange& range) const {
  if (range.first > range.second) {
    throw std::runtime_error("SequenceNumber range invalid, range must "
      "be ascending.");
  }
  if (range.second < low() && range.first > high()) {
    throw std::runtime_error("SequenceNumber range invalid with respect"
      " to existing DisjointSequence SequenceNumbers.");
  }
}

void
DisjointSequence::dump() const
{
  ACE_DEBUG ((LM_DEBUG, "(%P|%t) DisjointSequence::dump(%X) Ranges of seen "
    "SequenceNumbers:\n", this));
  for (RangeSet::const_iterator iter = this->sequences_.begin();
       iter != this->sequences_.end(); ++iter) {
    ACE_DEBUG ((LM_DEBUG, "(%P|%t) DisjointSequence::dump(%X) %d-%d\n",
      this, iter->first.getValue(), iter->second.getValue()));
  }
}
} // namespace DCPS
} // namespace OpenDDS
