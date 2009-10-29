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
#include <cassert>

namespace OpenDDS {
namespace DCPS {

DisjointSequence::DisjointSequence(SequenceNumber value)
{
  insert(value);
}

DisjointSequence::~DisjointSequence()
{
}

void
DisjointSequence::insert(SequenceNumber value) {
  std::pair<values_type::iterator, bool> pair = this->values_.insert(value);
  assert(pair.second);
}

void
DisjointSequence::update(SequenceNumber value)
{
  if (value <= low()) return;

  insert(value);
  normalize();
}

void
DisjointSequence::skip(SequenceNumber value)
{
  this->values_.clear();
  insert(value);
}

void
DisjointSequence::normalize()
{
  // Remove contiguities from the beginning of the
  // set; set should minimally contain one value.
  values_type::iterator first = this->values_.begin();
  while (first != this->values_.end()) {
    values_type::iterator second(first);
    second++;

    if (second != this->values_.end() && *second == *first + 1) {
      this->values_.erase(first);
      first = second;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS
