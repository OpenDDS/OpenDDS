/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_JSON_VALUE_WRITER_H
#define OPENDDS_DCPS_JSON_VALUE_WRITER_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "dcps_export.h"

#include "ValueWriter.h"

#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include "dds/DdsDcpsTopicC.h"

#include <iosfwd>
#include <sstream>
#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// Convert values to JSON.
// Currently, this class does not produce value JSON nor does it adhere to the DDS JSON spec.
class OpenDDS_Dcps_Export JsonValueWriter : public ValueWriter {
public:
  explicit JsonValueWriter(std::ostream& out)
    : out_(out)
  {}

  void begin_struct();
  void end_struct();
  void begin_union();
  void end_union();
  void begin_discriminator();
  void end_discriminator();
  void begin_field(const char* name);
  void end_field();

  void begin_array();
  void end_array();
  void begin_sequence();
  void end_sequence();
  void begin_element(size_t idx);
  void end_element();

  void write_boolean(ACE_CDR::Boolean value);
  void write_byte(ACE_CDR::Octet value);
  void write_int8(ACE_CDR::Char value);
  void write_uint8(ACE_CDR::Octet value);
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

private:
  std::ostream& out_;
  std::vector<size_t> value_count_; // The number of values written in each scope.
};

template<typename T>
std::string to_json(const T& sample)
{
  std::ostringstream str;
  JsonValueWriter jvw(str);
  vwrite(jvw, sample);
  return str.str();
}

template<typename T>
std::string to_json(const DDS::TopicDescription_ptr topic,
                    const T& sample, const DDS::SampleInfo& sample_info)
{
  std::ostringstream str;
  JsonValueWriter jvw(str);
  jvw.begin_struct();
  jvw.begin_field("topic");
  jvw.begin_struct();
  jvw.begin_field("name");
  jvw.write_string(topic->get_name());
  jvw.end_field();
  jvw.begin_field("type_name");
  jvw.write_string(topic->get_type_name());
  jvw.end_field();
  jvw.end_struct();
  jvw.end_field();
  jvw.begin_field("sample");
  vwrite(jvw, sample);
  jvw.end_field();
  jvw.begin_field("sample_info");
  vwrite(jvw, sample_info);
  jvw.end_field();
  jvw.end_struct();
  return str.str();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

#endif  /* OPENDDS_DCPS_JSON_VALUE_WRITER_H */
