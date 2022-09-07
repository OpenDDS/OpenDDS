/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_WRITE_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_WRITE_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicDataWriteImpl
  : public virtual DCPS::LocalObject<DDS::DynamicData> {
public:
  DynamicDataWriteImpl(DDS::DynamicType_ptr type);

  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& value, MemberId id);
  DDS::ReturnCode_t set_descriptor(MemberId id, DDS::MemberDescriptor* value);

  CORBA::Boolean equals(DDS::DynamicData_ptr other);

  MemberId get_member_id_by_name(const char* name);
  MemberId get_member_id_at_index(ACE_CDR::ULong index);
  ACE_CDR::ULong get_item_count();

  DDS::ReturnCode_t clear_all_values();
  DDS::ReturnCode_t clear_nonkey_values();
  DDS::ReturnCode_t clear_value(DDS::MemberId /*id*/);
  DDS::DynamicData_ptr loan_value(DDS::MemberId /*id*/);
  DDS::ReturnCode_t return_loaned_value(DDS::DynamicData_ptr /*value*/);

  DDS::DynamicData_ptr clone();

  DDS::ReturnCode_t get_int32_value(CORBA::Long&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int32_value(DDS::MemberId id,
                                    CORBA::Long value);

  DDS::ReturnCode_t get_uint32_value(CORBA::ULong&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint32_value(DDS::MemberId id,
                                     CORBA::ULong value);

  DDS::ReturnCode_t get_int8_value(CORBA::Int8&,
                                   DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int8_value(DDS::MemberId id,
                                   CORBA::Int8 value);

  DDS::ReturnCode_t get_uint8_value(CORBA::UInt8&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint8_value(DDS::MemberId id,
                                    CORBA::UInt8 value);

  DDS::ReturnCode_t get_int16_value(CORBA::Short&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int16_value(DDS::MemberId id,
                                    CORBA::Short value);

  DDS::ReturnCode_t get_uint16_value(CORBA::UShort&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint16_value(DDS::MemberId id,
                                     CORBA::UShort value);

  DDS::ReturnCode_t get_int64_value(CORBA::LongLong&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int64_value(DDS::MemberId id,
                                    CORBA::LongLong value);

  DDS::ReturnCode_t get_uint64_value(CORBA::ULongLong&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint64_value(DDS::MemberId id,
                                     CORBA::ULongLong value);

  DDS::ReturnCode_t get_float32_value(CORBA::Float&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float32_value(DDS::MemberId id,
                                      CORBA::Float value);

  DDS::ReturnCode_t get_float64_value(CORBA::Double&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float64_value(DDS::MemberId id,
                                      CORBA::Double value);

  DDS::ReturnCode_t get_float128_value(CORBA::LongDouble&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float128_value(DDS::MemberId id,
                                       CORBA::LongDouble value);

  DDS::ReturnCode_t get_char8_value(CORBA::Char&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char8_value(DDS::MemberId id,
                                    CORBA::Char value);

  DDS::ReturnCode_t get_char16_value(CORBA::WChar&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char16_value(DDS::MemberId id,
                                     CORBA::WChar value);

  DDS::ReturnCode_t get_byte_value(CORBA::Octet&,
                                   DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_byte_value(DDS::MemberId id,
                                   CORBA::Octet value);

  DDS::ReturnCode_t get_boolean_value(CORBA::Boolean&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_boolean_value(DDS::MemberId id,
                                      CORBA::Boolean value);

  DDS::ReturnCode_t get_string_value(char*&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_string_value(DDS::MemberId id,
                                     const char* value);

  DDS::ReturnCode_t get_wstring_value(CORBA::WChar*&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_wstring_value(DDS::MemberId id,
                                      const CORBA::WChar* value);

  DDS::ReturnCode_t get_complex_value(DDS::DynamicData_ptr&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_complex_value(DDS::MemberId id,
                                      DDS::DynamicData_ptr value);

  DDS::ReturnCode_t get_int32_values(DDS::Int32Seq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int32_values(DDS::MemberId id,
                                     const DDS::Int32Seq& value);

  DDS::ReturnCode_t get_uint32_values(DDS::UInt32Seq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint32_values(DDS::MemberId id,
                                      const DDS::UInt32Seq& value);

  DDS::ReturnCode_t get_int8_values(DDS::Int8Seq&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int8_values(DDS::MemberId id,
                                    const DDS::Int8Seq& value);

  DDS::ReturnCode_t get_uint8_values(DDS::UInt8Seq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint8_values(DDS::MemberId id,
                                     const DDS::UInt8Seq& value);

  DDS::ReturnCode_t get_int16_values(DDS::Int16Seq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int16_values(DDS::MemberId id,
                                     const DDS::Int16Seq& value);

  DDS::ReturnCode_t get_uint16_values(DDS::UInt16Seq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint16_values(DDS::MemberId id,
                                      const DDS::UInt16Seq& value);

  DDS::ReturnCode_t get_int64_values(DDS::Int64Seq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int64_values(DDS::MemberId id,
                                     const DDS::Int64Seq& value);

  DDS::ReturnCode_t get_uint64_values(DDS::UInt64Seq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint64_values(DDS::MemberId id,
                                      const DDS::UInt64Seq& value);

  DDS::ReturnCode_t get_float32_values(DDS::Float32Seq&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float32_values(DDS::MemberId id,
                                       const DDS::Float32Seq& value);

  DDS::ReturnCode_t get_float64_values(DDS::Float64Seq&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float64_values(DDS::MemberId id,
                                       const DDS::Float64Seq& value);

  DDS::ReturnCode_t get_float128_values(DDS::Float128Seq&,
                                        DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float128_values(DDS::MemberId id,
                                        const DDS::Float128Seq& value);

  DDS::ReturnCode_t get_char8_values(DDS::CharSeq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char8_values(DDS::MemberId id,
                                     const DDS::CharSeq& value);

  DDS::ReturnCode_t get_char16_values(DDS::WcharSeq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char16_values(DDS::MemberId id,
                                      const DDS::WcharSeq& value);

  DDS::ReturnCode_t get_byte_values(DDS::ByteSeq&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_byte_values(DDS::MemberId id,
                                    const DDS::ByteSeq& value);

  DDS::ReturnCode_t get_boolean_values(DDS::BooleanSeq&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_boolean_values(DDS::MemberId id,
                                       const DDS::BooleanSeq& value);

  DDS::ReturnCode_t get_string_values(DDS::StringSeq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_string_values(DDS::MemberId id,
                                      const DDS::StringSeq& value);

  DDS::ReturnCode_t get_wstring_values(DDS::WstringSeq&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_wstring_values(DDS::MemberId id,
                                       const DDS::WstringSeq& value);

private:
  // The actual, non-alias DynamicType of the associated type.
  DDS::DynamicType_var type_;

  // TODO: Contain data for an instance of a basic type.
  struct SingleValue {
  };

  // TODO: Contain data for an instance of a sequence of a basic type.
  struct SequenceValue {
  };

  // Container for all data written to this DynamicData object.
  struct DataContainer {
    OPENDDS_MAP(DDS::MemberId, SingleValue) single_map_;
    OPENDDS_MAP(DDS::MemberId, SequenceValue) sequence_map_;
    OPENDDS_MAP(DDS::MemberId, DDS::DynamicData_ptr) complex_map;
  };

  DataContainer container_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_WRITE_IMPL_H
