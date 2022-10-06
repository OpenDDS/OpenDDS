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
#include "XTypes/DynamicDataAdapter.h"

#ifdef OPENDDS_SECURITY
#  include <dds/DdsSecurityCoreC.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * Servant for DataWriter interface of the Traits::MessageType data type.
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
  typedef NativeSample<MessageType> Sample;

  DataWriterImpl_T(const AbstractTopicType* topic_type)
    : DataWriterImpl(topic_type)
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

    const DDS::ReturnCode_t ret = get_or_create_instance_handle(registered_handle, instance, timestamp);
    if (ret != DDS::RETCODE_OK && log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, ACE_TEXT("(%P|%t) NOTICE: %CDataWriterImpl::register_instance_w_timestamp: ")
                 ACE_TEXT("register failed: %C\n"),
                 topic_type_->name().c_str(),
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
    AbstractSample_rch sample = make_sample(instance_data, /* key_only = */ true);
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
    //  This operation assumes the provided handle is valid. The handle
    //  provided will not be verified.

    if (handle == DDS::HANDLE_NIL) {
      DDS::InstanceHandle_t registered_handle = DDS::HANDLE_NIL;
      const DDS::ReturnCode_t ret =
        this->get_or_create_instance_handle(registered_handle, instance_data, source_timestamp);
      if (ret != DDS::RETCODE_OK && log_level >= LogLevel::Notice) {
        ACE_ERROR_RETURN((LM_NOTICE, ACE_TEXT("(%P|%t) NOTICE: %CDataWriterImpl::write_w_timestamp: ")
                          ACE_TEXT("register failed: %C.\n"),
                          topic_type_->name().c_str(),
                          retcode_to_string(ret)),
                         ret);
      }

      handle = registered_handle;
    }

    // list of reader RepoIds that should not get data
    OpenDDS::DCPS::GUIDSeq_var filter_out;
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    if (TheServiceParticipant->publisher_content_filter()) {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, reader_info_guard, this->reader_info_lock_, DDS::RETCODE_ERROR);
      for (RepoIdToReaderInfoMap::iterator iter = reader_info_.begin(),
           end = reader_info_.end(); iter != end; ++iter) {
        const ReaderInfo& ri = iter->second;
        if (!ri.eval_.is_nil()) {
          if (!filter_out.ptr()) {
            filter_out = new OpenDDS::DCPS::GUIDSeq;
          }
          if (!ri.eval_->eval(instance_data, ri.expression_params_)) {
            push_back(filter_out.inout(), iter->first);
          }
        }
      }
    }
#endif

    Message_Block_Ptr serialized(serialize_sample(instance_data));
    return OpenDDS::DCPS::DataWriterImpl::write(move(serialized),
                                                handle,
                                                source_timestamp,
                                                filter_out._retn(),
                                                &instance_data);
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
#if defined(OPENDDS_SECURITY) && !defined(OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE)
    XTypes::DynamicDataAdapter<MessageType> dda(dynamic_type_, getMetaStruct<MessageType>(), instance_data);
    DDS::Security::SecurityException ex;

    if (security_config_ &&
        participant_permissions_handle_ != DDS::HANDLE_NIL &&
        !security_config_->get_access_control()->check_local_datawriter_dispose_instance(participant_permissions_handle_, this, &dda, ex)) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE,
                   "(%P|%t) NOTICE: DataWriterImpl_T::dispose_instance_w_timestamp: unable to dispose instance SecurityException[%d.%d]: %C\n",
                   ex.code, ex.minor_code, ex.message.in()));
      }
      return DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY;
    }
#endif

    AbstractSample_rch sample = make_sample(instance_data, /* key_only = */ true);
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
    AbstractSample_rch sample = make_sample(instance_data, /* key_only = */ true);
    return DataWriterImpl::lookup_instance(sample);
  }

private:
  AbstractSample_rch make_sample(const MessageType& data, bool key_only = false)
  {
    return dynamic_rchandle_cast<AbstractSample>(make_rch<Sample>(data, key_only));
  }

  const MessageType& get_data(AbstractSample_rch& sample)
  {
    return dynamic_rchandle_cast<Sample>(sample)->data();
  }

  /**
   * Serialize the instance data.
   *
   * @param instance_data The data to serialize.
   * @param key_only Only serialize key fields
   * @return returns the serialized data.
   */
  ACE_Message_Block* serialize_sample(const MessageType& instance_data, bool key_only = false)
  {
    AbstractSample_rch sample = make_sample(instance_data, key_only);
    return DataWriterImpl::serialize_sample(sample);
  }

  /**
   * Find the instance handle for the given instance_data using the data type's
   * key(s). If the instance does not already exist create a new instance
   * handle for it.
   */
  DDS::ReturnCode_t get_or_create_instance_handle(
    DDS::InstanceHandle_t& handle,
    const MessageType& instance_data,
    const DDS::Time_t& source_timestamp)
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, get_lock(), DDS::RETCODE_ERROR);

    AbstractSample_rch sample = make_sample(instance_data);
    handle = DataWriterImpl::lookup_instance(sample);
    const bool needs_creation = handle == DDS::HANDLE_NIL;

    if (needs_creation || !get_handle_instance(handle)) {
#if defined(OPENDDS_SECURITY) && !defined(OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE)
      XTypes::DynamicDataAdapter<MessageType> dda(dynamic_type_, getMetaStruct<MessageType>(), instance_data);
      DDS::Security::SecurityException ex;

      if (security_config_ &&
          participant_permissions_handle_ != DDS::HANDLE_NIL &&
          !security_config_->get_access_control()->check_local_datawriter_register_instance(participant_permissions_handle_, this, &dda, ex)) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE,
                     "(%P|%t) NOTICE: DataWriterImpl_T::get_or_create_instance_handle: unable to register instance SecurityException[%d.%d]: %C\n",
                     ex.code, ex.minor_code, ex.message.in()));
        }
        return DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY;
      }
#endif

      // don't use fast allocator for registration.
      Message_Block_Ptr serialized(serialize_sample(instance_data, /* key_only = */ true));
      if (!serialized) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: %CDataWriterImpl::get_or_create_instance_handle: "
          "failed to serialize sample\n",
          topic_type_->name().c_str()));
        return DDS::RETCODE_ERROR;
      }

      // tell DataWriterLocal and Publisher about the instance.
      const DDS::ReturnCode_t ret = register_instance_i(handle, move(serialized), source_timestamp);
      // note: the WriteDataContainer/PublicationInstance maintains ownership
      // of the marshalled sample.
      if (ret != DDS::RETCODE_OK) {
        handle = DDS::HANDLE_NIL;
        return ret;
      }

      if (needs_creation && !insert_instance(handle, sample)) {
        handle = DDS::HANDLE_NIL;
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %CDataWriterImpl::get_or_create_instance_handle: ")
                          ACE_TEXT("insert instance failed\n"),
                          topic_type_->name().c_str()),
                         DDS::RETCODE_ERROR);
      }

      send_all_to_flush_control(guard);
    }

    return DDS::RETCODE_OK;
  }

  // A class, normally provided by an unit test, that needs access to
  // private methods/members.
  friend class ::DDS_TEST;
};

} // namespace DCPS
} // namepsace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DDS_DCPS_DATAWRITERIMPL_T_H */
