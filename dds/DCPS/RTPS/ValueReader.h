/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_VALUE_READER_H
#define OPENDDS_DCPS_RTPS_VALUE_READER_H

#include "rtps_export.h"

#include <dds/DCPS/ValueReader.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

const DCPS::Encoding encoding_plain_native(DCPS::Encoding::KIND_XCDR1);

class ValueReader : public DCPS::ValueReader {
public:
  ValueReader(const ACE_Message_Block& in)
    : in_(in.duplicate())
    , serializer_(in_.get(), encoding_plain_native)
  {}

  virtual bool begin_struct(const XTypes::TypeIdentifier& /* type_identifier */)
  {
    member_idx_stack_.push_back(0);
    return serializer_.good_bit();
  }
  virtual bool end_struct(const XTypes::TypeIdentifier& /* type_identifier */)
  {
    member_idx_stack_.pop_back();
    return true;
  }
  virtual bool begin_struct_member(const XTypes::TypeIdentifier& type_identifier,
                                   XTypes::MemberId& member_id,
                                   const DCPS::MemberHelper& helper)
  {
    bool have_member = helper.get_value(member_id, member_idx_stack_.back()++);

    if ((type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_DataSubmessageX_xtag>() ||
         type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_DataFragSubmessageX_xtag>()) &&
        have_member &&
        member_id == 5 /* inlineQos */) {
      serializer_.skip(static_cast<unsigned short>(serializer_.length() - length_at_inline_qos_));
      have_member = submessage_header_flags_ & RTPS::FLAG_Q;
    } else if ((type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_InfoReplySubmessageX_xtag>() ||
                type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_InfoReplyIp4SubmessageX_xtag>()) &&
               have_member &&
               member_id == 1 /* multicastLocatorList/multicastLocator */) {
      have_member = submessage_header_flags_ & RTPS::FLAG_M;
    }

    return serializer_.good_bit() && have_member;
  }
  virtual bool end_struct_member(const XTypes::TypeIdentifier& type_identifier,
                                 XTypes::MemberId member_id)
  {
    if (type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_SubmessageHeader_xtag>()) {
      if (member_id == 0 /* submessageId */) {
        enum_stack_.push_back(last_octet_);
      } else if (member_id == 1 /* flags */) {
        submessage_header_flags_ = last_octet_;
        serializer_.swap_bytes(ACE_CDR_BYTE_ORDER != (submessage_header_flags_ & RTPS::FLAG_E));
      } else if (member_id == 2 /* submessageLength */) {
        submessage_length_ = last_uint16_;
        length_at_submessage_start_ = serializer_.length();
      }
    } else if ((type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_DataSubmessageX_xtag>() ||
                type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_DataFragSubmessageX_xtag>()) &&
               member_id == 1 /* octetsToInlineQos */) {
      length_at_inline_qos_ = serializer_.length() - last_uint16_;
    }

    return true;
  }

  virtual bool begin_union()
  {
    return serializer_.good_bit();
  }
  virtual bool end_union()
  {
    return true;
  }
  virtual bool begin_discriminator()
  {
    return serializer_.good_bit();
  }
  virtual bool end_discriminator()
  {
    return true;
  }
  virtual bool begin_union_member()
  {
    return serializer_.good_bit();
  }
  virtual bool end_union_member()
  {
    return true;
  }

  virtual bool begin_array()
  {
    return serializer_.good_bit();
  }
  virtual bool end_array()
  {
    return true;
  }
  virtual bool begin_array_element()
  {
    return serializer_.good_bit();
  }
  virtual bool end_array_element()
  {
    return true;
  }

  virtual bool begin_sequence(const XTypes::TypeIdentifier& type_identifier)
  {
    if (type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_EnvelopeSeq_xtag>()) {
      remaining_sequence_elements_stack_.push_back(-1);
      return true;
    } else if (type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_LongSeq8_xtag>()) {
      remaining_sequence_elements_stack_.push_back((last_uint32_ + 31) / 32);
      return true;
    }

    ACE_CDR::ULong size;
    if (!(serializer_ >> size)) {
      return false;
    }
    remaining_sequence_elements_stack_.push_back(size);
    return true;
  }
  virtual bool elements_remaining()
  {
    return serializer_.good_bit() && remaining_sequence_elements_stack_.back() != 0;
  }
  virtual bool end_sequence(const XTypes::TypeIdentifier& /* type_identifier */)
  {
    remaining_sequence_elements_stack_.pop_back();
    return true;
  }
  virtual bool begin_sequence_element(const XTypes::TypeIdentifier& /* sequence_type_identifier */)
  {
    return serializer_.good_bit();
  }
  virtual bool end_sequence_element(const XTypes::TypeIdentifier& sequence_type_identifier)
  {
    if (sequence_type_identifier == DCPS::getMinimalTypeIdentifier<DCPS::OpenDDS_RTPS_EnvelopeSeq_xtag>()) {
      const size_t read = length_at_submessage_start_ - serializer_.length();
      if (submessage_length_ > read) {
        serializer_.skip(static_cast<unsigned short>(submessage_length_ - read));
      }
      if (submessage_length_ == 0 || serializer_.length() == 0) {
        remaining_sequence_elements_stack_.back() = 1;
      }
    }

    --remaining_sequence_elements_stack_.back();
    return true;
  }

  virtual bool read_boolean(ACE_CDR::Boolean& value)
  {
    return serializer_ >> ACE_InputCDR::to_boolean(value);
  }
  virtual bool read_byte(ACE_CDR::Octet& value)
  {
    const bool retval = serializer_ >> ACE_InputCDR::to_octet(value);
    last_octet_ = value;
    return retval;
  }
  virtual bool read_int8(ACE_CDR::Char& value)
  {
    return serializer_ >> ACE_InputCDR::to_char(value);
  }
  virtual bool read_uint8(ACE_CDR::Octet& value)
  {
    return serializer_ >> ACE_InputCDR::to_octet(value);
  }
  virtual bool read_int16(ACE_CDR::Short& value)
  {
    return serializer_ >> value;
  }
  virtual bool read_uint16(ACE_CDR::UShort& value)
  {
    const bool retval = serializer_ >> value;
    last_uint16_ = value;
    return retval;
  }
  virtual bool read_int32(ACE_CDR::Long& value)
  {
    return serializer_ >> value;
  }
  virtual bool read_uint32(ACE_CDR::ULong& value)
  {
    const bool retval = serializer_ >> value;
    last_uint32_ = value;
    return retval;
  }
  virtual bool read_int64(ACE_CDR::LongLong& value)
  {
    return serializer_ >> value;
  }
  virtual bool read_uint64(ACE_CDR::ULongLong& value)
  {
    return serializer_ >> value;
  }
  virtual bool read_float32(ACE_CDR::Float& value)
  {
    return serializer_ >> value;
  }
  virtual bool read_float64(ACE_CDR::Double& value)
  {
    return serializer_ >> value;
  }
  virtual bool read_float128(ACE_CDR::LongDouble& value)
  {
    return serializer_ >> value;
  }


  virtual bool read_fixed(OpenDDS::FaceTypes::Fixed& value)
  {
    ACE_DEBUG((LM_DEBUG, "### %N:%l %C\n", __func__));
    return false;
  }
  virtual bool read_char8(ACE_CDR::Char& value)
  {
    return serializer_ >> ACE_InputCDR::to_char(value);
  }
  virtual bool read_char16(ACE_CDR::WChar& value)
  {
    return serializer_ >> ACE_InputCDR::to_wchar(value);
    return false;
  }
  virtual bool read_string(DCPS::String& value)
  {
    ACE_CDR::Char* str = 0;
    ACE_CDR::ULong size = serializer_.read_string(str);
    value = DCPS::String(str, size);
    serializer_.free_string(str);
    return true;
  }
  virtual bool read_wstring(DCPS::WString& value)
  {
    ACE_DEBUG((LM_DEBUG, "### %N:%l %C\n", __func__));
    return false;
  }

  virtual bool read_long_enum(ACE_CDR::Long& value, const DCPS::EnumHelper& /*helper*/)
  {
    if (!enum_stack_.empty()) {
      value = enum_stack_.back();
      enum_stack_.pop_back();
      return true;
    }

    ACE_DEBUG((LM_DEBUG, "### %N:%l %C\n", __func__));
    return false;
  }

private:
  DCPS::Message_Block_Ptr in_;
  DCPS::Serializer serializer_;
  std::vector<size_t> member_idx_stack_;
  std::vector<size_t> remaining_sequence_elements_stack_;
  std::vector<ACE_CDR::Long> enum_stack_;

  ACE_CDR::Octet last_octet_;
  ACE_CDR::UShort last_uint16_;
  ACE_CDR::ULong last_uint32_;

  ACE_CDR::Octet submessage_header_flags_;
  ACE_CDR::UShort submessage_length_;

  // Handle skipping data.
  size_t length_at_submessage_start_;

  // Handle inline qos.
  size_t length_at_inline_qos_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  // OPENDDS_DCPS_RTPS_VALUE_READER_H
