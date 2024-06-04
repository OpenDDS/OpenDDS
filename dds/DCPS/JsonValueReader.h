/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_JSON_VALUE_READER_H
#define OPENDDS_DCPS_JSON_VALUE_READER_H

#if defined OPENDDS_RAPIDJSON && !defined OPENDDS_SAFETY_PROFILE
#  define OPENDDS_HAS_JSON_VALUE_READER 1
#else
#  define OPENDDS_HAS_JSON_VALUE_READER 0
#endif

#if OPENDDS_HAS_JSON_VALUE_READER

#include "dcps_export.h"
#include "ValueReader.h"
#include "RapidJsonWrapper.h"
#include "TypeSupportImpl.h"
#include "Definitions.h"

#include <iosfwd>
#include <sstream>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Convert JSON to values.
template <typename InputStream = rapidjson::StringStream>
class JsonValueReader
  : public ValueReader,
    public rapidjson::BaseReaderHandler<rapidjson::UTF8<> > {
public:
  explicit JsonValueReader(InputStream& input_stream)
    : token_type_(kUnknown)
    , input_stream_(input_stream)
    , bool_value_(false)
    , int_value_(0)
    , uint_value_(0)
    , int64_value_(0)
    , uint64_value_(0)
    , double_value_(0)
  {
    reader_.IterativeParseInit();
  }

  bool begin_struct(Extensibility extensibility = FINAL);
  bool end_struct();
  bool begin_struct_member(XTypes::MemberId& member_id, const MemberHelper& helper);
  bool members_remaining();
  bool member_has_value();
  bool end_struct_member();

  bool begin_union(Extensibility extensibility = FINAL);
  bool end_union();
  bool begin_discriminator();
  bool end_discriminator();
  bool begin_union_member();
  bool end_union_member();

  bool begin_array(XTypes::TypeKind elem_kind = XTypes::TK_NONE);
  bool end_array();
  bool begin_sequence(XTypes::TypeKind elem_kind = XTypes::TK_NONE);
  bool elements_remaining();
  bool end_sequence();
  bool begin_element();
  bool end_element();

  bool begin_map();
  bool end_map();
  bool pairs_remaining();
  bool begin_pair();
  bool begin_pair_key();
  bool end_pair_key();
  bool begin_pair_value();
  bool end_pair_value();
  bool end_pair();

  bool read_boolean(ACE_CDR::Boolean& value);
  bool read_byte(ACE_CDR::Octet& value);
#if OPENDDS_HAS_EXPLICIT_INTS
  bool read_int8(ACE_CDR::Int8& value);
  bool read_uint8(ACE_CDR::UInt8& value);
#endif
  bool read_int16(ACE_CDR::Short& value);
  bool read_uint16(ACE_CDR::UShort& value);
  bool read_int32(ACE_CDR::Long& value);
  bool read_uint32(ACE_CDR::ULong& value);
  bool read_int64(ACE_CDR::LongLong& value);
  bool read_uint64(ACE_CDR::ULongLong& value);
  bool read_float32(ACE_CDR::Float& value);
  bool read_float64(ACE_CDR::Double& value);
  bool read_float128(ACE_CDR::LongDouble& value);
  bool read_fixed(ACE_CDR::Fixed& value);
  bool read_char8(ACE_CDR::Char& value);
  bool read_char16(ACE_CDR::WChar& value);
  bool read_string(std::string& value);
  bool read_wstring(std::wstring& value);
  bool read_long_enum(ACE_CDR::Long& value, const EnumHelper& helper);
  bool read_bitmask(ACE_CDR::ULongLong& value, const BitmaskHelper& helper);

  bool Null() { token_type_ = kNull; return true; }
  bool Bool(bool b) { token_type_ = kBool; bool_value_ = b; return true; }
  bool Int(int i) { token_type_ = kInt; int_value_ = i; return true; }
  bool Uint(unsigned i) { token_type_ = kUint; uint_value_ = i; return true; }
  bool Int64(int64_t i) { token_type_ = kInt64; int64_value_ = i; return true; }
  bool Uint64(uint64_t i) { token_type_ = kUint64; uint64_value_ = i; return true; }
  bool Double(double d) { token_type_ = kDouble; double_value_ = d; return true; }
  bool RawNumber(const Ch* /* str */, rapidjson::SizeType /* length */, bool /* copy */) { token_type_ = kRawNumber; return true; }
  bool String(const Ch* str, rapidjson::SizeType length, bool /* copy */)
  {
    token_type_ = kString;
    string_value_ = std::string(str, length);
    return true;
  }
  bool StartObject() { token_type_ = kStartObject; return true; }
  bool Key(const Ch* str, rapidjson::SizeType /* length */, bool /* copy */) { token_type_ = kKey; key_value_ = str; return true; }
  bool EndObject(rapidjson::SizeType /* memberCount */) { token_type_ = kEndObject; return true; }
  bool StartArray() { token_type_ = kStartArray; return true; }
  bool EndArray(rapidjson::SizeType /* elementCount */) { token_type_ = kEndArray; return true; }

private:
  enum TokenType {
    kUnknown,
    kNull,
    kBool,
    kInt,
    kUint,
    kInt64,
    kUint64,
    kDouble,
    kRawNumber,
    kString,
    kStartObject,
    kKey,
    kEndObject,
    kStartArray,
    kEndArray,
    kError,
    kEnd
  };

  TokenType peek()
  {
    if (token_type_ != kUnknown) {
      return token_type_;
    }

    if (reader_.IterativeParseComplete()) {
      token_type_ = kEnd;
      return token_type_;
    }

    if (!reader_.IterativeParseNext<rapidjson::kParseStopWhenDoneFlag>(input_stream_, *this)) {
      token_type_ = kError;
    }

    return token_type_;
  }

  bool consume(TokenType expected)
  {
    if (token_type_ == expected) {
      token_type_ = kUnknown;
      return true;
    }
    return false;
  }

  // consume tokens until peek() would return 'expected'
  // skips over objects and arrays atomically
  bool skip_to(TokenType expected)
  {
    int skip_level = 0;

    while (peek() != kEnd) {
      if (skip_level == 0 && token_type_ == expected) {
        return true;
      }
      switch (token_type_) {
      case kStartArray:
      case kStartObject:
        ++skip_level;
        break;
      case kEndArray:
      case kEndObject:
        --skip_level;
        break;
      default:
        break;
      }
      consume(token_type_);
    }

    return false;
  }

  TokenType token_type_;
  InputStream& input_stream_;
  rapidjson::Reader reader_;
  bool bool_value_;
  int int_value_;
  unsigned int uint_value_;
  int64_t int64_value_;
  uint64_t uint64_value_;
  double double_value_;
  std::string string_value_;
  std::string key_value_;
};

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_struct(Extensibility /*extensibility*/)
{
  peek();
  return consume(kStartObject);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_struct()
{
  if (peek() == kKey) { // skip any unknown members at the end
    if (!skip_to(kEndObject)) {
      return false;
    }
  }
  return consume(kEndObject);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_struct_member(XTypes::MemberId& member_id, const MemberHelper& helper)
{
  while (members_remaining()) {
    consume(kKey);
    if (helper.get_value(member_id, key_value_.c_str())) {
      return true;
    }
    if (!skip_to(kKey)) { // skip unknown members when expecting a known member
      return false;
    }
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::members_remaining()
{
  return peek() == kKey;
}

// Should only be called on optional members.
template <typename InputStream>
bool JsonValueReader<InputStream>::member_has_value()
{
  if (peek() != kNull) {
    return true;
  }
  consume(kNull);
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_struct_member()
{
  return true;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_union(Extensibility /*extensibility*/)
{
  peek();
  return consume(kStartObject);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_union()
{
  peek();
  return consume(kEndObject);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_discriminator()
{
  if (peek() == kKey && key_value_ == "$discriminator") {
    return consume(kKey);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_discriminator()
{
  return true;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_union_member()
{
  if (peek() == kKey) {
    return consume(kKey);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_union_member()
{
  return true;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_array(XTypes::TypeKind /*elem_kind*/)
{
  peek();
  return consume(kStartArray);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_array()
{
  peek();
  return consume(kEndArray);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_sequence(XTypes::TypeKind /*elem_kind*/)
{
  peek();
  return consume(kStartArray);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::elements_remaining()
{
  peek();
  return token_type_ != kEndArray;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_sequence()
{
  peek();
  return consume(kEndArray);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_element()
{
  return true;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_element()
{
  return true;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_map()
{
  peek();
  return consume(kStartArray);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::pairs_remaining()
{
  peek();
  return token_type_ != kEndArray;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_pair()
{
  peek();
  return consume(kStartObject);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_pair_key()
{
  if (peek() == kKey) {
    return consume(kKey);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_pair_key()
{
  return true;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::begin_pair_value()
{
  if (peek() == kKey) {
    return consume(kKey);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_pair_value()
{
  return true;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_pair()
{
  peek();
  return consume(kEndObject);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::end_map()
{
  peek();
  return consume(kEndArray);
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_boolean(ACE_CDR::Boolean& value)
{
  if (peek() == kBool) {
    value = bool_value_;
    return consume(kBool);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_byte(ACE_CDR::Octet& value)
{
  if (peek() == kUint) {
    value = uint_value_;
    return consume(kUint);
  }
  return false;
}

#if OPENDDS_HAS_EXPLICIT_INTS
template <typename InputStream>
bool JsonValueReader<InputStream>::read_int8(ACE_CDR::Int8& value)
{
  switch (peek()) {
  case kInt:
    value = int_value_;
    return consume(kInt);
  case kUint:
    value = uint_value_;
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_uint8(ACE_CDR::UInt8& value)
{
  if (peek() == kUint) {
    value = uint_value_;
    return consume(kUint);
  }
  return false;
}
#endif

template <typename InputStream>
bool JsonValueReader<InputStream>::read_int16(ACE_CDR::Short& value)
{
  peek();
  switch (peek())  {
  case kInt:
    value = int_value_;
    return consume(kInt);
  case kUint:
    value = uint_value_;
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_uint16(ACE_CDR::UShort& value)
{
  if (peek() == kUint) {
    value = uint_value_;
    return consume(kUint);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_int32(ACE_CDR::Long& value)
{
  switch (peek()) {
  case kInt:
    value = int_value_;
    return consume(kInt);
  case kUint:
    value = uint_value_;
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_uint32(ACE_CDR::ULong& value)
{
  if (peek() == kUint) {
    value = uint_value_;
    return consume(kUint);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_int64(ACE_CDR::LongLong& value)
{
  switch (peek()) {
  case kInt64:
    value = int64_value_;
    return consume(kInt64);
  case kUint64:
    value = uint64_value_;
    return consume(kUint64);
  case kInt:
    value = int_value_;
    return consume(kInt);
  case kUint:
    value = uint_value_;
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_uint64(ACE_CDR::ULongLong& value)
{
  switch (peek()) {
  case kUint64:
    value = uint64_value_;
    return consume(kUint64);
  case kUint:
    value = uint_value_;
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_float32(ACE_CDR::Float& value)
{
  switch (peek()) {
  case kDouble:
    value = ACE_CDR::Float(double_value_);
    return consume(kDouble);
  case kUint64:
    value = ACE_CDR::Float(uint64_value_);
    return consume(kUint64);
  case kUint:
    value = ACE_CDR::Float(uint_value_);
    return consume(kUint);
  case kInt64:
    value = ACE_CDR::Float(int64_value_);
    return consume(kInt64);
  case kInt:
    value = ACE_CDR::Float(int_value_);
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_float64(ACE_CDR::Double& value)
{
  switch (peek()) {
  case kDouble:
    value = double_value_;
    return consume(kDouble);
  case kUint64:
    value = ACE_CDR::Double(uint64_value_);
    return consume(kUint64);
  case kUint:
    value = uint_value_;
    return consume(kUint);
  case kInt64:
    value = ACE_CDR::Double(int64_value_);
    return consume(kInt64);
  case kInt:
    value = int_value_;
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_float128(ACE_CDR::LongDouble& value)
{
  switch (peek()) {
  case kDouble:
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(value, double_value_);
    return consume(kDouble);
  case kUint64:
#ifdef ACE_WIN32
#  pragma warning(disable:4244)
#endif
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(value, uint64_value_);
#ifdef ACE_WIN32
#  pragma warning(default:4244)
#endif
    return consume(kUint64);
  case kUint:
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(value, uint_value_);
    return consume(kUint);
  case kInt64:
#ifdef ACE_WIN32
#  pragma warning(disable:4244)
#endif
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(value, int64_value_);
#ifdef ACE_WIN32
#  pragma warning(default:4244)
#endif
    return consume(kInt64);
  case kInt:
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(value, int_value_);
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_fixed(ACE_CDR::Fixed& value)
{
  if (peek() == kString) {
    value = ACE_CDR::Fixed::from_string(string_value_.c_str());
    return consume(kString);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_char8(ACE_CDR::Char& value)
{
  if (peek() == kString) {
    if (string_value_.length() == 1) {
      value = string_value_[0];
      return consume(kString);
    }
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_char16(ACE_CDR::WChar& value)
{
  if (peek() == kString) {
    rapidjson::StringStream source(string_value_.c_str());
    rapidjson::GenericStringBuffer<rapidjson::UTF16<> > target;

    while (source.Tell() != string_value_.size()) {
      if (!rapidjson::Transcoder<rapidjson::UTF8<>, rapidjson::UTF16<> >::Transcode(source, target)) {
        return false;
      }
    }

    if (target.GetLength() == 1) {
      value = target.GetString()[0];
      return consume(kString);
    }
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_string(std::string& value)
{
  if (peek() == kString) {
    value = string_value_;
    return consume(kString);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_wstring(std::wstring& value)
{
  if (peek() == kString) {
    rapidjson::StringStream source(string_value_.c_str());
    rapidjson::GenericStringBuffer<rapidjson::UTF16<> > target;

    while (source.Tell() != string_value_.size()) {
      if (!rapidjson::Transcoder<rapidjson::UTF8<>, rapidjson::UTF16<> >::Transcode(source, target)) {
        return false;
      }
    }

    value = target.GetString();
    return consume(kString);
  }
  return false;
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_long_enum(ACE_CDR::Long& value, const EnumHelper& helper)
{
  switch (peek()) {
  case kString:
    if (helper.get_value(value, string_value_.c_str())) {
      return consume(kString);
    }
    return false;
  case kInt:
    value = int_value_;
    return consume(kInt);
  case kUint:
    value = uint_value_;
    return consume(kUint);
  default:
    return false;
  }
}

template <typename InputStream>
bool JsonValueReader<InputStream>::read_bitmask(ACE_CDR::ULongLong& value, const BitmaskHelper& helper)
{
  switch (peek()) {
  case kString:
    value = string_to_bitmask(string_value_, helper);
    return consume(kString);
  case kInt:
    value = int_value_;
    return consume(kInt);
  case kUint:
    value = uint_value_;
    return consume(kUint);
  case kInt64:
    value = int64_value_;
    return consume(kInt64);
  case kUint64:
    value = uint64_value_;
    return consume(kUint64);
  default:
    return false;
  }
}

template<typename T, typename InputStream>
bool from_json(T& value, InputStream& stream)
{
  set_default(value);
  JsonValueReader<InputStream> jvr(stream);
  return vread(jvr, value);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

#endif  /* OPENDDS_DCPS_JSON_VALUE_READER_H */
