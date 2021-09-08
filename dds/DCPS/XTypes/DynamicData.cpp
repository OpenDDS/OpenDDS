/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DynamicData.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicData::DynamicData()
{}

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
        /*
        DynamicTypeMember_rch curr_dtm;
        if(type_.get_member_by_index(curr_dtm, i) != DDS::RETCODE_OK) {
          if (DCPS_debug_level >= 10) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DynamicData::get_int32_value -")
                       ACE_TEXT(" Failed to get DynamicTypeMember for member index %d\n"), i));
            return DDS::RETCODE_ERROR;
          }
        }
        MemberDescriptor curr_md;
        curr_dtm->get_descriptor(curr_md);
        const size_t skip_size = num_bytes_to_skip(curr_md.id);

        if (!strm_.skip(skip_size)) {
        */
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
  // TODO(sonndinh)
}

bool DynamicData::skip_all()
{
  // TODO(sonndinh)
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
