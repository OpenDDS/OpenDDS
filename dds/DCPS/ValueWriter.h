/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_VALUE_WRITER_H
#define OPENDDS_DCPS_VALUE_WRITER_H

#include "Definitions.h"
#include "XTypes/MemberDescriptorImpl.h"

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

  virtual void begin_struct() {}
  virtual void end_struct() {}
  virtual void begin_struct_member(const DDS::MemberDescriptor& /*descriptor*/) {}
  virtual void end_struct_member() {}

  virtual void begin_union() {}
  virtual void end_union() {}
  virtual void begin_discriminator() {}
  virtual void end_discriminator() {}
  virtual void begin_union_member(const char* /*name*/) {}
  virtual void end_union_member() {}

  virtual void begin_array() {}
  virtual void end_array() {}
  virtual void begin_sequence() {}
  virtual void end_sequence() {}
  virtual void begin_element(size_t /*idx*/) {}
  virtual void end_element() {}

  virtual void write_boolean(ACE_CDR::Boolean /*value*/) = 0;
  virtual void write_byte(ACE_CDR::Octet /*value*/) = 0;
#if OPENDDS_HAS_EXPLICIT_INTS
  virtual void write_int8(ACE_CDR::Int8 /*value*/) = 0;
  virtual void write_uint8(ACE_CDR::UInt8 /*value*/) = 0;
#endif
  virtual void write_int16(ACE_CDR::Short /*value*/) = 0;
  virtual void write_uint16(ACE_CDR::UShort /*value*/) = 0;
  virtual void write_int32(ACE_CDR::Long /*value*/) = 0;
  virtual void write_uint32(ACE_CDR::ULong /*value*/) = 0;
  virtual void write_int64(ACE_CDR::LongLong /*value*/) = 0;
  virtual void write_uint64(ACE_CDR::ULongLong /*value*/) = 0;
  virtual void write_float32(ACE_CDR::Float /*value*/) = 0;
  virtual void write_float64(ACE_CDR::Double /*value*/) = 0;
  virtual void write_float128(ACE_CDR::LongDouble /*value*/) = 0;

#ifdef NONNATIVE_LONGDOUBLE
  void write_float128(long double value);
#endif

  virtual void write_fixed(const OpenDDS::FaceTypes::Fixed& /*value*/) = 0;
  virtual void write_char8(ACE_CDR::Char /*value*/) = 0;
  virtual void write_char16(ACE_CDR::WChar /*value*/) = 0;
  virtual void write_string(const ACE_CDR::Char* /*value*/, size_t /*length*/) = 0;
  void write_string(const ACE_CDR::Char* value) { write_string(value, std::strlen(value)); }
  void write_string(const std::string& value) { write_string(value.c_str(), value.length()); }
  virtual void write_wstring(const ACE_CDR::WChar* /*value*/, size_t /*length*/) = 0;
  void write_wstring(const ACE_CDR::WChar* value)
  {
#ifdef DDS_HAS_WCHAR
    write_wstring(value, std::wcslen(value));
#else
    ACE_UNUSED_ARG(value);
#endif
  }
  void write_wstring(const std::wstring& value)
  {
#ifdef DDS_HAS_WCHAR
    write_wstring(value.c_str(), value.length());
#else
    ACE_UNUSED_ARG(value);
#endif
  }

  virtual void write_enum(const char* /*name*/, ACE_CDR::Long /*value*/) = 0;
  template <typename T>
  void write_enum(const char* name, const T& value)
  {
    write_enum(name, static_cast<ACE_CDR::Long>(value));
  }

  /// Array write operations
  /// Note: the portion written starts at x and ends
  ///    at x + length.
  ///@{
  virtual void write_boolean_array(const ACE_CDR::Boolean* x, size_t length);
  virtual void write_byte_array(const ACE_CDR::Octet* x, size_t length);
#if OPENDDS_HAS_EXPLICIT_INTS
  virtual void write_int8_array(const ACE_CDR::Int8* x, size_t length);
  virtual void write_uint8_array(const ACE_CDR::UInt8* x, size_t length);
#endif
  virtual void write_int16_array(const ACE_CDR::Short* x, size_t length);
  virtual void write_uint16_array(const ACE_CDR::UShort* x, size_t length);
  virtual void write_int32_array(const ACE_CDR::Long* x, size_t length);
  virtual void write_uint32_array(const ACE_CDR::ULong* x, size_t length);
  virtual void write_int64_array(const ACE_CDR::LongLong* x, size_t length);
  virtual void write_uint64_array(const ACE_CDR::ULongLong* x, size_t length);
  virtual void write_float32_array(const ACE_CDR::Float* x, size_t length);
  virtual void write_float64_array(const ACE_CDR::Double* x, size_t length);
  virtual void write_float128_array(const ACE_CDR::LongDouble* x, size_t length);
  virtual void write_char8_array(const ACE_CDR::Char* x, size_t length);
  virtual void write_char16_array(const ACE_CDR::WChar* x, size_t length);
  ///@}
};

template <typename T>
void vwrite(ValueWriter& value_writer, const T& value);

// Implementations of this interface will call vwrite(value_writer, v)
// where v is the resulting of casting data to the appropriate type.
struct ValueWriterDispatcher {
  virtual ~ValueWriterDispatcher() {}

  virtual void write(ValueWriter& value_writer, const void* data) const = 0;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_VALUE_WRITER_H */
