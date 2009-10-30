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
  // Set should minimally contain one value.
  this->values_.insert(value);
}

DisjointSequence::~DisjointSequence()
{
}

void
DisjointSequence::update(SequenceNumber value)
{
  if (value <= low()) return;
  this->values_.insert(value);
  normalize();
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
  values_type::iterator first = this->values_.begin();
  while (first != this->values_.end()) {
    values_type::iterator second(first);
    second++;

    if (second == this->values_.end() ||
        *second != *first + 1) break; // short-circuit

    this->values_.erase(first);
    first = second;
  }
}

} // namespace DCPS
} // namespace OpenDDS
