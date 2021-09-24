/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DynamicData.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicData::DynamicData(Serializer& ser, DynamicType_rch type)
  : strm_(ser)
  , start_rpos_(strm_.rpos())
{
  if (type->get_kind() == TK_ALIAS) {
    type_ = get_base_type(type);
  } else {
    type_ = type;
  }

  descriptor_ = type_->get_descriptor();
}

DDS::ReturnCode_t DynamicData::get_descriptor(MemberDescriptor& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_descriptor(MemberId id, const MemberDescriptor& value)
{
}

bool DynamicData::equals(const DynamicData& other) const
{
}

MemberId DynamicData::get_member_id_by_name(String name) const
{
}

MemberId DynamicData::get_member_id_at_index(ACE_CDR::ULong) const
{
}

ACE_CDR::ULong DynamicData::get_item_count() const
{
}

DDS::ReturnCode_t DynamicData::clear_all_values()
{
}

DDS::ReturnCode_t DynamicData::clear_nonkey_values()
{
}

DDS::ReturnCode_t DynamicData::clear_value(MemberId id)
{
}

DynamicData DynamicData::loan_value(MemberId id)
{
}

DDS::ReturnCode_t DynamicData::return_loaned_value(const DynamicData& value)
{
}

DynamicData DynamicData::clone() const
{
}

template<typename MemberType, typename MemberTypeKind>
bool DynamicData::get_value_from_struct(MemberType& value, MemberId id)
{
  if (!find_member(id, MemberTypeKind)) {
    return false;
  }

  ACE_CDR::ULong primitive_size;
  if (is_primitive(MemberTypeKind, primitive_size) ||
      MemberTypeKind == TK_STRING8 || MemberTypeKind == TK_STRING16) {
    return (strm_ >> value);
  } else {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_value_from_struct -")
                 ACE_TEXT(" Called on an unsupported type kind %C\n"),
                 typekind_to_string(MemberTypeKind)));
    }
    return false;
  }
}

template<typename MemberType, typename MemberTypeKind>
bool DynamicData::get_value_from_union(MemberType& value, MemberId id)
{
  // 1. Read discriminator value to get the label of selected member.
  DynamicType_rch disc_type = get_base_type(descriptor_.discriminator_type);
  ACE_CDR::Long label;
  if (!read_discriminator(disc_type->get_kind(), label)) {
    return false;
  }

  // 2. Loop through the labels of each members to find the selected member.
  DynamicTypeMembersById members;
  type_->get_all_members(members);

  DynamicTypeMembersById::const_iterator it = members.begin();
  for (; it != members.end(); ++it) {
    MemberDescriptor md = it->second->get_descriptor();
    const UnionCaseLabelSeq& labels = md.label;
    for (ACE_CDR::ULong i = 0; i < labels.size(); ++i) {
      if (label == labels[i]) {
        // 3. Verify that ID matches.
        if (md.id != id) {
          if (DCPS_debug_level >= 1) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_value_from_union -")
                       ACE_TEXT(" ID of the selected member (%d) is not the requested ID (%d)"),
                       md.id, id));
          }
          return false;
        }

        // 4. Verify that type kind of the requested member matches.
        if (md.type->get_kind() != MemberTypeKind) {
          if (DCPS_debug_level >= 1) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_value_from_union -")
                       ACE_TEXT(" The selected member has type %C, not %C"),
                       typekind_to_string(md.type->get_kind()), typekind_to_string(MemberTypeKind)));
          }
          return false;
        }

        // TODO(sonndinh) 5. Use the type of the selected member to deserialize its data.
        // Need to update this method to consider different extensibility types.

      }
    }
  }
  return false;
}

template<typename ElementType, typename ElementTypeKind>
bool DynamicData::get_value_from_sequence(ElementType& value, MemberId id)
{
  const TypeKind elem_tk = get_base_type(descriptor_.element_type)->get_kind();
  if (elem_tk != ElementTypeKind) {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_%C_value -")
                 ACE_TEXT(" Called on sequence of type %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_tk)));
    }
    return false;
  }

  ACE_CDR::ULong primitive_size;
  if (is_primitive(elem_tk, primitive_size)) {
    ACE_CDR::ULong length, index;
    return (strm_ >> length) &&
      get_index_from_id(id, index, length) &&
      strm_.skip(index, primitive_size) &&
      (strm_ >> value);
  } else if (ElementTypeKind == TK_STRING8 || ElementTypeKind == TK_STRING16) {
    ACE_CDR::ULong dheader, length, index;
    if (!(strm_ >> dheader) || !(strm_ >> length) || !get_index_from_id(id, index, length)) {
      return false;
    }
    ACE_CDR::ULong i = 0;
    while (i++ < index) {
      ACE_CDR::ULong bytes;
      if (!(strm_ >> bytes) || !strm_.skip(bytes)) {
        return false;
      }
    }
    return (strm_ >> value);
  } else {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_value_from_sequence -")
                 ACE_TEXT(" Called on an unsupported type kind %C\n"),
                 typekind_to_string(ElementTypeKind)));
    }
    return false;
  }
}

template<typename ElementType, typename ElementTypeKind>
bool DynamicData::get_value_from_array(ElementType& value, MemberId id)
{
  const TypeKind elem_tk = get_base_type(descriptor_.element_type)->get_kind();
  if (elem_tk != ElementTypeKind) {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_%C_value -")
                 ACE_TEXT(" Called on array of type %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_tk)));
    }
    return false;
  }

  ACE_CDR::ULong length = 1;
  for (ACE_CDR::ULong i = 0; i < descriptor_.bound.length(); ++i) {
    length *= descriptor_.bound[i];
  }

  ACE_CDR::ULong primitive_size;
  if (is_primitive(elem_tk, primitive_size)) {
    ACE_CDR::ULong index;
    return get_index_from_id(id, index, length) &&
      strm_.skip(index, primitive_size) &&
      (strm_ >> value);
  } else if (ElementTypeKind == TK_STRING8 || ElementTypeKind == TK_STRING16) {
    ACE_CDR::ULong dheader;
    if (!(strm_ >> dheader) || !get_index_from_id(id, index, length)) {
      return false;
    }
    ACE_CDR::ULong i = 0;
    while (i++ < index) {
      ACE_CDR::ULong bytes;
      if (!(strm_ >> bytes) || !strm_.skip(bytes)) {
        return false;
      }
    }
    return (strm_ >> value);
  } else {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_value_from_array -")
                 ACE_TEXT(" Called on an unsupported type kind %C\n"),
                 typekind_to_string(ElementTypeKind)));
    }
    return false;
  }
}

template<typename ElementType, typename ElementTypeKind>
bool DynamicData::get_value_from_map(ElementType& value, MemberId id)
{
  const TypeKind elem_tk = get_base_type(descriptor_.element_type)->get_kind();
  if (elem_tk != ElementTypeKind) {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_%C_value -")
                 ACE_TEXT(" Called on map with element type %C\n"),
                 typekind_to_string(ElementTypeKind), typekind_to_string(elem_tk)));
    }
    return false;
  }

  const TypeKind key_tk = get_base_type(descriptor_.key_element_type)->get_kind();
  ACE_CDR::ULong elem_primitive_size, key_primitive_size;
  const bool primitive_elem = is_primitive(elem_tk, elem_primitive_size);

  if (is_primitive(key_tk, key_primitive_size) && primitive_elem) {
    ACE_CDR::ULong length, index;
    if (!(strm_ >> length) || !get_index_from_id(id, index, length)) {
      return false;
    }
    ACE_CDR::ULong i = 0;
    while (i++ < index) {
      if (!strm_.skip(1, key_primitive_size) || !strm_.skip(1, elem_primitive_size)) {
        return false;
      }
    }
    return strm_.skip(1, key_primitive_size) && (strm_ >> value);
  } else if (ElementTypeKind == TK_STRING8 || ElementTypeKind == TK_STRING16) {
    ACE_CDR::ULong dheader;
    if (!(strm_ >> dheader) || !get_index_from_id(id, index, ACE_UINT32_MAX)) {
      return false;
    }
    size_t end_of_map = strm_.rpos() + dheader;
    ACE_CDR::ULong i = 0;

    while (i++ < index) {
      if (strm_.rpos() >= end_of_map) {
        return false;
      }
      // TODO(sonndinh): Implement skip_map_key() to skip a key from a map. A key in this
      // case can be of any type, not just primitive or string or wstring.
      if (!skip_map_key()) { return false; }
      if (primitive_elem) {
        if (!strm_.skip(1, elem_primitive_size)) { return false; }
      } else {
        ACE_CDR::ULong bytes;
        if (!(strm_ >> bytes) || !strm_.skip(bytes)) { return false; }
      }
    }
    return (strm_.rpos() < end_of_map) && skip_map_key() && (strm_ >> value);
  } else {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_value_from_map -")
                 ACE_TEXT(" Called on an unsupported type kind %C\n"),
                 typekind_to_string(ElementTypeKind)));
    }
    return false;
  }
}

template<typename SequenceType, typename ElementTypeKind>
DDS::ReturnCode_t DynamicData::get_values_from_struct(SequenceType& sequence, MemberId id)
{
  DDS::ReturnCode_t retcode;
  if (!find_member(id, ElementTypeKind, true)) {
    reset_rpos();
    return DDS::RETCODE_ERROR;
  }

  bool good = true;
  ACE_CDR::ULong size;
  bool primitive = is_primitive(ElementTypeKind, size);

  if (primitive || ElementTypeKind == TK_STRING8 || ElementTypeKind == TK_STRING16) {
    ACE_CDR::ULong len;
    if (primitive) {
      good = (strm_ >> len);
    } else {
      ACE_CDR::ULong dheader;
      good = (strm_ >> dheader) && (strm_ >> len);
    }

    if (good) {
      sequence.length(len);
      for (ACE_CDR::ULong i = 0; i < len; ++i) {
        good = (strm_ >> sequence[i]);
        if (!good) { break; }
      }
    }
  } else {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_sequence_values -")
                 ACE_TEXT(" Called on a sequence of unsupported type kind %C\n"),
                 typekind_to_string(ElementTypeKind)));
    }
    reset_rpos();
    return DDS::RETCODE_ERROR;
  }

  if (!good && DCPS_debug_level >= 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_%C_%values -")
               ACE_TEXT(" Failed to read a sequence member with ID %d\n"),
               typekind_to_string(ElementTypeKind), id));
  }
  reset_rpos();
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicData::get_int32_value(ACE_CDR::Long& value, MemberId id)
{
  const TypeKind tk = type_->get_kind();
  bool good = true;

  switch (tk) {
  case TK_INT32:
  case TK_ENUM:
    // XTypes spec says the value of a DynamicData object of primitive type or TK_ENUM is
    // accessed with Id MEMBER_ID_INVALID. Do we need to verify the passed Id before returning
    // the value? There is only an int32 value in this DynamicData object, so checking Id
    // is perhaps unnecessary.
    if (tk == TK_ENUM) {
      const LBound bit_bound = descriptor_.bound[0];
      if (bit_bound < 17) {
        if (DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                     ACE_TEXT(" Called on an enum type with bit_bound %d\n"), bit_bound));
        }
        good = false;
        break;
      }
    }

    if (!(strm_ >> value)) {
      if (DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Dynamic::get_int32_value -")
                   ACE_TEXT(" Failed to read DynamicData object of type %C\n"),
                   typekind_to_string(tk)));
      }
      good = false;
    }
    break;
  case TK_STRUCTURE:
    good = get_value_from_struct<ACE_CDR::Long, TK_INT32>(value, id);
    break;
  case TK_UNION:
    good = get_value_from_union<ACE_CDR::Long, TK_INT32>(value, id);
    break;
  case TK_SEQUENCE:
    good = get_value_from_sequence<ACE_CDR::Long, TK_INT32>(value, id);
    break;
  case TK_ARRAY:
    good = get_value_from_array<ACE_CDR::Long, TK_INT32>(value, id);
    break;
  case TK_MAP:
    good = get_value_from_map<ACE_CDR::Long, TK_INT32>(value, id);
    break;
  default:
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                 ACE_TEXT(" Called on an incompatible type %C\n"), typekind_to_string(tk)));
    }
    return DDS::RETCODE_ERROR;
  }

  reset_rpos();
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicData::set_int32_value(MemberId id, ACE_CDR::Long value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint32_value(ACE_CDR::ULong& value, MemberId id)
{
  const TypeKind tk = type_->get_kind();

  switch (tk) {
  case TK_UINT32:
  case TK_BITMASK:
    if (tk == TK_BITMASK) {
      const LBound bit_bound = descriptor_.bound[0];
      if (bit_bound < 17 || bit_bound > 32) {
        if (DCPS_debug_level >= 1) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_uint32_value -")
                     ACE_TEXT(" Called on a bitmask type with bit_bound %d\n"), bit_bound));
        }
        good = false;
        break;
      }
    }

    if (!(strm_ >> value)) {
      if (DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_uint32_value -")
                   ACE_TEXT(" Failed to read DynamicData object of type %C\n"),
                   typekind_to_string(tk)));
      }
      good = false;
    }
    break;
  case TK_STRUCTURE:
    return get_value_from_struct<ACE_CDR::ULong, TK_UINT32>(value, id);
  case TK_UNION:
    return get_value_from_union<ACE_CDR::ULong, TK_UINT32>(value, id);
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
  default:
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_uint32_value -")
                 ACE_TEXT(" Called on an incompatible type %C\n"), typekind_to_string(tk)));
    }
    good = false;
    break;
  }

  reset_rpos();
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicData::set_uint32_value(MemberId id, ACE_CDR::ULong value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int8_value(ACE_CDR::Int8& value, MemberId id)
{
  return get_individual_value<ACE_CDR::Int8, TK_INT8>(value, id);
}

DDS::ReturnCode_t DynamicData::set_int8_value(MemberId id, ACE_CDR::Int8 value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint8_value(ACE_CDR::UInt8& value, MemberId id)
{
  return get_individual_value<ACE_CDR::UInt8, TK_UINT8>(value, id);
}

DDS::ReturnCode_t DynamicData::set_uint8_value(MemberId id, ACE_CDR::UInt8 value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int16_value(ACE_CDR::Short& value, MemberId id)
{
  return get_individual_value<ACE_CDR::Short, TK_INT16>(value, id);
}

DDS::ReturnCode_t DynamicData::set_int16_value(MemberId id, ACE_CDR::Short value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint16_value(ACE_CDR::UShort& value, MemberId id)
{
  return get_individual_value<ACE_CDR::UShort, TK_UINT16>(value, id);
}

DDS::ReturnCode_t DynamicData::set_uint16_value(MemberId id, ACE_CDR::UShort value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int64_value(ACE_CDR::LongLong& value, MemberId id)
{
  return get_individual_value<ACE_CDR::LongLong, TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicData::set_int64_value(MemberId id, ACE_CDR::LongLong value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint64_value(ACE_CDR::ULongLong& value, MemberId id)
{
  return get_individual_value<ACE_CDR::ULongLong, TK_UINT64>(value, id);
}

DDS::ReturnCode_t DynamicData::set_uint64_value(MemberId id, ACE_CDR::ULongLong value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float32_value(ACE_CDR::Float& value, MemberId id)
{
  return get_individual_value<ACE_CDR::Float, TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicData::set_float32_value(MemberId id, ACE_CDR::Float value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float64_value(ACE_CDR::Double& value, MemberId id)
{
  return get_individual_value<ACE_CDR::Double, TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicData::set_float64_value(MemberId id, ACE_CDR::Double value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float128_value(ACE_CDR::LongDouble& value, MemberId id)
{
  return get_individual_value<ACE_CDR::LongDouble, TK_FLOAT128>(value, id);
}

DDS::ReturnCode_t DynamicData::set_float128_value(MemberId id, ACE_CDR::LongDouble value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_char8_value(ACE_CDR::Char& value, MemberId id)
{
  return get_individual_value<ACE_CDR::Char, TK_CHAR8>(value, id);
}

DDS::ReturnCode_t DynamicData::set_char8_value(MemberId id, ACE_CDR::Char value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_char16_value(ACE_CDR::WChar& value, MemberId id)
{
  return get_individual_value<ACE_CDR::WChar, TK_CHAR16>(value, id);
}

DDS::ReturnCode_t DynamicData::set_char16_value(MemberId id, ACE_CDR::WChar value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_byte_value(ACE_CDR::Octet& value, MemberId id)
{
  return get_individual_value<ACE_CDR::Octet, TK_BYTE>(value, id);
}

DDS::ReturnCode_t DynamicData::set_byte_value(MemberId id, ACE_CDR::Octet value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_boolean_value(ACE_CDR::Boolean& value, MemberId id)
{
  return get_individual_value<ACE_CDR::Boolean, TK_BOOLEAN>(value, id);
}

DDS::ReturnCode_t DynamicData::set_boolean_value(MemberId id, ACE_CDR::Boolean value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_string_value(DCPS::String& value, MemberId id)
{
  return get_individual_value<DCPS::String, TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicData::set_string_value(MemberId id, DCPS::String value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_wstring_value(DCPS::WString& value, MemberId id)
{
  return get_individual_value<DCPS::WString, TK_STRING16>(value, id);
}

DDS::ReturnCode_t DynamicData::set_wstring_value(MemberId id, DCPS::WString value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

// Even though a DynamicData can be constructed for most types (except annotations
// and bitsets), we will only return a new DynamicData if the requested member's type
// is not primitive or string (and wstring). If the member is a primitive or a string
// or a wstring, caller should call one of the other interfaces.
// This avoids unnecessary complications added to the code.
// Or maybe allowing any type is just as fine?
DDS::ReturnCode_t DynamicData::get_complex_value(DynamicData& value, MemberId id)
{
  // TODO(sonndinh)
}

DDS::ReturnCode_t DynamicData::set_complex_value(MemberId id, DynamicData value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int32_values(Int32Seq& value, MemberId id)
{
  return get_sequence_values<Int32Seq, TK_INT32>(value, id);
}

DDS::ReturnCode_t DynamicData::set_int32_values(MemberId id, const Int32Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint32_values(UInt32Seq& value, MemberId id)
{
  return get_sequence_values<UInt32Seq, TK_UINT32>(value, id);
}

DDS::ReturnCode_t DynamicData::set_uint32_values(MemberId id, const UInt32Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int8_values(Int8Seq& value, MemberId id)
{
  return get_sequence_values<Int8Seq, TK_INT8>(value, id);
}

DDS::ReturnCode_t DynamicData::set_int8_values(MemberId id, const Int8Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint8_values(UInt8Seq& value, MemberId id)
{
  return get_sequence_values<UInt8Seq, TK_UINT8>(value, id);
}

DDS::ReturnCode_t DynamicData::set_uint8_values(MemberId id, const UInt8Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int16_values(Int16Seq& value, MemberId id)
{
  return get_sequence_values<Int16Seq, TK_INT16>(value, id);
}

DDS::ReturnCode_t DynamicData::set_int16_values(MemberId id, const Int16Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint16_values(UInt16Seq& value, MemberId id)
{
  return get_sequence_values<UInt16Seq, TK_UINT16>(value, id);
}

DDS::ReturnCode_t DynamicData::set_uint16_values(MemberId id, const UInt16Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int64_values(Int64Seq& value, MemberId id)
{
  return get_sequence_values<Int64Seq, TK_INT64>(value, id);
}

DDS::ReturnCode_t DynamicData::set_int64_values(MemberId id, const Int64Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint64_values(UInt64Seq& value, MemberId id)
{
  return get_sequence_values<UInt64Seq, TK_UINT64>(value, id);
}

DDS::ReturnCode_t DynamicData::set_uint64_values(MemberId id, const UInt64Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float32_values(Float32Seq& value, MemberId id)
{
  return get_sequence_values<Float32Seq, TK_FLOAT32>(value, id);
}

DDS::ReturnCode_t DynamicData::set_float32_values(MemberId id, const Float32Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float64_values(Float64Seq& value, MemberId id)
{
  return get_sequence_values<Float64Seq, TK_FLOAT64>(value, id);
}

DDS::ReturnCode_t DynamicData::set_float64_values(MemberId id, const Float64Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float128_values(Float128Seq& value, MemberId id)
{
  return get_sequence_values<Float128Seq, TK_FLOAT128>(value, id);
}

DDS::ReturnCode_t DynamicData::set_float128_values(MemberId id, const Float128Seq& value);
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_char8_values(CharSeq& value, MemberId id)
{
  return get_sequence_values<CharSeq, TK_CHAR8>(value, id);
}

DDS::ReturnCode_t DynamicData::set_char8_values(MemberId id, const CharSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_char16_values(WCharSeq& value, MemberId id)
{
  return get_sequence_values<WCharSeq, TK_CHAR16>(value, id);
}

DDS::ReturnCode_t DynamicData::set_char16_values(MemberId id, const WCharSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_byte_values(ByteSeq& value, MemberId id)
{
  return get_sequence_values<ByteSeq, TK_BYTE>(value, id);
}

DDS::ReturnCode_t DynamicData::set_byte_values(MemberId id, const ByteSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_boolean_values(BooleanSeq& value, MemberId id)
{
  return get_sequence_values<BooleanSeq, TK_BOOLEAN>(value, id);
}

DDS::ReturnCode_t DynamicData::set_boolean_values(MemberId id, const BooleanSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_string_values(StringSeq& value, MemberId id)
{
  return get_sequence_values<StringSeq, TK_STRING8>(value, id);
}

DDS::ReturnCode_t DynamicData::set_string_values(MemberId id, const StringSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_wstring_values(WStringSeq& value, MemberId id)
{
  return get_sequence_values<WStringSeq, TK_STRING16>(value, id);
}

DDS::ReturnCode_t DynamicData::set_wstring_values(MemberId id, const WStringSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

bool DynamicData::find_member(MemberId id, TypeKind kind, bool is_sequence)
{
  const OPENDDS_MAP<MemberId, size_t>::const_iterator it = offset_lookup_table_.find(id);
  if (it != offset_lookup_table_.end()) {
    const size_t offset = it->second;
    std::ostringstream err_msg;
    err_msg << "Failed to skip to the member with ID " << id;
    if (!skip("find_member", err_msg.str().c_str(), offset)) {
      return false;
    }
    return true;
  }

  DynamicTypeMember_rch member;
  const DDS::ReturnCode_t retcode = type_->get_member(member, id);
  if (retcode != DDS::RETCODE_OK) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::find_member -")
                 ACE_TEXT(" Failed to get DynamicTypeMember for member with ID %d\n"), id));
    }
    return false;
  }

  MemberDescriptor member_desc;
  member->get_descriptor(member_desc);
  const TypeKind member_kind = member_desc.type->get_kind();

  if ((!is_sequence && member_kind != kind) ||
      (is_sequence && member_kind != TK_SEQUENCE)) {
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::find_member -")
                 ACE_TEXT(" Member with ID %d has kind %C, not %C\n"),
                 id, typekind_to_string(member_kind),
                 is_sequence ? typekind_to_string(TK_SEQUENCE) : typekind_to_string(kind)));
    }
    return false;
  }

  if (member_kind == TK_SEQUENCE) {
    TypeDescriptor member_td;
    member_desc.type->get_descriptor(member_td);
    const TypeKind elem_kind = member_td.element_type->get_kind();
    if (elem_kind != kind) {
      if (DCPS_debug_level >= 1) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::find_member -")
                   ACE_TEXT(" Member with ID is a sequence of %C, not %C\n"),
                   id, typekind_to_string(elem_kind), typekind_to_string(kind)));
      }
      return false;
    }
  }

  const ExtensibilityKind ek = descriptor_.extensibility_kind;
  const Encoding& encoding = ser_.encoding();
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    if (ek == FINAL || ek == APPENDABLE) {
      if (ek == APPENDABLE) {
        size_t dheader = 0;
        if (!strm_.read_delimiter(dheader)) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::find_member -")
                       ACE_TEXT(" Failed to read DHEADER for member ID %d\n"), id));
          }
          return false;
        }
      }

      // Skip preceding members until reach the requested member.
      ACE_CDR::ULong i = 0;
      while (i < member_desc.index) {
        if (!skip_member_by_index(i)) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::find_member -")
                       ACE_TEXT(" Failed to skip member at index %d\n"), i));
          }
          return false;
        }
        ++i;
      }
    } else {
      size_t dheader = 0;
      if (!strm_.read_delimiter(dheader)) {
        if (DCPS_debug_level >= 10) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::find_member -")
                     ACE_TEXT(" Failed to read DHEADER for member ID %d\n"), id));
        }
        return false;
      }

      const size_t end_of_sample = strm_.rpos() + dheader;
      ACE_CDR::ULong member_id;
      size_t member_size;
      while (true) {
        if (strm_.rpos() >= end_of_sample) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::find_member -")
                       ACE_TEXT(" Could not find a member of kind %C with ID %d\n"),
                       typekind_to_string(kind), id));
          }
          return false;
        }

        bool must_understand = false;
        if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::find_member -")
                       ACE_TEXT(" Failed to read EMHEADER while finding member ID %d\n"), id));
          }
          return false;
        }

        if (member_id == id) {
          break;
        } else {
          std::ostringstream err_msg;
          err_msg << "Failed to skip a member with ID " << member_id;
          if (!skip("find_member", err_msg.str().c_str(), member_size)) {
            return false;
          }
        }
      }
    }
    offset_lookup_table_[id] = strm_.rpos() - start_rpos_;

  } else {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DynamicData::find_member - The sample has")
               ACE_TEXT(" unsupported encoding type %C"),
               encoding.xcdr_version() == Encoding::XCDR_VERSION_1 ? "XCDR1" : "Non-XCDR"));
    return false;
  }
}

void DynamicData::reset_rpos()
{
  // TODO(sonndinh): Move the read pointer to the beginning of the stream.
}

bool DynamicData::skip_member_by_index(ACE_CDR::ULong index)
{
  DynamicTypeMember_rch member;
  if (type_.get_member_by_index(dtm, member) != DDS::RETCODE_OK) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member_by_index -")
                 ACE_TEXT(" Failed to get DynamicTypeMember for member index %d\n"), index));
    }
    return DDS::RETCODE_ERROR;
  }

  MemberDescriptor member_descriptor;
  member->get_descriptor(member_descriptor);
  return skip_member(member_descriptor.type);
}

bool DynamicData::skip_member(DynamicType_rch member_type)
{
  TypeKind member_kind = member_type->get_kind();
  if (member_kind == TK_ALIAS) {
    member_type = get_base_type(member_type);
    member_kind = member_type->get_kind();
  }

  switch (member_kind) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
    if (!skip("skip_member", "Failed to skip a member of size 1 byte", 1, 1)) {
      return false;
    }
    break;
  case TK_INT16:
  case TK_UINT16:
  case TK_CHAR16:
    if (!skip("skip_member", "Failed to skip a member of size 2 bytes", 1, 2)) {
      return false;
    }
    break;
  case TK_INT32:
  case TK_UINT32:
  case TK_FLOAT32:
    if (!skip("skip_member", "Failed to skip a member of size 4 bytes", 1, 4)) {
      return false;
    }
    break;
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT64:
    if (!skip("skip_member", "Failed to skip a member of size 8 bytes", 1, 8)) {
      return false;
    }
    break;
  case TK_FLOAT128:
    if (!skip("skip_member", "Failed to skip a member of size 16 bytes", 1, 16)) {
      return false;
    }
    break;
  case TK_STRING8:
  case TK_STRING16:
    {
      const char* str_kind = member_kind == TK_STRING8 ? "string" : "wstring";
      ACE_CDR::ULong length;
      if (!(strm_ >> length)) {
        if (DCPS_debug_level >= 10) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                     ACE_TEXT(" Failed to read length of a %C member\n"), str_kind));
        }
        return false;
      }

      std::ostringstream err_msg;
      err_msg << "Failed to skip a " << str_kind << " member";
      if (!skip("skip_member", err_msg.str().c_str(), length)) {
        return false;
      }
      break;
    }
  case TK_ENUM:
  case TK_BITMASK:
    {
      TypeDescriptor member_td;
      member_type->get_descriptor(member_td);
      const ACE_CDR::ULong bit_bound = member_td.bound[0];
      const char* err_msg = member_kind == TK_ENUM ?
        "Failed to skip an enum member" : "Failed to skip a bitmask member";

      if (bit_bound >= 1 && bit_bound <= 8) {
        if (!skip("skip_member", err_msg, 1, 1)) {
          return false;
        }
      } else if (bit_bound >= 9 && bit_bound <= 16) {
        if (!skip("skip_member", err_msg, 1, 2)) {
          return false;
        }
      } else if (bit_bound >= 17 && bit_bound <= 32) {
        if (!skip("skip_member", err_msg, 1, 4)) {
          return false;
        }
      } else if (bit_bound >= 33 && bit_bound <= 64 && member_kind == TK_BITMASK) {
        if (!skip("skip_member", err_msg, 1, 8)) {
          return false;
        }
      } else {
        if (DCPS_debug_level >= 10) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member - Found a%C")
                     ACE_TEXT(" member with bit bound %d\n"),
                     member_kind == TK_ENUM ? "n enum" : " bitmask", bit_bound));
        }
        return false;
      }
      break;
    }
  case TK_STRUCTURE:
    return skip_struct_member(member_type);
  case TK_UNION:
    return skip_union_member(member_type);
  case TK_SEQUENCE:
    return skip_sequence_member(member_type);
  case TK_ARRAY:
    return skip_array_member(member_type);
  case TK_MAP:
    return skip_map_member(member_type);
  default:
    {
      if (DCPS_debug_level >= 10) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                   ACE_TEXT(" Found a member of kind %C\n"), typekind_to_string(member_kind)));
      }
      return false;
    }
  }

  return true;
}

bool DynamicData::skip_sequence_member(DynamicType_rch seq_type)
{
  TypeDescriptor descriptor;
  seq_type->get_descriptor(descriptor);
  DynamicType_rch elem_type = descriptor.element_type;
  if (elem_type->get_kind() == TK_ALIAS) {
    elem_type = get_base_type(elem_type);
  }

  ACE_CDR::ULong primitive_size = 0;
  if (is_primitive(elem_type->get_kind(), primitive_size)) {
    ACE_CDR::ULong length;
    if (!(strm_ >> length)) {
      if (DCPS_debug_level >= 10) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_sequence_member -")
                   ACE_TEXT(" Failed to deserialize a primitive sequence member\n")));
      }
      return false;
    }

    return skip("skip_sequence_member", "Failed to skip a primitive sequence member",
                length, primitive_size)
  } else {
    return skip_collection_member(TK_SEQUENCE);
  }
}

bool DynamicData::skip_array_member(DynamicType_rch array_type)
{
  TypeDescriptor descriptor;
  array_type->get_descriptor(descriptor);
  DynamicType_rch elem_type = descriptor.element_type;
  if (elem_type->get_kind() == TK_ALIAS) {
    elem_type = get_base_type(elem_type);
  }

  ACE_CDR::ULong primitive_size = 0;
  if (is_primitive(elem_type->get_kind(), primitive_size)) {
    const LBoundSeq& bounds = descriptor.bound;
    ACE_CDR::ULong num_elems = 1;
    for (unsigned i = 0; i < bounds.size(); ++i) {
      num_elems *= bounds[i];
    }

    return skip("skip_array_member", "Failed to skip a primitive array member",
                num_elems, primitive_size)
  } else {
    return skip_collection_member(TK_ARRAY);
  }
}

bool DynamicData::skip_map_member(DynamicType_rch map_type)
{
  TypeDescriptor descriptor;
  map_type->get_descriptor(descriptor);

  DynamcType_rch elem_type = descriptor.element_type;
  if (elem_type->get_kind() == TK_ALIAS) {
    elem_type = get_base_type(elem_type);
  }

  DynamicType_rch key_type = descriptor.key_element_type;
  if (key_type->get_kind() == TK_ALIAS) {
    key_type = get_base_type(key_type);
  }

  ACE_CDR::ULong key_primitive_size = 0, elem_primitive_size = 0;
  if (is_primitive(key_type->get_kind(), key_primitive_size) &&
      is_primitive(elem_type->get_kind(), elem_primitive_size)) {
    ACE_CDR::ULong length;
    if (!(strm_ >> length)) {
      if (DCPS_debug_level >= 10) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_map_member -")
                   ACE_TEXT(" Failed to deserialize length of a primitive map member\n")));
      }
      return false;
    }

    for (unsigned i = 0; i < length; ++i) {
      if (!skip("skip_map_member", "Failed to skip a key of a primitive map member",
                1, key_primitive_size)) {
        return false;
      }
      if (!skip("skip_map_member", "Failed to skip an element of a primitive map member",
                1, elem_primitive_size)) {
        return false;
      }
    }
    return true;
  } else {
    return skip_collection_member(TK_MAP);
  }
}

bool DynamicData::skip_collection_member(TypeKind kind)
{
  if (kind != TK_SEQUENCE && kind != TK_ARRAY && kind != TK_MAP) {
    return false;
  }
  const char* kind_str = typekind_to_string(kind);

  ACE_CDR::ULong dheader;
  if (!(strm_ >> dheader)) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_collection_member -")
                 ACE_TEXT(" Failed to deserialize DHEADER of a non-primitive %C member\n"),
                 kind_str));
    }
    return false;
  }

  std::ostringstream err_msg;
  err_msg << "Failed to skip a non-primitive " << kind_str <<  " member";
  return skip("skip_collection_member", err_msg.str().c_str(), dheader);
}

bool DynamicData::skip_struct_member(DynamicType_rch struct_type)
{
  // TODO(sonndinh): The internal data buffer must be shared between the current DynamicData
  // object and the struct_data DynamicData object.
  DynamicData struct_data(strm_, struct_type);
  return struct_data.skip_all();
}

bool DynamicData::skip_union_member(DynamicType_rch union_type)
{
  // TODO(sonndinh)
  DynamicData union_data(strm_, union_type);
  return union_data.skip_all();
}

// This assumes any header ahead of the actual discriminator value is already read or skipped.
bool DynamicData::read_discriminator(TypeKind disc_tk, ACE_CDR::Long& label)
{
  switch (disc_tk) {
  case TK_BOOLEAN:
    {
      ACE_CDR::Boolean value;
      if (!(strm_ >> ACE_InputCDR::to_boolean(value))) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_BYTE:
    {
      ACE_CDR::Octet value;
      if (!(strm_ >> ACE_InputCDR::to_octet(value))) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_CHAR8:
    {
      ACE_CDR::Char value;
      if (!(strm_ >> ACE_InputCDR::to_char(value))) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_CHAR16:
    {
      ACE_CDR::WChar value;
      if (!(strm_ >> ACE_InputCDR::to_wchar(value))) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_INT8:
    {
      ACE_CDR::Int8 value;
      if (!(strm_ >> ACE_InputCDR::to_int8(value))) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_UINT8:
    {
      ACE_CDR::UInt8 value;
      if (!(strm_ >> ACE_InputCDR::to_uint8(value))) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_INT16:
    {
      ACE_CDR::Short value;
      if (!(strm_ >> value)) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_UINT16:
    {
      ACE_CDR::UShort value;
      if (!(strm_ >> value)) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_INT32:
    return (strm_ >> label);
  case TK_UINT32:
    {
      ACE_CDR::ULong value;
      if (!(strm_ >> value)) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_INT64:
    {
      ACE_CDR::LongLong value;
      if (!(strm_ >> value)) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_UINT64:
    {
      ACE_CDR::ULongLong value;
      if (!(strm_ >> value)) { return false; }
      label = std::static_cast<ACE_CDR::Long>(value);
      return true;
    }
  case TK_ENUM:
    {
      TypeDescriptor disc_td = disc_type->get_descriptor();
      const ACE_CDR::ULong bit_bound = disc_td.bound[0];
      if (bit_bound >= 1 && bit_bound <= 8) {
        ACE_CDR::Int8 value;
        if (!(strm_ >> ACE_InputCDR::to_int8(value))) { return false; }
        label = std::static_cast<ACE_CDR::Long>(value);
      } else if (bit_bound >= 9 && bit_bound <= 16) {
        ACE_CDR::Short value;
        if (!(strm_ >> value)) { return false; }
        label = std::static_cast<ACE_CDR::Long>(value);
      } else {
        return (strm_ >> label);
      }
      return true;
    }
  default:
    if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::read_discriminator - Union has")
                 ACE_TEXT(" unsupported discriminator type\n")));
    }
    return false;
  }
}

bool DynamicData::skip_all()
{
  const ExtensibilityKind extensibility = descriptor_.extensibility_kind;

  if (extensibility == APPENDABLE || extensibility == MUTABLE) {
    ACE_CDR::ULong dheader;
    if (!(strm_ >> dheader)) {
      if (DCPS_debug_level >= 10) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_all - Failed to read")
                   ACE_TEXT(" DHEADER of the sample\n")));
      }
      return false;
    }
    return skip("skip_all", "Failed to skip the whole sample\n", dheader);

  } else {
    const TypeKind tk = type_->get_kind();
    if (tk == TK_STRUCTURE) {
      const ACE_CDR::ULong member_count = type_->get_member_count();
      for (ACE_CDR::ULong i = 0; i < member_count; ++i) {
        if (!skip_member_by_index(i)) {
          return false;
        }
      }
      return true;
    } else if (tk == TK_UNION) {
      DynamicType_rch disc_type = get_base_type(descriptor_.discriminator_type);
      ACE_CDR::Long label;
      if (!read_discriminator(disc_type->get_kind(), label)) {
        return false;
      }

      DynamicTypeMembersById members;
      type_->get_all_members(members);

      DynamicTypeMembersById::const_iterator it = members.begin();
      for (; it != members.end(); ++it) {
        MemberDescriptor md = it->second->get_descriptor();
        const UnionCaseLabelSeq& labels = md.label;
        for (ACE_CDR::ULong i = 0; i < labels.size(); ++i) {
          if (label == labels[i]) {
            return skip_member(md.type);
          }
        }
      }
      if (DCPS_debug_level >= 10) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_all - Not found a member")
                   ACE_TEXT(" of the union type with label %d\n"), label));
      }
      return false;
    }
  }
}

bool DynamicData::skip(const char* func_name, const char* description, size_t n, int size)
{
  if (!strm_.skip(n, size)) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::%C - %C\n"), func_name, description));
    }
    return false;
  }
  return true;
}

DynamicType_rch DynamicData::get_base_type(DynamicType_rch type) const
{
  if (type->get_kind() != TK_ALIAS) {
    return type;
  }

  TypeDescriptor descriptor = type->get_descriptor();
  return get_base_type(descriptor.base_type);
}

bool DynamicData::is_primitive(TypeKind tk, ACE_CDR::ULong& size) const
{
  size = 0;

  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
    size = 1;
    break;
  case TK_INT16:
  case TK_UINT16:
  case TK_CHAR16:
    size = 2;
    break;
  case TK_INT32:
  case TK_UINT32:
  case TK_FLOAT32:
    size = 4;
    break;
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT64:
    size = 8;
    break;
  case TK_FLOAT128:
    size = 16;
    break;
  default:
    return false;
  }
  return true;
}

const char* typekind_to_string(TypeKind tk) const
{
  switch (tk) {
  case TK_BOOLEAN:
    return "boolean";
  case TK_BYTE:
    return "byte";
  case TK_INT16:
    return "int16";
  case TK_INT32:
    return "int32";
  case TK_INT64:
    return "int64";
  case TK_UINT16:
    return "uint16";
  case TK_UINT32:
    return "uint32";
  case TK_UINT64:
    return "uint64";
  case TK_FLOAT32:
    return "float32";
  case TK_FLOAT64:
    return "float64";
  case TK_FLOAT128:
    return "float128";
  case TK_INT8:
    return "int8";
  case TK_UINT8:
    return "uint8";
  case TK_CHAR8:
    return "char8";
  case TK_CHAR16:
    return "char16";
  case TK_STRING8:
    return "string";
  case TK_STRING16:
    return "wstring";
  case TK_ALIAS:
    return "alias";
  case TK_ENUM:
    return "enum";
  case TK_BITMASK:
    return "bitmask";
  case TK_ANNOTATION:
    return "annotation";
  case TK_STRUCTURE:
    return "structure";
  case TK_UNION:
    return "union";
  case TK_BITSET:
    return "bitset";
  case TK_SEQUENCE:
    return "sequence";
  case TK_ARRAY:
    return "array";
  case TK_MAP:
    return "map";
  default:
    return "unknown";
  }
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
