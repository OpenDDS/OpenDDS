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
  validate(SequenceRange(value, value));
  value = previous_sequence_number(value, low());  // non-inclusive

  if (seen(value)) return; // nothing to shift

  this->sequences_.insert(value);

  SequenceSet::iterator first(this->sequences_.begin());
  SequenceSet::iterator last(this->sequences_.lower_bound(value));

  if (first == last) return;  // nothing to shift

  // Shift low-water mark to inserted value:
  this->sequences_.erase(first, last);

  normalize ();
}

bool
DisjointSequence::update(SequenceNumber value)
{
  validate(SequenceRange(value, value));
  if (seen(value)) return false;  // nothing to update

  std::pair<SequenceSet::iterator, bool> pair = this->sequences_.insert(value);
  if (!pair.second) return false; // nothing to update

  normalize();

  return true;
}

bool
DisjointSequence::update(const SequenceRange& range)
{
  validate(range);
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
        *second != *first + 1) break; // short-circuit

    this->sequences_.erase(first);
    first = second;
  }
}


DisjointSequence::range_iterator&
DisjointSequence::range_iterator::operator++()
{
  if (this->pos_ != this->end_) {
    SequenceSet::iterator prev(this->pos_++);

    if (this->pos_ != this->end_) {
      const SequenceNumber rangeLow(prev->getValue() + 1);
      this->value_ = SequenceRange(rangeLow,
                                   previous_sequence_number(this->pos_->getValue(), rangeLow));
      if (this->value_.first > this->value_.second) {
        operator++();
      }
    }

  }
  return *this;
}

SequenceNumber
DisjointSequence::previous_sequence_number(const SequenceNumber value, SequenceNumber in_reference_to) {
  // if all of the identifiable sequence is positive, then we do not know
  return SequenceNumber(((value.getValue() != 0) || (in_reference_to.getValue() < 0)) ?
    SequenceNumber(value.getValue() - 1) : SequenceNumber(SequenceNumber::MAX_VALUE));
}

void
DisjointSequence::validate(const SequenceRange& range) const {
  if (range.first > range.second)
    throw std::runtime_error("SequenceNumber range invalid, range must be assending.");
  if ((range.second < low()) && (range.first > high()))
    throw std::runtime_error("SequenceNumber range not valid with respect"
      " to existing DisjointSequence SequenceNumbers.");
}

void
DisjointSequence::dump()
{
  SequenceSet::iterator iter(this->sequences_.begin());
  while (iter != this->sequences_.end()) {
    ACE_DEBUG ((LM_DEBUG, "(%P|%t) DisjointSequence::dump(%X) %d\n", this, iter->getValue()));
    ++ iter;
  }
}
} // namespace DCPS
} // namespace OpenDDS
