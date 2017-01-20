/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef GUIDUTILS_H
#define GUIDUTILS_H

#include "dds/DdsDcpsGuidC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/PoolAllocator.h"

#include "dcps_export.h"

#include "tao/Basic_Types.h"

#ifndef OPENDDS_SAFETY_PROFILE
#include <iosfwd>
#endif

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Vendor Id value specified for OCI is used for OpenDDS.
const GuidVendorId_t VENDORID_OCI = { 0x01, 0x03 };

/// Nil value for the GUID prefix (participant identifier).
const GuidPrefix_t GUIDPREFIX_UNKNOWN = { 0 };

/// Entity Id values specified in Version 2.1 of RTPS specification.
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
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER = { {0x00,0x02,0x00}, 0xc2};
const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER = { {0x00,0x02,0x00}, 0xc7};

/// Nil value for GUID.
const GUID_t GUID_UNKNOWN = { {0,0,0,0,0,0,0,0,0,0,0,0}, { {0,0,0}, 0} };

/// Summary kinds of entities within the service.
enum EntityKind {     // EntityId_t.entityKind value(s)
  KIND_UNKNOWN,       // 0x3f & 0x00 (and all other unspecified values)
  KIND_PARTICIPANT,   // 0x3f & 0x01
  KIND_WRITER,        // 0x3f & 0x02 | 0x3f & 0x03
  KIND_READER,        // 0x3f & 0x04 | 0x3f & 0x07
  KIND_TOPIC,         // 0x3f & 0x05

  KIND_PUBLISHER,     // OpenDDS specific Publisher Guid values
  KIND_SUBSCRIBER,    // OpenDDS specific Subscriber Guid values
  KIND_USER           // OpenDDS specific other Guid values
};

struct OpenDDS_Dcps_Export GUID_tKeyLessThan {
  static bool entity_less(const EntityId_t& v1, const EntityId_t& v2)
  {
    if (v1.entityKey[2] < v2.entityKey[2]) return true;
    if (v2.entityKey[2] < v1.entityKey[2]) return false;
    if (v1.entityKey[1] < v2.entityKey[1]) return true;
    if (v2.entityKey[1] < v1.entityKey[1]) return false;
    if (v1.entityKey[0] < v2.entityKey[0]) return true;
    if (v2.entityKey[0] < v1.entityKey[0]) return false;
    if (v1.entityKind < v2.entityKind) return true;
    if (v2.entityKind < v1.entityKind) return false;
    return false;
  }

  bool operator()(const GUID_t& v1, const GUID_t& v2) const
  {
    if (v1.guidPrefix[11] < v2.guidPrefix[11]) return true;
    if (v2.guidPrefix[11] < v1.guidPrefix[11]) return false;
    if (v1.guidPrefix[10] < v2.guidPrefix[10]) return true;
    if (v2.guidPrefix[10] < v1.guidPrefix[10]) return false;
    if (v1.guidPrefix[ 9] < v2.guidPrefix[ 9]) return true;
    if (v2.guidPrefix[ 9] < v1.guidPrefix[ 9]) return false;
    if (v1.guidPrefix[ 8] < v2.guidPrefix[ 8]) return true;
    if (v2.guidPrefix[ 8] < v1.guidPrefix[ 8]) return false;
    if (v1.guidPrefix[ 7] < v2.guidPrefix[ 7]) return true;
    if (v2.guidPrefix[ 7] < v1.guidPrefix[ 7]) return false;
    if (v1.guidPrefix[ 6] < v2.guidPrefix[ 6]) return true;
    if (v2.guidPrefix[ 6] < v1.guidPrefix[ 6]) return false;
    if (v1.guidPrefix[ 5] < v2.guidPrefix[ 5]) return true;
    if (v2.guidPrefix[ 5] < v1.guidPrefix[ 5]) return false;
    if (v1.guidPrefix[ 4] < v2.guidPrefix[ 4]) return true;
    if (v2.guidPrefix[ 4] < v1.guidPrefix[ 4]) return false;
    if (v1.guidPrefix[ 3] < v2.guidPrefix[ 3]) return true;
    if (v2.guidPrefix[ 3] < v1.guidPrefix[ 3]) return false;
    if (v1.guidPrefix[ 2] < v2.guidPrefix[ 2]) return true;
    if (v2.guidPrefix[ 2] < v1.guidPrefix[ 2]) return false;
    if (v1.guidPrefix[ 1] < v2.guidPrefix[ 1]) return true;
    if (v2.guidPrefix[ 1] < v1.guidPrefix[ 1]) return false;
    if (v1.guidPrefix[ 0] < v2.guidPrefix[ 0]) return true;
    if (v2.guidPrefix[ 0] < v1.guidPrefix[ 0]) return false;
    return entity_less(v1.entityId, v2.entityId);
  }
};

typedef OPENDDS_SET_CMP(RepoId, GUID_tKeyLessThan) RepoIdSet;

inline size_t
gen_max_marshaled_size(const GUID_t&)
{
  return 16;
}

#ifndef OPENDDS_SAFETY_PROFILE
inline bool
operator==(const GUID_t& lhs, const GUID_t& rhs)
{
  GUID_tKeyLessThan lessThan;
  return !lessThan(lhs, rhs) && !lessThan(rhs, lhs);
}

inline bool
operator!=(const GUID_t& lhs, const GUID_t& rhs)
{
  return !(lhs == rhs);
}
#endif

struct GuidPrefixEqual {

  bool
  operator() (const GuidPrefix_t& lhs, const GuidPrefix_t& rhs) const
  {
    return std::memcmp(&lhs, &rhs, sizeof(GuidPrefix_t)) == 0;
  }
};

#ifndef OPENDDS_SAFETY_PROFILE
inline bool
operator==(const EntityId_t& lhs, const EntityId_t& rhs)
{
  return !GUID_tKeyLessThan::entity_less(lhs, rhs)
    && !GUID_tKeyLessThan::entity_less(rhs, lhs);
}

inline bool
operator!=(const EntityId_t& lhs, const EntityId_t& rhs)
{
  return !(lhs == rhs);
}
#endif

struct EntityIdConverter {
  explicit EntityIdConverter(const unsigned char (&o)[4])
  {
    std::memcpy(&entityId, o, sizeof(EntityId_t));
  }

  operator EntityId_t() const { return entityId; }

  EntityId_t entityId;
};

OpenDDS_Dcps_Export OPENDDS_STRING
to_string(const GUID_t& guid);

#ifndef OPENDDS_SAFETY_PROFILE
// Serialize to ASCII Hex string: "xxxx.xxxx.xxxx.xxxx"
OpenDDS_Dcps_Export std::ostream&
operator<<(std::ostream& os, const GUID_t& rhs);

// Deserialize from ASCII Hex string: "xxxx.xxxx.xxxx.xxxx"
OpenDDS_Dcps_Export std::istream&
operator>>(std::istream& is, GUID_t& rhs);
#endif
} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* GUIDUTILS_H */
