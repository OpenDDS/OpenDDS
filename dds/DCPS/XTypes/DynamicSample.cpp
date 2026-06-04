/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "DynamicSample.h"

#include "DynamicDataImpl.h"
#include "DynamicDataXcdrReadImpl.h"
#include "Utils.h"

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/debug.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

using namespace OpenDDS::DCPS;

namespace {

enum ValidationResult {
  VALID,
  ABSENT,
  INVALID
};

ValidationResult validate_dynamic_data(DDS::DynamicData_ptr data, DDS::DynamicType_ptr type);

ValidationResult result(DDS::ReturnCode_t rc)
{
  return rc == DDS::RETCODE_OK ? VALID : rc == DDS::RETCODE_NO_DATA ? ABSENT : INVALID;
}

ValidationResult validate_scalar(DDS::DynamicData_ptr data, DDS::MemberId id, DDS::DynamicType_ptr type)
{
  const DDS::DynamicType_var base = get_base_type(type);
  if (!data || !base) {
    return INVALID;
  }

  switch (base->get_kind()) {
  case TK_BOOLEAN: {
    DDS::Boolean value = false;
    return result(data->get_boolean_value(value, id));
  }
  case TK_BYTE: {
    DDS::Byte value = 0;
    return result(data->get_byte_value(value, id));
  }
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64: {
    DDS::UInt64 value = 0;
    return result(get_uint_value(value, data, id, base->get_kind()));
  }
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64: {
    DDS::Int64 value = 0;
    return result(get_int_value(value, data, id, base->get_kind()));
  }
  case TK_FLOAT32: {
    DDS::Float32 value = 0;
    return result(data->get_float32_value(value, id));
  }
  case TK_FLOAT64: {
    DDS::Float64 value = 0;
    return result(data->get_float64_value(value, id));
  }
  case TK_FLOAT128: {
    DDS::Float128 value = ACE_CDR_LONG_DOUBLE_INITIALIZER;
    return result(data->get_float128_value(value, id));
  }
  case TK_CHAR8: {
    CORBA::Char value = 0;
    return result(data->get_char8_value(value, id));
  }
  case TK_CHAR16: {
    CORBA::WChar value = 0;
    return result(data->get_char16_value(value, id));
  }
  case TK_STRING8: {
    CORBA::String_var value;
    return result(data->get_string_value(value, id));
  }
  case TK_STRING16: {
    CORBA::WString_var value;
    return result(data->get_wstring_value(value, id));
  }
  case TK_ENUM: {
    DDS::Int32 value = 0;
    return result(get_enum_value(value, base, data, id));
  }
  case TK_BITMASK: {
    DDS::UInt64 value = 0;
    return result(get_bitmask_value(value, base, data, id));
  }
  default:
    return INVALID;
  }
}

ValidationResult validate_member(DDS::DynamicData_ptr data, DDS::MemberId id, DDS::DynamicType_ptr type)
{
  const DDS::DynamicType_var base = get_base_type(type);
  if (!base) {
    return INVALID;
  }

  if (is_complex(base->get_kind())) {
    DDS::DynamicData_var nested;
    const DDS::ReturnCode_t rc = data->get_complex_value(nested, id);
    if (rc != DDS::RETCODE_OK) {
      return result(rc);
    }
    return validate_dynamic_data(nested, base);
  }

  return validate_scalar(data, id, base);
}

ValidationResult validate_dynamic_data(DDS::DynamicData_ptr data, DDS::DynamicType_ptr type)
{
  const DDS::DynamicType_var base = get_base_type(type);
  if (!data || !base) {
    return INVALID;
  }

  switch (base->get_kind()) {
  case TK_STRUCTURE: {
    const DDS::UInt32 count = base->get_member_count();
    for (DDS::UInt32 i = 0; i != count; ++i) {
      DDS::DynamicTypeMember_var member;
      if (base->get_member_by_index(member, i) != DDS::RETCODE_OK) {
        return INVALID;
      }
      DDS::MemberDescriptor_var md;
      if (member->get_descriptor(md) != DDS::RETCODE_OK) {
        return INVALID;
      }
      if (validate_member(data, md->id(), md->type()) == INVALID) {
        return INVALID;
      }
    }
    return VALID;
  }
  case TK_UNION: {
    DDS::TypeDescriptor_var td;
    if (base->get_descriptor(td) != DDS::RETCODE_OK) {
      return INVALID;
    }
    if (validate_scalar(data, DISCRIMINATOR_ID, td->discriminator_type()) != VALID) {
      return INVALID;
    }
    if (data->get_item_count() <= 1) {
      return VALID;
    }
    const DDS::MemberId branch_id = data->get_member_id_at_index(1);
    DDS::DynamicTypeMember_var member;
    if (base->get_member(member, branch_id) != DDS::RETCODE_OK) {
      return INVALID;
    }
    DDS::MemberDescriptor_var md;
    if (member->get_descriptor(md) != DDS::RETCODE_OK) {
      return INVALID;
    }
    return validate_member(data, branch_id, md->type()) == INVALID ? INVALID : VALID;
  }
  case TK_ARRAY:
  case TK_SEQUENCE: {
    DDS::TypeDescriptor_var td;
    if (base->get_descriptor(td) != DDS::RETCODE_OK) {
      return INVALID;
    }
    const DDS::DynamicType_var element_type = get_base_type(td->element_type());
    const DDS::UInt32 count = base->get_kind() == TK_ARRAY ? bound_total(td) : data->get_item_count();
    for (DDS::UInt32 i = 0; i != count; ++i) {
      if (validate_member(data, i, element_type) != VALID) {
        return INVALID;
      }
    }
    return VALID;
  }
  case TK_MAP:
    return VALID;
  default:
    return validate_scalar(data, MEMBER_ID_INVALID, base);
  }
}

}

DynamicSample::DynamicSample()
{}

DynamicSample::DynamicSample(const DynamicSample& d)
  : Sample(d.mutability_, d.extent_)
  , data_(d.data_)
{}

DynamicSample& DynamicSample::operator=(const DynamicSample& rhs)
{
  if (this != &rhs) {
    mutability_ = rhs.mutability_;
    extent_ = rhs.extent_;
    data_ = rhs.data_;
  }
  return *this;
}

size_t DynamicSample::serialized_size(const Encoding& enc) const
{
  const DynamicDataBase* const ddb = dynamic_cast<DynamicDataBase*>(data_.in());
  if (!ddb) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicSample::serialized_size: "
                 "DynamicData must be a DynamicDataBase\n"));
    }
    return 0;
  }
  size_t size = 0;
  if (!ddb->serialized_size(enc, size, extent_)) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicSample::serialized_size: "
                 "DynamicDataBase::serialized_size failed!\n"));
    }
    return 0;
  }
  return size;
}

bool DynamicSample::serialize(Serializer& ser) const
{
  const DynamicDataBase* const ddb = dynamic_cast<DynamicDataBase*>(data_.in());
  if (!ddb) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicSample::serialize: "
                 "DynamicData must be a DynamicDataBase\n"));
    }
    return false;
  }
  return ddb->serialize(ser, extent_);
}

bool DynamicSample::deserialize(Serializer& ser)
{
  // DynamicDataXcdrReadImpl uses a message block to read the data on demand,
  // but it can't be the same message block that 'ser' already has.  That one
  // (or more than one if there's chaining) uses the allocators and locking
  // strategy from the transport.
  const ACE_CDR::ULong len = static_cast<ACE_CDR::ULong>(ser.length());
  ACE_Allocator* const alloc = ACE_Allocator::instance();
  const Message_Block_Ptr mb(new(alloc->malloc(sizeof(ACE_Message_Block)))
    ACE_Message_Block(len, ACE_Message_Block::MB_DATA, 0, 0, alloc, 0,
      ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY, ACE_Time_Value::zero,
      ACE_Time_Value::max_time, alloc, alloc));
  unsigned char* const out = reinterpret_cast<unsigned char*>(mb->wr_ptr());
  if (!ser.read_octet_array(out, len)) {
    return false;
  }
  mb->wr_ptr(len);

  if (!data_) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicSample::deserialize: data_ is null, cannot deserialize\n"));
    }
    return false;
  }

  const DDS::DynamicType_var type = data_->type();
  DDS::DynamicData_var back = new DynamicDataXcdrReadImpl(mb.get(), ser.encoding(), type, extent_);
  if (validate_dynamic_data(back, type) != VALID) {
    return false;
  }
  data_ = new DynamicDataImpl(type, back);
  return true;
}

bool DynamicSample::compare(const Sample& other) const
{
  const DynamicSample* const other_same_kind = dynamic_cast<const DynamicSample*>(&other);
  OPENDDS_ASSERT(other_same_kind);
  bool is_less_than = false;
  DDS::ReturnCode_t rc = key_less_than(is_less_than, data_, other_same_kind->data_);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicSample::compare: "
        "key_less_than returned %C\n", retcode_to_string(rc)));
    }
  }
  return is_less_than;
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
