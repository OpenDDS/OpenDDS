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
#include <algorithm>
#include <iterator>

#ifndef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

bool
DisjointSequence::insert_i(const SequenceRange& range,
                           std::vector<SequenceRange>* gaps /* = 0 */)
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

  const SequenceNumber previous = range.first.previous();
  // find the lower_bound for the SequenceNumber just before this range
  // to see if any ranges need to combine
  const RangeSet::iterator range_below =
    sequences_.lower_bound(SequenceRange(0 /*ignored*/, previous));
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
  //TODO
  return false;
}

std::vector<SequenceRange>
DisjointSequence::missing_sequence_ranges() const
{
  std::vector<SequenceRange> missing;
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

std::vector<SequenceRange>
DisjointSequence::present_sequence_ranges() const
{
  std::vector<SequenceRange> present;
  std::copy(sequences_.begin(), sequences_.end(), std::back_inserter(present));
  return present;
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
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DisjointSequence[%X]::dump Ranges of seen "
                       "SequenceNumbers:\n", this));
  for (RangeSet::const_iterator iter = sequences_.begin();
       iter != sequences_.end(); ++iter) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DisjointSequence[%X]::dump\t%q-%q\n",
               this, iter->first.getValue(), iter->second.getValue()));
  }
}

} // namespace DCPS
} // namespace OpenDDS
