/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_VALUE_READER_H
#define OPENDDS_DCPS_VALUE_READER_H

#include "Definitions.h"
#include "XTypes/TypeObject.h"

#include <FACE/Fixed.h>
#include <dds/Versioned_Namespace.h>

#include <ace/CDR_Base.h>

#include <cstddef>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// A ValueReader produces events and values for the recitation of a
/// value.  To use it, one manually or automatically, e.g., code
/// generation in the IDL compiler, defines a vread function for a
/// given type V.
///
///   bool vread(ValueReader& vw, V& value)
///
/// The vread function should invoke the appropriate methods of the
/// ValueReader and dispatch to other vread functions.
class MemberHelper {
public:
  virtual ~MemberHelper() {}
  virtual bool get_value(XTypes::MemberId& value,
                         const char* name) const = 0;
};

class ListMemberHelper : public MemberHelper {
public:
  struct Pair {
    const char* name;
    XTypes::MemberId value;
  };

  ListMemberHelper(const Pair* pairs)
    : pairs_(pairs)
  {}

  bool get_value(XTypes::MemberId& value,
                 const char* name) const
  {
    for (const Pair* ptr = pairs_; ptr->name; ++ptr) {
      if (std::strcmp(ptr->name, name) == 0) {
        value = ptr->value;
        return true;
      }
    }

    return false;
  }

private:
  const Pair* pairs_;
};

class EnumHelper {
public:
  virtual ~EnumHelper() {}
  virtual bool get_value(ACE_CDR::Long& value,
                         const char* name) const = 0;
};

class ListEnumHelper : public EnumHelper {
public:
  struct Pair {
    const char* name;
    ACE_CDR::Long value;
  };

  ListEnumHelper(const Pair* pairs)
    : pairs_(pairs)
  {}

  bool get_value(ACE_CDR::Long& value,
                 const char* name) const
  {
    for (const Pair* ptr = pairs_; ptr->name; ++ptr) {
      if (std::strcmp(ptr->name, name) == 0) {
        value = ptr->value;
        return true;
      }
    }

    return false;
  }

private:
  const Pair* pairs_;
};

struct ValueReader {
  virtual ~ValueReader() {}

  virtual bool begin_struct() = 0;
  virtual bool end_struct() = 0;
  virtual bool begin_struct_member(XTypes::MemberId& member_id, const MemberHelper& helper) = 0;
  virtual bool end_struct_member() = 0;

  virtual bool begin_union() = 0;
  virtual bool end_union() = 0;
  virtual bool begin_discriminator() = 0;
  virtual bool end_discriminator() = 0;
  virtual bool begin_union_member() = 0;
  virtual bool end_union_member() = 0;

  virtual bool begin_array() = 0;
  virtual bool end_array() = 0;
  virtual bool begin_sequence() = 0;
  virtual bool elements_remaining() = 0;
  virtual bool end_sequence() = 0;
  virtual bool begin_element() = 0;
  virtual bool end_element() = 0;

  virtual bool read_boolean(ACE_CDR::Boolean& value) = 0;
  virtual bool read_byte(ACE_CDR::Octet& value) = 0;
#if OPENDDS_HAS_EXPLICIT_INTS
  virtual bool read_int8(ACE_CDR::Int8& value) = 0;
  virtual bool read_uint8(ACE_CDR::UInt8& value) = 0;
#endif
  virtual bool read_int16(ACE_CDR::Short& value) = 0;
  virtual bool read_uint16(ACE_CDR::UShort& value) = 0;
  virtual bool read_int32(ACE_CDR::Long& value) = 0;
  virtual bool read_uint32(ACE_CDR::ULong& value) = 0;
  virtual bool read_int64(ACE_CDR::LongLong& value) = 0;
  virtual bool read_uint64(ACE_CDR::ULongLong& value) = 0;
  virtual bool read_float32(ACE_CDR::Float& value) = 0;
  virtual bool read_float64(ACE_CDR::Double& value) = 0;
  virtual bool read_float128(ACE_CDR::LongDouble& value) = 0;

#ifdef NONNATIVE_LONGDOUBLE
  bool read_float128(long double& value)
  {
    ACE_CDR::LongDouble ld;
    if (!read_float128(ld)) {
      return false;
    }
    value = ld;
    return true;
  }
#endif

  virtual bool read_fixed(OpenDDS::FaceTypes::Fixed& value) = 0;
  virtual bool read_char8(ACE_CDR::Char& value) = 0;
  virtual bool read_char16(ACE_CDR::WChar& value) = 0;
  virtual bool read_string(String& value) = 0;
  virtual bool read_wstring(WString& value) = 0;

  virtual bool read_long_enum(ACE_CDR::Long& value, const EnumHelper& helper) = 0;
  template <typename T>
  bool read_enum(T& value, const EnumHelper& helper)
  {
    ACE_CDR::Long lvalue;
    if (!read_long_enum(lvalue, helper)) {
      return false;
    }
    value = static_cast<T>(lvalue);
    return true;
  }

};

template <typename T>
bool vread(ValueReader& value_reader, T& value);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_VALUE_READER_H */
