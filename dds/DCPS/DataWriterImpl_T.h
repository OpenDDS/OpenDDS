/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAWRITERIMPL_T_H
#define OPENDDS_DCPS_DATAWRITERIMPL_T_H

#include "AbstractSample.h"
#include "PublicationInstance.h"
#include "DataWriterImpl.h"
#include "Util.h"
#include "TypeSupportImpl.h"
#include "dcps_export.h"
#include "SafetyProfileStreams.h"
#include "DCPS_Utils.h"

#ifdef OPENDDS_SECURITY
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

  virtual DDS::InstanceHandle_t register_instance(const MessageType& instance)
  {
    return register_instance_w_timestamp(instance, SystemTimePoint::now().to_dds_time());
  }

  virtual DDS::InstanceHandle_t register_instance_w_timestamp(
    const MessageType& instance, const DDS::Time_t& timestamp)
  {
    DDS::InstanceHandle_t registered_handle = DDS::HANDLE_NIL;

    AbstractSample_rch sample = make_sample(instance, SampleKeyOnly);
    const DDS::ReturnCode_t ret = get_or_create_instance_handle(registered_handle, sample, timestamp);
    if (ret != DDS::RETCODE_OK && log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, ACE_TEXT("(%P|%t) NOTICE: %CDataWriterImpl::register_instance_w_timestamp: ")
                 ACE_TEXT("register failed: %C\n"),
                 get_type_support()->name(),
                 retcode_to_string(ret)));
    }

    return registered_handle;
  }

  virtual DDS::ReturnCode_t unregister_instance(const MessageType& instance, DDS::InstanceHandle_t handle)
  {
    return unregister_instance_w_timestamp(instance, handle, SystemTimePoint::now().to_dds_time());
  }

  virtual DDS::ReturnCode_t unregister_instance_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t instance_handle,
    const DDS::Time_t& timestamp)
  {
    AbstractSample_rch sample = make_sample(instance_data, SampleKeyOnly);
    return DataWriterImpl::unregister_instance_w_timestamp(sample, instance_handle, timestamp);
  }

  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  virtual DDS::ReturnCode_t
  write(const MessageType& instance_data, DDS::InstanceHandle_t handle)
  {
    return write_w_timestamp(instance_data, handle, SystemTimePoint::now().to_dds_time());
  }

  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  virtual DDS::ReturnCode_t write_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t handle,
    const DDS::Time_t& source_timestamp)
  {
    return DataWriterImpl::write_w_timestamp(make_sample(instance_data), handle, source_timestamp);
  }

  virtual DDS::ReturnCode_t
  dispose(const MessageType& instance_data, DDS::InstanceHandle_t instance_handle)
  {
    return dispose_w_timestamp(instance_data, instance_handle, SystemTimePoint::now().to_dds_time());
  }

  virtual DDS::ReturnCode_t dispose_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t instance_handle,
    const DDS::Time_t& source_timestamp)
  {
    AbstractSample_rch sample = make_sample(instance_data, SampleKeyOnly);
    return DataWriterImpl::dispose_w_timestamp(sample, instance_handle, source_timestamp);
  }

  virtual DDS::ReturnCode_t get_key_value(
    MessageType& key_holder,
    DDS::InstanceHandle_t handle)
  {
    AbstractSample_rch sample;
    const DDS::ReturnCode_t rc = DataWriterImpl::get_key_value(sample, handle);
    if (sample) {
      key_holder = get_data(sample);
    }
    return rc;
  }

  DDS::InstanceHandle_t lookup_instance(const MessageType& instance_data)
  {
    AbstractSample_rch sample = make_sample(instance_data, SampleKeyOnly);
    return DataWriterImpl::lookup_instance(sample);
  }

private:
  typedef Sample<MessageType> SampleType;

  AbstractSample_rch make_sample(const MessageType& data, SampleExtent extent = SampleFull)
  {
    return dynamic_rchandle_cast<AbstractSample>(make_rch<SampleType>(data, extent));
  }

  const MessageType& get_data(AbstractSample_rch& sample)
  {
    return dynamic_rchandle_cast<SampleType>(sample)->data();
  }

  // A class, normally provided by an unit test, that needs access to
  // private methods/members.
  friend class ::DDS_TEST;
};

} // namespace DCPS
} // namepsace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DDS_DCPS_DATAWRITERIMPL_T_H */
