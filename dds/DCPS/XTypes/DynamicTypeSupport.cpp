/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicTypeSupport.h"

#  include "DynamicDataImpl.h"
#  include "DynamicDataXcdrReadImpl.h"
#  include "DynamicTypeImpl.h"
#  include "Utils.h"

#  include <dds/DCPS/debug.h>
#  include <dds/DCPS/DCPS_Utils.h>

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
  const DynamicDataImpl* const ddi = dynamic_cast<DynamicDataImpl*>(data_.in());
  if (!ddi) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicSample::serialized_size: "
        "DynamicData must be DynamicDataImpl, the type supplied by DynamicDataFactory\n"));
    }
    return 0;
  }
  return DCPS::serialized_size(enc, *ddi);
}

bool DynamicSample::serialize(Serializer& ser) const
{
  const DynamicDataImpl* const ddi = dynamic_cast<DynamicDataImpl*>(data_.in());
  if (!ddi) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicSample::serialize: "
        "DynamicData must be DynamicDataImpl, the type supplied by DynamicDataFactory\n"));
    }
    return false;
  }
  return ser << *ddi;
}

bool DynamicSample::deserialize(Serializer& ser)
{
  //TODO: review design
  const DDS::DynamicType_var type = data_->type();
  data_ = new DynamicDataXcdrReadImpl(ser, type);
  return true;
}

bool DynamicSample::compare(const Sample& other) const
{
  ACE_UNUSED_ARG(other);
  // TODO
  return false;
}

void DynamicDataReaderImpl::install_type_support(TypeSupportImpl* ts)
{
  type_ = ts->get_type();
}

void DynamicDataReaderImpl::imbue_type(DynamicSample& ds)
{
  const DDS::DynamicData_var data = new DynamicDataImpl(type_);
  ds = DynamicSample(data);
}

} // namespace XTypes

namespace DCPS {

bool operator>>(Serializer& strm, XTypes::DynamicSample& sample)
{
  return sample.deserialize(strm);
}

bool operator>>(Serializer& strm, const KeyOnly<XTypes::DynamicSample>& sample)
{
  sample.value.set_key_only(true);
  return sample.value.deserialize(strm);
}

template <>
void DataReaderImpl_T<XTypes::DynamicSample>::dynamic_hook(XTypes::DynamicSample& sample)
{
  XTypes::DynamicDataReaderImpl* const self = dynamic_cast<XTypes::DynamicDataReaderImpl*>(this);
  if (self) {
    self->imbue_type(sample);
  }
}

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
template <>
DDS::ReturnCode_t
DataReaderImpl_T<XTypes::DynamicSample>::read_generic(GenericBundle&,
                                                      DDS::SampleStateMask,
                                                      DDS::ViewStateMask,
                                                      DDS::InstanceStateMask,
                                                      bool)
{
  return DDS::RETCODE_UNSUPPORTED;
}

template <>
DDS::ReturnCode_t
DataReaderImpl_T<XTypes::DynamicSample>::take(AbstractSamples&,
                                              DDS::SampleStateMask,
                                              DDS::ViewStateMask,
                                              DDS::InstanceStateMask)
{
  return DDS::RETCODE_UNSUPPORTED;
}

template <>
struct MetaStructImpl<XTypes::DynamicSample> : MetaStruct {
  typedef XTypes::DynamicSample T;

#ifndef OPENDDS_NO_MULTI_TOPIC
  void* allocate() const { return 0; }

  void deallocate(void*) const {}

  size_t numDcpsKeys() const { return 0; }
#endif /* OPENDDS_NO_MULTI_TOPIC */

  bool isDcpsKey(const char* field) const
  {
    //TODO
    return false;
  }

  ACE_CDR::ULong map_name_to_id(const char* field) const
  {
    //TODO
    return 0;
  }

  Value getValue(const void* stru, DDS::MemberId memberId) const
  {
    const T& typed = *static_cast<const T*>(stru);
    Value v(0);
    //TODO
    return v;
  }

  Value getValue(const void* stru, const char* field) const
  {
    const T& typed = *static_cast<const T*>(stru);
    Value v(0);
    //TODO
    return v;
  }

  Value getValue(Serializer& strm, const char* field) const
  {
    Value v(0);
    //TODO
    return v;
  }

  ComparatorBase::Ptr create_qc_comparator(const char* field, ComparatorBase::Ptr next) const
  {
    //TODO
    return ComparatorBase::Ptr();
  }

#ifndef OPENDDS_NO_MULTI_TOPIC
  const char** getFieldNames() const
  {
    return 0;
  }

  const void* getRawField(const void*, const char*) const
  {
    return 0;
  }

  void assign(void*, const char*, const void*, const char*, const MetaStruct&) const
  {
  }
#endif

  bool compare(const void* lhs, const void* rhs, const char* field) const
  {
    //TODO
    return false;
  }
};

template <>
OpenDDS_Dcps_Export
const MetaStruct& getMetaStruct<XTypes::DynamicSample>()
{
  static const MetaStructImpl<XTypes::DynamicSample> m;
  return m;
}

template <>
OpenDDS_Dcps_Export
const MetaStruct& getMetaStruct<DDS::DynamicData*>() //TODO: this is needed to resolve a linker error
{
  static const MetaStructImpl<XTypes::DynamicSample> m;
  return m;
}
#endif

} // namespace DCPS
} // namespace OpenDDS

namespace DDS {

using namespace OpenDDS::XTypes;

void DynamicTypeSupport::representations_allowed_by_type(DataRepresentationIdSeq& seq)
{
  // TODO: Need to be able to read annotations?
  seq.length(1);
  seq[0] = XCDR2_DATA_REPRESENTATION;
}

size_t DynamicTypeSupport::key_count() const
{
  size_t count = 0;
  const ReturnCode_t rc = OpenDDS::XTypes::key_count(type_, count);
  if (rc != RETCODE_OK && log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicTypeSupport::key_count: "
      "could not get correct key count for DynamicType %C: %C\n",
      name(), retcode_to_string(rc)));
  }
  return count;
}

Extensibility DynamicTypeSupport::base_extensibility() const
{
  Extensibility ext = OpenDDS::DCPS::FINAL;
  const ReturnCode_t rc = extensibility(type_, ext);
  if (rc != RETCODE_OK && log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicTypeSupport::base_extensibility: "
      "could not get correct extensibility for DynamicType %C: %C\n",
      name(), retcode_to_string(rc)));
  }
  return ext;
}

Extensibility DynamicTypeSupport::max_extensibility() const
{
  Extensibility ext = OpenDDS::DCPS::FINAL;
  const ReturnCode_t rc = OpenDDS::XTypes::max_extensibility(type_, ext);
  if (rc != RETCODE_OK && log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicTypeSupport::max_extensibility: "
      "could not get correct max extensibility for DynamicType %C: %C\n",
      name(), retcode_to_string(rc)));
  }
  return ext;
}

DataWriter_ptr DynamicTypeSupport::create_datawriter()
{
  return new DynamicDataWriterImpl();
}

DataReader_ptr DynamicTypeSupport::create_datareader()
{
  return new DynamicDataReaderImpl();
}

#  ifndef OPENDDS_NO_MULTI_TOPIC
DataReader_ptr DynamicTypeSupport::create_multitopic_datareader()
{
  // TODO
  return 0;
}
#  endif

const TypeIdentifier& DynamicTypeSupport::getMinimalTypeIdentifier() const
{
  DynamicTypeImpl* const dti = dynamic_cast<DynamicTypeImpl*>(type_.in());
  return dti->get_minimal_type_identifier();
}

const TypeMap& DynamicTypeSupport::getMinimalTypeMap() const
{
  DynamicTypeImpl* const dti = dynamic_cast<DynamicTypeImpl*>(type_.in());
  return dti->get_minimal_type_map();
}

const TypeIdentifier& DynamicTypeSupport::getCompleteTypeIdentifier() const
{
  DynamicTypeImpl* const dti = dynamic_cast<DynamicTypeImpl*>(type_.in());
  return dti->get_complete_type_identifier();
}

const TypeMap& DynamicTypeSupport::getCompleteTypeMap() const
{
  DynamicTypeImpl* const dti = dynamic_cast<DynamicTypeImpl*>(type_.in());
  return dti->get_complete_type_map();
}

DynamicTypeSupport_ptr DynamicTypeSupport::_duplicate(DynamicTypeSupport_ptr obj)
{
  if (obj) {
    obj->_add_ref();
  }
  return obj;
}

} // namespace DDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

TAO_BEGIN_VERSIONED_NAMESPACE_DECL
namespace TAO {

DDS::DynamicTypeSupport_ptr Objref_Traits<DDS::DynamicTypeSupport>::duplicate(DDS::DynamicTypeSupport_ptr p)
{
  return DDS::DynamicTypeSupport::_duplicate(p);
}

void Objref_Traits<DDS::DynamicTypeSupport>::release(DDS::DynamicTypeSupport_ptr p)
{
  CORBA::release(p);
}

DDS::DynamicTypeSupport_ptr Objref_Traits<DDS::DynamicTypeSupport>::nil()
{
  return static_cast<DDS::DynamicTypeSupport_ptr>(0);
}

CORBA::Boolean Objref_Traits<DDS::DynamicTypeSupport>::marshal(
  const DDS::DynamicTypeSupport_ptr, TAO_OutputCDR&)
{
  return false;
}

} // namespace TAO
TAO_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
