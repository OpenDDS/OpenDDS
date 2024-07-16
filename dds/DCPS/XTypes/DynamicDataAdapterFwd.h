/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_FWD_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_FWD_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

template <typename T, typename Tag>
DDS::DynamicData_ptr get_dynamic_data_adapter(DDS::DynamicType_ptr type, const T& value);
template <typename T, typename Tag>
DDS::DynamicData_ptr get_dynamic_data_adapter(DDS::DynamicType_ptr type, T& value);

template <typename T>
DDS::DynamicData_ptr get_dynamic_data_adapter(DDS::DynamicType_ptr type, T& value)
{
  return get_dynamic_data_adapter<T, T>(type, value);
}

template <typename T>
DDS::DynamicData_ptr get_dynamic_data_adapter(DDS::DynamicType_ptr type, const T& value)
{
  return get_dynamic_data_adapter<T, T>(type, value);
}

template <typename T, typename Tag>
const T* get_dynamic_data_adapter_value(DDS::DynamicData_ptr dda);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_FWD_H
