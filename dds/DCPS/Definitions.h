/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DEFINITION_H
#define OPENDDS_DCPS_DEFINITION_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "Cached_Allocator_With_Overflow_T.h"
#include "dds/DCPS/Serializer.h"
#include "ace/Message_Block.h"
#include "ace/Global_Macros.h"

#include <functional>
#include <utility>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// More strict check than ACE does: if we have GNU lib C++ without support for
// wchar_t (std::wstring, std::wostream, etc.) then we don't have DDS_HAS_WCHAR
#if defined (ACE_HAS_WCHAR) && \
    (!defined (_GLIBCPP_VERSION) || defined(_GLIBCPP_USE_WCHAR_T)) && \
    !defined (__ANDROID__)
#define DDS_HAS_WCHAR
#endif

#if defined (ACE_HAS_CPP11)
#define OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(CLASS)         \
  CLASS(const CLASS&) = delete;           \
  CLASS(CLASS&&) = delete;           \
  CLASS& operator=(const CLASS&) = delete; \
  CLASS& operator=(CLASS&&) = delete;
#else
#define OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(CLASS)         \
  ACE_UNIMPLEMENTED_FUNC(CLASS(const CLASS&))           \
  ACE_UNIMPLEMENTED_FUNC(CLASS& operator=(const CLASS&))
#endif

#if defined (ACE_DES_FREE_THIS)
#define OPENDDS_DES_FREE_THIS ACE_DES_FREE_THIS
#else
// The macro ACE_DES_FREE_THIS is part of ACE 6.4.2 or newer, define it within
// OpenDDS at the moment we compile against an older ACE version
# define OPENDDS_DES_FREE_THIS(DEALLOCATOR,CLASS) \
   do { \
        this->~CLASS (); \
        DEALLOCATOR (this); \
      } \
   while (0)
#endif /* ACE_DES_FREE_THIS */


// If features content_filtered_topic, multi_topic, and query_condition
// are all disabled, define a macro to indicate common code these
// three features depend on should not be built.
#if defined(OPENDDS_NO_QUERY_CONDITION) && defined(OPENDDS_NO_CONTENT_FILTERED_TOPIC) && defined(OPENDDS_NO_MULTI_TOPIC)
#define OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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

  bool operator<(const SequenceNumber& rvalue) const {
    return (this->high_ < rvalue.high_)
      || (this->high_ == rvalue.high_ && this->low_ < rvalue.low_);
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

  static SequenceNumber ZERO() {
    return SequenceNumber(0, 0);
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DEFINITION_H */
