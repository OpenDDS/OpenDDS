#ifndef RTPSRELAY_UTILITY_H_
#define RTPSRELAY_UTILITY_H_

#include "RelayC.h"

#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/TimeDuration.h"

#include <ace/INET_Addr.h>

#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

namespace RtpsRelay {

typedef std::set<std::string> StringSet;

inline std::string guid_to_string(const OpenDDS::DCPS::GUID_t& a_guid)
{
  std::stringstream ss;
  ss << a_guid;
  return ss.str();
}

struct GuidAddr {
  OpenDDS::DCPS::RepoId guid;
  ACE_INET_Addr address;

  GuidAddr() : guid(OpenDDS::DCPS::GUID_UNKNOWN) {}
  GuidAddr(const OpenDDS::DCPS::RepoId& a_guid, const ACE_INET_Addr& a_address)
    : guid(a_guid)
    , address(a_address)
  {}

  bool operator==(const GuidAddr& other) const
  {
    return guid == other.guid && address == other.address;
  }

  bool operator!=(const GuidAddr& other) const
  {
    return guid != other.guid || address != other.address;
  }

  bool operator<(const GuidAddr& other) const
  {
    if (guid != other.guid) {
      OpenDDS::DCPS::GUID_tKeyLessThan gc;
      return gc(guid, other.guid);
    }
    return address < other.address;
  }
};

inline void assign(EntityId_t& eid, const OpenDDS::DCPS::EntityId_t& a_eid)
{
  std::memcpy(&eid._entityKey[0], a_eid.entityKey, sizeof(a_eid.entityKey));
  eid.entityKind(a_eid.entityKind);
}

inline void assign(GUID_t& guid, const OpenDDS::DCPS::RepoId& a_guid)
{
  std::memcpy(&guid._guidPrefix[0], a_guid.guidPrefix, sizeof(a_guid.guidPrefix));
  assign(guid.entityId(), a_guid.entityId);
}

inline Duration_t time_diff_to_duration(const OpenDDS::DCPS::TimeDuration& d)
{
  Duration_t duration;
  const auto x = d.to_dds_duration();
  duration.sec(x.sec);
  duration.nanosec(x.nanosec);
  return duration;
}

inline bool operator<(const Duration_t& x, const Duration_t& y)
{
  if (x.sec() != y.sec()) {
    return x.sec() < y.sec();
  }
  return x.nanosec() < y.nanosec();
}

inline OpenDDS::DCPS::RepoId guid_to_repoid(const GUID_t& a_guid)
{
  OpenDDS::DCPS::RepoId retval;
  std::memcpy(&retval, &a_guid, sizeof(OpenDDS::DCPS::RepoId));
  return retval;
}

inline GUID_t repoid_to_guid(const OpenDDS::DCPS::RepoId& a_guid)
{
  GUID_t retval;
  std::memcpy(&retval.guidPrefix(), a_guid.guidPrefix, sizeof(a_guid.guidPrefix));
  std::memcpy(&retval.entityId().entityKey(), a_guid.entityId.entityKey, sizeof(a_guid.entityId.entityKey));
  retval.entityId().entityKind(a_guid.entityId.entityKind);
  return retval;
}

struct GuidHash {
  std::size_t operator() (const OpenDDS::DCPS::RepoId& guid) const
  {
    return
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[0]) << 15) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[1]) << 14) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[2]) << 13) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[3]) << 12) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[4]) << 11) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[5]) << 10) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[6]) << 9) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[7]) << 8) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[8]) << 7) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[9]) << 6) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[10]) << 5) ^
      (std::hash<::CORBA::Octet>{}(guid.guidPrefix[11]) << 4) ^
      (std::hash<::CORBA::Octet>{}(guid.entityId.entityKey[0]) << 3) ^
      (std::hash<::CORBA::Octet>{}(guid.entityId.entityKey[1]) << 2) ^
      (std::hash<::CORBA::Octet>{}(guid.entityId.entityKey[2]) << 1) ^
      (std::hash<::CORBA::Octet>{}(guid.entityId.entityKind) << 0);
  }
};
typedef std::unordered_set<OpenDDS::DCPS::RepoId, GuidHash> GuidSet;

}

#endif // RTPSRELAY_UTILITY_H_
