#ifndef OPENDDS_DCPS_SERIALIZED_SIZE_VALUE_WRITER_H
#define OPENDDS_DCPS_SERIALIZED_SIZE_VALUE_WRITER_H

#include "ValueWriter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SerializedSizeValueWriter : public ValueWriter {
public:
  explicit SerializedSizeValueWriter(Serializer& ser)
    : encoding_(ser.encoding())
    , size_(0)
  {}

  void begin_struct(DDS::ExtensibilityKind ek);
  void end_struct();
  void begin_struct_member(const DDS::MemberDescriptor& descriptor, bool present = true);
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
  void write_enum(const char* name, ACE_CDR::Long value);
  void write_absent_value();

  size_t get_serialized_size() { return size_; }

private:
  const Encoding& encoding_;
  size_t size_;

  // States needed when computing the size of a nested member.
  struct Metadata {
    explicit Metadata(DDS::ExtensibilityKind ek)
      : extensibility(ek)
      , mutable_running_total(0)
      , cache_pos(0)
    {}

    DDS::ExtensibilityKind extensibility;
    size_t mutable_running_total;
    size_t cache_pos;
  };

  std::stack<Metadata> state_;

  // struct ComponentSize {};

  // struct DheaderContents : public ComponentSize {
  //   explicit DheaderContents(size_t val) : value(val) {}
  //   size_t value;
  // };

  // struct NonNestedSize : public ComponentSize {
  //   explicit NonNestedSize(size_t val) : value(val) {}
  //   size_t value;
  // };

  // struct NestedSize : public ComponentSize {
  //   vector<ComponentSize> values;
  // }

  // Record the sizes of the components in the byte stream.
  // The following components will be recorded:
  // - The total size of a struct, union, sequence, array, including the size for Dheader.
  //   (The write_delimiter function in Serializer will subtract the size for the Dheader itself.)
  //   This is used to write the Dheader.
  // - Size of each individual member.
  //   + If the member is not nested (e.g., scalar types), its size is recorded directly.
  //   + If the member is nested (i.e., struct, union, sequence, array), the component sizes
  //     for this member is recorded recursively using these two kinds of components above.
  vector<size_t> size_cache_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_SERIALIZED_SIZE_VALUE_WRITER_H
