/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_JSON_VALUE_WRITER_H
#define OPENDDS_DCPS_JSON_VALUE_WRITER_H

#if defined OPENDDS_RAPIDJSON && !defined OPENDDS_SAFETY_PROFILE
#  define OPENDDS_HAS_JSON_VALUE_WRITER 1
#else
#  define OPENDDS_HAS_JSON_VALUE_WRITER 0
#endif

#if OPENDDS_HAS_JSON_VALUE_WRITER

#include "ValueWriter.h"
#include "ValueHelper.h"
#include "RapidJsonWrapper.h"
#include "dcps_export.h"
#include "Definitions.h"

#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsTopicC.h>

#include <iosfwd>
#include <sstream>
#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Convert values to JSON.
template <typename Writer>
class JsonValueWriter : public ValueWriter {
public:
  explicit JsonValueWriter(Writer& writer)
    : writer_(writer)
    , mode_(NormalMode)
  {}

  bool begin_struct(Extensibility extensibility = FINAL);
  bool end_struct();
  bool begin_struct_member(MemberParam params);
  bool end_struct_member();

  bool begin_union(Extensibility extensibility = FINAL);
  bool end_union();
  bool begin_discriminator(MemberParam params);
  bool end_discriminator();
  bool begin_union_member(MemberParam params);
  bool end_union_member();

  bool begin_array(XTypes::TypeKind elem_kind = XTypes::TK_NONE);
  bool end_array();
  bool begin_sequence(XTypes::TypeKind elem_kind = XTypes::TK_NONE, ACE_CDR::ULong length = 0);
  bool end_sequence();
  bool begin_element(ACE_CDR::ULong idx);
  bool end_element();

  bool begin_map(XTypes::TypeKind key_kind, XTypes::TypeKind value_kind);
  bool end_map();
  bool begin_key();
  bool end_key();
  bool begin_value();
  bool end_value();

  bool write_boolean(ACE_CDR::Boolean value);
  bool write_byte(ACE_CDR::Octet value);
#if OPENDDS_HAS_EXPLICIT_INTS
  bool write_int8(ACE_CDR::Int8 value);
  bool write_uint8(ACE_CDR::UInt8 value);
#endif
  bool write_int16(ACE_CDR::Short value);
  bool write_uint16(ACE_CDR::UShort value);
  bool write_int32(ACE_CDR::Long value);
  bool write_uint32(ACE_CDR::ULong value);
  bool write_int64(ACE_CDR::LongLong value);
  bool write_uint64(ACE_CDR::ULongLong value);
  bool write_float32(ACE_CDR::Float value);
  bool write_float64(ACE_CDR::Double value);
  bool write_float128(ACE_CDR::LongDouble value);
  bool write_fixed(const ACE_CDR::Fixed& value);
  bool write_char8(ACE_CDR::Char value);
  bool write_char16(ACE_CDR::WChar value);
  bool write_string(const ACE_CDR::Char* value, size_t length);
  using ValueWriter::write_string;
  bool write_wstring(const ACE_CDR::WChar* value, size_t length);
  bool write_enum(ACE_CDR::Long value, const EnumHelper& helper);
  bool write_bitmask(ACE_CDR::ULongLong value, const BitmaskHelper& helper);
  bool write_absent_value();

private:
  Writer& writer_;
  enum {NormalMode, MapKeyMode} mode_;
};

template <typename Writer>
bool JsonValueWriter<Writer>::begin_struct(Extensibility)
{
  return writer_.StartObject();
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_struct()
{
  return writer_.EndObject();
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_struct_member(MemberParam params)
{
  return writer_.Key(params.name);
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_struct_member()
{
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_union(Extensibility)
{
  return writer_.StartObject();
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_union()
{
  return writer_.EndObject();
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_discriminator(MemberParam)
{
  return writer_.Key("$discriminator");
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_discriminator()
{
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_union_member(MemberParam params)
{
  return writer_.Key(params.name);
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_union_member()
{
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_array(XTypes::TypeKind /*elem_tk*/)
{
  return writer_.StartArray();
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_array()
{
  return writer_.EndArray();
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_sequence(XTypes::TypeKind /*elem_tk*/, ACE_CDR::ULong /*length*/)
{
  return writer_.StartArray();
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_sequence()
{
  return writer_.EndArray();
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_map(XTypes::TypeKind, XTypes::TypeKind)
{
  return writer_.StartObject();
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_map()
{
  return writer_.EndObject();
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_key()
{
  mode_ = MapKeyMode;
  // The next write_*() will become a JSON Key instead of a value.
  // Only simple types are supported as keys, so we don't need to keep
  // a stack of modes to implement nested objects here.
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_key()
{
  mode_ = NormalMode;
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_value()
{
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_value()
{
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::begin_element(ACE_CDR::ULong /*idx*/)
{
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::end_element()
{
  return true;
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_boolean(ACE_CDR::Boolean value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(value ? "true" : "false", value ? 4 : 5);
  }
  return writer_.Bool(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_byte(ACE_CDR::Octet value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Uint(value);
}

#if OPENDDS_HAS_EXPLICIT_INTS
template <typename Writer>
bool JsonValueWriter<Writer>::write_int8(ACE_CDR::Int8 value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Int(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_uint8(ACE_CDR::UInt8 value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Uint(value);
}
#endif

template <typename Writer>
bool JsonValueWriter<Writer>::write_int16(ACE_CDR::Short value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Int(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_uint16(ACE_CDR::UShort value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Uint(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_int32(ACE_CDR::Long value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Int(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_uint32(ACE_CDR::ULong value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Uint(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_int64(ACE_CDR::LongLong value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Int64(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_uint64(ACE_CDR::ULongLong value)
{
  if (mode_ == MapKeyMode) {
    return writer_.Key(to_dds_string(value).c_str());
  }
  return writer_.Uint64(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_float32(ACE_CDR::Float value)
{
  return writer_.Double(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_float64(ACE_CDR::Double value)
{
  return writer_.Double(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_float128(ACE_CDR::LongDouble value)
{
  // TODO
  return writer_.Double(value);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_fixed(const ACE_CDR::Fixed& value)
{
  char buffer[ACE_CDR::Fixed::MAX_STRING_SIZE];
  if (value.to_string(buffer, sizeof buffer)) {
    return writer_.String(buffer);
  }
  return false;
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_char8(ACE_CDR::Char value)
{
  const ACE_CDR::Char s[2] = { value, 0 };
  return mode_ == MapKeyMode ?  writer_.Key(s, 1) : writer_.String(s, 1);
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_char16(ACE_CDR::WChar value)
{
  const ACE_CDR::WChar s[2] = { value, 0 };

  rapidjson::GenericStringStream<rapidjson::UTF16<> > source(s);
  rapidjson::GenericStringBuffer<rapidjson::UTF8<> > target;

  if (!rapidjson::Transcoder<rapidjson::UTF16<>, rapidjson::UTF8<> >::Transcode(source, target)) {
    return false;
  }

  return mode_ == MapKeyMode
    ? writer_.Key(target.GetString(), static_cast<rapidjson::SizeType>(target.GetLength()))
    : writer_.String(target.GetString(), static_cast<rapidjson::SizeType>(target.GetLength()));
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_string(const ACE_CDR::Char* value, size_t length)
{
  return mode_ == MapKeyMode
    ? writer_.Key(value, static_cast<rapidjson::SizeType>(length))
    : writer_.String(value, static_cast<rapidjson::SizeType>(length));
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_wstring(const ACE_CDR::WChar* value, size_t length)
{
  rapidjson::GenericStringStream<rapidjson::UTF16<> > source(value);
  rapidjson::GenericStringBuffer<rapidjson::UTF8<> > target;

  while (source.Tell() != length) {
    if (!rapidjson::Transcoder<rapidjson::UTF16<>, rapidjson::UTF8<> >::Transcode(source, target)) {
      return false;
    }
  }

  return mode_ == MapKeyMode
    ? writer_.Key(target.GetString(), static_cast<rapidjson::SizeType>(target.GetLength()))
    : writer_.String(target.GetString(), static_cast<rapidjson::SizeType>(target.GetLength()));
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_enum(ACE_CDR::Long value, const EnumHelper& helper)
{
  const char* name = 0;
  if (helper.get_name(name, value)) {
    return write_string(name, std::strlen(name));
  }
  return false;
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_bitmask(ACE_CDR::ULongLong value, const BitmaskHelper& helper)
{
  const String s = bitmask_to_string(value, helper);
  return write_string(s.c_str(), s.length());
}

template <typename Writer>
bool JsonValueWriter<Writer>::write_absent_value()
{
  return writer_.Null();
}

template<typename T>
std::string to_json(const T& sample)
{
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);
  vwrite(jvw, sample);
  writer.Flush();
  return buffer.GetString();
}

template<typename T, typename Writer>
bool to_json(const T& sample, Writer& writer)
{
  JsonValueWriter<Writer> jvw(writer);
  return vwrite(jvw, sample);
}

template<typename T>
std::string to_json(const DDS::TopicDescription_ptr topic,
                    const T& sample, const DDS::SampleInfo& sample_info)
{
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);
  jvw.begin_struct();
  jvw.begin_struct_member("topic");
  jvw.begin_struct();
  jvw.begin_struct_member("name");
  CORBA::String_var topic_name = topic->get_name();
  static_cast<ValueWriter&>(jvw).write_string(topic_name);
  jvw.end_struct_member();
  jvw.begin_struct_member("type_name");
  CORBA::String_var type_name = topic->get_type_name();
  static_cast<ValueWriter&>(jvw).write_string(type_name);
  jvw.end_struct_member();
  jvw.end_struct();
  jvw.end_struct_member();
  jvw.begin_struct_member("sample");
  vwrite(jvw, sample);
  jvw.end_struct_member();
  jvw.begin_struct_member("sample_info");
  vwrite(jvw, sample_info);
  jvw.end_struct_member();
  jvw.end_struct();
  writer.Flush();
  return buffer.GetString();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

#endif  /* OPENDDS_DCPS_JSON_VALUE_WRITER_H */
