/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H

#ifndef OPENDDS_SAFETY_PROFILE
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include <dds/DdsDynamicDataC.h>
#include <dds/DCPS/FilterEvaluator.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

template <typename T>
class DynamicDataAdapter : public DDS::DynamicData {
public:
  DynamicDataAdapter(DDS::DynamicType_ptr type,
                    const DCPS::MetaStruct& meta_struct,
                    const T& value)
    : type_(DDS::DynamicType::_duplicate(type))
    , meta_struct_(meta_struct)
    , value_(value)
  {}

  DDS::DynamicType_ptr type()
  {
    return type_;
  }

  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& value,
                                   DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    return dtm->get_descriptor(value);
  }

  DDS::ReturnCode_t set_descriptor(DDS::MemberId,
                                   DDS::MemberDescriptor*)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  CORBA::Boolean equals(DDS::DynamicData_ptr)
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataAdapter::equals is not implemented\n"));
    return 0;
  }

  DDS::MemberId get_member_id_by_name(const char* name)
  {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member_by_name(dtm, name) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }

    return dtm->get_id();
  }

  DDS::MemberId get_member_id_at_index(CORBA::ULong index)
  {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member_by_index(dtm, index) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }

    return dtm->get_id();
  }

  CORBA::ULong get_item_count()
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataAdapter::get_item_count is not implemented\n"));
    return 0;
  }

  DDS::ReturnCode_t clear_all_values()
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t clear_nonkey_values()
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t clear_value(DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::DynamicData_ptr loan_value(DDS::MemberId)
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataAdapter::loan_value is not implemented\n"));
    return 0;
  }

  DDS::ReturnCode_t return_loaned_value(DDS::DynamicData_ptr)
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataAdapter::return_loaned_value is not implemented\n"));
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::DynamicData_ptr clone()
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataAdapter::clone is not implemented\n"));
    return 0;
  }

  DDS::ReturnCode_t get_int32_value(CORBA::Long& value,
                                    DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_INT32) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.i_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_int32_value(DDS::MemberId,
                                    CORBA::Long)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint32_value(CORBA::ULong& value,
                                     DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_UINT32) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.u_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_uint32_value(DDS::MemberId,
                                     CORBA::ULong)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int8_value(CORBA::Int8& value,
                                   DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_INT8) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.i_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_int8_value(DDS::MemberId,
                                   CORBA::Int8)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint8_value(CORBA::UInt8& value,
                                    DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_UINT8) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.u_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_uint8_value(DDS::MemberId,
                                    CORBA::UInt8)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int16_value(CORBA::Short& value,
                                    DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_INT16) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.i_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_int16_value(DDS::MemberId,
                                    CORBA::Short)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint16_value(CORBA::UShort& value,
                                     DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_UINT16) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.u_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_uint16_value(DDS::MemberId,
                                     CORBA::UShort)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int64_value(CORBA::LongLong& value,
                                    DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_INT64) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.l_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_int64_value(DDS::MemberId,
                                    CORBA::LongLong)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint64_value(CORBA::ULongLong& value,
                                     DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_UINT64) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.m_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_uint64_value(DDS::MemberId,
                                     CORBA::ULongLong)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float32_value(CORBA::Float& value,
                                      DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_FLOAT32) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    // FUTURE: Remove cast once float type is available.
    value = static_cast<float>(v.f_);
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_float32_value(DDS::MemberId,
                                      CORBA::Float)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float64_value(CORBA::Double& value,
                                      DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_FLOAT64) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.f_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_float64_value(DDS::MemberId,
                                      CORBA::Double)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float128_value(CORBA::LongDouble& value,
                                       DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_FLOAT128) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.ld_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_float128_value(DDS::MemberId,
                                       CORBA::LongDouble)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_char8_value(CORBA::Char& value,
                                    DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_CHAR8) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.c_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_char8_value(DDS::MemberId,
                                    CORBA::Char)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_char16_value(CORBA::WChar&,
                                     DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_char16_value(DDS::MemberId,
                                     CORBA::WChar)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_byte_value(CORBA::Octet&,
                                   DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_byte_value(DDS::MemberId,
                                   CORBA::Octet)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_boolean_value(CORBA::Boolean& value,
                                      DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_BOOLEAN) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = v.b_;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_boolean_value(DDS::MemberId,
                                      CORBA::Boolean)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_string_value(char*& value,
                                     DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member(member, id) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return DDS::RETCODE_ERROR;
    }

    DDS::DynamicType_var type = get_base_type(md->type());
    if (type->get_kind() != TK_STRING8) {
      return DDS::RETCODE_ERROR;
    }

    const DCPS::Value v = meta_struct_.getValue(&value_, id);
    value = CORBA::string_dup(v.s_);
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t set_string_value(DDS::MemberId,
                                     const char*)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_wstring_value(CORBA::WChar *&,
                                      DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_wstring_value(DDS::MemberId,
                                      const CORBA::WChar*)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_complex_value(DDS::DynamicData_ptr&,
                                      DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_complex_value(DDS::MemberId,
                                      DDS::DynamicData_ptr)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int32_values(DDS::Int32Seq&,
                                     DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_int32_values(DDS::MemberId,
                                     const DDS::Int32Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint32_values(DDS::UInt32Seq&,
                                      DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_uint32_values(DDS::MemberId,
                                      const DDS::UInt32Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int8_values(DDS::Int8Seq&,
                                    DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_int8_values(DDS::MemberId,
                                    const DDS::Int8Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint8_values(DDS::UInt8Seq&,
                                     DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_uint8_values(DDS::MemberId,
                                     const DDS::UInt8Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int16_values(DDS::Int16Seq&,
                                     DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_int16_values(DDS::MemberId,
                                     const DDS::Int16Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint16_values(DDS::UInt16Seq&,
                                      DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_uint16_values(DDS::MemberId,
                                      const DDS::UInt16Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int64_values(DDS::Int64Seq&,
                                     DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_int64_values(DDS::MemberId,
                                     const DDS::Int64Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint64_values(DDS::UInt64Seq&,
                                      DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_uint64_values(DDS::MemberId,
                                      const DDS::UInt64Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float32_values(DDS::Float32Seq&,
                                       DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_float32_values(DDS::MemberId,
                                       const DDS::Float32Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float64_values(DDS::Float64Seq&,
                                       DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_float64_values(DDS::MemberId,
                                       const DDS::Float64Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float128_values(DDS::Float128Seq&,
                                        DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_float128_values(DDS::MemberId,
                                        const DDS::Float128Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_char8_values(DDS::CharSeq&,
                                     DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_char8_values(DDS::MemberId,
                                     const DDS::CharSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_char16_values(DDS::WcharSeq&,
                                      DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_char16_values(DDS::MemberId,
                                      const DDS::WcharSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_byte_values(DDS::ByteSeq&,
                                    DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_byte_values(DDS::MemberId,
                                    const DDS::ByteSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_boolean_values(DDS::BooleanSeq&,
                                       DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_boolean_values(DDS::MemberId,
                                       const DDS::BooleanSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_string_values(DDS::StringSeq&,
                                      DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_string_values(DDS::MemberId,
                                      const DDS::StringSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_wstring_values(DDS::WstringSeq&,
                                       DDS::MemberId)
  {
    // FUTURE
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t set_wstring_values(DDS::MemberId,
                                       const DDS::WstringSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

private:
  DDS::DynamicType_var type_;
  const DCPS::MetaStruct& meta_struct_;
  const T& value_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H
