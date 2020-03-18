#ifndef RTPSRELAY_UTILITY_H_
#define RTPSRELAY_UTILITY_H_

#include "lib/RelayC.h"

#include "dds/DCPS/GuidUtils.h"

#include <ace/INET_Addr.h>

#include <sstream>
#include <string>

namespace RtpsRelay {

inline std::string addr_to_string(const ACE_INET_Addr& a_addr)
{
  std::array<ACE_TCHAR, 256> as_string{};
  if (a_addr.addr_to_string(as_string.data(), as_string.size()) != 0) {
    ACE_ERROR((LM_ERROR, "(%P:%t) %N:%l ERROR: addr_to_string failed to convert address to string"));
    return "";
  }
  return ACE_TEXT_ALWAYS_CHAR(as_string.data());
}

inline std::string guid_to_string(const OpenDDS::DCPS::GUID_t& a_guid)
{
  std::stringstream ss;
  ss << a_guid;
  return ss.str();
}

struct RelayAddressesLessThan {
  inline bool operator()(const RelayAddresses& x,
                         const RelayAddresses& y) const
  {
    if (x.spdp_relay_address() != y.spdp_relay_address()) {
      return x.spdp_relay_address() < y.spdp_relay_address();
    }
    if (x.sedp_relay_address() != y.sedp_relay_address()) {
      return x.sedp_relay_address() < y.sedp_relay_address();
    }
    return x.data_relay_address() < y.data_relay_address();
  }
};

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

}

#endif // RTPSRELAY_UTILITY_H_
