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

DisjointSequence::DisjointSequence()
{
}

DisjointSequence::~DisjointSequence()
{
}

bool
DisjointSequence::update(SequenceNumber value)
{
  if (value <= this->low_ || value == this->high_) {
    return false; // value already seen; ignore
  }

  if (value == this->low_ + 1) {
    // Update value of low water mark; normalize
    // set to eliminate additional contiguities.
    this->low_ = value;
    normalize();

    return true;
  }

  if (value > this->high_) {
    // Swap new high water mark with previous value;
    // this intentionally falls through to insert
    // the previous alue to the set.
    std::swap(value, this->high_);
  }

  std::pair<values_type::iterator, bool> pair = this->values_.insert(value);
  return pair.second;
}

void
DisjointSequence::skip(SequenceNumber value)
{
  this->low_ = this->high_ = value;
  this->values_.clear();
}

void
DisjointSequence::normalize()
{
  // Remove all contiguities from the beginning of
  // the set; update the low water mark if needed.
  for (values_type::iterator it = this->values_.begin();
       it != this->values_.end(); ++it) {
    SequenceNumber value = *it;
    if (value = this->low_ + 1) {
      this->low_ = value;
      this->values_.erase(it);
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS
