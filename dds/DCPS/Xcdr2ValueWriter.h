#ifndef OPENDDS_DCPS_XCDR2VALUEWRITER_H
#define OPENDDS_DCPS_XCDR2VALUEWRITER_H

#include "ValueWriter.h"

#include <vector>
#include <stack>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export Xcdr2ValueWriter : public ValueWriter {
public:
  explicit Xcdr2ValueWriter(Encoding& encoding)
    : mode_(SERIALIZATION_SIZE_MODE)
    , encoding_(encoding)
    , pos_(0)
    , ser_(0)
  {}

  void begin_struct(Extensibility extensibility);
  void end_struct();
  void begin_struct_member(VWriterMemberParam params);
  void end_struct_member();

  void begin_union(Extensibility extensibility);
  void end_union();
  void begin_discriminator(VWriterMemberParam params);
  void end_discriminator();
  void begin_union_member(VWriterMemberParam params);
  void end_union_member();

  void begin_array(XTypes::TypeKind elem_kind);
  void end_array();
  void begin_sequence(XTypes::TypeKind elem_kind, ACE_CDR::ULong length);
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
  void write_enum(const char* name, ACE_CDR::Long value, XTypes::TypeKind as_int = XTypes::TK_INT32);
  void write_absent_value();

  void write_boolean_array(const ACE_CDR::Boolean* x, size_t length);
  void write_byte_array(const ACE_CDR::Octet* x, size_t length);
#if OPENDDS_HAS_EXPLICIT_INTS
  void write_int8_array(const ACE_CDR::Int8* x, size_t length);
  void write_uint8_array(const ACE_CDR::UInt8* x, size_t length);
#endif
  void write_int16_array(const ACE_CDR::Short* x, size_t length);
  void write_uint16_array(const ACE_CDR::UShort* x, size_t length);
  void write_int32_array(const ACE_CDR::Long* x, size_t length);
  void write_uint32_array(const ACE_CDR::ULong* x, size_t length);
  void write_int64_array(const ACE_CDR::LongLong* x, size_t length);
  void write_uint64_array(const ACE_CDR::ULongLong* x, size_t length);
  void write_float32_array(const ACE_CDR::Float* x, size_t length);
  void write_float64_array(const ACE_CDR::Double* x, size_t length);
  void write_float128_array(const ACE_CDR::LongDouble* x, size_t length);
  void write_char8_array(const ACE_CDR::Char* x, size_t length);
  void write_char16_array(const ACE_CDR::WChar* x, size_t length);

  size_t get_serialized_size() const;
  const std::vector<size_t>& get_serialized_sizes() const;

  // Reset the operation mode to only computing serialization size.
  void reset()
  {
    mode_ = SERIALIZATION_SIZE_MODE;
    pos_ = 0;
    ser_ = 0;
  }

  // Switch the operation mode to serializing to a byte stream.
  void set_serializer(Serializer* ser)
  {
    mode_ = SERIALIZATION_MODE;
    pos_ = 0;
    ser_ = ser;
  }

private:
  enum CollectionKind { SEQUENCE_KIND, ARRAY_KIND, NOT_COLLECTION_KIND };

  // Common internal methods for computing serialized sizes.
  void begin_ssize_complex(Extensibility extensiblity, CollectionKind ck);
  void end_ssize_complex();
  void begin_ssize_aggregated_member(bool optional, bool present);

  // Common internal methods for serialization.
  void begin_serialize_complex(Extensibility extensibility, CollectionKind ck, ACE_CDR::ULong seq_length);
  void end_serialize_complex();
  void begin_serialize_aggregated_member(unsigned id, bool must_understand, bool optional, bool present);

  // Argument seq_length is only used if the type that triggers this call is a sequence.
  void begin_complex(Extensibility extensibility, CollectionKind ck = NOT_COLLECTION_KIND,
                     ACE_CDR::ULong seq_length = 0);
  void end_complex();
  void begin_aggregated_member(unsigned id, bool must_understand, bool optional, bool present);

  enum OperationMode { SERIALIZATION_SIZE_MODE, SERIALIZATION_MODE };

  OperationMode mode_;

  const Encoding& encoding_;

  // Maintain the states necessary to compute and cache the sizes of
  // the top-level type or its nested members.
  struct SerializedSizeState {
    SerializedSizeState(Extensibility exten, CollectionKind ck)
      : extensibility(exten)
      , total_size(0)
      , mutable_running_total(0)
      , collection_kind(ck)
      , cache_pos(0)
    {}

    // Extensibility of the corresponding member.
    Extensibility extensibility;

    // The total size of the corresponding member, including delimiter if any.
    size_t total_size;

    // Only used for mutable members.
    size_t mutable_running_total;

    // Help determine whether we encounter the outermost dimension of an array.
    CollectionKind collection_kind;

    // Position in the size cache to store the size of this member.
    size_t cache_pos;
  };

  std::stack<SerializedSizeState> size_states_;

  // Record the total size of the top-level type and its members if needed.
  // The size of a member is only recorded if it is required by a header,
  // either a Dheader or an Emheader/Nextint. The total size of the top-level
  // type is at index zero. The subsequent values are in the same order as
  // the order that the members are serialized in the byte stream.
  std::vector<size_t> size_cache_;

  // Current position in the size cache from which the size
  // can be obtained to write to the byte stream.
  size_t pos_;

  struct SerializeState {
    SerializeState(Extensibility exten, CollectionKind ck)
      : extensibility(exten)
      , collection_kind(ck)
    {}

    Extensibility extensibility;
    CollectionKind collection_kind;
  };

  std::stack<SerializeState> serialize_states_;

  Serializer* ser_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_XCDR2_VALUE_WRITER_H