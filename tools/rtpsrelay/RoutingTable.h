#ifndef RTPSRELAY_ROUTING_TABLE_H_
#define RTPSRELAY_ROUTING_TABLE_H_

#include "RelayTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>

#include <ace/Time_Value.h>

#include <string>

class RoutingTable {
public:
  RoutingTable(const ACE_Time_Value& a_renew_after, const ACE_Time_Value& a_lifespan)
    : renew_after_(a_renew_after), lifespan_(a_lifespan) {}

  bool initialize(DDS::DomainParticipant_var a_dp, const std::string& a_topic_name);
  void update(const std::string& a_guid,
              const std::string& a_horizontal_relay_address,
              const std::string& a_address,
              const ACE_Time_Value& a_now);
  std::string horizontal_relay_address(const std::string& a_guid) const;
  std::string address(const std::string& a_guid) const;

private:
  const ACE_Time_Value renew_after_;
  const ACE_Time_Value lifespan_;
  RtpsRelay::RoutingEntryDataWriter_ptr writer_;
  RtpsRelay::RoutingEntryDataReader_ptr reader_;

  bool fetch(RtpsRelay::RoutingEntry& a_entry) const;
};

#endif // RTPSRELAY_ROUTING_TABLE_H_
