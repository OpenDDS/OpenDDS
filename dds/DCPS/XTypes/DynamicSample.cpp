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

  const DDS::DynamicType_var type = data_->type();
  DDS::DynamicData_var back = new DynamicDataXcdrReadImpl(mb.get(), ser.encoding(), type, extent_);
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
