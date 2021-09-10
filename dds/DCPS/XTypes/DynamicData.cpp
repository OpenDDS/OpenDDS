/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DynamicData.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

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

DDS::ReturnCode_t DynamicData::get_int32_value(ACE_CDR::Long& value, MemberId id) const
{
  const OPENDDS_MAP<MemberId, size_t>::const_iterator it = offset_lookup_table_.find(id);
  if (it != offset_lookup_table_.cend()) {
    size_t offset = it->second;
    strm_.skip(offset - strm_.rpos());

    if (!strm_ >> value) {
      if (DCPS_debug_level >= 10) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                   ACE_TEXT(" Failed to read an int32 member with ID %d\n"), id));
        return DDS::RETCODE_ERROR;
      }
    }

    // TODO(sonndinh): Move the read pointer back to the beginning of the sample.

    return DDS::RETCODE_OK;
  }

  // Check if the requested member has the given type.
  DynamicTypeMember_rch member;
  DDS::ReturnCode_t retcode = type_->get_member(member, id);
  if (retcode != DDS::RETCODE_OK) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                 ACE_TEXT(" Failed to get DynamicTypeMember with ID %d\n"), id));
    }
    return retcode;
  }

  MemberDescriptor md;
  retcode = member->get_descriptor(md);
  if (retcode != DDS::RETCODE_OK) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                 ACE_TEXT(" Failed to get MemberDescriptor for member ID %d\n"), id));
    }
    return retcode;
  }

  if (md.type->get_kind() != TK_INT32) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                 ACE_TEXT(" Member with ID %d is not an int32\n"), id));
      return DDS::RETCODE_ERROR;
    }
  }

  // Find the value of the member in the sample.
  // Factor out the code that find the offset of the requested member?
  TypeDescriptor td;
  retcode = type_->get_descriptor(td);
  if (retcode != DDS::RETCODE_OK) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                 ACE_TEXT(" Failed to get type descriptor\n")));
      return retcode;
    }
  }
  ExtensibilityKind ek = td.extensibility_kind;

  const Encoding& encoding = ser_.encoding();
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    if (ek == FINAL || ek == APPENDABLE) {
      if (ek == APPENDABLE) {
        size_t total_size = 0;
        if (!strm_.read_delimiter(total_size)) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                       ACE_TEXT(" Failed to read delimiter while reading member ID %d\n"), id));
          }
          return DDS::RETCODE_ERROR;
        }
      }

      // Skip preceding members until reach the requested member.
      ACE_CDR::ULong i = 0;
      while (i < md.index) {
        if (!strm_.skip_member(i)) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                       ACE_TEXT(" Failed to skip member at index %d\n"), i));
            return DDS::RETCODE_ERROR;
          }
        }
        ++i;
      }
    } else {
      size_t total_size = 0;
      if (!strm_.read_delimiter(total_size)) {
        if (DCPS_debug_level >= 10) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                     ACE_TEXT(" Failed to read delimiter while reading member ID %d\n"), id));
          return DDS::RETCODE_ERROR;
        }
      }

      const size_t end_of_sample = strm_.rpos() + total_size;
      unsigned member_id;
      size_t member_size;
      while (true) {
        if (strm_.rpos() >= end_of_sample) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                       ACE_TEXT(" Did not find member with ID %d\n"), id));
            return DDS::RETCODE_ERROR;
          }
        }

        bool must_understand = false;
        if (!strm_.read_parameter_id(member_id, member_size, must_understand)) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                       ACE_TEXT(" Failed to read EMHEADER while finding member ID %d\n"), id));
            return DDS::RETCODE_ERROR;
          }
        }

        if (member_id == id) {
          break;
        } else {
          strm_.skip(member_size);
        }
      }
    }

    offset_lookup_table_[id] = strm_.rpos();
    if (!strm_ >> value) {
      if (DCPS_debug_level >= 10) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                   ACE_TEXT(" Failed to read an int32 member with ID %d\n"), id));
        return DDS::RETCODE_ERROR;
      }
    }
  } else {
    // XCDR1 currently is not supported.
    return DDS::RETCODE_UNSUPPORTED;
  }

  // TODO: After done with getting this member's value, need to go back to the beginning
  // of the stream so that the next call to some get_*_value() method can work.

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicData::set_int32_value(MemberId id, ACE_CDR::Long value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint32_value(ACE_CDR::ULong& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint32_value(MemberId id, ACE_CDR::ULong value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int8_value(ACE_CDR::Int8& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int8_value(MemberId id, ACE_CDR::Int8 value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint8_value(ACE_CDR::UInt8& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint8_value(MemberId id, ACE_CDR::UInt8 value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int16_value(ACE_CDR::Short& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int16_value(MemberId id, ACE_CDR::Short value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint16_value(ACE_CDR::UShort& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint16_value(MemberId id, ACE_CDR::UShort value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int64_value(ACE_CDR::LongLong& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int64_value(MemberId id, ACE_CDR::LongLong value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint64_value(ACE_CDR::ULongLong& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint64_value(MemberId id, ACE_CDR::ULongLong value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float32_value(ACE_CDR::Float& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float32_value(MemberId id, ACE_CDR::Float value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float64_value(ACE_CDR::Double& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float64_value(MemberId id, ACE_CDR::Double value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float128_value(ACE_CDR::LongDouble& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float128_value(MemberId id, ACE_CDR::LongDouble value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_char8_value(ACE_CDR::Char& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_char8_value(MemberId id, ACE_CDR::Char value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_char16_value(ACE_CDR::WChar& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_char16_value(MemberId id, ACE_CDR::WChar value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_byte_value(ACE_CDR::Octet& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_byte_value(MemberId id, ACE_CDR::Octet value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_boolean_value(ACE_CDR::Boolean& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_boolean_value(MemberId id, ACE_CDR::Boolean value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_string_value(DCPS::String& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_string_value(MemberId id, DCPS::String value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_wstring_value(DCPS::WString& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_wstring_value(MemberId id, DCPS::WString value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_complex_value(DynamicData& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_complex_value(MemberId id, DynamicData value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int32_values(Int32Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int32_values(MemberId id, const Int32Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint32_values(UInt32Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint32_values(MemberId id, const UInt32Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int8_values(Int8Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int8_values(MemberId id, const Int8Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint8_values(UInt8Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint8_values(MemberId id, const UInt8Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int16_values(Int16Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int16_values(MemberId id, const Int16Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint16_values(UInt16Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint16_values(MemberId id, const UInt16Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_int64_values(Int64Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int64_values(MemberId id, const Int64Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_uint64_values(UInt64Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint64_values(MemberId id, const UInt64Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float32_values(Float32Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float32_values(MemberId id, const Float32Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float64_values(Float64Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float64_values(MemberId id, const Float64Seq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_float128_values(Float128Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float128_values(MemberId id, const Float128Seq& value);
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_char8_values(CharSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_char8_values(MemberId id, const CharSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_char16_values(WCharSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_char16_values(MemberId id, const WCharSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_byte_values(ByteSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_byte_values(MemberId id, const ByteSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_boolean_values(BooleanSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_boolean_values(MemberId id, const BooleanSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_string_values(StringSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_string_values(MemberId id, const StringSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

DDS::ReturnCode_t DynamicData::get_wstring_values(WStringSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_wstring_values(MemberId id, const WStringSeq& value)
{
  // TODO: Implement this.
  return DDS::RETCODE_UNSUPPORTED;
}

bool DynamicData::skip_member(ACE_CDR::ULong index)
{
  DynamicTypeMember_rch member;
  if(type_.get_member_by_index(dtm, member) != DDS::RETCODE_OK) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                 ACE_TEXT(" Failed to get DynamicTypeMember for member index %d\n"), index));
      return DDS::RETCODE_ERROR;
    }
  }

  MemberDescriptor member_descriptor;
  member->get_descriptor(member_descriptor);
  DynamicType_rch member_type = member_descriptor.type;
  TypeKind member_kind = member_type->get_kind();

  switch (member_kind) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
    strm_.skip(1, 1);
    break;
  case TK_INT16:
  case TK_UINT16:
  case TK_CHAR16:
    strm_.skip(1, 2);
    break;
  case TK_INT32:
  case TK_UINT32:
    strm_.skip(1, 4);
    break;
  case TK_INT64:
  case TK_UINT64:
    strm_.skip(1, 8);
    break;
  case TK_FLOAT32:
    strm_.skip(1, 4);
    break;
  case TK_FLOAT64:
    strm_.skip(1, 8);
    break;
  case TK_FLOAT128:
    strm_.skip(1, 16);
    break;
  case TK_STRING8:
  case TK_STRING16:
    {
      ACE_CDR::ULong len;
      if (!(strm_ >> len)) {
        if (DCPS_debug_level >= 10) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                     ACE_TEXT(" Failed to deserialize member with index %d\n"), index));
          return false;
        }
      }
      strm_.skip(len);
      break;
    }
  case TK_ALIAS:
    // TODO
  case TK_ENUM:
    {
      TypeDescriptor member_td;
      member_type->get_descriptor(member_td);
      const ACE_CDR::ULong bit_bound = member_td.bound[0];
      if (bit_bound >= 1 && bit_bound <= 8) {
        strm_.skip(1, 1);
      } else if (bit_bound >= 9 && bit_bound <= 16) {
        strm_.skip(1, 2);
      } else {
        strm_.skip(1, 4);
      }
      break;
    }
  case TK_BITMASK:
    {
      TypeDescriptor member_td;
      member_type->get_descriptor(member_td);
      const ACE_CDR::ULong bit_bound = member_td.bound[0];
      if (bit_bound >= 1 && bit_bound <= 8) {
        strm_.skip(1, 1);
      } else if (bit_bound >= 9 && bit_bound <= 16) {
        strm_.skip(1, 2);
      } else if (bit_bound >= 17 && bit_bound <= 32) {
        strm_.skip(1, 4);
      } else {
        strm_.skip(1, 8);
      }
      break;
    }
  case TK_STRUCTURE:
    // TODO
  case TK_UNION:
    // TODO
  case TK_BITSET:
    // TODO
  case TK_SEQUENCE:
    skip_sequence_member(member_type);
    break;
  case TK_ARRAY:
    // TODO
  case TK_MAP:
    // TODO
  default:
    {
      DCPS::String kind;
      switch (member_kind) {
      case TK_ANNOTATION:
        kind = "TK_ANNOTATION";
        break;
      case TK_NONE:
        kind = "TK_NONE";
        break;
      default:
        kind = "Unknown";
        break;
      }

      if (DCPS_debug_level >= 10) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                   ACE_TEXT(" Found a member with kind %C at index %d\n"),
                   kind.c_str(), index));
        return false;
      }
    }
  }

  return true;
}

bool DynamicData::skip_all()
{
  // TODO(sonndinh)
}

bool DynamicData::skip_sequence_member(DynamicType_rch seq_type)
{
  TypeDescriptor seq_descriptor;
  seq_type->get_descriptor(seq_descriptor);
  DynamicType_rch elem_type = seq_descriptor.element_type;
  const TypeKind elem_kind = elem_type->get_kind();
  if (elem_kind == TK_ALIAS) {
    DynamicType_rch base_type = get_base_type(elem_type);
    elem_kind = base_type->get_kind();
  }

  const ACE_CDR::ULong primitive_size = 0;
  switch (elem_kind) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
    primitive_size = 1;
    break;
  case TK_INT16:
  case TK_UINT16:
  case TK_CHAR16:
    primitive_size = 2;
    break;
  case TK_INT32:
  case TK_UINT32:
  case TK_FLOAT32:
    primitive_size = 4;
    break;
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT64:
    primitive_size = 8;
    break;
  case TK_FLOAT128:
    primitive_size = 16;
    break;
  default: // Non-primitive types.
    {
      ACE_CDR::ULong dheader;
      if (!(strm_ >> dheader)) {
        if (DCPS_debug_level >= 10) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                     ACE_TEXT(" Failed to deserialize DHEADER of a sequence member at index %d\n"), index));
          return false;
        }
      }
      strm_.skip(dheader);
      return true;
    }
  }

  // Skip a sequence of a primitive type.
  ACE_CDR::ULong length;
  if (!(strm_ >> length)) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                 ACE_TEXT(" Failed to deserialize a sequence member at index %d\n"), index));
      return false;
    }
  }

  if (!strm_.skip(length, primitive_size)) {
    if (DCPS_debug_level >= 10) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::skip_member -")
                 ACE_TEXT(" Failed to skip sequence member at index %d\n"), index));
      return false;
    }
  }
  return true;
}

DynamicType_rch DynamicData::get_base_type(DynamicType_rch alias_type) const
{
  // TODO(sonndinh)
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
