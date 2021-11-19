#ifndef OPENDDS_RTPSRELAYLIB_UTILITY_H
#define OPENDDS_RTPSRELAYLIB_UTILITY_H

#include "RelayC.h"

#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/TimeDuration.h"

#include <ace/INET_Addr.h>

#include <cstring>
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

enum Port {
  SPDP,
  SEDP,
  DATA
};

enum class MessageType {
  Unknown,
  Rtps,
  Stun,
};

struct AddrPort {
  ACE_INET_Addr addr;
  Port port;

  AddrPort() : port(SPDP) {}
  AddrPort(const ACE_INET_Addr& a, Port p) : addr(a), port(p) {}

  bool operator==(const AddrPort& other) const
  {
    return addr == other.addr && port == other.port;
  }

  bool operator!=(const AddrPort& other) const
  {
    return !(*this == other);
  }

  bool operator<(const AddrPort& other) const
  {
    return addr < other.addr || (addr == other.addr && port < other.port);
  }
};

struct GuidAddr {
  OpenDDS::DCPS::GUID_t guid;
  AddrPort address;

  GuidAddr() : guid(OpenDDS::DCPS::GUID_UNKNOWN) {}
  GuidAddr(const OpenDDS::DCPS::GUID_t& a_guid, const AddrPort& a_address)
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
      static const OpenDDS::DCPS::GUID_tKeyLessThan gc;
      return gc(guid, other.guid);
    }
    return address < other.address;
  }
};

inline bool operator==(const EntityId_t& x, const EntityId_t& y)
{
  return std::memcmp(&x, &y, sizeof(x)) == 0;
}

inline bool operator!=(const EntityId_t& x, const EntityId_t& y)
{
  return std::memcmp(&x, &y, sizeof(x)) != 0;
}

inline void assign(EntityId_t& eid, const OpenDDS::DCPS::EntityId_t& a_eid)
{
  std::memcpy(&eid._entityKey[0], a_eid.entityKey, sizeof(a_eid.entityKey));
  eid.entityKind(a_eid.entityKind);
}

inline bool operator==(const GUID_t& x, const GUID_t& y)
{
  return std::memcmp(&x, &y, sizeof(x)) == 0;
}

inline bool operator!=(const GUID_t& x, const GUID_t& y)
{
  return std::memcmp(&x, &y, sizeof(x)) != 0;
}

inline void assign(GUID_t& guid, const OpenDDS::DCPS::GUID_t& a_guid)
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

inline bool operator==(const Duration_t& x, const Duration_t& y)
{
  return x.sec() == y.sec() && x.nanosec() == y.nanosec();
}

inline bool operator!=(const Duration_t& x, const Duration_t& y)
{
  return x.sec() != y.sec() || x.nanosec() != y.nanosec();
}

inline bool operator<(const Duration_t& x, const Duration_t& y)
{
  if (x.sec() != y.sec()) {
    return x.sec() < y.sec();
  }
  return x.nanosec() < y.nanosec();
}

inline OpenDDS::DCPS::GUID_t relay_guid_to_rtps_guid(const GUID_t& a_guid)
{
  OpenDDS::DCPS::GUID_t retval;
  std::memcpy(&retval, &a_guid, sizeof(OpenDDS::DCPS::GUID_t));
  return retval;
}

inline GUID_t rtps_guid_to_relay_guid(const OpenDDS::DCPS::GUID_t& a_guid)
{
  GUID_t retval;
  std::memcpy(&retval.guidPrefix(), a_guid.guidPrefix, sizeof(a_guid.guidPrefix));
  std::memcpy(&retval.entityId().entityKey(), a_guid.entityId.entityKey, sizeof(a_guid.entityId.entityKey));
  retval.entityId().entityKind(a_guid.entityId.entityKind);
  return retval;
}

struct GuidHash {
  std::size_t operator() (const OpenDDS::DCPS::GUID_t& guid) const
  {
    return
      (static_cast<std::size_t>(guid.guidPrefix[0]) << 15) ^
      (static_cast<std::size_t>(guid.guidPrefix[1]) << 14) ^
      (static_cast<std::size_t>(guid.guidPrefix[2]) << 13) ^
      (static_cast<std::size_t>(guid.guidPrefix[3]) << 12) ^
      (static_cast<std::size_t>(guid.guidPrefix[4]) << 11) ^
      (static_cast<std::size_t>(guid.guidPrefix[5]) << 10) ^
      (static_cast<std::size_t>(guid.guidPrefix[6]) << 9) ^
      (static_cast<std::size_t>(guid.guidPrefix[7]) << 8) ^
      (static_cast<std::size_t>(guid.guidPrefix[8]) << 7) ^
      (static_cast<std::size_t>(guid.guidPrefix[9]) << 6) ^
      (static_cast<std::size_t>(guid.guidPrefix[10]) << 5) ^
      (static_cast<std::size_t>(guid.guidPrefix[11]) << 4) ^
      (static_cast<std::size_t>(guid.entityId.entityKey[0]) << 3) ^
      (static_cast<std::size_t>(guid.entityId.entityKey[1]) << 2) ^
      (static_cast<std::size_t>(guid.entityId.entityKey[2]) << 1) ^
      (static_cast<std::size_t>(guid.entityId.entityKind) << 0);
  }
};
typedef std::unordered_set<OpenDDS::DCPS::GUID_t, GuidHash> GuidSet;

}

#endif // RTPSRELAY_UTILITY_H_
