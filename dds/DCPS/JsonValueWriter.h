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
template <typename Writer>
class JsonValueWriter : public ValueWriter {
public:
  explicit JsonValueWriter(Writer& writer)
    : writer_(writer)
  {}

  void begin_struct();
  void end_struct();
  void begin_struct_member(const DDS::MemberDescriptor& /*descriptor*/);
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
  void write_string(const ACE_CDR::Char* value, size_t length);
  void write_wstring(const ACE_CDR::WChar* value, size_t length);
  void write_enum(const char* /*name*/, ACE_CDR::Long value);

private:
  Writer& writer_;
};

template <typename Writer>
void JsonValueWriter<Writer>::begin_struct()
{
  writer_.StartObject();
}

template <typename Writer>
void JsonValueWriter<Writer>::end_struct()
{
  writer_.EndObject();
}

template <typename Writer>
void JsonValueWriter<Writer>::begin_struct_member(const DDS::MemberDescriptor& descriptor)
{
  writer_.Key(descriptor.name());
}

template <typename Writer>
void JsonValueWriter<Writer>::end_struct_member()
{}

template <typename Writer>
void JsonValueWriter<Writer>::begin_union()
{
  writer_.StartObject();
}

template <typename Writer>
void JsonValueWriter<Writer>::end_union()
{
  writer_.EndObject();
}

template <typename Writer>
void JsonValueWriter<Writer>::begin_discriminator()
{
  writer_.Key("$discriminator");
}

template <typename Writer>
void JsonValueWriter<Writer>::end_discriminator()
{}

template <typename Writer>
void JsonValueWriter<Writer>::begin_union_member(const char* name)
{
  writer_.Key(name);
}

template <typename Writer>
void JsonValueWriter<Writer>::end_union_member()
{}

template <typename Writer>
void JsonValueWriter<Writer>::begin_array()
{
  writer_.StartArray();
}

template <typename Writer>
void JsonValueWriter<Writer>::end_array()
{
  writer_.EndArray();
}

template <typename Writer>
void JsonValueWriter<Writer>::begin_sequence()
{
  writer_.StartArray();
}

template <typename Writer>
void JsonValueWriter<Writer>::end_sequence()
{
  writer_.EndArray();
}

template <typename Writer>
void JsonValueWriter<Writer>::begin_element(size_t /*idx*/)
{}

template <typename Writer>
void JsonValueWriter<Writer>::end_element()
{}

template <typename Writer>
void JsonValueWriter<Writer>::write_boolean(ACE_CDR::Boolean value)
{
  writer_.Bool(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_byte(ACE_CDR::Octet value)
{
  writer_.Uint(value);
}

#if OPENDDS_HAS_EXPLICIT_INTS
template <typename Writer>
void JsonValueWriter<Writer>::write_int8(ACE_CDR::Int8 value)
{
  writer_.Int(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_uint8(ACE_CDR::UInt8 value)
{
  writer_.Uint(value);
}
#endif

template <typename Writer>
void JsonValueWriter<Writer>::write_int16(ACE_CDR::Short value)
{
  writer_.Int(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_uint16(ACE_CDR::UShort value)
{
  writer_.Uint(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_int32(ACE_CDR::Long value)
{
  writer_.Int(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_uint32(ACE_CDR::ULong value)
{
  writer_.Uint(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_int64(ACE_CDR::LongLong value)
{
  writer_.Int64(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_uint64(ACE_CDR::ULongLong value)
{
  writer_.Uint64(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_float32(ACE_CDR::Float value)
{
  writer_.Double(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_float64(ACE_CDR::Double value)
{
  writer_.Double(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_float128(ACE_CDR::LongDouble value)
{
  // TODO
  writer_.Double(value);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_fixed(const OpenDDS::FaceTypes::Fixed& /*value*/)
{
  // TODO
  writer_.String("fixed");
}

template <typename Writer>
void JsonValueWriter<Writer>::write_char8(ACE_CDR::Char value)
{
  ACE_CDR::Char s[2] = { value, 0 };
  writer_.String(s, 1);
}

template <typename Writer>
void JsonValueWriter<Writer>::write_char16(ACE_CDR::WChar value)
{
  ACE_CDR::WChar s[2] = { value, 0 };

  rapidjson::GenericStringStream<rapidjson::UTF16<> > source(s);
  rapidjson::GenericStringBuffer<rapidjson::UTF8<> > target;

  if (!rapidjson::Transcoder<rapidjson::UTF16<>, rapidjson::UTF8<> >::Transcode(source, target)) {
    return;
  }

  writer_.String(target.GetString(), static_cast<rapidjson::SizeType>(target.GetLength()));
}

template <typename Writer>
void JsonValueWriter<Writer>::write_string(const ACE_CDR::Char* value, size_t length)
{
  writer_.String(value, static_cast<rapidjson::SizeType>(length));
}

template <typename Writer>
void JsonValueWriter<Writer>::write_wstring(const ACE_CDR::WChar* value, size_t length)
{
  rapidjson::GenericStringStream<rapidjson::UTF16<> > source(value);
  rapidjson::GenericStringBuffer<rapidjson::UTF8<> > target;

  while (source.Tell() != length) {
    if (!rapidjson::Transcoder<rapidjson::UTF16<>, rapidjson::UTF8<> >::Transcode(source, target)) {
      return;
    }
  }

  writer_.String(target.GetString(), static_cast<rapidjson::SizeType>(target.GetLength()));
}

template <typename Writer>
void JsonValueWriter<Writer>::write_enum(const char* name,
                                         ACE_CDR::Long /*value*/)
{
  writer_.String(name);
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
  jvw.begin_struct_member(XTypes::MemberDescriptorImpl("topic", false));
  jvw.begin_struct();
  jvw.begin_struct_member(XTypes::MemberDescriptorImpl("name", false));
  CORBA::String_var topic_name = topic->get_name();
  static_cast<ValueWriter&>(jvw).write_string(topic_name);
  jvw.end_struct_member();
  jvw.begin_struct_member(XTypes::MemberDescriptorImpl("type_name", false));
  CORBA::String_var type_name = topic->get_type_name();
  static_cast<ValueWriter&>(jvw).write_string(type_name);
  jvw.end_struct_member();
  jvw.end_struct();
  jvw.end_struct_member();
  jvw.begin_struct_member(XTypes::MemberDescriptorImpl("sample", false));
  vwrite(jvw, sample);
  jvw.end_struct_member();
  jvw.begin_struct_member(XTypes::MemberDescriptorImpl("sample_info", false));
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
