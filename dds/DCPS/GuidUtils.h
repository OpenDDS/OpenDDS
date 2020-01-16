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

///@{
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
///@}

/// Nil value for GUID.
const GUID_t GUID_UNKNOWN = { { 0 }, { { 0 }, 0 } };

/**
 * Summary kinds of entities within the service.
 *
 * See dds/DdsDcpsGuid.idl for the values these map to.
 */
enum EntityKind {     // EntityId_t.entityKind value(s)

  /// Represents ENTITYKIND_USER_UNKNOWN and ENTITYKIND_BUILTIN_UNKNOWN
  KIND_UNKNOWN,

  /// Represents ENTITYKIND_BUILTIN_PARTICIPANT
  KIND_PARTICIPANT,

  /// Represents ENTITYKIND_USER_WRITER_WITH_KEY and ENTITYKIND_USER_WRITER_NO_KEY
  KIND_USER_WRITER,
  /// Represents ENTITYKIND_USER_READER_WITH_KEY and ENTITYKIND_USER_READER_NO_KEY
  KIND_USER_READER,

  /// Represents ENTITYKIND_OPENDDS_TOPIC
  KIND_USER_TOPIC,

  /// Represents ENTITYKIND_BUILTIN_WRITER_WITH_KEY and ENTITYKIND_USER_WRITER_NO_KEY
  KIND_BUILTIN_WRITER,
  /// Represents ENTITYKIND_BUILTIN_READER_WITH_KEY and ENTITYKIND_USER_READER_NO_KEY
  KIND_BUILTIN_READER,
  /// Represents ENTITYKIND_BUILTIN_TOPIC
  KIND_BUILTIN_TOPIC,

  /// OpenDDS specific Publisher Guid values
  KIND_PUBLISHER,
  /// OpenDDS specific Subscriber Guid values
  KIND_SUBSCRIBER,
  /// OpenDDS specific other Guid values
  KIND_USER
};

struct OpenDDS_Dcps_Export GUID_tKeyLessThan {
  static bool entity_less(const EntityId_t& v1, const EntityId_t& v2)
  {
    return std::memcmp(&v1, &v2, sizeof(EntityId_t)) < 0;
  }

  bool operator()(const GUID_t& v1, const GUID_t& v2) const
  {
    return std::memcmp(&v1, &v2, sizeof(GUID_t)) < 0;
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
  return memcmp(&lhs, &rhs, sizeof(GUID_t)) == 0;
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
  return memcmp(&lhs, &rhs, sizeof(EntityId_t)) == 0;
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

OpenDDS_Dcps_Export OPENDDS_STRING
to_string(const EntityId_t& entityId);

#ifndef OPENDDS_SAFETY_PROFILE
// Serialize to ASCII Hex string: "xxxx.xxxx.xxxx.xxxx"
OpenDDS_Dcps_Export std::ostream&
operator<<(std::ostream& os, const GUID_t& rhs);

// Deserialize from ASCII Hex string: "xxxx.xxxx.xxxx.xxxx"
OpenDDS_Dcps_Export std::istream&
operator>>(std::istream& is, GUID_t& rhs);
#endif

OpenDDS_Dcps_Export inline GUID_t make_guid(
  const GuidPrefix_t& prefix, const EntityId_t& entity)
{
  GUID_t result;
  std::memcpy(result.guidPrefix, prefix, sizeof(GuidPrefix_t));
  std::memcpy(&result.entityId, &entity, sizeof(EntityId_t));
  return result;
}

OpenDDS_Dcps_Export
void intersect(const RepoIdSet& a, const RepoIdSet& b, RepoIdSet& result);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* GUIDUTILS_H */
