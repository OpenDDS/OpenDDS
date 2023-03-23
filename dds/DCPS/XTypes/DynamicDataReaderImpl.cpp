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
