#include "SerializedSizeValueWriter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void SerializedSizeValueWriter::begin_aggregated(DDS::ExtensibilityKind extensibility)
{
  // This is where we keep a bookmark for when the aggregated value starts in the stream.
  // To derive the size of the whole struct or union, we need to
  // find out the size of paddings in front of the Dheader.
  //
  // - If this is the top-level type:
  //   + There is NO padding regardless whether it is appendable or mutable.
  //   + If this type is final, there is no Dheader for the top-level type.
  //     The nested members still need to compute the total size if they are appendable or mutable.
  //
  // - If this is a member of a mutable containing type, there is NO padding ahead of the Dheader
  //   since it is preceded by the Emheader or Nextint which is already aligned.
  //
  // - If this is a member of final or appendable type, there CAN BE paddings before it begins,
  //   i.e., before its Dheader in case it's appendable or mutable, or before its first member
  //   in case it's final.
  //   Figuring out the total size of the struct without the paddings in front of the Dheaher may
  //   require a different scheme than currently provided by Serializer.
  //
  // For mutable struct, each member needs its own entry in state_ and its size cached,
  // even if the member is not nested type. This is because its size is needed for Emheader.
  // For final or appendable struct, only member which is an appendable or mutable struct (or
  // in general type that requires Dheader) will need its own entry in state_ and in the cache.

  // Always create a new Metadata instance when encountering a struct.
  // But only struct whose size is needed, in order to fill its Dheader or
  // the Emheader (and possibly Nextint) in case it's a member of a mutable
  // struct, has a cache entry for its size.
  // Also, for mutable struct, each of its members has its size cached,
  // regardless of whether it is a nested member or not.
  if (!state_.empty()) {
    const DDS::ExtensibilityKind enclosing_ek = state_.top().extensibility;
    if (enclosing_ek == DDS::FINAL || enclosing_ek == DDS::APPENDABLE) {
      if (extensibility == DDS::FINAL) {
        // We don't need to compute and cache the size for this struct.
        // Instead, what we need is the total size of the closest ancestor type for which
        // the size is required by a Dheader or a Emheader.
        // Copy the current size so that the total size can be built up continuously
        // in the deeper nesting levels.
        Metadata metadata(extensibility);
        metadata.total_size = state_.top().total_size;
        state_.push(metadata);
        return;
      } else {
        // The alignment before the Dheader is accounted in size of the containing type.
        // And then we can compute the size of this struct separately.
        encoding_.align(state_.top().total_size, uint32_cdr_size);
        state_.push(Metadata(extensibility));
        serialized_size_delimiter(encoding_, state_.top().total_size);

        // Preserve a slot to cache the size of this struct.
        // The actual size will be written in end_struct.
        size_cache_.push_back(0);
        state_.top().cache_pos = size_cache_.size() - 1;
        return;
      }
    } else { // Enclosing struct is MUTABLE.
      // Regardless of the extensibility of this struct, we must track its size for the
      // corresponding Emheader in the enclosing struct.
      // Since this is a member of a mutable struct, there was a call to
      // serialized_size_parameter_id before this which already accounts for any alignment
      // ahead of this struct (more precisely, the alignment happens after the previous member).
      state_.push(Metadata(extensibility));
      if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
        serialized_size_delimiter(encoding_, state_.top().total_size);
      }
      size_cache_.push_back(0);
      state_.top().cache_pos = size_cache_.size() - 1;
    }
  } else {
    // Top-level type
    state_.push(Metadata(extensibility));
    if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
      serialized_size_delimiter(encoding_, state_.top().total_size);
    }

    size_cache_.push_back(0);
    state_.top().cache_pos = size_cache_.size() - 1;
  }
}

void SerializedSizeValueWriter::end_aggregated()
{
  Metadata& state = state_.top();
  const DDS::ExtensibilityKind extensibility = state.extensibility;

  if (extensibility == DDS::MUTABLE) {
    serialized_size_list_end_parameter_id(encoding_, state.total_size, state.mutable_running_total);
  }

  const size_t total_size = state.total_size;
  const size_t pos = state.cache_pos;
  state_.pop();

  if (!state_.empty()) {
    const DDS::ExtensibilityKind enclosing_ek = state_.top().extensibility;
    if (enclosing_ek == DDS::FINAL || enclosing_ek == DDS::APPENDABLE) {
      if (extensibility == DDS::FINAL) {
        // Copy the accumulated size back to the entry for the containing type.
        state_.top().total_size = total_size;
        return;
      } else {
        // The total size of this struct is ready, now update the total size of
        // the containing struct. Also update the cached size for this struct.
        size_cache_[pos] = total_size;
        state_.top().total_size += total_size;
      }
    } else { // Enclosing struct is MUTABLE.
      size_cache_[pos] = total_size;
      state_.top().total_size += total_size;
    }
  } else {
    // We have done working through the whole thing.
    size_cache_[pos] = total_size;
  }
}

void SerializedSizeValueWriter::begin_aggregated_member(bool optional, bool present = true)
{
  Metadata& state = state_.top();
  const DDS::ExtensibilityKind extensibility = state.extensibility;
  size_t& total_size = state.total_size;
  size_t& mutable_running_total = state.mutable_running_total;

  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    primitive_serialized_size_boolean(encoding_, total_size);
    return;
  }
  if (extensibility == DDS::MUTABLE && present) {
    serialized_size_parameter_id(encoding_, total_size, mutable_running_total);
  }
}

void SerializedSizeValueWriter::begin_struct(DDS::ExtensibilityKind extensibility)
{
  begin_aggregated(extensibility);
}

void SerializedSizeValueWriter::end_struct()
{
  end_aggregated();
}

void SerializedSizeValueWriter::begin_struct_member(const char* /*name*/, bool optional,
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
  begin_aggregated_member(0, false);
}

void SerializedSizeValueWriter::end_discriminator()
{
  return;
}

void SerializedSizeValueWriter::begin_union_member(const char* /*name*/, bool optional,
                                                   bool present = true)
{
  begin_aggregated_member(0, optional, present);
}

void SerializedSizeValueWriter::end_union_member()
{
  return;
}

// Array can be treated similar to final/appendable struct with each element is a member.
// If the element type is primitive, it is similar to final struct.
// If the element type is not primitive, it is similar to appendable struct with Dheader.
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

// Sequence is similar to final or appendable struct with each element is a member.
// If element type is primitive, it can be treated as final struct which has no Dheader.
// If element type is not primitive, it can be treated as appendable struct which has Dheader.
// In both case, elements are serialized back-to-back similarly to final/appendable struct.
// One difference is that sequence has length ahead of all the elements.
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

// When this is a member of a mutable struct, its size needs to be recorded
// so that we can write the Emheader (and Nextint) later.
void SerializedSizeValueWriter::write_boolean(ACE_CDR::Boolean /*value*/)
{
  primitive_serialized_size_boolean(encoding_, state_.top().total_size);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cache_.push_back(boolean_cdr_size);
  }
}

void SerializedSizeValueWriter::write_byte(ACE_CDR::Octet /*value*/)
{
  primitive_serialized_size_octet(encoding_, state_.top().total_size);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(byte_cdr_size);
  }
}

#if OPENDDS_HAS_EXPLICIT_INTS
void SerializedSizeValueWriter::write_int8(ACE_CDR::Int8 /*value*/)
{
  primitive_serialized_size_int8(encoding_, state_.top().total_size);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(int8_cdr_size);
  }
}

void SerializedSizeValueWriter::write_uint8(ACE_CDR::UInt8 /*value*/)
{
  primitive_serialized_size_uint8(encoding_, state_.top().total_size);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(uint8_cdr_size);
  }
}
#endif

void SerializedSizeValueWriter::write_int16(ACE_CDR::Short value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(int16_cdr_size);
  }
}

void SerializedSizeValueWriter::write_uint16(ACE_CDR::UShort value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(uint16_cdr_size);
  }
}

void SerializedSizeValueWriter::write_int32(ACE_CDR::Long value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(int32_cdr_size);
  }
}

void SerializedSizeValueWriter::write_uint32(ACE_CDR::ULong value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(uint32_cdr_size);
  }
}

void SerializedSizeValueWriter::write_int64(ACE_CDR::LongLong value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(int64_cdr_size);
  }
}

void SerializedSizeValueWriter::write_uint64(ACE_CDR::ULongLong value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(uint64_cdr_size);
  }
}

void SerializedSizeValueWriter::write_float32(ACE_CDR::Float value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(float32_cdr_size);
  }
}

void SerializedSizeValueWriter::write_float64(ACE_CDR::Double value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(float64_cdr_size);
  }
}

void SerializedSizeValueWriter::write_float128(ACE_CDR::LongDouble value)
{
  primitive_serialized_size(encoding_, state_.top().total_size, value);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(float128_cdr_size);
  }
}

void SerializedSizeValueWriter::write_fixed(const OpenDDS::FaceTypes::Fixed& /*value*/)
{
  return;
}

void SerializedSizeValueWriter::write_char8(ACE_CDR::Char /*value*/)
{
  primitive_serialized_size_char(encoding_, state_.top().total_size);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(char8_cdr_size);
  }
}

void SerializedSizeValueWriter::write_char16(ACE_CDR::WChar /*value*/)
{
  primitive_serialized_size_wchar(encoding_, state_.top().total_size);

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(char16_cdr_size);
  }
}

void SerializedSizeValueWriter::write_string(const ACE_CDR::Char* value, size_t length)
{
  size_t& size = state_.top().total_size;
  primitive_serialized_size_ulong(encoding_, size);
  if (value) {
    size += length + 1; // Include null termination
  }

  if (state_.top().extensibility == DDS::MUTABLE) {
    // It's safe to do this since before every member of a mutable type,
    // the total_size variable is set to zero.
    size_cached_.push_back(size);
  }
}

void SerializedSizeValueWriter::write_wstring(const ACE_CDR::WChar* value, size_t length)
{
  size_t& size = state_.top().total_size;
  primitive_serialized_size_ulong(encoding_, size);
  if (value) {
    size += length * char16_cdr_size; // Not include null termination
  }

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(size);
  }
}

void SerializedSizeValueWriter::write_enum(const char* /*name*/, ACE_CDR::Long /*value*/,
                                           DDS::TypeKind treat_as = TK_INT32)
{
  size_t& size = state_.top().total_size;
  switch (treat_as) {
  case TK_INT8:
    primitive_serialized_size_int8(encoding_, size);
    break;
  case TK_INT16:
    primitive_serialized_size(encoding_, size, ACE_CDR::Short());
    break;
  case TK_INT32:
    primitive_serialized_size(encoding_, size, ACE_CDR::Long());
    break;
  default:
    return;
  }

  if (state_.top().extensibility == DDS::MUTABLE) {
    size_cached_.push_back(size);
  }
}

void SerializedSizeValueWriter::write_absent_value()
{
  return;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
