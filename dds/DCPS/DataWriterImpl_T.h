/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAWRITERIMPL_T_H
#define OPENDDS_DCPS_DATAWRITERIMPL_T_H

#include "DCPS_Utils.h"
#include "DataWriterImpl.h"
#include "PublicationInstance.h"
#include "SafetyProfileStreams.h"
#include "Sample.h"
#include "TypeSupportImpl.h"
#include "Util.h"
#include "dcps_export.h"

#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY
#  include <dds/DdsSecurityCoreC.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * Servant for DataWriter interface of the MessageType data type.
 *
 * See the DDS specification, OMG formal/2015-04-10, for a description of
 * this interface.
 */
template <typename MessageType>
class
#if ( __GNUC__ == 4 && __GNUC_MINOR__ == 1)
  OpenDDS_Dcps_Export
#endif
DataWriterImpl_T
: public virtual LocalObject<typename DDSTraits<MessageType>::DataWriterType>
, public virtual DataWriterImpl
{
public:
  DataWriterImpl_T()
  {
  }

  virtual ~DataWriterImpl_T()
  {
  }

  DDS::InstanceHandle_t register_instance(const MessageType& instance)
  {
    return register_instance_w_timestamp(instance, SystemTimePoint::now().to_dds_time());
  }

  DDS::InstanceHandle_t register_instance_w_timestamp(
    const MessageType& instance, const DDS::Time_t& timestamp)
  {
    const SampleType sample(instance, Sample::KeyOnly);
    return DataWriterImpl::register_instance_w_timestamp(sample, timestamp);
  }

  DDS::ReturnCode_t unregister_instance(const MessageType& instance, DDS::InstanceHandle_t handle)
  {
    return unregister_instance_w_timestamp(instance, handle, SystemTimePoint::now().to_dds_time());
  }

  DDS::ReturnCode_t unregister_instance_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t instance_handle,
    const DDS::Time_t& timestamp)
  {
    const SampleType sample(instance_data, Sample::KeyOnly);
    return DataWriterImpl::unregister_instance_w_timestamp(sample, instance_handle, timestamp);
  }

  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  DDS::ReturnCode_t write(const MessageType& instance_data, DDS::InstanceHandle_t handle)
  {
    return write_w_timestamp(instance_data, handle, SystemTimePoint::now().to_dds_time());
  }

  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  DDS::ReturnCode_t write_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t handle,
    const DDS::Time_t& source_timestamp)
  {
    const SampleType sample(instance_data);
    return DataWriterImpl::write_w_timestamp(sample, handle, source_timestamp);
  }

  DDS::ReturnCode_t dispose(const MessageType& instance_data, DDS::InstanceHandle_t instance_handle)
  {
    return dispose_w_timestamp(instance_data, instance_handle, SystemTimePoint::now().to_dds_time());
  }

  DDS::ReturnCode_t dispose_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t instance_handle,
    const DDS::Time_t& source_timestamp)
  {
    const SampleType sample(instance_data, Sample::KeyOnly);
    return DataWriterImpl::dispose_w_timestamp(sample, instance_handle, source_timestamp);
  }

  DDS::ReturnCode_t get_key_value(MessageType& key_holder, DDS::InstanceHandle_t handle)
  {
    Sample_rch sample;
    const DDS::ReturnCode_t rc = DataWriterImpl::get_key_value(sample, handle);
    if (sample) {
      key_holder = dynamic_rchandle_cast<SampleType>(sample)->data();
    }
    return rc;
  }

  DDS::InstanceHandle_t lookup_instance(const MessageType& instance_data)
  {
    const SampleType sample(instance_data, Sample::KeyOnly);
    return DataWriterImpl::lookup_instance(sample);
  }

private:
  typedef Sample_T<MessageType> SampleType;

  // A class, normally provided by an unit test, that needs access to
  // private methods/members.
  friend class ::DDS_TEST;
};

} // namespace DCPS
} // namepsace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DDS_DCPS_DATAWRITERIMPL_T_H */
