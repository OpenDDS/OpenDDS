/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SEQUENCENUMBER_H
#define OPENDDS_DCPS_SEQUENCENUMBER_H

#include "Serializer.h"

#include <ace/Global_Macros.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <utility>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Sequence number abstraction.  Only allows positive 64 bit values.
class OpenDDS_Dcps_Export SequenceNumber {
public:
  typedef ACE_INT64 Value;
  /// Construct with a value, default to one (starting point).
  SequenceNumber(Value value = INITIAL_VALUE) {
    setValue(value);
  }

  /// Pre-increment.
  SequenceNumber& operator++() {
    if (this->low_ == ACE_UINT32_MAX) {
      if (this->high_ == ACE_INT32_MAX) {
        // this code is here, despite the RTPS spec statement:
        // "sequence numbers never wrap"
        this->high_ = 0;
        this->low_ = 1;
      } else {
        ++this->high_;
        this->low_ = 0;
      }
    } else {
      ++this->low_;
    }
    return *this;
  }

  /// Post-increment.
  SequenceNumber operator++(int) {
    SequenceNumber value(*this);
    ++*this;
    return value;
  }

  SequenceNumber previous() const {
    SequenceNumber retVal(*this);
    if ((this->low_ == 0) && (this->high_ == 0)) {
      retVal.high_ = ACE_INT32_MAX;
      retVal.low_  = ACE_UINT32_MAX;
      return retVal;
    }
    if (this->low_ == 0) {
      --retVal.high_;
      retVal.low_ = ACE_UINT32_MAX;
    } else {
      --retVal.low_;
    }
    return retVal;
  }

  void setValue(Value value) {
    if (value < MIN_VALUE) {
      value = MIN_VALUE;
    }
    this->high_ = ACE_INT32(value / LOW_BASE);
    this->low_  = ACE_UINT32(value % LOW_BASE);
  }

  void setValue(ACE_INT32 high, ACE_UINT32 low) {
    this->high_ = high;
    this->low_ = low;
    if (this->getValue() < MIN_VALUE) {
      this->setValue(MIN_VALUE);
    }
  }

  Value getValue() const {
    return LOW_BASE * this->high_ + this->low_;
  }

  bool operator<(const SequenceNumber& rvalue) const {
    return (this->high_ < rvalue.high_)
      || (this->high_ == rvalue.high_ && this->low_ < rvalue.low_);
  }

  /// Derive a full suite of logical operations.
  bool operator==(const SequenceNumber& rvalue) const {
    return (this->high_ == rvalue.high_) &&
           (this->low_ == rvalue.low_);
  }
  bool operator!=(const SequenceNumber& rvalue) const {
    return (this->high_ != rvalue.high_) ||
           (this->low_ != rvalue.low_);
  }
  bool operator>=(const SequenceNumber& rvalue) const {
    return !(*this  < rvalue);
  }
  bool operator<=(const SequenceNumber& rvalue) const {
    return !(rvalue < *this);
  }
  bool operator>(const SequenceNumber& rvalue) const {
    return (rvalue < *this)
           && (*this != rvalue);
  }

  ACE_INT32 getHigh() const {
    return high_;
  }
  ACE_UINT32 getLow() const {
    return low_;
  }

  // SEQUENCENUMBER_UNKNOWN is defined by the RTPS spec.
  static SequenceNumber SEQUENCENUMBER_UNKNOWN() {
    return SequenceNumber(-1, 0);
  }

  static SequenceNumber ZERO() {
    return SequenceNumber(0, 0);
  }

  static const Value MAX_VALUE = ACE_INT64_MAX;
  static const Value INITIAL_VALUE = 1;
  static const Value MIN_VALUE = 0;
  static const Value LOW_BASE = 0x0000000100000000LL;

  friend ACE_CDR::Boolean operator>>(Serializer& s, SequenceNumber& x);

private:

  // Private constructor used to force construction of SEQUENCENUMBER_UNKNOWN.
  // Also used by operator>> to allow deserialization of the same value.
  SequenceNumber(ACE_INT32 high, ACE_UINT32 low)
    : high_(high), low_(low) {
  }

  ACE_INT32  high_;
  ACE_UINT32 low_;
};

inline ACE_CDR::Boolean
operator<<(Serializer& s, const SequenceNumber& x) {
  s << x.getHigh();
  s << x.getLow();
  return s.good_bit();
}

inline ACE_CDR::Boolean
operator>>(Serializer& s, SequenceNumber& x) {
  ACE_INT32 high;
  ACE_UINT32 low;
  if (!(s >> high)) {
    return false;
  }
  if (!(s >> low)) {
    return false;
  }
  x = SequenceNumber(high, low);
  return true;
}

inline SequenceNumber
operator+(const SequenceNumber& lhs, int rhs)
{
  return SequenceNumber(lhs.getValue() + rhs);
}

inline SequenceNumber
operator+=(SequenceNumber& lhs, int rhs)
{
  lhs.setValue(lhs.getValue() + rhs);
  return lhs;
}

inline SequenceNumber
operator+(int lhs, const SequenceNumber& rhs)
{
  return rhs + lhs;
}

inline
void serialized_size(const Encoding& encoding, size_t& size,
  const SequenceNumber& /*sn*/)
{
  primitive_serialized_size_ulong(encoding, size, 2);
}

typedef std::pair<SequenceNumber, SequenceNumber> SequenceRange;
extern OpenDDS_Dcps_Export const SequenceRange unknown_sequence_range;

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SEQUENCENUMBER_H */
