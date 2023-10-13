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
