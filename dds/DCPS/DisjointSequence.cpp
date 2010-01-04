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
DisjointSequence::shift(SequenceNumber value)
{
  value = SequenceNumber(value - 1);  // non-inclusive

  if (seen(value)) return; // nothing to shift

  this->sequences_.insert(value);

  SequenceSet::iterator first(this->sequences_.begin());
  SequenceSet::iterator last(this->sequences_.upper_bound(value));

  if (first == last) return;  // nothing to shift

  // Shift low-water mark to inserted value:
  this->sequences_.erase(first, last);
}

void
DisjointSequence::skip(SequenceNumber value)
{
  this->sequences_.clear();
  this->sequences_.insert(value);
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

    if (seen(value)) continue;  // nothing to update

    this->sequences_.insert(value);
    normalize();

    updated = true;
  }

  return updated;
}

void
DisjointSequence::normalize()
{
  // Ensure the set does not span more than MAX_DEPTH
  // sequences; this will cause comparisons to fail.
  if (depth() > MAX_DEPTH) {
    skip(high());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("DisjointSequence::normalize: ")
               ACE_TEXT("MAX_DEPTH exceeded; skipping to: 0x%x!\n"),
               static_cast<ACE_UINT16>(low())));
    return;
  }

  // Remove contiguities from the beginning of the
  // set; set should minimally contain one value.
  SequenceSet::iterator first(this->sequences_.begin());
  while (first != this->sequences_.end()) {
    SequenceSet::iterator second(first);
    second++;

    if (second == this->sequences_.end() ||
        *second != *first + 1) break; // short-circuit

    this->sequences_.erase(first);
    first = second;
  }
}

} // namespace DCPS
} // namespace OpenDDS
