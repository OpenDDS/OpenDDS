#include "SerializedSizeValueWriter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void SerializedSizeValueWriter::begin_aggregated(DDS::ExtensibilityKind extensibility)
{
}

void SerializedSizeValueWriter::end_aggregated()
{
}

void SerializedSizeValueWriter::begin_aggregated_member(bool optional, bool present = true)
{
  const DDS::ExtensibilityKind extensibility = state_.top().extensibility;
  size_t& mutable_running_total = state_.top().mutable_running_total;

  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    primitive_serialized_size_boolean(encoding_, size_);
    return;
  }
  if (extensibility == DDS::MUTABLE && present) {
    serialized_size_parameter_id(encoding_, size_, mutable_running_total);
  }
}

void SerializedSizeValueWriter::begin_struct(DDS::ExtensibilityKind extensibility)
{
  // TODO(sonndinh):
  // This is where we keep a bookmark for when the aggregated value starts in the stream.
  // To derive the size of the whole struct or union, we need to
  // find out the size of paddings in front of the Dheader.
  //
  // - If this is the top-level type:
  //   + There is NO padding regardless whether it is appendable or mutable.
  //   + If this type is final, there is NO Dheader.
  //
  // - If this is a member of a mutable containing type, there is NO padding ahead of the Dheader
  //   since it is preceded by the Emheader or Nextint which is already aligned.
  //
  // - If this is a member of final or appendable type, there CAN BE paddings before it begins,
  //   i.e., before its Dheader in case it's appendable or mutable, or before its first member
  //   in case it's final (final doesn't have Dheader so we don't worry about this case).
  //   Figuring out the total size of the struct without the paddings in front of the Dheaher may
  //   require a different scheme than currently provided by Serializer.

  state_.push(Metadata(extensibility));
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    serialized_size_delimiter(encoding_, size_);
  }

  // TODO(sonndinh): Insert an entry for the total size of the struct excluding the Dheader.
  // This is used to write the Dheader of the struct. This is only a placeholder.
  // end_struct will fill the actual value when the caller has gone through all members.
  size_cache_.push_back(0);
  state_.top().cache_pos = size_cache_.size() - 1;
}

void SerializedSizeValueWriter::end_struct()
{
  // TODO(sonndinh):
  // This is where we know when the aggregated value ends in the stream, and thus
  // can compute the total size the value.
  const DDS::ExtensibilityKind extensibility = state_.top().extensibility;
  size_t& mutable_running_total = state_.top().mutable_running_total;
  if (extensibility_ == DDS::MUTABLE) {
    serialized_size_list_end_parameter_id(encoding_, size_, mutable_runing_total);
  }
  state_.pop();

  // TODO(sonndinh): Fill the entry for the accumulated size of the struct.
  size_t pos = state_.top().cache_pos;
}

void SerializedSizeValueWriter::begin_struct_member(const char* name, bool optional,
                                                    bool present = true)
{
  begin_aggregated_member(optional, present);
}

void SerializedSizeValueWriter::end_struct_member()
{
  return;
}

void SerializedSizeValueWriter::begin_union(DDS::ExtensibilityKind extensibility)
{
  begin_aggregated(extensibility);
}

void SerializedSizeValueWriter::end_union()
{
  end_aggregated();
}

void SerializedSizeValueWriter::begin_discriminator()
{
  const DDS::ExtensibilityKind extensibility = state_.top().extensibility;
  size_t& mutable_running_total = state_.top().mutable_running_total;
  if (extensibility == DDS::MUTABLE) {
    serialized_size_parameter_id(encoding_, size_, mutable_running_total);
  }
}

void SerializedSizeValueWriter::end_discriminator()
{
  return;
}

void SerializedSizeValueWriter::begin_union_member(const char* /*name*/, bool optional,
                                                   bool present = true)
{
  begin_aggregated_member(optional(), present);
}

void SerializedSizeValueWriter::end_union_member()
{
  return;
}

void SerializedSizeValueWriter::begin_array(DDS::TypeKind elem_tk)
{
  if (!is_primitive(elem_tk)) {
    serialized_size_delimiter(encoding_, size_); // Dheader
  }
}

void SerializedSizeValueWriter::end_array()
{
  return;
}

void SerializedSizeValueWriter::begin_sequence(DDS::TypeKind elem_tk)
{
  if (!is_primitive(elem_tk)) {
    serialized_size_delimiter(encoding_, size_); // Dheader
  }
  primitive_serialized_size_ulong(encoding_, size_); // Length
}

void SerializedSizeValueWriter::end_sequence()
{
  return;
}

void SerializedSizeValueWriter::begin_element()
{
  return;
}

void SerializedSizeValueWriter::end_element()
{
  return;
}

void SerializedSizeValueWriter::write_boolean(ACE_CDR::Boolean /*value*/)
{
  primitive_serialized_size_boolean(encoding_, size_);
  size_cache_.push_back(boolean_cdr_size);
}

void SerializedSizeValueWriter::write_byte(ACE_CDR::Octet /*value*/)
{
  primitive_serialized_size_octet(encoding_, size_);
  size_cached_.push_back(byte_cdr_size);
}

#if OPENDDS_HAS_EXPLICIT_INTS
void SerializedSizeValueWriter::write_int8(ACE_CDR::Int8 /*value*/)
{
  primitive_serialized_size_int8(encoding_, size_);
}

void SerializedSizeValueWriter::write_uint8(ACE_CDR::UInt8 /*value*/)
{
  primitive_serialized_size_uint8(encoding_, size_);
}
#endif

void SerializedSizeValueWriter::write_int16(ACE_CDR::Short value)
{
  primitive_serialized_size(encoding_, size_, value);
}

void SerializedSizeValueWriter::write_uint16(ACE_CDR::UShort value)
{
  primitive_serialized_size(encoding_, size, value);
}

void SerializedSizeValueWriter::write_int32(ACE_CDR::Long value)
{
  primitive_serialized_size(encoding_, size, value);
}

void SerializedSizeValueWriter::write_uint32(ACE_CDR::ULong value)
{
  primitive_serialized_size(encoding_, size, value);
}

void SerializedSizeValueWriter::write_int64(ACE_CDR::LongLong value)
{
  primitive_serialized_size(encoding_, size, value);
}

void SerializedSizeValueWriter::write_uint64(ACE_CDR::ULongLong value)
{
  primitive_serialized_size(encoding_, size, value);
}

void SerializedSizeValueWriter::write_float32(ACE_CDR::Float value)
{
  primitive_serialized_size(encoding_, size, value);
}

void SerializedSizeValueWriter::write_float64(ACE_CDR::Double value)
{
  primitive_serialized_size(encoding_, size, value);
}

void SerializedSizeValueWriter::write_float128(ACE_CDR::LongDouble value)
{
  primitive_serialized_size(encoding_, size, value);
}

void SerializedSizeValueWriter::write_fixed(const OpenDDS::FaceTypes::Fixed& /*value*/)
{
  return;
}

void SerializedSizeValueWriter::write_char8(ACE_CDR::Char /*value*/)
{
  primitive_serialized_size_char(encoding_, size_);
}

void SerializedSizeValueWriter::write_char16(ACE_CDR::WChar /*value*/)
{
  primitive_serialized_size_wchar(encoding_, size_);
}

void SerializedSizeValueWriter::write_string(const ACE_CDR::Char* value, size_t length)
{
  primitive_serialized_size_ulong(encoding_, size_);
  if (value) {
    size_ += length + 1; // Include null termination
  }
}

void SerializedSizeValueWriter::write_wstring(const ACE_CDR::WChar* value, size_t length)
{
  primitive_serialized_size_ulong(encoding_, size_);
  if (value) {
    size_ += length * char16_cdr_size; // Not include null termination
  }
}

void SerializedSizeValueWriter::write_enum(const char* /*name*/, ACE_CDR::Long /*value*/,
                                           DDS::TypeKind treat_as = TK_INT32)
{
  switch (treat_as) {
  case TK_INT8:
    primitive_serialized_size_int8(encoding_, size_);
    break;
  case TK_INT16:
    primitive_serialized_size(encoding_, size_, ACE_CDR::Short());
    break;
  case TK_INT32:
    primitive_serialized_size(encoding_, size_, ACE_CDR::Long());
    break;
  default:
    break;
  }
}

void SerializedSizeValueWriter::write_absent_value()
{
  return;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
