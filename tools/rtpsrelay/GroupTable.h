#ifndef RTPSRELAY_GROUP_TABLE_H_
#define RTPSRELAY_GROUP_TABLE_H_

#include <dds/DCPS/Service_Participant.h>

#include "RelayTypeSupportImpl.h"

class GroupTable
{
public:
  using GuidSet = std::set<std::string>;
  using GroupSet = std::set<std::string>;

  GroupTable(ACE_Time_Value const & a_renew_after, ACE_Time_Value const & a_lifespan)
    : m_renew_after(a_renew_after), m_lifespan(a_lifespan) {}
  int initialize(DDS::DomainParticipant_var a_dp, std::string const & a_topic_name);
  void update(std::string const & a_guid, GroupSet const & a_groups, ACE_Time_Value const & a_now);
  GuidSet guids(std::string const & a_group) const;
  GroupSet groups(std::string const & a_guid) const;

private:
  ACE_Time_Value const m_renew_after;
  ACE_Time_Value const m_lifespan;
  RtpsRelay::GroupEntryDataWriter_ptr m_writer;
  RtpsRelay::GroupEntryDataReader_ptr m_reader;
  DDS::QueryCondition_var m_group_query_condition;
  DDS::QueryCondition_var m_guid_query_condition;

  void insert(std::string const & a_guid, std::string const & a_group, ACE_Time_Value const & a_now);
  void remove(std::string const & a_guid, std::string const & a_group);
};

#endif /* RTPSRELAY_GROUP_TABLE_H_ */
