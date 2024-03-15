/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "Xcdr2ValueWriter.h"
#include "debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// Called whenever we encounter a struct, union, sequence, or array.
// When that happens, create a new instance of SerializedSizeState that stores the states of
// the byte stream and the information of the type encountered, including its kind
// and extensibility that helps decide when a header, e.g. Dheader or Emheader, is
// required. If a header is needed, a position in the size cache is also stored so
// that the actual size will be written when it is determined (see end_ssize_complex).
bool Xcdr2ValueWriter::begin_ssize_complex(Extensibility extensibility, CollectionKind coll_kind)
{
  SerializedSizeState state(extensibility, coll_kind);
  bool must_cache_size = true;

  if (!size_states_.empty()) {
    const Extensibility enclosing_ek = size_states_.top().extensibility;
    if (enclosing_ek == FINAL || enclosing_ek == APPENDABLE) {
      if (extensibility == FINAL) {
        // We don't need to compute and cache the size for final extensibility type.
        // Instead, what we need is the total size of the closest ancestor type
        // for which the size is required, for example, by a Dheader or a Emheader.
        // Copy the current size from the enclosing type so that the total size
        // can be built up cumulatively by this type.
        state.total_size = size_states_.top().total_size;
        if (coll_kind == SEQUENCE_KIND) { // Sequence length
          primitive_serialized_size_ulong(encoding_, state.total_size);
        }
        must_cache_size = false;
      } else {
        // The alignment in front of the Dheader is accounted to the size of the
        // containing type. Then we can compute the size of this type separately.
        encoding_.align(size_states_.top().total_size, uint32_cdr_size);
        serialized_size_delimiter(encoding_, state.total_size);
        if (coll_kind == SEQUENCE_KIND) { // Sequence length
          primitive_serialized_size_ulong(encoding_, state.total_size);
        }
      }
    } else { // Enclosing type is mutable.
      // Regardless of the extensibility of this type, we must track its size
      // for the corresponding Emheader/Nextint in the enclosing type.
      // Since this is a member of a mutable type, there was a call to
      // serialized_size_parameter_id before this which already accounts for
      // any alignment in front of the Emheader for this member.
      // For members which are not struct, union, sequence, or array, the size
      // of the member is cached directly in the write_* methods.
      if (extensibility == APPENDABLE || extensibility == MUTABLE) {
        serialized_size_delimiter(encoding_, state.total_size);
      }
      if (coll_kind == SEQUENCE_KIND) { // Sequence length
        primitive_serialized_size_ulong(encoding_, state.total_size);
      }
    }
  } else {
    // We are starting from the top-level type.
    // Clear the size cache in case it has data from a previous vwrite call.
    size_cache_.clear();
    if (extensibility == APPENDABLE || extensibility == MUTABLE) {
      serialized_size_delimiter(encoding_, state.total_size);
    }
  }

  if (must_cache_size) {
    size_cache_.push_back(0);
    state.cache_pos = size_cache_.size() - 1;
  }
  size_states_.push(state);
  return true;
}

bool Xcdr2ValueWriter::end_ssize_complex()
{
  SerializedSizeState& state = size_states_.top();
  const Extensibility extensibility = state.extensibility;

  if (extensibility == MUTABLE) {
    serialized_size_list_end_parameter_id(encoding_, state.total_size, state.mutable_running_total);
  }

  const size_t total_size = state.total_size;
  const size_t pos = state.cache_pos;
  size_states_.pop();

  if (!size_states_.empty()) {
    const Extensibility enclosing_ek = size_states_.top().extensibility;
    if (enclosing_ek == FINAL || enclosing_ek == APPENDABLE) {
      if (extensibility == FINAL) {
        // Since the computed total size was built on top of the size of the
        // containing type, we can now copy it back to the entry of the containing type.
        size_states_.top().total_size = total_size;
      } else {
        // The total size is ready, now update the total size of the containing type.
        // Also update the size of this type in the size cache.
        size_cache_[pos] = total_size;
        size_states_.top().total_size += total_size;
      }
    } else { // Enclosing type is mutable.
      size_cache_[pos] = total_size;
      size_states_.top().total_size += total_size;
    }
  } else {
    // We have finished working through the whole thing.
    size_cache_[pos] = total_size;
  }
  return true;
}

bool Xcdr2ValueWriter::begin_ssize_aggregated_member(bool optional, bool present)
{
  SerializedSizeState& state = size_states_.top();
  const Extensibility extensibility = state.extensibility;
  size_t& total_size = state.total_size;
  size_t& mutable_running_total = state.mutable_running_total;

  if (optional && (extensibility == FINAL || extensibility == APPENDABLE)) {
    primitive_serialized_size_boolean(encoding_, total_size);
  }
  if (extensibility == MUTABLE && present) {
    serialized_size_parameter_id(encoding_, total_size, mutable_running_total);
  }
  return true;
}

bool Xcdr2ValueWriter::begin_serialize_complex(Extensibility extensibility, CollectionKind ck,
                                               ACE_CDR::ULong seq_length)
{
  if (!ser_) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Xcdr2ValueWriter::begin_serialize_complex:"
                 " Serializer object is not available\n"));
    }
    return false;
  }

  if (serialize_states_.empty() && extensibility == FINAL) {
    // The first entry is the total size and does not correspond to any header.
    ++pos_;
  }

  if (!serialize_states_.empty()) {
    const Extensibility enclosing_ek = serialize_states_.top().extensibility;
    if (extensibility == APPENDABLE || extensibility == MUTABLE) {
      if (enclosing_ek == MUTABLE) {
        // The same size is used for both the member header of the enclosing mutable type
        // and the Dheader of this struct. pos_ was proactively increased by a preceding
        // call to begin_*_member of the containing type.
        --pos_;
      }
    }
  }

  // Write Dheader
  if (extensibility == APPENDABLE || extensibility == MUTABLE)  {
    if (pos_ >= size_cache_.size()) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Xcdr2ValueWriter::begin_serialize_complex:"
                   " Size value is not available at index %u. Cache length: %u\n",
                   pos_, size_cache_.size()));
      }
      return false;
    }
    if (!ser_->write_delimiter(size_cache_.at(pos_))) {
      return false;
    }
    ++pos_;
  }

  if (ck == SEQUENCE_KIND) {
    if (!(*ser_ << seq_length)) {
      return false;
    }
  }

  serialize_states_.push(SerializeState(extensibility, ck));
  return true;
}

bool Xcdr2ValueWriter::end_serialize_complex()
{
  if (serialize_states_.top().extensibility == MUTABLE) {
    if (!ser_->write_list_end_parameter_id()) {
      return false;
    }
  }
  serialize_states_.pop();

  // This is needed if the same sample is serialized more than once.
  if (serialize_states_.empty()) {
    pos_ = 0;
  }
  return true;
}

bool Xcdr2ValueWriter::begin_serialize_aggregated_member(unsigned id, bool must_understand,
                                                         bool optional, bool present)
{
  const Extensibility extensibility = serialize_states_.top().extensibility;
  if (optional && (extensibility == FINAL || extensibility == APPENDABLE)) {
    if (!(*ser_ << ACE_OutputCDR::from_boolean(present))) {
      return false;
    }
  }
  if (extensibility == MUTABLE && present) {
    if (!ser_->write_parameter_id(id, size_cache_.at(pos_) , must_understand)) {
      return false;
    }
    ++pos_;
  }
  return true;
}

bool Xcdr2ValueWriter::begin_complex(Extensibility extensibility, CollectionKind ck, ACE_CDR::ULong seq_length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return begin_ssize_complex(extensibility, ck);
  } else {
    return begin_serialize_complex(extensibility, ck, seq_length);
  }
}

bool Xcdr2ValueWriter::end_complex()
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return end_ssize_complex();
  } else {
    return end_serialize_complex();
  }
}

bool Xcdr2ValueWriter::begin_aggregated_member(unsigned id, bool must_understand,
                                               bool optional, bool present)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return begin_ssize_aggregated_member(optional, present);
  } else {
    return begin_serialize_aggregated_member(id, must_understand, optional, present);
  }
}

bool Xcdr2ValueWriter::begin_struct(Extensibility extensibility)
{
  return begin_complex(extensibility);
}

bool Xcdr2ValueWriter::end_struct()
{
  return end_complex();
}

bool Xcdr2ValueWriter::begin_struct_member(MemberParam params)
{
  return begin_aggregated_member(params.id, params.must_understand, params.optional, params.present);
}

bool Xcdr2ValueWriter::end_struct_member()
{
  return true;
}

bool Xcdr2ValueWriter::begin_union(Extensibility extensibility)
{
  return begin_complex(extensibility);
}

bool Xcdr2ValueWriter::end_union()
{
  return end_complex();
}

bool Xcdr2ValueWriter::begin_discriminator(MemberParam params)
{
  return begin_aggregated_member(params.id, params.must_understand, params.optional, params.present);
}

bool Xcdr2ValueWriter::end_discriminator()
{
  return true;
}

bool Xcdr2ValueWriter::begin_union_member(MemberParam params)

{
  return begin_aggregated_member(params.id, params.must_understand, params.optional, params.present);
}

bool Xcdr2ValueWriter::end_union_member()
{
  return true;
}

// Array and sequence can be treated similar to final/appendable struct.
// If the element type is primitive, it is similar to a final struct with no delimiter.
// If the element type is not primitive, it is similar to an appendable struct with a delimiter.
// In both case, elements are serialized back-to-back similarly to final/appendable struct.
// One difference is that sequence has a length field ahead of the elements.
bool Xcdr2ValueWriter::begin_array(XTypes::TypeKind elem_tk)
{
  // TODO: Revisit for multi-dimensional array with typedef.
  // See https://github.com/OpenDDS/OpenDDS/issues/4453 for related issue in opendds_idl.
  // Potential solution: resolve all the intermediary typedefs to get the actual final
  // element type and at the same time keep track the dimensions of the array in the
  // vwrite_array function in DynamicVwrite.cpp.

  Extensibility arr_exten = FINAL;
  // In case the element type is not primitive, account for the Dheader only when this is
  // called for the outermost dimension of the array.
  const bool primitive_elem_kind = XTypes::is_primitive(elem_tk);
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    if (size_states_.top().collection_kind != ARRAY_KIND && !primitive_elem_kind) {
      arr_exten = APPENDABLE;
    }
  } else {
    if (serialize_states_.top().collection_kind != ARRAY_KIND && !primitive_elem_kind) {
      arr_exten = APPENDABLE;
    }
  }

  return begin_complex(arr_exten, ARRAY_KIND);
}

bool Xcdr2ValueWriter::end_array()
{
  return end_complex();
}

bool Xcdr2ValueWriter::begin_sequence(XTypes::TypeKind elem_tk, ACE_CDR::ULong length)
{
  Extensibility seq_exten = FINAL;
  if (!XTypes::is_primitive(elem_tk)) {
    seq_exten = APPENDABLE;
  }

  return begin_complex(seq_exten, SEQUENCE_KIND, length);
}

bool Xcdr2ValueWriter::end_sequence()
{
  return end_complex();
}

bool Xcdr2ValueWriter::begin_element(ACE_CDR::ULong /*idx*/)
{
  return true;
}

bool Xcdr2ValueWriter::end_element()
{
  return true;
}

// When this is a member of a mutable type, its size needs to be recorded
// so that we can write the Emheader (and Nextint) later.
bool Xcdr2ValueWriter::write_boolean(ACE_CDR::Boolean value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_boolean(encoding_, size_states_.top().total_size);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(boolean_cdr_size);
    }
    return true;
  } else {
    return *ser_ << ACE_OutputCDR::from_boolean(value);
  }
}

bool Xcdr2ValueWriter::write_byte(ACE_CDR::Octet value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_octet(encoding_, size_states_.top().total_size);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(byte_cdr_size);
    }
    return true;
  } else {
    return *ser_ << ACE_OutputCDR::from_octet(value);
  }
}

#if OPENDDS_HAS_EXPLICIT_INTS
bool Xcdr2ValueWriter::write_int8(ACE_CDR::Int8 value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_int8(encoding_, size_states_.top().total_size);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(int8_cdr_size);
    }
    return true;
  } else {
    return *ser_ << ACE_OutputCDR::from_int8(value);
  }
}

bool Xcdr2ValueWriter::write_uint8(ACE_CDR::UInt8 value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_uint8(encoding_, size_states_.top().total_size);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(uint8_cdr_size);
    }
    return true;
  } else {
    return *ser_ << ACE_OutputCDR::from_uint8(value);
  }
}
#endif

bool Xcdr2ValueWriter::write_int16(ACE_CDR::Short value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(int16_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_uint16(ACE_CDR::UShort value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(uint16_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_int32(ACE_CDR::Long value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(int32_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_uint32(ACE_CDR::ULong value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(uint32_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_int64(ACE_CDR::LongLong value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(int64_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_uint64(ACE_CDR::ULongLong value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(uint64_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_float32(ACE_CDR::Float value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(float32_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_float64(ACE_CDR::Double value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(float64_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_float128(ACE_CDR::LongDouble value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size(encoding_, size_states_.top().total_size, value);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(float128_cdr_size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_fixed(const ACE_CDR::Fixed& /*value*/)
{
  return true;
}

bool Xcdr2ValueWriter::write_char8(ACE_CDR::Char value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_char(encoding_, size_states_.top().total_size);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(char8_cdr_size);
    }
    return true;
  } else {
    return *ser_ << ACE_OutputCDR::from_char(value);
  }
}

bool Xcdr2ValueWriter::write_char16(ACE_CDR::WChar value)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_wchar(encoding_, size_states_.top().total_size);

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(char16_cdr_size);
    }
    return true;
  } else {
    return *ser_ << ACE_OutputCDR::from_wchar(value);
  }
}

bool Xcdr2ValueWriter::write_string(const ACE_CDR::Char* value, size_t length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    size_t& size = size_states_.top().total_size;
    primitive_serialized_size_ulong(encoding_, size);
    size += length + 1; // Include null termination

    if (size_states_.top().extensibility == MUTABLE) {
      // It's safe to do this since before every member of a mutable type,
      // the total_size variable is set to zero.
      size_cache_.push_back(size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_wstring(const ACE_CDR::WChar* value, size_t length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    size_t& size = size_states_.top().total_size;
    primitive_serialized_size_ulong(encoding_, size);
    size += length * char16_cdr_size; // Not include null termination

    if (size_states_.top().extensibility == MUTABLE) {
      size_cache_.push_back(size);
    }
    return true;
  } else {
    return *ser_ << value;
  }
}

bool Xcdr2ValueWriter::write_enum(ACE_CDR::Long value, const EnumHelper& helper)
{
  XTypes::TypeKind as_int = helper.get_equivalent_int();
  bool invalid_int_type = false;

  if (mode_ == SERIALIZATION_SIZE_MODE) {
    size_t& size = size_states_.top().total_size;
    switch (as_int) {
    case XTypes::TK_INT8:
      primitive_serialized_size_int8(encoding_, size);
      break;
    case XTypes::TK_INT16:
      primitive_serialized_size(encoding_, size, ACE_CDR::Short());
      break;
    case XTypes::TK_INT32:
      primitive_serialized_size(encoding_, size, ACE_CDR::Long());
      break;
    default:
      invalid_int_type = true;
      break;
    }
    if (!invalid_int_type) {
      if (size_states_.top().extensibility == MUTABLE) {
        size_cache_.push_back(size);
      }
    }
  } else {
    switch (as_int) {
    case XTypes::TK_INT8:
      return *ser_ << ACE_OutputCDR::from_int8(static_cast<ACE_CDR::Int8>(value));
    case XTypes::TK_INT16:
      return *ser_ << static_cast<ACE_CDR::Short>(value);
    case XTypes::TK_INT32:
      return *ser_ << value;
    default:
      invalid_int_type = true;
      break;
    }
  }

  if (invalid_int_type) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Xcdr2ValueWriter::write_enum:"
                 " Enum cannot be serialized as %C\n", XTypes::typekind_to_string(as_int)));
    }
    return false;
  }
  return true;
}

bool Xcdr2ValueWriter::write_bitmask(ACE_CDR::ULongLong value, const BitmaskHelper& helper)
{
  XTypes::TypeKind as_uint = helper.get_equivalent_uint();
  bool invalid_uint_type = false;

  if (mode_ == SERIALIZATION_SIZE_MODE) {
    size_t& size = size_states_.top().total_size;
    switch (as_uint) {
    case XTypes::TK_UINT8:
      primitive_serialized_size_uint8(encoding_, size);
      break;
    case XTypes::TK_UINT16:
      primitive_serialized_size(encoding_, size, ACE_CDR::UShort());
      break;
    case XTypes::TK_UINT32:
      primitive_serialized_size(encoding_, size, ACE_CDR::ULong());
      break;
    case XTypes::TK_UINT64:
      primitive_serialized_size(encoding_, size, ACE_CDR::ULongLong());
      break;
    default:
      invalid_uint_type = true;
      break;
    }
    if (!invalid_uint_type) {
      if (size_states_.top().extensibility == MUTABLE) {
        size_cache_.push_back(size);
      }
    }
  } else {
    switch (as_uint) {
    case XTypes::TK_UINT8:
      return *ser_ << ACE_OutputCDR::from_uint8(static_cast<ACE_CDR::UInt8>(value));
    case XTypes::TK_UINT16:
      return *ser_ << static_cast<ACE_CDR::UShort>(value);
    case XTypes::TK_UINT32:
      return *ser_ << static_cast<ACE_CDR::ULong>(value);
    case XTypes::TK_UINT64:
      return *ser_ << value;
    default:
      invalid_uint_type = true;
      break;
    }
  }

  if (invalid_uint_type) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: Xcdr2ValueWriter::write_bitmask:"
                 " Bitmask cannot be serialized as %C\n", XTypes::typekind_to_string(as_uint)));
    }
    return false;
  }
  return true;
}

bool Xcdr2ValueWriter::write_absent_value()
{
  return true;
}

bool Xcdr2ValueWriter::write_boolean_array(const ACE_CDR::Boolean* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_boolean(encoding_, size_states_.top().total_size, length);
    return true;
  } else {
    return ser_->write_boolean_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_byte_array(const ACE_CDR::Octet* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_octet(encoding_, size_states_.top().total_size, length);
    return true;
  } else {
    return ser_->write_octet_array(x, length);
  }
}

#if OPENDDS_HAS_EXPLICIT_INTS
bool Xcdr2ValueWriter::write_int8_array(const ACE_CDR::Int8* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_int8(encoding_, size_states_.top().total_size, length);
    return true;
  } else {
    return ser_->write_int8_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_uint8_array(const ACE_CDR::UInt8* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_uint8(encoding_, size_states_.top().total_size, length);
    return true;
  } else {
    return ser_->write_uint8_array(x, length);
  }
}
#endif

bool Xcdr2ValueWriter::write_int16_array(const ACE_CDR::Short* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::Short(), length);
  } else {
    return ser_->write_short_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_uint16_array(const ACE_CDR::UShort* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::UShort(), length);
  } else {
    return ser_->write_ushort_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_int32_array(const ACE_CDR::Long* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::Long(), length);
  } else {
    return ser_->write_long_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_uint32_array(const ACE_CDR::ULong* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::ULong(), length);
  } else {
    return ser_->write_ulong_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_int64_array(const ACE_CDR::LongLong* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::LongLong(), length);
  } else {
    return ser_->write_longlong_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_uint64_array(const ACE_CDR::ULongLong* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::ULongLong(), length);
  } else {
    return ser_->write_ulonglong_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_float32_array(const ACE_CDR::Float* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::Float(), length);
  } else {
    return ser_->write_float_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_float64_array(const ACE_CDR::Double* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::Double(), length);
  } else {
    return ser_->write_double_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_float128_array(const ACE_CDR::LongDouble* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    return primitive_serialized_size(encoding_, size_states_.top().total_size, ACE_CDR::LongDouble(), length);
  } else {
    return ser_->write_longdouble_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_char8_array(const ACE_CDR::Char* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_char(encoding_, size_states_.top().total_size, length);
    return true;
  } else {
    return ser_->write_char_array(x, length);
  }
}

bool Xcdr2ValueWriter::write_char16_array(const ACE_CDR::WChar* x, ACE_CDR::ULong length)
{
  if (mode_ == SERIALIZATION_SIZE_MODE) {
    primitive_serialized_size_wchar(encoding_, size_states_.top().total_size, length);
    return true;
  } else {
    return ser_->write_wchar_array(x, length);
  }
}

size_t Xcdr2ValueWriter::get_serialized_size() const
{
  if (!size_cache_.empty()) {
    return size_cache_[0];
  }

  if (log_level >= LogLevel::Warning) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: Xcdr2ValueWriter::get_serialized_size:"
               " serialized size has not been computed yet!\n"));
  }
  return 0;
}

const OPENDDS_VECTOR(size_t)& Xcdr2ValueWriter::get_serialized_sizes() const
{
  return size_cache_;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
