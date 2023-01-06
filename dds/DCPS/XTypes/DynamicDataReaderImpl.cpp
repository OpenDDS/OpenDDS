/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#include "DynamicDataReaderImpl.h"

#include "DynamicDataImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

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
#endif

  template <>
  void DataReaderImpl_T<XTypes::DynamicSample>::dynamic_hook(XTypes::DynamicSample& sample)
  {
    XTypes::DynamicDataReaderImpl* const self = dynamic_cast<XTypes::DynamicDataReaderImpl*>(this);
    if (self) {
      self->imbue_type(sample);
    }
  }
}

namespace XTypes {
  using namespace OpenDDS::DCPS;

  void DynamicDataReaderImpl::install_type_support(TypeSupportImpl* ts)
  {
    type_ = ts->get_type();
  }

  void DynamicDataReaderImpl::imbue_type(DynamicSample& ds)
  {
    const DDS::DynamicData_var data = new DynamicDataImpl(type_);
    ds = DynamicSample(data);
  }
}

}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
