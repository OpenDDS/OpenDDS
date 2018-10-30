#ifndef RTPSRELAY_ROUTING_TABLE_H_
#define RTPSRELAY_ROUTING_TABLE_H_

#include <ace/INET_Addr.h>
#include <dds/DCPS/Service_Participant.h>

#include "RelayTypeSupportImpl.h"

class RoutingTable
{
 public:
  RoutingTable(ACE_Time_Value const & a_renew_after,
               ACE_Time_Value const & a_lifespan)
    : m_renew_after(a_renew_after), m_lifespan(a_lifespan) {}
  int initialize(DDS::DomainParticipant_var a_dp,
                 std::string const & a_topic_name);
  void update(std::string const & a_guid,
              std::string const & a_horizontal_relay_address,
              std::string const & a_address,
              ACE_Time_Value const & a_now);
  std::string horizontal_relay_address(std::string const & a_guid) const;
  std::string address(std::string const & a_guid) const;

 private:
  ACE_Time_Value const m_renew_after;
  ACE_Time_Value const m_lifespan;
  RtpsRelay::RoutingEntryDataWriter_ptr m_writer;
  RtpsRelay::RoutingEntryDataReader_ptr m_reader;

  bool fetch(RtpsRelay::RoutingEntry & a_entry) const;
};

#endif // RTPSRELAY_ROUTING_TABLE_H_
