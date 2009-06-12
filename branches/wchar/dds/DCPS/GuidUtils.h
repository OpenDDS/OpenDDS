// -*- C++ -*-
// ============================================================================
/**
 *  @file   GuidUtils.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef GUIDUTILS_H
#define GUIDUTILS_H

#include <iosfwd>

#include "tao/Basic_Types.h"

#include "dds/DdsDcpsGuidC.h"

#include "dcps_export.h"


namespace OpenDDS
{
namespace DCPS
{
/// Vendor Id value specified for OCI is used for OpenDDS.
const GuidVendorId_t VENDORID_OCI = { 0x00, 0x03 };

/// Nil value for the GUID prefix (participant identifier).
const GuidPrefix_t GUIDPREFIX_UNKNOWN = { 0 };

/// Entity Id values specified in Version 2.0 of RTPS specification.
const EntityId_t ENTITYID_UNKNOWN                                = { {0x00,0x00,0x00}, 0x00};
const EntityId_t ENTITYID_PARTICIPANT                            = { {0x00,0x00,0x01}, 0xc1};
const EntityId_t ENTITYID_SEDP_BUILTIN_TOPIC_WRITER              = { {0x00,0x00,0x02}, 0xc2};
const EntityId_t ENTITYID_SEDP_BUILTIN_TOPIC_READER              = { {0x00,0x00,0x02}, 0xc7};
const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER       = { {0x00,0x00,0x03}, 0xc2};
const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER       = { {0x00,0x00,0x03}, 0xc7};
const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER      = { {0x00,0x00,0x04}, 0xc2};
const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER      = { {0x00,0x00,0x04}, 0xc7};
const EntityId_t ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER        = { {0x00,0x01,0x00}, 0xc2};
const EntityId_t ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER        = { {0x00,0x01,0x00}, 0xc7};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER = { {0x00,0x02,0x00}, 0xC2};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER = { {0x00,0x02,0x00}, 0xC7};

/// Nil value for GUID.
const GUID_t GUID_UNKNOWN = { {0,0,0,0,0,0,0,0,0,0,0,0}, { {0,0,0}, 0} };

/// Summary kinds of entities within the service.
enum EntityKind {     // EntityId_t.entityKind value(s)
  KIND_UNKNOWN,       // 0x3f & 0x00 (and all other unspecified values)
  KIND_PARTICIPANT,   // 0x3f & 0x01
  KIND_WRITER,        // 0x3f & 0x02 | 0x3f & 0x03
  KIND_READER,        // 0x3f & 0x04 | 0x3f & 0x07
  KIND_TOPIC          // 0x3f & 0x05
};

} // namespace
} // namespace

// Check for equality using the generated logical functor.
inline OpenDDS_Dcps_Export bool
operator==( const OpenDDS::DCPS::GUID_t& lhs, const OpenDDS::DCPS::GUID_t& rhs)
{
  GUID_tKeyLessThan lessThan;
  return !lessThan(lhs, rhs) && !lessThan(rhs, lhs);
}

// Serialize to ASCII Hex string: "xxxx.xxxx.xxxx.xxxx"
OpenDDS_Dcps_Export std::ostream&
operator<<(std::ostream& os, const OpenDDS::DCPS::GUID_t& rhs);

// Deserialize from ASCII Hex string: "xxxx.xxxx.xxxx.xxxx"
OpenDDS_Dcps_Export std::istream&
operator>>(std::istream& is, OpenDDS::DCPS::GUID_t& rhs);

#endif /* GUIDUTILS_H */

