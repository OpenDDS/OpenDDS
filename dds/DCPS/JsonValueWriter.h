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
/// Currently, this class does not produce value JSON nor does it adhere to the DDS JSON spec.
template <typename Buffer = rapidjson::StringBuffer>
class JsonValueWriter : public ValueWriter {
public:
  JsonValueWriter()
    : buffer_(default_buffer_)
    , writer_(buffer_)
  {}

  explicit JsonValueWriter(Buffer& buffer)
    : buffer_(buffer)
    , writer_(buffer_)
  {}

  void begin_struct();
  void end_struct();
  void begin_struct_member(const char* name);
  void end_struct_member();

  void begin_union();
  void end_union();
  void begin_discriminator();
  void end_discriminator();
  void begin_union_member(const char* name);
  void end_union_member();

  void begin_array();
  void end_array();
  void begin_sequence();
  void end_sequence();
  void begin_element(size_t idx);
  void end_element();

  void write_boolean(ACE_CDR::Boolean value);
  void write_byte(ACE_CDR::Octet value);
#if OPENDDS_HAS_EXPLICIT_INTS
  void write_int8(ACE_CDR::Int8 value);
  void write_uint8(ACE_CDR::UInt8 value);
#endif
  void write_int16(ACE_CDR::Short value);
  void write_uint16(ACE_CDR::UShort value);
  void write_int32(ACE_CDR::Long value);
  void write_uint32(ACE_CDR::ULong value);
  void write_int64(ACE_CDR::LongLong value);
  void write_uint64(ACE_CDR::ULongLong value);
  void write_float32(ACE_CDR::Float value);
  void write_float64(ACE_CDR::Double value);
  void write_float128(ACE_CDR::LongDouble value);
  void write_fixed(const OpenDDS::FaceTypes::Fixed& value);
  void write_char8(ACE_CDR::Char value);
  void write_char16(ACE_CDR::WChar value);
  void write_string(const ACE_CDR::Char* value);
  void write_wstring(const ACE_CDR::WChar* value);
  void write_enum(const char* /*name*/, ACE_CDR::Long value);

  const Buffer& buffer() const { return buffer_; }

private:
  Buffer default_buffer_;
  Buffer& buffer_;
  rapidjson::Writer<Buffer> writer_;
};

template <typename Buffer>
void JsonValueWriter<Buffer>::begin_struct()
{
  writer_.StartObject();
}

template <typename Buffer>
void JsonValueWriter<Buffer>::end_struct()
{
  writer_.EndObject();
}

template <typename Buffer>
void JsonValueWriter<Buffer>::begin_struct_member(const char* name)
{
  writer_.Key(name);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::end_struct_member()
{}

template <typename Buffer>
void JsonValueWriter<Buffer>::begin_union()
{
  writer_.StartObject();
}

template <typename Buffer>
void JsonValueWriter<Buffer>::end_union()
{
  writer_.EndObject();
}

template <typename Buffer>
void JsonValueWriter<Buffer>::begin_discriminator()
{
  writer_.Key("$discriminator");
}

template <typename Buffer>
void JsonValueWriter<Buffer>::end_discriminator()
{}

template <typename Buffer>
void JsonValueWriter<Buffer>::begin_union_member(const char* name)
{
  writer_.Key(name);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::end_union_member()
{}

template <typename Buffer>
void JsonValueWriter<Buffer>::begin_array()
{
  writer_.StartArray();
}

template <typename Buffer>
void JsonValueWriter<Buffer>::end_array()
{
  writer_.EndArray();
}

template <typename Buffer>
void JsonValueWriter<Buffer>::begin_sequence()
{
  writer_.StartArray();
}

template <typename Buffer>
void JsonValueWriter<Buffer>::end_sequence()
{
  writer_.EndArray();
}

template <typename Buffer>
void JsonValueWriter<Buffer>::begin_element(size_t /*idx*/)
{}

template <typename Buffer>
void JsonValueWriter<Buffer>::end_element()
{}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_boolean(ACE_CDR::Boolean value)
{
  writer_.Bool(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_byte(ACE_CDR::Octet value)
{
  writer_.Uint(value);
}

#if OPENDDS_HAS_EXPLICIT_INTS
template <typename Buffer>
void JsonValueWriter<Buffer>::write_int8(ACE_CDR::Int8 value)
{
  writer_.Int(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_uint8(ACE_CDR::UInt8 value)
{
  writer_.Uint(value);
}
#endif

template <typename Buffer>
void JsonValueWriter<Buffer>::write_int16(ACE_CDR::Short value)
{
  writer_.Int(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_uint16(ACE_CDR::UShort value)
{
  writer_.Uint(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_int32(ACE_CDR::Long value)
{
  writer_.Int(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_uint32(ACE_CDR::ULong value)
{
  writer_.Uint(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_int64(ACE_CDR::LongLong value)
{
  writer_.Int64(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_uint64(ACE_CDR::ULongLong value)
{
  writer_.Uint64(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_float32(ACE_CDR::Float value)
{
  writer_.Double(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_float64(ACE_CDR::Double value)
{
  writer_.Double(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_float128(ACE_CDR::LongDouble value)
{
  // TODO
  writer_.Double(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_fixed(const OpenDDS::FaceTypes::Fixed& /*value*/)
{
  // TODO
  writer_.String("fixed");
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_char8(ACE_CDR::Char value)
{
  writer_.Int(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_char16(ACE_CDR::WChar value)
{
  writer_.Int(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_string(const ACE_CDR::Char* value)
{
  writer_.String(value);
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_wstring(const ACE_CDR::WChar*)
{
  // TODO
  writer_.String("wide string");
}

template <typename Buffer>
void JsonValueWriter<Buffer>::write_enum(const char* name,
                                         ACE_CDR::Long /*value*/)
{
  writer_.String(name);
}

template<typename T>
std::string to_json(const T& sample)
{
  JsonValueWriter<> jvw;
  vwrite(jvw, sample);
  return jvw.buffer().GetString();
}

template<typename T>
std::string to_json(const DDS::TopicDescription_ptr topic,
                    const T& sample, const DDS::SampleInfo& sample_info)
{
  JsonValueWriter<> jvw;
  jvw.begin_struct();
  jvw.begin_struct_member("topic");
  jvw.begin_struct();
  jvw.begin_struct_member("name");
  CORBA::String_var topic_name = topic->get_name();
  jvw.write_string(topic_name);
  jvw.end_struct_member();
  jvw.begin_struct_member("type_name");
  CORBA::String_var type_name = topic->get_type_name();
  jvw.write_string(type_name);
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
  return jvw.buffer().GetString();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

#endif  /* OPENDDS_DCPS_JSON_VALUE_WRITER_H */
