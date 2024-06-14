/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_VALUE_WRITER_H
#define OPENDDS_DCPS_VALUE_WRITER_H

#include "ValueCommon.h"
#include "Definitions.h"
#include "XTypes/MemberDescriptorImpl.h"
#include "XTypes/TypeObject.h"

#include <dds/Versioned_Namespace.h>
#include <FACE/Fixed.h>

#include <ace/CDR_Base.h>
#include <tao/String_Manager_T.h>

#include <cstddef>
#include <cstring>
#include <cwchar>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct MemberParam {
  MemberParam()
    : id()
    , must_understand(0)
    , optional(false)
    , present(true)
    , name(0)
  {}

  MemberParam(unsigned m_id, bool m_must_understand)
    : id(m_id)
    , must_understand(m_must_understand)
    , optional(false)
    , present(true)
    , name(0)
  {}

  MemberParam(const char* m_name)
    : id(0)
    , must_understand(false)
    , optional(false)
    , present(true)
    , name(m_name)
  {}

  MemberParam(unsigned m_id, bool m_must_understand,
              const char* m_name, bool m_optional, bool m_present)
    : id(m_id)
    , must_understand(m_must_understand)
    , optional(m_optional)
    , present(m_present)
    , name(m_name)
  {}

  unsigned id;
  bool must_understand;
  bool optional;
  bool present;
  const char* name;
};

/// A ValueWriter receives events and values from the recitation of a
/// value.  Typical examples of value recitation are serializing an
/// object for transmission, formatting an object for printing, or
/// copying an object to another representation, e.g., C++ to v8.  To
/// use it, one manually or automatically, e.g., code generation in the
/// IDL compiler, defines a vwrite function for a given type V.
///
///   void vwrite(ValueWriter& vw, const V& value)
///
/// The vwrite function should invoke the appropriate methods of the
/// ValueWriter and dispatch for other vwrite functions.
class OpenDDS_Dcps_Export ValueWriter {
public:
  ValueWriter()  {}
  virtual ~ValueWriter() {}

  virtual bool begin_struct(Extensibility extensibility) = 0;
  virtual bool end_struct() = 0;
  virtual bool begin_struct_member(MemberParam params) = 0;
  virtual bool end_struct_member() = 0;

  virtual bool begin_union(Extensibility extensibility) = 0;
  virtual bool end_union() = 0;
  virtual bool begin_discriminator(MemberParam params) = 0;
  virtual bool end_discriminator() = 0;
  virtual bool begin_union_member(MemberParam params) = 0;
  virtual bool end_union_member() = 0;

  virtual bool begin_array(XTypes::TypeKind elem_kind) = 0;
  virtual bool end_array() = 0;
  virtual bool begin_sequence(XTypes::TypeKind elem_kind, ACE_CDR::ULong length) = 0;
  virtual bool end_sequence() = 0;
  virtual void begin_map() {}
  virtual void end_map() {}
  virtual void begin_pair() {}
  virtual void end_pair() {}
  virtual void write_key() {}
  virtual void write_value() {}
  virtual bool begin_element(ACE_CDR::ULong idx) = 0;
  virtual bool end_element() = 0;

  virtual bool write_boolean(ACE_CDR::Boolean value) = 0;
  virtual bool write_byte(ACE_CDR::Octet value) = 0;
#if OPENDDS_HAS_EXPLICIT_INTS
  virtual bool write_int8(ACE_CDR::Int8 value) = 0;
  virtual bool write_uint8(ACE_CDR::UInt8 value) = 0;
#endif
  virtual bool write_int16(ACE_CDR::Short value) = 0;
  virtual bool write_uint16(ACE_CDR::UShort value) = 0;
  virtual bool write_int32(ACE_CDR::Long value) = 0;
  virtual bool write_uint32(ACE_CDR::ULong value) = 0;
  virtual bool write_int64(ACE_CDR::LongLong value) = 0;
  virtual bool write_uint64(ACE_CDR::ULongLong value) = 0;
  virtual bool write_float32(ACE_CDR::Float value) = 0;
  virtual bool write_float64(ACE_CDR::Double value) = 0;
  virtual bool write_float128(ACE_CDR::LongDouble value) = 0;

#ifdef NONNATIVE_LONGDOUBLE
  bool write_float128(long double value);
#endif

  virtual bool write_fixed(const ACE_CDR::Fixed& value) = 0;
  virtual bool write_char8(ACE_CDR::Char value) = 0;
  virtual bool write_char16(ACE_CDR::WChar value) = 0;
  virtual bool write_string(const ACE_CDR::Char* value, size_t length) = 0;
  bool write_string(const ACE_CDR::Char* value)
  {
    return write_string(value, std::strlen(value));
  }
  bool write_string(const std::string& value)
  {
    return write_string(value.c_str(), value.length());
  }

  virtual bool write_wstring(const ACE_CDR::WChar* value, size_t length) = 0;
  bool write_wstring(const ACE_CDR::WChar* value)
  {
#ifdef DDS_HAS_WCHAR
    return write_wstring(value, std::wcslen(value));
#else
    ACE_UNUSED_ARG(value);
    return false;
#endif
  }
  bool write_wstring(const std::wstring& value)
  {
#ifdef DDS_HAS_WCHAR
    return write_wstring(value.c_str(), value.length());
#else
    ACE_UNUSED_ARG(value);
    return false;
#endif
  }

  virtual bool write_enum(ACE_CDR::Long value, const EnumHelper& helper) = 0;

  template <typename T>
  bool write_enum(const T& value, const EnumHelper& helper)
  {
    return write_enum(static_cast<ACE_CDR::Long>(value), helper);
  }

  virtual bool write_bitmask(ACE_CDR::ULongLong value, const BitmaskHelper& helper) = 0;

  template <typename T>
  bool write_bitmask(const T& value, const BitmaskHelper& helper)
  {
    return write_bitmask(static_cast<ACE_CDR::ULongLong>(value), helper);
  }

  virtual bool write_absent_value() = 0;

  /// Array write operations
  /// Note: the portion written starts at x and ends
  ///    at x + length.
  ///@{
  virtual bool write_boolean_array(const ACE_CDR::Boolean* x, ACE_CDR::ULong length);
  virtual bool write_byte_array(const ACE_CDR::Octet* x, ACE_CDR::ULong length);
#if OPENDDS_HAS_EXPLICIT_INTS
  virtual bool write_int8_array(const ACE_CDR::Int8* x, ACE_CDR::ULong length);
  virtual bool write_uint8_array(const ACE_CDR::UInt8* x, ACE_CDR::ULong length);
#endif
  virtual bool write_int16_array(const ACE_CDR::Short* x, ACE_CDR::ULong length);
  virtual bool write_uint16_array(const ACE_CDR::UShort* x, ACE_CDR::ULong length);
  virtual bool write_int32_array(const ACE_CDR::Long* x, ACE_CDR::ULong length);
  virtual bool write_uint32_array(const ACE_CDR::ULong* x, ACE_CDR::ULong length);
  virtual bool write_int64_array(const ACE_CDR::LongLong* x, ACE_CDR::ULong length);
  virtual bool write_uint64_array(const ACE_CDR::ULongLong* x, ACE_CDR::ULong length);
  virtual bool write_float32_array(const ACE_CDR::Float* x, ACE_CDR::ULong length);
  virtual bool write_float64_array(const ACE_CDR::Double* x, ACE_CDR::ULong length);
  virtual bool write_float128_array(const ACE_CDR::LongDouble* x, ACE_CDR::ULong length);
  virtual bool write_char8_array(const ACE_CDR::Char* x, ACE_CDR::ULong length);
  virtual bool write_char16_array(const ACE_CDR::WChar* x, ACE_CDR::ULong length);
  ///@}

private:
  template <typename T>
  bool write_array_common(const T* buffer, ACE_CDR::ULong length, bool (ValueWriter::*pmf)(T));
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_VALUE_WRITER_H */
