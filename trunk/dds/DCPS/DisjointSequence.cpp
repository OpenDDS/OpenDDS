/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "DisjointSequence.h"

#include "ace/Log_Msg.h"

#ifndef __ACE_INLINE__
# include "DisjointSequence.inl"
#endif /* __ACE_INLINE__ */

#include <climits>

namespace OpenDDS {
namespace DCPS {

const size_t DisjointSequence::MAX_DEPTH(SHRT_MAX / 2 - 1);

DisjointSequence::DisjointSequence(SequenceNumber value)
{
  this->sequences_.insert(value);
}

void
DisjointSequence::reset(SequenceNumber value)
{
  this->sequences_.clear();
  this->sequences_.insert(value);
}

void
DisjointSequence::shift(SequenceNumber value)
{
  value = SequenceNumber(value - 1);  // non-inclusive

  if (seen(value)) return; // nothing to shift

  this->sequences_.insert(value);

  SequenceSet::iterator first(this->sequences_.begin());
  SequenceSet::iterator last(this->sequences_.lower_bound(value));

  if (first == last) return;  // nothing to shift

  // Shift low-water mark to inserted value:
  this->sequences_.erase(first, last);

  normalize();
}

bool
DisjointSequence::update(SequenceNumber value)
{
  if (seen(value)) return false;  // nothing to update

  std::pair<SequenceSet::iterator, bool> pair = this->sequences_.insert(value);
  if (!pair.second) return false; // nothing to update

  normalize();

  return true;
}

bool
DisjointSequence::update(const SequenceRange& range)
{
  if (seen(range.second)) return false; // nothing to update

  bool updated(false);
  for (SequenceNumber value(range.first);
       value != range.second + 1; ++value) {

    if (update(value)) updated = true;
  }

  return updated;
}

void
DisjointSequence::normalize()
{
  // Remove contiguities from the beginning of the
  // set; set should minimally contain one value.
  SequenceSet::iterator first(this->sequences_.begin());
  while (first != this->sequences_.end()) {
    SequenceSet::iterator second(first);
    second++;

    if (second == this->sequences_.end() ||
      (first->value_ != 0x7fff && *second != *first + 1) || 
      (first->value_ == 0x7fff && second->value_ != 0x0)) { // Wrap from max back to zero.
        break; // short-circuit
    }

    this->sequences_.erase(first);
    first = second;
  }
}

} // namespace DCPS
} // namespace OpenDDS
