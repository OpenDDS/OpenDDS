/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DisjointSequence.h"

#ifndef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#include <algorithm>

namespace OpenDDS {
namespace DCPS {

DisjointSequence::DisjointSequence(SequenceNumber value)
{
  this->values_.insert(value);
}

bool
DisjointSequence::range(RangeSet& values, size_t max_interval)
{
  for (SequenceSet::iterator first = this->values_.begin();
       first != this->values_.end(); ++first) {
    SequenceSet::iterator second(first);
    second++;

    if (second == this->values_.end()) break;

    SequenceNumber low(first->value_ + 1);
    SequenceNumber high(second->value_ - 1);

    for (SequenceNumber value(low); value <= high;
         value = SequenceNumber(value.value_ + max_interval)) {
      SequenceNumber n(value.value_ + max_interval - 1);
      
      std::pair<RangeSet::iterator, bool> pair =
        values.insert(RangePair(value, std::min(high, n)));
      if (pair.first == values.end()) return false;
    }
  }

  return true;
}

bool
DisjointSequence::update(SequenceNumber value)
{
  if (value <= low()) return false;
  this->values_.insert(value);
  normalize();
  return true;
}

void
DisjointSequence::skip(SequenceNumber value)
{
  this->values_.clear();
  this->values_.insert(value);
}

void
DisjointSequence::normalize()
{
  // Remove contiguities from the beginning of the
  // set; set should minimally contain one value.
  SequenceSet::iterator first = this->values_.begin();
  while (first != this->values_.end()) {
    SequenceSet::iterator second(first);
    second++;

    if (second == this->values_.end() ||
        *second != *first + 1) break; // short-circuit

    this->values_.erase(first);
    first = second;
  }
}

} // namespace DCPS
} // namespace OpenDDS
