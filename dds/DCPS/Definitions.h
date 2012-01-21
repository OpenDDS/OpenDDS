/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DEFINITION_H
#define OPENDDS_DCPS_DEFINITION_H

#include "Cached_Allocator_With_Overflow_T.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/Serializer.h"
#include "ace/Message_Block.h"
#include "ace/Global_Macros.h"

#include <functional>
#include <utility>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// Newer versions of ACE+TAO do not define ACE_THROW_SPEC
#ifndef ACE_THROW_SPEC
#define ACE_THROW_SPEC(X)
#endif

// More strict check than ACE does: if we have GNU lib C++ without support for
// wchar_t (std::wstring, std::wostream, etc.) then we don't have DDS_HAS_WCHAR
#if defined (ACE_HAS_WCHAR) && \
    (!defined (_GLIBCPP_VERSION) || defined(_GLIBCPP_USE_WCHAR_T)) && \
    !defined (__ANDROID__)
#define DDS_HAS_WCHAR
#endif

#if defined __GNUC__ && (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 4))
// GCC 3.3.x doesn't have using-declarations and has some strange bugs
// regarding when the "template" keyword should be used to disambiguate.
#define OPENDDS_GCC33
#define OPENDDS_GCC33_TEMPLATE_NON_DEPENDENT template
#define OPENDDS_GCC33_TEMPLATE_DEPENDENT
#else
#define OPENDDS_GCC33_TEMPLATE_NON_DEPENDENT
#define OPENDDS_GCC33_TEMPLATE_DEPENDENT template
#endif

namespace OpenDDS {
namespace DCPS {

typedef ACE_UINT16 CoherencyGroup;
typedef RepoId PublicationId;
typedef RepoId SubscriptionId;

/// Sequence number abstraction.  Only allows positive 64 bit values.
class OpenDDS_Dcps_Export SequenceNumber {
public:
  typedef ACE_INT64 Value;
  /// Construct with a value, default to one (starting point).
  SequenceNumber(Value value = MIN_VALUE) {
    setValue(value);
  }

  // N.B: Default copy constructor is sufficient.

  /// Allow assignments.
  SequenceNumber& operator=(const SequenceNumber& rhs) {
    high_ = rhs.high_;
    low_  = rhs.low_;
    return *this;
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
    return *this ;
  }

  /// Post-increment.
  SequenceNumber operator++(int) {
    SequenceNumber value(*this);
    ++*this;
    return value ;
  }

  SequenceNumber previous() const {
    SequenceNumber retVal(*this);
    if ((this->low_ == 1) && (this->high_ == 0)) {
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
    return retVal ;
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

  /// N.B. This comparison assumes that the shortest distance between
  ///      the values being compared is the positive counting
  ///      sequence between them.  This means that MAX-2 is less
  ///      than 2 since they are separated by only four counts from
  ///      MAX-2 to 2.  But that 2 is less than MAX/2 since the
  ///      shortest distance is from 2 to MAX/2.
  bool operator<(const SequenceNumber& rvalue) const {
    const ACE_INT64 distance = ACE_INT64(rvalue.high_ - high_)*2;
    return (distance == 0) ?
             (this->low_ < rvalue.low_) :   // High values equal, compare low
             (distance < 0) ?               // Otherwise just use high
               (ACE_INT32_MAX < -distance) :
               (distance < ACE_INT32_MAX);
  }

  /// Derive a full suite of logical operations.
  bool operator==(const SequenceNumber& rvalue) const {
    return (this->high_ == rvalue.high_) &&
           (this->low_ == rvalue.low_) ;
  }
  bool operator!=(const SequenceNumber& rvalue) const {
    return (this->high_ != rvalue.high_) ||
           (this->low_ != rvalue.low_) ;
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

  // SEQUENCENUMBER_UNKOWN is defined by the RTPS spec.
  static SequenceNumber SEQUENCENUMBER_UNKNOWN() {
    return SequenceNumber(-1, 0);
  }

  static const Value MAX_VALUE = ACE_INT64_MAX;
  static const Value MIN_VALUE = 1;
  static const Value LOW_BASE = 0x0000000100000000LL;

  friend ACE_CDR::Boolean operator>>(Serializer& s, SequenceNumber& x);

private:

  // Private constructor used to force construction of SEQUENCENUMBER_UNKNOWN.
  // Also used by operator>> to allow deserialization of the same value.
  SequenceNumber(ACE_INT32 high, ACE_UINT32 low)
    : high_(high), low_(low) {
  }

  //Value value_;
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
  s >> high;
  if (!s.good_bit()) return false;
  s >> low;
  if (!s.good_bit()) return false;
  x = SequenceNumber(high, low);
  return true;
}

inline void
gen_find_size(const SequenceNumber& /*sn*/, size_t& size, size_t& padding) {
  find_size_ulong(size, padding);
  size += gen_max_marshaled_size(CORBA::Long());
}

typedef std::pair<SequenceNumber, SequenceNumber> SequenceRange;

typedef Cached_Allocator_With_Overflow<ACE_Message_Block, ACE_Thread_Mutex>
MessageBlockAllocator;
typedef Cached_Allocator_With_Overflow<ACE_Data_Block, ACE_Thread_Mutex>
DataBlockAllocator;
struct DataSampleHeader;
typedef Cached_Allocator_With_Overflow<DataSampleHeader, ACE_Null_Mutex>
DataSampleHeaderAllocator;

#define DUP true
#define NO_DUP false

/// This struct holds both object reference and the corresponding servant.
template <typename T_impl, typename T, typename T_ptr, typename T_var>
struct Objref_Servant_Pair {
  Objref_Servant_Pair()
    : svt_(0)
  {}

  Objref_Servant_Pair(T_impl* svt, T_ptr obj, bool dup)
    : svt_(svt)
  {
    if (dup) {
      obj_ = T::_duplicate(obj);

    } else {
      obj_ = obj;
    }
  }

  ~Objref_Servant_Pair()
  {}

  bool operator==(const Objref_Servant_Pair & pair) const {
    return pair.svt_ == this->svt_;
  }

  bool operator<(const Objref_Servant_Pair & pair) const {
    return this->svt_ < pair.svt_;
  }

  T_impl* svt_;
  T_var   obj_;
};

/// Use a Foo_var in a std::set or std::map with this comparison function,
/// for example std::set<Foo_var, VarLess<Foo> >
template <class T, class V = typename T::_var_type>
struct VarLess : public std::binary_function<V, V, bool> {
  bool operator()(const V& x, const V& y) const {
    return x.in() < y.in();
  }
};

} // namespace OpenDDS
} // namespace DCPS

inline OpenDDS_Dcps_Export OpenDDS::DCPS::SequenceNumber
operator+(const OpenDDS::DCPS::SequenceNumber& lhs, int rhs)
{
  return OpenDDS::DCPS::SequenceNumber(lhs.getValue() + rhs);
}

inline OpenDDS_Dcps_Export OpenDDS::DCPS::SequenceNumber
operator+(int lhs, const OpenDDS::DCPS::SequenceNumber& rhs)
{
  return rhs + lhs;
}

#endif /* OPENDDS_DCPS_DEFINITION_H */
