/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_XML_TYPE_PROVIDER_H
#define OPENDDS_DCPS_XTYPES_XML_TYPE_PROVIDER_H

#include <dds/DCPS/dcps_export.h>
#include <dds/DdsDynamicDataC.h>

#include <ace/SString.h>

#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

#if defined OPENDDS_XERCES3 && !defined OPENDDS_SAFETY_PROFILE

OpenDDS_Dcps_Export DDS::ReturnCode_t load_xml_type(
  DDS::DynamicType_var& type,
  const ACE_TString& xml_file,
  const std::string& fully_qualified_type_name);

#endif // OPENDDS_XERCES3 && !OPENDDS_SAFETY_PROFILE

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_XTYPES_XML_TYPE_PROVIDER_H */
