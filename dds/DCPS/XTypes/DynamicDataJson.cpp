/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#if defined OPENDDS_RAPIDJSON && !defined OPENDDS_SAFETY_PROFILE

#include "DynamicDataJson.h"

#include "DynamicDataFactory.h"
#include "DynamicTypeImpl.h"
#include "TypeObject.h"
#include "Utils.h"

#include <dds/DCPS/RapidJsonWrapper.h>
#include <dds/DCPS/debug.h>

#include <ace/Basic_Types.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_string.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

namespace {

const char DISCRIMINATOR_JSON_NAME[] = "$discriminator";

typedef rapidjson::Value JsonValue;
typedef rapidjson::Document JsonDocument;
typedef rapidjson::StringBuffer JsonStringBuffer;
typedef rapidjson::Writer<JsonStringBuffer> JsonWriter;

void report_error(const std::string& message)
{
  ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: %C\n", message.c_str()));
}

template <typename T>
std::string value_to_string(const T& value)
{
  std::ostringstream out;
  out << value;
  return out.str();
}

std::string type_name(DDS::DynamicType_ptr type)
{
  if (!type) {
    return "<nil>";
  }
  CORBA::String_var name = type->get_name();
  return name.in() && name.in()[0] ? name.in() : typekind_to_string(type->get_kind());
}

std::string path_member(const std::string& path, const char* name)
{
  return path.empty() ? std::string(name) : path + "." + name;
}

std::string path_index(const std::string& path, DDS::UInt32 index)
{
  return path + '[' + value_to_string(index) + ']';
}

bool get_type_descriptor(DDS::TypeDescriptor_var& descriptor, DDS::DynamicType_ptr type)
{
  return type && type->get_descriptor(descriptor) == DDS::RETCODE_OK && descriptor;
}

DDS::ReturnCode_t get_member_descriptor_by_name(
  DDS::MemberDescriptor_var& descriptor,
  DDS::DynamicType_ptr type,
  const char* name)
{
  descriptor = 0;
  DDS::DynamicTypeMember_var member;
  DDS::ReturnCode_t rc = type->get_member_by_name(member, name);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return member->get_descriptor(descriptor);
}

DDS::ReturnCode_t get_member_descriptor_by_id(
  DDS::MemberDescriptor_var& descriptor,
  DDS::DynamicType_ptr type,
  DDS::MemberId id)
{
  descriptor = 0;
  DDS::DynamicTypeMember_var member;
  DDS::ReturnCode_t rc = type->get_member(member, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return member->get_descriptor(descriptor);
}

bool parse_integer_text(const char* text, DDS::Int64& value)
{
  if (!text || !text[0]) {
    return false;
  }
  errno = 0;
  char* end = 0;
  const long long parsed = std::strtoll(text, &end, 0);
  if (errno || !end || *end) {
    return false;
  }
  value = static_cast<DDS::Int64>(parsed);
  return true;
}

bool parse_unsigned_text(const char* text, DDS::UInt64& value)
{
  if (!text || !text[0]) {
    return false;
  }
  errno = 0;
  char* end = 0;
  const unsigned long long parsed = std::strtoull(text, &end, 0);
  if (errno || !end || *end) {
    return false;
  }
  value = static_cast<DDS::UInt64>(parsed);
  return true;
}

bool json_to_int64(const JsonValue& value, DDS::Int64& out)
{
  if (value.IsInt64()) {
    out = value.GetInt64();
    return true;
  }
  if (value.IsUint64() && value.GetUint64() <= static_cast<DDS::UInt64>(std::numeric_limits<DDS::Int64>::max())) {
    out = static_cast<DDS::Int64>(value.GetUint64());
    return true;
  }
  if (value.IsString()) {
    return parse_integer_text(value.GetString(), out);
  }
  return false;
}

bool json_to_uint64(const JsonValue& value, DDS::UInt64& out)
{
  if (value.IsUint64()) {
    out = value.GetUint64();
    return true;
  }
  if (value.IsInt64() && value.GetInt64() >= 0) {
    out = static_cast<DDS::UInt64>(value.GetInt64());
    return true;
  }
  if (value.IsString()) {
    return parse_unsigned_text(value.GetString(), out);
  }
  return false;
}

bool json_to_double(const JsonValue& value, double& out)
{
  if (value.IsNumber()) {
    out = value.GetDouble();
    return true;
  }
  if (value.IsString()) {
    errno = 0;
    char* end = 0;
    const double parsed = std::strtod(value.GetString(), &end);
    if (!errno && end && !*end) {
      out = parsed;
      return true;
    }
  }
  return false;
}

bool decode_utf8_codepoint(const char*& ptr, const char* end, DDS::UInt32& codepoint)
{
  if (ptr == end) {
    return false;
  }
  const unsigned char first = static_cast<unsigned char>(*ptr++);
  if (first < 0x80) {
    codepoint = first;
    return true;
  }

  DDS::UInt32 result = 0;
  unsigned int extra = 0;
  if ((first & 0xe0) == 0xc0) {
    result = first & 0x1f;
    extra = 1;
  } else if ((first & 0xf0) == 0xe0) {
    result = first & 0x0f;
    extra = 2;
  } else if ((first & 0xf8) == 0xf0) {
    result = first & 0x07;
    extra = 3;
  } else {
    return false;
  }

  for (unsigned int i = 0; i != extra; ++i) {
    if (ptr == end) {
      return false;
    }
    const unsigned char c = static_cast<unsigned char>(*ptr++);
    if ((c & 0xc0) != 0x80) {
      return false;
    }
    result = (result << 6) | (c & 0x3f);
  }
  codepoint = result;
  return true;
}

bool utf8_to_wchars(const char* input, std::vector<CORBA::WChar>& output)
{
  output.clear();
  if (!input) {
    return false;
  }
  const char* ptr = input;
  const char* const end = input + std::strlen(input);
  while (ptr != end) {
    DDS::UInt32 cp = 0;
    if (!decode_utf8_codepoint(ptr, end, cp)) {
      return false;
    }
    if (cp > static_cast<DDS::UInt32>(std::numeric_limits<CORBA::WChar>::max())) {
      return false;
    }
    output.push_back(static_cast<CORBA::WChar>(cp));
  }
  output.push_back(0);
  return true;
}

bool utf8_first_wchar(const char* input, CORBA::WChar& output)
{
  if (!input || !input[0]) {
    return false;
  }
  const char* ptr = input;
  const char* const end = input + std::strlen(input);
  DDS::UInt32 cp = 0;
  if (!decode_utf8_codepoint(ptr, end, cp)) {
    return false;
  }
  if (cp > static_cast<DDS::UInt32>(std::numeric_limits<CORBA::WChar>::max())) {
    return false;
  }
  output = static_cast<CORBA::WChar>(cp);
  return true;
}

void append_utf8(std::string& output, DDS::UInt32 cp)
{
  if (cp < 0x80) {
    output += static_cast<char>(cp);
  } else if (cp < 0x800) {
    output += static_cast<char>(0xc0 | (cp >> 6));
    output += static_cast<char>(0x80 | (cp & 0x3f));
  } else if (cp < 0x10000) {
    output += static_cast<char>(0xe0 | (cp >> 12));
    output += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
    output += static_cast<char>(0x80 | (cp & 0x3f));
  } else {
    output += static_cast<char>(0xf0 | (cp >> 18));
    output += static_cast<char>(0x80 | ((cp >> 12) & 0x3f));
    output += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
    output += static_cast<char>(0x80 | (cp & 0x3f));
  }
}

std::string wchars_to_utf8(const CORBA::WChar* input)
{
  std::string output;
  if (!input) {
    return output;
  }
  for (const CORBA::WChar* ptr = input; *ptr; ++ptr) {
    append_utf8(output, static_cast<DDS::UInt32>(*ptr));
  }
  return output;
}

int hex_nibble(char c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

char* float128_bytes(ACE_CDR::LongDouble& value)
{
#if ACE_SIZEOF_LONG_DOUBLE == 16
  return reinterpret_cast<char*>(&value);
#else
  return value.ld;
#endif
}

const char* float128_bytes(const ACE_CDR::LongDouble& value)
{
#if ACE_SIZEOF_LONG_DOUBLE == 16
  return reinterpret_cast<const char*>(&value);
#else
  return value.ld;
#endif
}

void float128_big_endian_bytes(const ACE_CDR::LongDouble& value, char bytes[16])
{
  const char* const src = float128_bytes(value);
#if defined(ACE_BIG_ENDIAN)
  ACE_OS::memcpy(bytes, src, 16);
#else
  for (size_t i = 0; i != 16; ++i) {
    bytes[i] = src[15 - i];
  }
#endif

}

bool parse_hex_float128(const char* text, ACE_CDR::LongDouble& value)
{
  if (!text) {
    return false;
  }
  if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    text += 2;
  }
  if (std::strlen(text) != 32) {
    return false;
  }

  char bytes[16];
  for (size_t i = 0; i != 16; ++i) {
    const int high = hex_nibble(text[i * 2]);
    const int low = hex_nibble(text[i * 2 + 1]);
    if (high < 0 || low < 0) {
      return false;
    }
    bytes[i] = static_cast<char>((high << 4) | low);
  }

  char* const dest = float128_bytes(value);
#if defined(ACE_BIG_ENDIAN)
  ACE_OS::memcpy(dest, bytes, 16);
#else
  for (size_t i = 0; i != 16; ++i) {
    dest[i] = bytes[15 - i];
  }
#endif
  return true;
}

std::string float128_to_hex(const ACE_CDR::LongDouble& value)
{
  static const char hex[] = "0123456789abcdef";
  char bytes[16];
  float128_big_endian_bytes(value, bytes);
  std::string output("0x");
  output.reserve(34);
  for (size_t i = 0; i != 16; ++i) {
    const unsigned char byte = static_cast<unsigned char>(bytes[i]);
    output += hex[byte >> 4];
    output += hex[byte & 0x0f];
  }
  return output;
}

bool set_float128_from_json(DDS::DynamicData_ptr data, DDS::MemberId id, const JsonValue& value)
{
  ACE_CDR::LongDouble ld = ACE_CDR_LONG_DOUBLE_INITIALIZER;
  if (value.IsString()) {
    if (!parse_hex_float128(value.GetString(), ld)) {
      double numeric = 0;
      if (!json_to_double(value, numeric)) {
        return false;
      }
#if ACE_SIZEOF_LONG_DOUBLE == 16
      ld = static_cast<ACE_CDR::LongDouble>(numeric);
#else
      ld.assign(numeric);
#endif
    }
  } else {
    double numeric = 0;
    if (!json_to_double(value, numeric)) {
      return false;
    }
#if ACE_SIZEOF_LONG_DOUBLE == 16
    ld = static_cast<ACE_CDR::LongDouble>(numeric);
#else
    ld.assign(numeric);
#endif
  }
  return data->set_float128_value(id, ld) == DDS::RETCODE_OK;
}

DDS::ReturnCode_t set_bitmask_value(
  DDS::DynamicType_ptr bitmask_type,
  DDS::DynamicData_ptr data,
  DDS::MemberId id,
  DDS::UInt64 value)
{
  DDS::TypeKind bound_kind = TK_NONE;
  DDS::ReturnCode_t rc = bitmask_bound(bitmask_type, bound_kind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return set_uint_value(data, id, bound_kind, value);
}

bool bitmask_text_to_value(DDS::DynamicType_ptr bitmask_type, const char* text, DDS::UInt64& output)
{
  DDS::UInt64 numeric = 0;
  if (parse_unsigned_text(text, numeric)) {
    output = numeric;
    return true;
  }

  output = 0;
  const char* token_begin = text;
  while (token_begin && *token_begin) {
    const char* token_end = std::strchr(token_begin, '|');
    const std::string flag(token_begin, token_end ? token_end - token_begin : std::strlen(token_begin));
    if (!flag.empty()) {
      DDS::MemberDescriptor_var md;
      if (get_member_descriptor_by_name(md, bitmask_type, flag.c_str()) != DDS::RETCODE_OK) {
        return false;
      }
      if (md->id() >= 64) {
        return false;
      }
      output |= (DDS::UInt64(1) << md->id());
    }
    token_begin = token_end ? token_end + 1 : 0;
  }
  return true;
}

bool json_to_bitmask_value(DDS::DynamicType_ptr bitmask_type, const JsonValue& value, DDS::UInt64& output)
{
  if (json_to_uint64(value, output)) {
    return true;
  }
  if (value.IsString()) {
    return bitmask_text_to_value(bitmask_type, value.GetString(), output);
  }
  return false;
}

bool json_to_discriminator(
  DDS::DynamicType_ptr discriminator_type,
  const JsonValue& value,
  DDS::Int32& output)
{
  DDS::DynamicType_var type = get_base_type(discriminator_type);
  if (!type) {
    return false;
  }

  switch (type->get_kind()) {
  case TK_BOOLEAN:
    if (value.IsBool()) {
      output = value.GetBool() ? 1 : 0;
      return true;
    }
    break;
  case TK_BYTE:
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_BITMASK: {
    DDS::UInt64 v = 0;
    if (type->get_kind() == TK_BITMASK) {
      if (!json_to_bitmask_value(type, value, v)) {
        return false;
      }
    } else if (!json_to_uint64(value, v)) {
      return false;
    }
    if (v > static_cast<DDS::UInt64>(std::numeric_limits<DDS::Int32>::max())) {
      return false;
    }
    output = static_cast<DDS::Int32>(v);
    return true;
  }
  case TK_CHAR8:
    if (value.IsString() && value.GetStringLength() > 0) {
      output = static_cast<unsigned char>(value.GetString()[0]);
      return true;
    }
    break;
  case TK_CHAR16: {
    if (value.IsString()) {
      CORBA::WChar wc = 0;
      if (utf8_first_wchar(value.GetString(), wc)) {
        output = static_cast<DDS::Int32>(wc);
        return true;
      }
    }
    break;
  }
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64: {
    DDS::Int64 v = 0;
    if (json_to_int64(value, v)) {
      if (v < std::numeric_limits<DDS::Int32>::min() ||
          v > std::numeric_limits<DDS::Int32>::max()) {
        return false;
      }
      output = static_cast<DDS::Int32>(v);
      return true;
    }
    break;
  }
  case TK_ENUM:
    if (value.IsString()) {
      DDS::Int32 v = 0;
      if (get_enumerator_value(v, value.GetString(), type) == DDS::RETCODE_OK) {
        output = v;
        return true;
      }
    } else {
      DDS::Int64 v = 0;
      if (json_to_int64(value, v)) {
        output = static_cast<DDS::Int32>(v);
        return true;
      }
    }
    break;
  default:
    break;
  }

  DDS::Int64 fallback = 0;
  if (json_to_int64(value, fallback)) {
    if (fallback < std::numeric_limits<DDS::Int32>::min() ||
        fallback > std::numeric_limits<DDS::Int32>::max()) {
      return false;
    }
    output = static_cast<DDS::Int32>(fallback);
    return true;
  }
  return false;
}

DDS::ReturnCode_t populate_value(
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const JsonValue& value,
  const DynamicDataJsonOptions& options,
  const std::string& path);

DDS::ReturnCode_t populate_member(
  DDS::DynamicData_ptr data,
  DDS::MemberId id,
  DDS::DynamicType_ptr type,
  const JsonValue& value,
  const DynamicDataJsonOptions& options,
  const std::string& path)
{
  DDS::DynamicType_var base = get_base_type(type);
  if (!base) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  const DDS::TypeKind kind = base->get_kind();
  if (is_complex(kind)) {
    DDS::DynamicData_var nested =
      DDS::DynamicDataFactory::get_instance()->create_data(base);
    if (!nested) {
      return DDS::RETCODE_ERROR;
    }
    DDS::ReturnCode_t rc = populate_value(nested, base, value, options, path);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    return data->set_complex_value(id, nested);
  }

  switch (kind) {
  case TK_BOOLEAN:
    if (!value.IsBool()) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return data->set_boolean_value(id, value.GetBool());
  case TK_BYTE: {
    DDS::UInt64 v = 0;
    if (!json_to_uint64(value, v)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return data->set_byte_value(id, static_cast<CORBA::Octet>(v));
  }
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64: {
    DDS::UInt64 v = 0;
    if (!json_to_uint64(value, v)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return set_uint_value(data, id, kind, v);
  }
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64: {
    DDS::Int64 v = 0;
    if (!json_to_int64(value, v)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return set_int_value(data, id, kind, v);
  }
  case TK_FLOAT32: {
    double v = 0;
    if (!json_to_double(value, v)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return data->set_float32_value(id, static_cast<CORBA::Float>(v));
  }
  case TK_FLOAT64: {
    double v = 0;
    if (!json_to_double(value, v)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return data->set_float64_value(id, v);
  }
  case TK_FLOAT128:
    return set_float128_from_json(data, id, value) ? DDS::RETCODE_OK : DDS::RETCODE_BAD_PARAMETER;
  case TK_CHAR8:
    if (!value.IsString() || value.GetStringLength() == 0) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return data->set_char8_value(id, value.GetString()[0]);
  case TK_CHAR16: {
    if (!value.IsString()) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    CORBA::WChar wc = 0;
    if (!utf8_first_wchar(value.GetString(), wc)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return data->set_char16_value(id, wc);
  }
  case TK_STRING8:
    if (!value.IsString()) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return data->set_string_value(id, value.GetString());
  case TK_STRING16: {
    if (!value.IsString()) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    std::vector<CORBA::WChar> wchars;
    if (!utf8_to_wchars(value.GetString(), wchars)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return data->set_wstring_value(id, &wchars[0]);
  }
  case TK_ENUM:
    if (value.IsString()) {
      return set_enum_value(base, data, id, value.GetString());
    } else {
      DDS::Int64 v = 0;
      if (!json_to_int64(value, v)) {
        return DDS::RETCODE_BAD_PARAMETER;
      }
      return set_enum_value(base, data, id, static_cast<DDS::Int32>(v));
    }
  case TK_BITMASK: {
    DDS::UInt64 v = 0;
    if (!json_to_bitmask_value(base, value, v)) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    return set_bitmask_value(base, data, id, v);
  }
  default:
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: unsupported type %C at %C\n",
      typekind_to_string(kind), path.c_str()));
    return DDS::RETCODE_UNSUPPORTED;
  }
}

DDS::ReturnCode_t populate_structure(
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const JsonValue& value,
  const DynamicDataJsonOptions& options,
  const std::string& path)
{
  if (!value.IsObject()) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  for (JsonValue::ConstMemberIterator it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
    DDS::MemberDescriptor_var md;
    DDS::ReturnCode_t rc = get_member_descriptor_by_name(md, type, it->name.GetString());
    if (rc != DDS::RETCODE_OK) {
      if (options.allow_unknown_members) {
        continue;
      }
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: unknown member %C in %C at %C\n",
        it->name.GetString(), type_name(type).c_str(), path.c_str()));
      return rc;
    }
    rc = populate_member(data, md->id(), md->type(), it->value, options,
      path_member(path, it->name.GetString()));
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
  }
  return DDS::RETCODE_OK;
}

bool find_single_union_value(
  const JsonValue& value,
  JsonValue::ConstMemberIterator& member)
{
  member = value.MemberEnd();
  for (JsonValue::ConstMemberIterator it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
    if (std::strcmp(it->name.GetString(), DISCRIMINATOR_JSON_NAME) == 0) {
      continue;
    }
    if (member != value.MemberEnd()) {
      return false;
    }
    member = it;
  }
  return member != value.MemberEnd();
}

DDS::ReturnCode_t populate_union_with_discriminator(
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const JsonValue& value,
  const DynamicDataJsonOptions& options,
  const std::string& path,
  JsonValue::ConstMemberIterator discriminator)
{
  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(td, type)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }
  DDS::DynamicType_var discriminator_type = get_base_type(td->discriminator_type());
  DDS::Int32 discriminator_value = 0;
  if (!json_to_discriminator(discriminator_type, discriminator->value, discriminator_value)) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: invalid discriminator at %C\n", path.c_str()));
    return DDS::RETCODE_BAD_PARAMETER;
  }

  bool found = false;
  DDS::MemberDescriptor_var selected_md;
  DDS::ReturnCode_t rc = get_selected_union_branch(
    DDS::DynamicType::_duplicate(type), discriminator_value, found, selected_md);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  JsonValue::ConstMemberIterator branch = value.MemberEnd();
  if (found) {
    branch = value.FindMember(selected_md->name());
  }
  if (branch == value.MemberEnd()) {
    const std::string key = value_to_string(discriminator_value);
    branch = value.FindMember(key.c_str());
  }
  if (branch == value.MemberEnd()) {
    JsonValue::ConstMemberIterator single = value.MemberEnd();
    if (find_single_union_value(value, single)) {
      branch = single;
    }
  }

  if (branch == value.MemberEnd()) {
    return populate_member(data, DISCRIMINATOR_ID, discriminator_type,
      discriminator->value, options, path_member(path, DISCRIMINATOR_JSON_NAME));
  }

  if (!found) {
    rc = get_member_descriptor_by_name(selected_md, type, branch->name.GetString());
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
  }
  rc = populate_member(data, selected_md->id(), selected_md->type(), branch->value,
    options, path_member(path, selected_md->name()));
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  return populate_member(data, DISCRIMINATOR_ID, discriminator_type,
    discriminator->value, options, path_member(path, DISCRIMINATOR_JSON_NAME));
}

DDS::ReturnCode_t populate_union_active_member(
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const JsonValue& value,
  const DynamicDataJsonOptions& options,
  const std::string& path)
{
  JsonValue::ConstMemberIterator branch = value.MemberEnd();
  if (!find_single_union_value(value, branch)) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: union %C at %C requires exactly one active member\n",
      type_name(type).c_str(), path.c_str()));
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::MemberDescriptor_var md;
  DDS::ReturnCode_t rc = get_member_descriptor_by_name(md, type, branch->name.GetString());
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: unknown union member %C in %C at %C\n",
      branch->name.GetString(), type_name(type).c_str(), path.c_str()));
    return rc;
  }
  return populate_member(data, md->id(), md->type(), branch->value,
    options, path_member(path, branch->name.GetString()));
}

DDS::ReturnCode_t populate_union(
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const JsonValue& value,
  const DynamicDataJsonOptions& options,
  const std::string& path)
{
  if (!value.IsObject()) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  JsonValue::ConstMemberIterator discriminator = value.FindMember(DISCRIMINATOR_JSON_NAME);
  if (discriminator != value.MemberEnd()) {
    return populate_union_with_discriminator(data, type, value, options, path, discriminator);
  }
  if (options.discriminator_format == DYNAMIC_DATA_JSON_DISCRIMINATOR_FIELD) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: missing $discriminator at %C\n", path.c_str()));
    return DDS::RETCODE_BAD_PARAMETER;
  }
  return populate_union_active_member(data, type, value, options, path);
}

DDS::ReturnCode_t populate_collection(
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const JsonValue& value,
  const DynamicDataJsonOptions& options,
  const std::string& path)
{
  if (!value.IsArray()) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(td, type)) {
    return DDS::RETCODE_BAD_PARAMETER;
  }
  DDS::DynamicType_var element_type = get_base_type(td->element_type());
  if (!element_type) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  if (type->get_kind() == TK_SEQUENCE && td->bound().length() && td->bound()[0] &&
      value.Size() > td->bound()[0]) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }
  if (type->get_kind() == TK_ARRAY) {
    const DDS::UInt32 bound = bound_total(td);
    if (value.Size() > bound) {
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }
  }

  for (rapidjson::SizeType i = 0; i != value.Size(); ++i) {
    DDS::ReturnCode_t rc = populate_member(data, i, element_type, value[i],
      options, path_index(path, i));
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t populate_value(
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const JsonValue& value,
  const DynamicDataJsonOptions& options,
  const std::string& path)
{
  DDS::DynamicType_var base = get_base_type(type);
  if (!data || !base) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  switch (base->get_kind()) {
  case TK_STRUCTURE:
    return populate_structure(data, base, value, options, path);
  case TK_UNION:
    return populate_union(data, base, value, options, path);
  case TK_MAP:
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: map type %C at %C is not supported\n",
      type_name(base).c_str(), path.c_str()));
    return DDS::RETCODE_UNSUPPORTED;
  case TK_ARRAY:
  case TK_SEQUENCE:
    return populate_collection(data, base, value, options, path);
  default:
    return populate_member(data, MEMBER_ID_INVALID, base, value, options, path);
  }
}

bool write_value(
  JsonWriter& writer,
  DDS::DynamicData_ptr data,
  DDS::MemberId id,
  DDS::DynamicType_ptr type,
  const DynamicDataJsonOptions& options);

bool write_complex_value(
  JsonWriter& writer,
  DDS::DynamicData_ptr data,
  DDS::MemberId id,
  DDS::DynamicType_ptr type,
  const DynamicDataJsonOptions& options)
{
  DDS::DynamicData_var nested;
  if (data->get_complex_value(nested, id) != DDS::RETCODE_OK) {
    return false;
  }
  return write_value(writer, nested, MEMBER_ID_INVALID, type, options);
}

bool write_scalar_value(
  JsonWriter& writer,
  DDS::DynamicData_ptr data,
  DDS::MemberId id,
  DDS::DynamicType_ptr type)
{
  DDS::DynamicType_var base = get_base_type(type);
  if (!base) {
    return false;
  }

  switch (base->get_kind()) {
  case TK_BOOLEAN: {
    CORBA::Boolean v = false;
    if (data->get_boolean_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Bool(v);
    return true;
  }
  case TK_BYTE: {
    CORBA::Octet v = 0;
    if (data->get_byte_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Uint(v);
    return true;
  }
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64: {
    DDS::UInt64 v = 0;
    if (get_uint_value(v, data, id, base->get_kind()) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Uint64(v);
    return true;
  }
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64: {
    DDS::Int64 v = 0;
    if (get_int_value(v, data, id, base->get_kind()) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Int64(v);
    return true;
  }
  case TK_FLOAT32: {
    CORBA::Float v = 0;
    if (data->get_float32_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Double(v);
    return true;
  }
  case TK_FLOAT64: {
    CORBA::Double v = 0;
    if (data->get_float64_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Double(v);
    return true;
  }
  case TK_FLOAT128: {
    ACE_CDR::LongDouble v = ACE_CDR_LONG_DOUBLE_INITIALIZER;
    if (data->get_float128_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    const std::string hex = float128_to_hex(v);
    writer.String(hex.c_str(), static_cast<rapidjson::SizeType>(hex.size()));
    return true;
  }
  case TK_CHAR8: {
    CORBA::Char v = 0;
    if (data->get_char8_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    writer.String(&v, 1);
    return true;
  }
  case TK_CHAR16: {
    CORBA::WChar v = 0;
    if (data->get_char16_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    std::string utf8;
    append_utf8(utf8, static_cast<DDS::UInt32>(v));
    writer.String(utf8.c_str(), static_cast<rapidjson::SizeType>(utf8.size()));
    return true;
  }
  case TK_STRING8: {
    CORBA::String_var v;
    if (data->get_string_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    writer.String(v.in());
    return true;
  }
  case TK_STRING16: {
    CORBA::WString_var v;
    if (data->get_wstring_value(v, id) != DDS::RETCODE_OK) {
      return false;
    }
    const std::string utf8 = wchars_to_utf8(v.in());
    writer.String(utf8.c_str(), static_cast<rapidjson::SizeType>(utf8.size()));
    return true;
  }
  case TK_ENUM: {
    DDS::Int32 v = 0;
    if (get_enum_value(v, base, data, id) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::String8_var name;
    if (get_enumerator_name(name, v, base) == DDS::RETCODE_OK) {
      writer.String(name.in());
    } else {
      writer.Int(v);
    }
    return true;
  }
  case TK_BITMASK: {
    DDS::UInt64 v = 0;
    if (get_bitmask_value(v, base, data, id) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Uint64(v);
    return true;
  }
  default:
    return false;
  }
}

bool write_struct_value(
  JsonWriter& writer,
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const DynamicDataJsonOptions& options)
{
  writer.StartObject();
  for (DDS::UInt32 i = 0; i != type->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var member;
    if (type->get_member_by_index(member, i) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Key(md->name());
    if (!write_value(writer, data, md->id(), md->type(), options)) {
      return false;
    }
  }
  writer.EndObject();
  return true;
}

bool write_union_value(
  JsonWriter& writer,
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const DynamicDataJsonOptions& options)
{
  writer.StartObject();
  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(td, type)) {
    return false;
  }
  DDS::DynamicType_var discriminator_type = get_base_type(td->discriminator_type());
  const bool write_discriminator =
    options.discriminator_format == DYNAMIC_DATA_JSON_DISCRIMINATOR_FIELD;
  if (write_discriminator) {
    writer.Key(DISCRIMINATOR_JSON_NAME);
    if (!write_scalar_value(writer, data, DISCRIMINATOR_ID, discriminator_type)) {
      return false;
    }
  }

  if (data->get_item_count() > 1) {
    const DDS::MemberId branch_id = data->get_member_id_at_index(1);
    DDS::MemberDescriptor_var md;
    if (get_member_descriptor_by_id(md, type, branch_id) != DDS::RETCODE_OK) {
      return false;
    }
    writer.Key(md->name());
    if (!write_value(writer, data, branch_id, md->type(), options)) {
      return false;
    }
  }
  writer.EndObject();
  return true;
}

bool write_collection_value(
  JsonWriter& writer,
  DDS::DynamicData_ptr data,
  DDS::DynamicType_ptr type,
  const DynamicDataJsonOptions& options)
{
  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(td, type)) {
    return false;
  }
  DDS::DynamicType_var element_type = get_base_type(td->element_type());
  if (!element_type) {
    return false;
  }
  DDS::UInt32 count = data->get_item_count();
  if (type->get_kind() == TK_ARRAY) {
    count = bound_total(td);
  }

  writer.StartArray();
  for (DDS::UInt32 i = 0; i != count; ++i) {
    if (!write_value(writer, data, i, element_type, options)) {
      return false;
    }
  }
  writer.EndArray();
  return true;
}

bool write_value(
  JsonWriter& writer,
  DDS::DynamicData_ptr data,
  DDS::MemberId id,
  DDS::DynamicType_ptr type,
  const DynamicDataJsonOptions& options)
{
  DDS::DynamicType_var base = get_base_type(type);
  if (!data || !base) {
    return false;
  }

  switch (base->get_kind()) {
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_ARRAY:
  case TK_SEQUENCE:
    if (id != MEMBER_ID_INVALID) {
      return write_complex_value(writer, data, id, base, options);
    }
    if (base->get_kind() == TK_STRUCTURE) {
      return write_struct_value(writer, data, base, options);
    }
    if (base->get_kind() == TK_UNION) {
      return write_union_value(writer, data, base, options);
    }
    return write_collection_value(writer, data, base, options);
  case TK_MAP:
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: map type %C is not supported\n",
      type_name(base).c_str()));
    return false;
  default:
    return write_scalar_value(writer, data, id, base);
  }
}

} // namespace

DynamicDataJsonOptions::DynamicDataJsonOptions()
  : discriminator_format(DYNAMIC_DATA_JSON_DISCRIMINATOR_AUTO)
  , allow_unknown_members(false)
{}

DDS::ReturnCode_t dynamic_data_from_json(
  DDS::DynamicData_ptr data,
  const char* json,
  const DynamicDataJsonOptions& options)
{
  if (!data || !json) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  JsonDocument document;
  document.Parse(json);
  if (document.HasParseError()) {
    report_error("failed to parse JSON at offset " + value_to_string(document.GetErrorOffset()));
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::DynamicType_var type = data->type();
  DDS::DynamicType_var base = get_base_type(type);
  DDS::ReturnCode_t rc = populate_value(data, base, document, options, "$");
  if (rc != DDS::RETCODE_OK) {
    report_error("failed to populate DynamicData for " + type_name(base));
  }
  return rc;
}

DDS::ReturnCode_t dynamic_data_from_json_file(
  DDS::DynamicData_ptr data,
  const ACE_TString& json_file,
  const DynamicDataJsonOptions& options)
{
  std::ifstream input(ACE_TEXT_ALWAYS_CHAR(json_file.c_str()));
  if (!input) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: dynamic_data_json: failed to open %C\n",
      ACE_TEXT_ALWAYS_CHAR(json_file.c_str())));
    return DDS::RETCODE_ERROR;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return dynamic_data_from_json(data, buffer.str().c_str(), options);
}

DDS::ReturnCode_t dynamic_data_to_json(
  std::string& json,
  DDS::DynamicData_ptr data,
  const DynamicDataJsonOptions& options)
{
  json.clear();
  if (!data) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::DynamicType_var type = data->type();
  JsonStringBuffer buffer;
  JsonWriter writer(buffer);
  if (!write_value(writer, data, MEMBER_ID_INVALID, type, options)) {
    return DDS::RETCODE_ERROR;
  }
  json.assign(buffer.GetString(), buffer.GetSize());
  return DDS::RETCODE_OK;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RAPIDJSON && !OPENDDS_SAFETY_PROFILE
