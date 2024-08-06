/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_WRITER_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_WRITER_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include <dds/DdsDynamicTypeSupportC.h>

#include <dds/DCPS/DataWriterImpl.h>

#include "DynamicSample.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicDataWriterImpl
  : public virtual DCPS::LocalObject<DDS::DynamicDataWriter>
  , public virtual DCPS::DataWriterImpl
{
public:
  DDS::InstanceHandle_t register_instance(DDS::DynamicData_ptr instance)
  {
    return register_instance_w_timestamp(instance, DCPS::SystemTimePoint::now().to_idl_struct());
  }

  DDS::InstanceHandle_t register_instance_w_timestamp(
    DDS::DynamicData_ptr instance, const DDS::Time_t& timestamp)
  {
    const DynamicSample sample(instance, DCPS::Sample::KeyOnly);
    return DataWriterImpl::register_instance_w_timestamp(sample, timestamp);
  }

  DDS::ReturnCode_t unregister_instance(
    DDS::DynamicData_ptr instance, DDS::InstanceHandle_t handle)
  {
    return unregister_instance_w_timestamp(instance, handle,
      DCPS::SystemTimePoint::now().to_idl_struct());
  }

  DDS::ReturnCode_t unregister_instance_w_timestamp(
    DDS::DynamicData_ptr instance, DDS::InstanceHandle_t handle, const DDS::Time_t& timestamp)
  {
    const DynamicSample sample(instance, DCPS::Sample::KeyOnly);
    return DataWriterImpl::unregister_instance_w_timestamp(sample, handle, timestamp);
  }

  DDS::ReturnCode_t write(DDS::DynamicData_ptr instance_data, DDS::InstanceHandle_t handle)
  {
    return write_w_timestamp(instance_data, handle, DCPS::SystemTimePoint::now().to_idl_struct());
  }

  DDS::ReturnCode_t write_w_timestamp(DDS::DynamicData_ptr instance_data,
    DDS::InstanceHandle_t handle, const DDS::Time_t& source_timestamp)
  {
    const DynamicSample sample(instance_data, DCPS::Sample::Full);
    return DataWriterImpl::write_w_timestamp(sample, handle, source_timestamp);
  }

  DDS::ReturnCode_t dispose(
    DDS::DynamicData_ptr instance_data, DDS::InstanceHandle_t instance_handle)
  {
    return dispose_w_timestamp(instance_data, instance_handle,
      DCPS::SystemTimePoint::now().to_idl_struct());
  }

  DDS::ReturnCode_t dispose_w_timestamp(DDS::DynamicData_ptr instance_data,
    DDS::InstanceHandle_t instance_handle, const DDS::Time_t& source_timestamp)
  {
    const DynamicSample sample(instance_data, DCPS::Sample::KeyOnly);
    return DataWriterImpl::dispose_w_timestamp(sample, instance_handle, source_timestamp);
  }

  DDS::ReturnCode_t get_key_value(DDS::DynamicData_ptr& key_holder, DDS::InstanceHandle_t handle)
  {
    DCPS::Sample_rch sample;
    const DDS::ReturnCode_t rc = DataWriterImpl::get_key_value(sample, handle);
    if (sample) {
      CORBA::release(key_holder);
      DDS::DynamicData_var result = sample->get_dynamic_data(0);
      key_holder = result._retn();
    }
    return rc;
  }

  DDS::InstanceHandle_t lookup_instance(DDS::DynamicData_ptr instance_data)
  {
    const DynamicSample sample(instance_data, DCPS::Sample::KeyOnly);
    return DataWriterImpl::lookup_instance(sample);
  }

  bool _is_a(const char* type_id)
  {
    return DDS::DynamicDataWriter::_is_a(type_id);
  }

  const char* _interface_repository_id() const
  {
    return DDS::DynamicDataWriter::_interface_repository_id();
  }

  bool marshal(TAO_OutputCDR&)
  {
    return false;
  }
};

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
#endif
