#ifndef RTPSRELAY_UTILITY_H_
#define RTPSRELAY_UTILITY_H_

#include <sstream>

inline std::string guid_to_string(const OpenDDS::DCPS::GUID_t& a_guid)
{
  std::stringstream ss;
  ss << a_guid;
  return ss.str();
}

inline OpenDDS::DCPS::RepoId guid_to_guid(const RtpsRelay::GUID_t& a_guid)
{
  OpenDDS::DCPS::RepoId retval;
  std::memcpy(&retval, &a_guid, sizeof(OpenDDS::DCPS::RepoId));
  return retval;
}

inline bool operator==(const RtpsRelay::RelayAddresses& x,
                       const RtpsRelay::RelayAddresses& y)
{
  return x.spdp_relay_address() == y.spdp_relay_address() &&
    x.sedp_relay_address() == y.sedp_relay_address() &&
    x.data_relay_address() == y.data_relay_address();
}

inline bool operator!=(const RtpsRelay::RelayAddresses& x,
                       const RtpsRelay::RelayAddresses& y)
{
  return x.spdp_relay_address() != y.spdp_relay_address() ||
    x.sedp_relay_address() != y.sedp_relay_address() ||
    x.data_relay_address() != y.data_relay_address();
}

struct RelayAddressesLessThan {
  inline bool operator()(const RtpsRelay::RelayAddresses& x,
                         const RtpsRelay::RelayAddresses& y) const
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

typedef std::set<OpenDDS::DCPS::RepoId, OpenDDS::DCPS::GUID_tKeyLessThan> GuidSet;

#endif // RTPSRELAY_UTILITY_H_
