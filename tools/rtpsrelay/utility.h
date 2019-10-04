#ifndef RTPSRELAY_UTILITY_H_
#define RTPSRELAY_UTILITY_H_

#include "dds/DCPS/GuidUtils.h"

#include <ace/INET_Addr.h>

#include <sstream>

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

inline OpenDDS::DCPS::RepoId guid_to_guid(const GUID_t& a_guid)
{
  OpenDDS::DCPS::RepoId retval;
  std::memcpy(&retval, &a_guid, sizeof(OpenDDS::DCPS::RepoId));
  return retval;
}

inline bool operator==(const RelayAddresses& x,
                       const RelayAddresses& y)
{
  return x.spdp_relay_address() == y.spdp_relay_address() &&
    x.sedp_relay_address() == y.sedp_relay_address() &&
    x.data_relay_address() == y.data_relay_address();
}

inline bool operator!=(const RelayAddresses& x,
                       const RelayAddresses& y)
{
  return x.spdp_relay_address() != y.spdp_relay_address() ||
    x.sedp_relay_address() != y.sedp_relay_address() ||
    x.data_relay_address() != y.data_relay_address();
}

typedef std::set<OpenDDS::DCPS::RepoId, OpenDDS::DCPS::GUID_tKeyLessThan> GuidSet;

}

#endif // RTPSRELAY_UTILITY_H_
