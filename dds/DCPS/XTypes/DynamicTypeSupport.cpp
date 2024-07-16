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
#include "DynamicDataXcdrReadImpl.h"
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
    static ComparatorBase::Ptr make_nested_cmp(const std::string& field, ComparatorBase::Ptr inner,
                                               ComparatorBase::Ptr next);

    static ComparatorBase::Ptr make_dynamic_cmp(const std::string& field,
                                                ComparatorBase::Ptr next = ComparatorBase::Ptr());

    template <>
    struct MetaStructImpl<XTypes::DynamicSample> : MetaStruct {

      Value getValue(const void* stru, DDS::MemberId memberId) const
      {
        const XTypes::DynamicSample& typed = *static_cast<const XTypes::DynamicSample*>(stru);
        const DDS::DynamicData_var dd = typed.dynamic_data();
        XTypes::DynamicDataBase* const ddb = dynamic_cast<XTypes::DynamicDataBase*>(dd.in());
        Value v(0);
        if (ddb) {
          ddb->get_simple_value(v, memberId);
        }
        return v;
      }

      Value getValue(const void* stru, const char* field) const
      {
        const XTypes::DynamicSample& typed = *static_cast<const XTypes::DynamicSample*>(stru);
        const DDS::DynamicData_var dd = typed.dynamic_data();
        return getValueImpl(dd, field);
      }

      Value getValue(Serializer& strm, const char* field, TypeSupportImpl* ts) const
      {
        DDS::DynamicType_var type = ts->get_type();
        const DDS::DynamicData_var dd = new XTypes::DynamicDataXcdrReadImpl(strm, type);
        return getValueImpl(dd, field);
      }

      static Value getValueImpl(const DDS::DynamicData_var& dd, const char* field)
      {
        const char* const dot = std::strchr(field, '.');
        if (dot) {
          const String local(field, dot - field);
          const DDS::MemberId id = dd->get_member_id_by_name(local.c_str());
          DDS::DynamicData_var nested;
          if (dd->get_complex_value(nested, id) != DDS::RETCODE_OK) {
            return Value(0);
          }
          return getValueImpl(nested, dot + 1);
        }
        const DDS::MemberId id = dd->get_member_id_by_name(field);
        XTypes::DynamicDataBase* const ddb = dynamic_cast<XTypes::DynamicDataBase*>(dd.in());
        Value v(0);
        if (ddb) {
          ddb->get_simple_value(v, id);
        }
        return v;
      }

      ComparatorBase::Ptr create_qc_comparator(const char* field, ComparatorBase::Ptr next) const
      {
        const char* const dot = std::strchr(field, '.');
        return dot ? make_nested_cmp(String(field, dot - field), make_dynamic_cmp(dot + 1), next)
          : make_dynamic_cmp(field, next);
      }

#ifndef OPENDDS_NO_MULTI_TOPIC
      void* allocate() const { return 0; }

      void deallocate(void*) const {}

      size_t numDcpsKeys() const { return 0; }

      const char** getFieldNames() const { return 0; }

      const void* getRawField(const void*, const char*) const { return 0; }

      void assign(void*, const char*, const void*, const char*, const MetaStruct&) const {}

      bool compare(const void*, const void*, const char*) const { return false; }
#endif
    };

    template <>
    OpenDDS_Dcps_Export
    const MetaStruct& getMetaStruct<XTypes::DynamicSample>()
    {
      static const MetaStructImpl<XTypes::DynamicSample> m;
      return m;
    }

    /// Dynamic version of FieldComparator in Comparator_T.h
    struct DynamicComparator : ComparatorBase {
      DynamicComparator(const std::string& field, Ptr next)
        : ComparatorBase(next)
        , field_(field)
      {}

      int cmp(void* lhs, void* rhs) const
      {
        const XTypes::DynamicSample& left = *static_cast<XTypes::DynamicSample*>(lhs);
        const XTypes::DynamicSample& right = *static_cast<XTypes::DynamicSample*>(rhs);
        const DDS::DynamicData_var l(left.dynamic_data()), r(right.dynamic_data());
        const DDS::MemberId id = l->get_member_id_by_name(field_.c_str());
        int result;
        if (XTypes::compare_members(result, l, r, id) != DDS::RETCODE_OK) {
          return 1; // warning already logged
        }
        return result;
      }

      bool less(void* lhs, void* rhs) const
      {
        return cmp(lhs, rhs) < 0;
      }

      bool equal(void* lhs, void* rhs) const
      {
        return cmp(lhs, rhs) == 0;
      }

      const std::string field_;
    };

    ComparatorBase::Ptr make_dynamic_cmp(const std::string& field, ComparatorBase::Ptr next)
    {
      return make_rch<DynamicComparator>(field, next);
    }

    /// Dynamic version of StructComparator in Comparator_T.h
    struct NestedComparator : ComparatorBase {
      NestedComparator(const std::string& field, Ptr inner, Ptr next)
        : ComparatorBase(next)
        , field_(field)
        , inner_(inner)
      {}

      XTypes::DynamicSample get_nested(void* outer) const
      {
        const XTypes::DynamicSample& outer_typed = *static_cast<XTypes::DynamicSample*>(outer);
        const DDS::DynamicData_var outer_dd = outer_typed.dynamic_data();
        DDS::DynamicData_var inner_dd;
        const DDS::MemberId id = outer_dd->get_member_id_by_name(field_.c_str());
        if (outer_dd->get_complex_value(inner_dd, id) != DDS::RETCODE_OK) {
          return XTypes::DynamicSample();
        }
        return XTypes::DynamicSample(inner_dd);
      }

      bool less(void* lhs, void* rhs) const
      {
        XTypes::DynamicSample left(get_nested(lhs)), right(get_nested(rhs));
        return inner_->less(&left, &right);
      }

      bool equal(void* lhs, void* rhs) const
      {
        XTypes::DynamicSample left(get_nested(lhs)), right(get_nested(rhs));
        return inner_->equal(&left, &right);
      }

      const std::string field_;
      const Ptr inner_;
    };

    ComparatorBase::Ptr make_nested_cmp(const std::string& field, ComparatorBase::Ptr inner,
                                        ComparatorBase::Ptr next)
    {
      return make_rch<NestedComparator>(field, inner, next);
    }

#endif

  }

  namespace DCPS {
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    template <>
    DDS::ReturnCode_t DataReaderImpl_T<XTypes::DynamicSample>::read_generic(
      GenericBundle&,
      DDS::SampleStateMask,
      DDS::ViewStateMask,
      DDS::InstanceStateMask,
      bool)
    {
      return DDS::RETCODE_UNSUPPORTED;
    }

    template <>
    DDS::ReturnCode_t DataReaderImpl_T<XTypes::DynamicSample>::take(
      AbstractSamples&,
      DDS::SampleStateMask,
      DDS::ViewStateMask,
      DDS::InstanceStateMask)
    {
      return DDS::RETCODE_UNSUPPORTED;
    }
#endif
  }

  namespace XTypes {
    template <>
    DDS::DynamicData_ptr get_dynamic_data_adapter<DynamicSample, DynamicSample>(
      DDS::DynamicType_ptr, const DynamicSample& value)
    {
      return value.dynamic_data()._retn();
    }

    template <>
    DDS::DynamicData_ptr get_dynamic_data_adapter<DynamicSample, DynamicSample>(
      DDS::DynamicType_ptr, DynamicSample& value)
    {
      return value.dynamic_data()._retn();
    }
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

  bool DynamicTypeSupport::is_dcps_key(const char* field) const
  {
    return OpenDDS::XTypes::is_key(type_, field);
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

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  const OpenDDS::DCPS::MetaStruct& DynamicTypeSupport::getMetaStructForType() const
  {
    return getMetaStruct<OpenDDS::XTypes::DynamicSample>();
  }
#endif

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
