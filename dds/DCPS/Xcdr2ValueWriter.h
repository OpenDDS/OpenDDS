#ifndef OPENDDS_DCPS_XCDR2VALUEWRITER_H
#define OPENDDS_DCPS_XCDR2VALUEWRITER_H

#include "ValueWriter.h"
#include "PoolAllocator.h"

#include <stack>
#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export Xcdr2ValueWriter : public ValueWriter {
public:
  explicit Xcdr2ValueWriter(const Encoding& encoding)
    : mode_(SERIALIZATION_SIZE_MODE)
    , encoding_(encoding)
    , pos_(0)
    , ser_(0)
  {
    if (encoding.xcdr_version() != Encoding::XCDR_VERSION_2) {
      throw std::runtime_error("Xcdr2ValueWriter only supports XCDR2");
    }
  }

  bool begin_struct(Extensibility extensibility);
  bool end_struct();
  bool begin_struct_member(MemberParam params);
  bool end_struct_member();

  bool begin_union(Extensibility extensibility);
  bool end_union();
  bool begin_discriminator(MemberParam params);
  bool end_discriminator();
  bool begin_union_member(MemberParam params);
  bool end_union_member();

  bool begin_array(XTypes::TypeKind elem_kind);
  bool end_array();
  bool begin_sequence(XTypes::TypeKind elem_kind, ACE_CDR::ULong length);
  bool end_sequence();
  bool begin_element(ACE_CDR::ULong idx);
  bool end_element();

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
  bool write_wstring(const ACE_CDR::WChar* value, size_t length);
  bool write_enum(ACE_CDR::Long value, const EnumHelper& helper);
  bool write_bitmask(ACE_CDR::ULongLong value, const BitmaskHelper& helper);
  bool write_absent_value();

  bool write_boolean_array(const ACE_CDR::Boolean* x, ACE_CDR::ULong length);
  bool write_byte_array(const ACE_CDR::Octet* x, ACE_CDR::ULong length);
#if OPENDDS_HAS_EXPLICIT_INTS
  bool write_int8_array(const ACE_CDR::Int8* x, ACE_CDR::ULong length);
  bool write_uint8_array(const ACE_CDR::UInt8* x, ACE_CDR::ULong length);
#endif
  bool write_int16_array(const ACE_CDR::Short* x, ACE_CDR::ULong length);
  bool write_uint16_array(const ACE_CDR::UShort* x, ACE_CDR::ULong length);
  bool write_int32_array(const ACE_CDR::Long* x, ACE_CDR::ULong length);
  bool write_uint32_array(const ACE_CDR::ULong* x, ACE_CDR::ULong length);
  bool write_int64_array(const ACE_CDR::LongLong* x, ACE_CDR::ULong length);
  bool write_uint64_array(const ACE_CDR::ULongLong* x, ACE_CDR::ULong length);
  bool write_float32_array(const ACE_CDR::Float* x, ACE_CDR::ULong length);
  bool write_float64_array(const ACE_CDR::Double* x, ACE_CDR::ULong length);
  bool write_float128_array(const ACE_CDR::LongDouble* x, ACE_CDR::ULong length);
  bool write_char8_array(const ACE_CDR::Char* x, ACE_CDR::ULong length);
  bool write_char16_array(const ACE_CDR::WChar* x, ACE_CDR::ULong length);

  size_t get_serialized_size() const;
  const OPENDDS_VECTOR(size_t)& get_serialized_sizes() const;

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
  bool begin_ssize_complex(Extensibility extensiblity, CollectionKind ck);
  bool end_ssize_complex();
  bool begin_ssize_aggregated_member(bool optional, bool present);

  // Common internal methods for serialization.
  bool begin_serialize_complex(Extensibility extensibility, CollectionKind ck, ACE_CDR::ULong seq_length);
  bool end_serialize_complex();
  bool begin_serialize_aggregated_member(unsigned id, bool must_understand, bool optional, bool present);

  // Argument seq_length is only used if the type that triggers this call is a sequence.
  bool begin_complex(Extensibility extensibility, CollectionKind ck = NOT_COLLECTION_KIND,
                     ACE_CDR::ULong seq_length = 0);
  bool end_complex();
  bool begin_aggregated_member(unsigned id, bool must_understand, bool optional, bool present);

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

  OPENDDS_STACK(SerializedSizeState) size_states_;

  // Record the total size of the top-level type and its members if needed.
  // The size of a member is only recorded if it is required by a header,
  // either a Dheader or an Emheader/Nextint. The total size of the top-level
  // type is at index zero. The subsequent values are in the same order as
  // the order that the members are serialized in the byte stream.
  OPENDDS_VECTOR(size_t) size_cache_;

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
