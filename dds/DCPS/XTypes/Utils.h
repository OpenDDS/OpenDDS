/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_UTILS_H
#define OPENDDS_DCPS_XTYPES_UTILS_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/Serializer.h>

#  include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

OpenDDS_Dcps_Export DDS::ReturnCode_t extensibility(
  DDS::DynamicType_ptr type, DCPS::Extensibility& ext);
OpenDDS_Dcps_Export DDS::ReturnCode_t max_extensibility(
  DDS::DynamicType_ptr type, DCPS::Extensibility& ext);
OpenDDS_Dcps_Export DCPS::Extensibility dds_to_opendds_ext(DDS::ExtensibilityKind ext);

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
#endif
