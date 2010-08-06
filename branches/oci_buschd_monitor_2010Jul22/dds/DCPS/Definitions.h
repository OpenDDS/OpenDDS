/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DEFINITION_H
#define OPENDDS_DCPS_DEFINITION_H

#include "Cached_Allocator_With_Overflow_T.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "ace/Message_Block.h"

#include <functional>
#include <utility>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// More strict check than ACE does: if we have GNU lib C++ without support for
// wchar_t (std::wstring, std::wostream, etc.) then we don't have DDS_HAS_WCHAR
#if defined (ACE_HAS_WCHAR) && \
    (!defined (_GLIBCPP_VERSION) || defined(_GLIBCPP_USE_WCHAR_T))
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

/// Lolipop sequencing (never wrap to negative).
/// This helps distinguish new and old sequence numbers. (?)
class OpenDDS_Dcps_Export SequenceNumber {
public:
  typedef ACE_INT32 Value;
  /// Construct with a value, default to negative starting point.
  SequenceNumber(ACE_INT64 value = MIN_VALUE) {
    setValue(value);
  }

  // N.B: Default copy constructor is sufficient.

  /// Allow assignments.
  SequenceNumber& operator=(const SequenceNumber& rhs) {
    value_ = rhs.value_;
    return *this;
  }

  /// Pre-increment.
  SequenceNumber& operator++() {
    /// Lolipop sequencing (never wrap to negative).
    if (this->value_ == MAX_VALUE) this->value_ = 0x0;
    else                           ++this->value_;
    return *this ;
  }

  /// Post-increment.
  SequenceNumber operator++(int) {
    SequenceNumber value(*this);
    ++*this;
    return value ;
  }

  void setValue(ACE_INT64 value) {
    if (value > MAX_VALUE) this->value_ = Value(value % (MAX_VALUE+1));
    else                   this->value_ = Value(value);
  }

  Value getValue() const {
    return value_;
  }

  /// This is the magic of the lollipop.
  /// N.B. This comparison assumes that the shortest distance between
  ///      the values being compared is the positive counting
  ///      sequence between them.  This means that MAX-2 is less
  ///      than 2 since they are separated by only four counts from
  ///      MAX-2 to 2.  But that 2 is less than MAX/2 since the
  ///      shortest distance is from 2 to MAX/2.
  bool operator<(const SequenceNumber& rvalue) const {
    // double the distance and SHRT_MAX/2, to avoid rounding error
    const ACE_INT64 distance = (ACE_INT64(rvalue.value_) - value_)*2;
    return (distance == 0)? false:                // Equal is not less than.
       (value_ < 0 || rvalue.value_ < 0) ? (value_ < rvalue.value_): // Stem of lollipop.
       (distance <  0)? (MAX_VALUE+1 < -distance): // Closest distance dominates.
       (distance < MAX_VALUE+1);
  }

  /// Derive a full suite of logical operations.
  bool operator==(const SequenceNumber& rvalue) const {
    return value_ == rvalue.value_ ;
  }
  bool operator!=(const SequenceNumber& rvalue) const {
    return value_ != rvalue.value_ ;
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

  static const Value MAX_VALUE = ACE_INT32_MAX - 1;
  static const Value MIN_VALUE = ACE_INT32_MIN;

private:
  Value value_;
};

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
