/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_JSON_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_JSON_H

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

#if defined OPENDDS_RAPIDJSON && !defined OPENDDS_SAFETY_PROFILE

enum DynamicDataJsonDiscriminatorFormat {
  /**
   * Input accepts either {"$discriminator": ..., "member": ...} or
   * {"member": ...}.  Output includes the "$discriminator" union member.
   */
  DYNAMIC_DATA_JSON_DISCRIMINATOR_AUTO,

  /// Include/require the DDS-JSON "$discriminator" union member.
  DYNAMIC_DATA_JSON_DISCRIMINATOR_FIELD,

  /// Use the dds-xtypes interop active-member-only union representation.
  DYNAMIC_DATA_JSON_DISCRIMINATOR_ACTIVE_MEMBER,
};

struct OpenDDS_Dcps_Export DynamicDataJsonOptions {
  DynamicDataJsonOptions();

  DynamicDataJsonDiscriminatorFormat discriminator_format;

  /**
   * When true, JSON members that do not correspond to any field in the type
   * are silently ignored rather than treated as an error.
   */
  bool allow_unknown_members;
};

/**
 * Notes on JSON representation:
 *
 * Bitmask values may be supplied as an unsigned integer or as a
 * pipe-separated list of flag names, e.g. "FLAG_A|FLAG_B".
 *
 * float128 values may be supplied as a 32-character hex string
 * (e.g. "0x3fff0000000000000000000000000000") or as a JSON number
 * (converted via double, losing precision for values outside double range).
 */

OpenDDS_Dcps_Export DDS::ReturnCode_t dynamic_data_from_json(
  DDS::DynamicData_ptr data,
  const char* json,
  const DynamicDataJsonOptions& options = DynamicDataJsonOptions());

OpenDDS_Dcps_Export DDS::ReturnCode_t dynamic_data_from_json_file(
  DDS::DynamicData_ptr data,
  const ACE_TString& json_file,
  const DynamicDataJsonOptions& options = DynamicDataJsonOptions());

OpenDDS_Dcps_Export DDS::ReturnCode_t dynamic_data_to_json(
  std::string& json,
  DDS::DynamicData_ptr data,
  const DynamicDataJsonOptions& options = DynamicDataJsonOptions());

#endif // OPENDDS_RAPIDJSON && !OPENDDS_SAFETY_PROFILE

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_JSON_H */
