/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_READER_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_READER_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include <dds/DCPS/DataReaderImpl_T.h>

#include "DynamicSample.h"
#include "DynamicTypeSupport.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace DCPS {

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  template <>
  DDS::ReturnCode_t
  DataReaderImpl_T<XTypes::DynamicSample>::read_generic(GenericBundle& gen,
                                                        DDS::SampleStateMask sample_states,
                                                        DDS::ViewStateMask view_states,
                                                        DDS::InstanceStateMask instance_states,
                                                        bool adjust_ref_count);

  template <>
  DDS::ReturnCode_t
  DataReaderImpl_T<XTypes::DynamicSample>::take(AbstractSamples& samples,
                                                DDS::SampleStateMask sample_states,
                                                DDS::ViewStateMask view_states,
                                                DDS::InstanceStateMask instance_states);
#endif

  template <>
  void DataReaderImpl_T<XTypes::DynamicSample>::dynamic_hook(XTypes::DynamicSample& sample);
}

namespace XTypes {
#if defined _MSC_VER && _MSC_VER < 1700
#define OPENDDS_MAYBE_EXPORT
#else
#define OPENDDS_MAYBE_EXPORT OpenDDS_Dcps_Export
#endif

  class OPENDDS_MAYBE_EXPORT DynamicDataReaderImpl
#undef OPENDDS_MAYBE_EXPORT
    : public DCPS::DataReaderImpl_T<DynamicSample>
  {
  public:
    typedef DCPS::DataReaderImpl_T<DynamicSample> Base;

    DDS::ReturnCode_t read_next_sample(DDS::DynamicData*& dyn, DDS::SampleInfo& si)
    {
      DynamicSample ds(dyn);
      const DDS::ReturnCode_t rc = Base::read_next_sample(ds, si);
      if (rc == DDS::RETCODE_OK) {
        CORBA::release(dyn);
        DDS::DynamicData_var result = ds.get_dynamic_data(0);
        dyn = result._retn();
      }
      return rc;
    }

    DDS::ReturnCode_t take_next_sample(DDS::DynamicData*& dyn, DDS::SampleInfo& si)
    {
      DynamicSample ds(dyn);
      const DDS::ReturnCode_t rc = Base::take_next_sample(ds, si);
      if (rc == DDS::RETCODE_OK) {
        CORBA::release(dyn);
        DDS::DynamicData_var result = ds.get_dynamic_data(0);
        dyn = result._retn();
      }
      return rc;
    }

    DDS::InstanceHandle_t lookup_instance(DDS::DynamicData* dyn)
    {
      DynamicSample ds(dyn);
      return Base::lookup_instance(ds);
    }

    DDS::ReturnCode_t get_key_value(DDS::DynamicData*& dyn, DDS::InstanceHandle_t ih)
    {
      DynamicSample ds(dyn);
      const DDS::ReturnCode_t rc = Base::get_key_value(ds, ih);
      if (rc == DDS::RETCODE_OK) {
        CORBA::release(dyn);
        DDS::DynamicData_var result = ds.get_dynamic_data(0);
        dyn = result._retn();
      }
      return rc;
    }

    void install_type_support(DCPS::TypeSupportImpl* typesupport);
    void imbue_type(DynamicSample& sample);

  private:
    DDS::DynamicType_var type_;

    // work around "hides overloaded virtual" warnings
    using Base::read_next_sample;
    using Base::take_next_sample;
    using Base::lookup_instance;
    using Base::get_key_value;
  };
}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
#endif
