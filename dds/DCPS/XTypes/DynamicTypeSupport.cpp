/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#include "DynamicTypeSupport.h"

#include "DynamicDataImpl.h"
#include "DynamicDataReaderImpl.h"
#include "DynamicDataWriterImpl.h"
#include "DynamicTypeImpl.h"
#include "Utils.h"

#include <dds/DCPS/debug.h>
#include <dds/DCPS/DCPS_Utils.h>

#include <ace/Malloc_Base.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
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

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    template <>
    struct MetaStructImpl<XTypes::DynamicSample> : MetaStruct {
      typedef XTypes::DynamicSample T;

#ifndef OPENDDS_NO_MULTI_TOPIC
      void* allocate() const { return 0; }

      void deallocate(void*) const {}

      size_t numDcpsKeys() const { return 0; }
#endif /* OPENDDS_NO_MULTI_TOPIC */

      bool isDcpsKey(const char* /*field*/) const
      {
        //TODO
        return false;
      }

      ACE_CDR::ULong map_name_to_id(const char* /*field*/) const
      {
        //TODO
        return 0;
      }

      Value getValue(const void* stru, DDS::MemberId /*memberId*/) const
      {
        const T& typed = *static_cast<const T*>(stru);
        ACE_UNUSED_ARG(typed);
        Value v(0);
        //TODO
        return v;
      }

      Value getValue(const void* stru, const char* /*field*/) const
      {
        const T& typed = *static_cast<const T*>(stru);
        ACE_UNUSED_ARG(typed);
        Value v(0);
        //TODO
        return v;
      }

      Value getValue(Serializer& /*strm*/, const char* /*field*/) const
      {
        Value v(0);
        //TODO
        return v;
      }

      ComparatorBase::Ptr create_qc_comparator(const char* /*field*/, ComparatorBase::Ptr /*next*/) const
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

      bool compare(const void*, const void*, const char*) const
      {
        return false;
      }
#endif
    };

    template <>
    OpenDDS_Dcps_Export
    const MetaStruct& getMetaStruct<XTypes::DynamicSample>()
    {
      static const MetaStructImpl<XTypes::DynamicSample> m;
      return m;
    }
#endif

  }
}

namespace DDS {

  using namespace OpenDDS::XTypes;
  using namespace OpenDDS::DCPS;

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

#ifndef OPENDDS_NO_MULTI_TOPIC
  DataReader_ptr DynamicTypeSupport::create_multitopic_datareader()
  {
    // TODO
    return 0;
  }
#endif

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

  const OpenDDS::XTypes::TypeInformation* DynamicTypeSupport::preset_type_info() const
  {
    DynamicTypeImpl* dti = dynamic_cast<DynamicTypeImpl*>(type_.in());
    return dti->get_preset_type_info();
  }

  DynamicTypeSupport_ptr DynamicTypeSupport::_duplicate(DynamicTypeSupport_ptr obj)
  {
    if (obj) {
      obj->_add_ref();
    }
    return obj;
  }

}
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

}
TAO_END_VERSIONED_NAMESPACE_DECL

#endif
