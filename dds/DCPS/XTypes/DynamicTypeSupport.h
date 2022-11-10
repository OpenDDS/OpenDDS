/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_SUPPORT_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_SUPPORT_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/Sample.h>
#  include <dds/DCPS/DataWriterImpl.h>
#  include <dds/DCPS/RcHandle_T.h>

#  include <dds/DdsDynamicTypeSupportC.h>
#  include <dds/DdsDcpsTypeSupportExtC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicSample : public DCPS::Sample {
public:
  DynamicSample(DDS::DynamicData_ptr data,
    DCPS::Sample::Mutability mutability, DCPS::Sample::Extent extent)
  : Sample(mutability, extent)
  , data_(data)
  {
  }

  DynamicSample(DDS::DynamicData_ptr data, DCPS::Sample::Extent extent)
  : Sample(DCPS::Sample::ReadOnly, extent)
  , data_(data)
  {
  }

  bool serialize(DCPS::Serializer& ser) const;
  bool deserialize(DCPS::Serializer& ser);
  size_t serialized_size(const DCPS::Encoding& enc) const;
  bool compare(const DCPS::Sample& other) const;

  bool to_message_block(ACE_Message_Block&) const
  {
    // Not needed?
    OPENDDS_ASSERT(false);
    return false;
  }

  bool from_message_block(const ACE_Message_Block&)
  {
    // Not needed?
    OPENDDS_ASSERT(false);
    return false;
  }

  DCPS::Sample_rch copy(DCPS::Sample::Mutability mutability, DCPS::Sample::Extent extent) const
  {
    return DCPS::dynamic_rchandle_cast<Sample>(DCPS::make_rch<DynamicSample>(
      data_->clone(), mutability, extent));
  }

  DDS::DynamicData_var get_dynamic_data(DDS::DynamicType_ptr) const
  {
    return data_;
  }

  const void* native_data() const
  {
    return 0;
  }

#  ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  bool eval(DCPS::FilterEvaluator& evaluator, const DDS::StringSeq& params) const
  {
    ACE_UNUSED_ARG(evaluator);
    ACE_UNUSED_ARG(params);
    return false;
  }
#  endif

protected:
  DDS::DynamicData_var data_;
};

class OpenDDS_Dcps_Export DynamicDataWriterImpl
: public virtual DCPS::LocalObject<DDS::DynamicDataWriter>
, public virtual DCPS::DataWriterImpl
{
public:
  DDS::InstanceHandle_t register_instance(DDS::DynamicData_ptr instance)
  {
    return register_instance_w_timestamp(instance, DCPS::SystemTimePoint::now().to_dds_time());
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
      DCPS::SystemTimePoint::now().to_dds_time());
  }

  DDS::ReturnCode_t unregister_instance_w_timestamp(
    DDS::DynamicData_ptr instance, DDS::InstanceHandle_t handle, const DDS::Time_t& timestamp)
  {
    const DynamicSample sample(instance, DCPS::Sample::KeyOnly);
    return DataWriterImpl::unregister_instance_w_timestamp(sample, handle, timestamp);
  }

  DDS::ReturnCode_t write(DDS::DynamicData_ptr instance_data, DDS::InstanceHandle_t handle)
  {
    return write_w_timestamp(instance_data, handle, DCPS::SystemTimePoint::now().to_dds_time());
  }

  DDS::ReturnCode_t write_w_timestamp(DDS::DynamicData_ptr instance_data,
    DDS::InstanceHandle_t handle, const DDS::Time_t& source_timestamp)
  {
    const DynamicSample sample(instance_data, DCPS::Sample::KeyOnly);
    return DataWriterImpl::write_w_timestamp(sample, handle, source_timestamp);
  }

  DDS::ReturnCode_t dispose(
    DDS::DynamicData_ptr instance_data, DDS::InstanceHandle_t instance_handle)
  {
    return dispose_w_timestamp(instance_data, instance_handle,
      DCPS::SystemTimePoint::now().to_dds_time());
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
      key_holder = sample->get_dynamic_data(0);
    }
    return rc;
  }

  DDS::InstanceHandle_t lookup_instance(DDS::DynamicData_ptr instance_data)
  {
    const DynamicSample sample(instance_data, DCPS::Sample::KeyOnly);
    return DataWriterImpl::lookup_instance(sample);
  }

  CORBA::Boolean _is_a(const char* type_id)
  {
    return DDS::DynamicDataWriter::_is_a(type_id);
  }

  const char* _interface_repository_id() const
  {
    return DDS::DynamicDataWriter::_interface_repository_id();
  }

  CORBA::Boolean marshal(TAO_OutputCDR&)
  {
    return false;
  }
};

} // namespace XTypes
} // namespace OpenDDS

namespace DDS {

class DynamicTypeSupport;
typedef DynamicTypeSupport* DynamicTypeSupport_ptr;

typedef TAO_Objref_Var_T<DynamicTypeSupport> DynamicTypeSupport_var;

class OpenDDS_Dcps_Export DynamicTypeSupport
: public virtual OpenDDS::DCPS::TypeSupportImpl
, public virtual DynamicTypeSupportInterf
{
public:
  typedef DynamicTypeSupport_ptr _ptr_type;
  typedef DynamicTypeSupport_var _var_type;

  DynamicTypeSupport(DynamicType_var type)
  : type_(type)
  , name_(type->get_name())
  {
  }

  virtual ~DynamicTypeSupport() {}

  const char* name() const
  {
    return name_.in();
  }

  size_t key_count() const;

  void representations_allowed_by_type(DataRepresentationIdSeq& seq);

  OpenDDS::DCPS::Extensibility base_extensibility() const;
  OpenDDS::DCPS::Extensibility max_extensibility() const;

  OpenDDS::DCPS::SerializedSizeBound serialized_size_bound(const OpenDDS::DCPS::Encoding&) const
  {
    return OpenDDS::DCPS::SerializedSizeBound();
  }

  OpenDDS::DCPS::SerializedSizeBound key_only_serialized_size_bound(
    const OpenDDS::DCPS::Encoding&) const
  {
    return OpenDDS::DCPS::SerializedSizeBound();
  }

  DataWriter_ptr create_datawriter();
  DataReader_ptr create_datareader();
#  ifndef OPENDDS_NO_MULTI_TOPIC
  DataReader_ptr create_multitopic_datareader();
#  endif

  const OpenDDS::XTypes::TypeIdentifier& getMinimalTypeIdentifier() const;
  const OpenDDS::XTypes::TypeMap& getMinimalTypeMap() const;
  const OpenDDS::XTypes::TypeIdentifier& getCompleteTypeIdentifier() const;
  const OpenDDS::XTypes::TypeMap& getCompleteTypeMap() const;

  DynamicType_ptr get_type()
  {
    return DynamicType::_duplicate(type_);
  }

#  ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  const OpenDDS::DCPS::MetaStruct& getMetaStructForType()
  {
    OPENDDS_ASSERT(false);
    const OpenDDS::DCPS::MetaStruct* const ms = 0;
    return *ms;
  }
#  endif

  CORBA::Boolean _is_a(const char* type_id)
  {
    return DynamicTypeSupportInterf::_is_a(type_id);
  }

  const char* _interface_repository_id() const
  {
    return DynamicTypeSupportInterf::_interface_repository_id();
  }

  CORBA::Boolean marshal(TAO_OutputCDR&)
  {
    return false;
  }

  static DynamicTypeSupport_ptr _duplicate(DynamicTypeSupport_ptr obj);

protected:
  DynamicType_var type_;
  CORBA::String_var name_;
};

} // namespace DDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#  ifndef DDS_DYNAMICTYPESUPPORT__TRAITS_
#    define DDS_DYNAMICTYPESUPPORT__TRAITS_
TAO_BEGIN_VERSIONED_NAMESPACE_DECL
namespace TAO {
template<>
struct OpenDDS_Dcps_Export Objref_Traits<DDS::DynamicTypeSupport> {
  static DDS::DynamicTypeSupport_ptr duplicate(DDS::DynamicTypeSupport_ptr p);
  static void release(DDS::DynamicTypeSupport_ptr p);
  static DDS::DynamicTypeSupport_ptr nil();
  static CORBA::Boolean marshal(const DDS::DynamicTypeSupport_ptr p, TAO_OutputCDR & cdr);
};
} // namespace TAO
TAO_END_VERSIONED_NAMESPACE_DECL
#  endif

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_TYPE_SUPPORT_H
