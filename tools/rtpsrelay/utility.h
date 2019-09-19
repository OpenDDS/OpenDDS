#ifndef RTPSRELAY_UTILITY_H_
#define RTPSRELAY_UTILITY_H_

#include <sstream>

inline std::string guid_to_string(const OpenDDS::DCPS::GUID_t& a_guid)
{
  std::stringstream ss;
  ss << a_guid;
  return ss.str();
}

inline std::string guid_to_string(const RtpsRelay::GUID_t& a_guid)
{
  OpenDDS::DCPS::GUID_t g;
  std::memcpy(&g, &a_guid, sizeof(RtpsRelay::GUID_t));
  return guid_to_string(g);
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

inline bool operator==(const RtpsRelay::GUID_t& v1, const RtpsRelay::GUID_t& v2) {
  return std::memcmp(&v1, &v2, sizeof(RtpsRelay::GUID_t)) == 0;
}

inline bool operator!=(const RtpsRelay::GUID_t& v1, const RtpsRelay::GUID_t& v2) {
  return std::memcmp(&v1, &v2, sizeof(RtpsRelay::GUID_t)) != 0;
}

struct GUID_tKeyLessThan {
  bool operator()(const RtpsRelay::GUID_t& v1, const RtpsRelay::GUID_t& v2) const
  {
    return std::memcmp(&v1, &v2, sizeof(RtpsRelay::GUID_t)) < 0;
  }
};

typedef std::set<RtpsRelay::GUID_t, GUID_tKeyLessThan> GuidSet;

const RtpsRelay::EntityId_t ENTITYID_UNKNOWN                                = { {0x00,0x00,0x00}, 0x00};

#endif // RTPSRELAY_UTILITY_H_
