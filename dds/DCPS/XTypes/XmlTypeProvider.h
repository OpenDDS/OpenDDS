/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_XML_TYPE_PROVIDER_H
#define OPENDDS_DCPS_XTYPES_XML_TYPE_PROVIDER_H

#include "XmlTypeProvider_export.h"
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

/**
 * Load a DynamicType from an XML type definition file.
 *
 * The XML format follows the OMG DDS-XTypes specification (Annex A).
 * Supported type elements: struct, union, enum, bitmask, module.
 * Struct and union elements accept an optional nested="true" attribute
 * corresponding to the @nested IDL annotation (DDS-XTypes 7.3.2.5).
 * Types without the nested attribute are treated as non-nested (publishable
 * as DDS topics), matching the IDL default.
 *
 * Returns RETCODE_OK on success, RETCODE_BAD_PARAMETER if the requested
 * type is not found in the file, or RETCODE_ERROR on parse failure.
 */
OpenDDS_XTypes_Xml_Export DDS::ReturnCode_t load_xml_type(
  DDS::DynamicType_var& type,
  const ACE_TString& xml_file,
  const std::string& fully_qualified_type_name);

#endif // OPENDDS_XERCES3 && !OPENDDS_SAFETY_PROFILE

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_XTYPES_XML_TYPE_PROVIDER_H */
