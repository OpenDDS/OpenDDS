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

namespace OpenDDS {
namespace DCPS {

DisjointSequence::DisjointSequence(SequenceNumber value)
{
  this->values_.insert(value);
}

void
DisjointSequence::skip(SequenceNumber value)
{
  this->values_.clear();
  this->values_.insert(value);
}

bool
DisjointSequence::update(SequenceNumber value)
{
  if (value <= low()) return false; // already seen

  this->values_.insert(value);
  normalize();

  return true;
}

bool
DisjointSequence::update(const range_type& range)
{
  if (range.second <= low()) return false;  // already seen

  for (SequenceNumber value(range.first);
       value != range.second + 1; ++value) {
    this->values_.insert(value);
    normalize();
  }

  return true;
}

void
DisjointSequence::normalize()
{
  // Remove contiguities from the beginning of the
  // set; set should minimally contain one value.
  set_type::iterator first(this->values_.begin());
  while (first != this->values_.end()) {
    set_type::iterator second(first);
    second++;

    if (second == this->values_.end() ||
        *second != *first + 1) break; // short-circuit

    this->values_.erase(first);
    first = second;
  }
}

} // namespace DCPS
} // namespace OpenDDS
