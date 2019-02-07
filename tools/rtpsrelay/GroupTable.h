#ifndef RTPSRELAY_GROUP_TABLE_H_
#define RTPSRELAY_GROUP_TABLE_H_

#include "RelayTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>

#include <ace/Time_Value.h>

#include <set>
#include <string>

class GroupTable
{
public:
  using GuidSet = std::set<std::string>;
  using GroupSet = std::set<std::string>;

  GroupTable(const ACE_Time_Value& a_renew_after, const ACE_Time_Value& a_lifespan)
    : renew_after_(a_renew_after), lifespan_(a_lifespan) {}

  bool initialize(DDS::DomainParticipant_var a_dp, const std::string& a_topic_name);
  void update(const std::string& a_guid, const GroupSet& a_groups, const ACE_Time_Value& a_now);
  GuidSet guids(const std::string& a_group) const;
  GroupSet groups(const std::string& a_guid) const;

private:
  const ACE_Time_Value renew_after_;
  const ACE_Time_Value lifespan_;
  RtpsRelay::GroupEntryDataWriter_ptr writer_;
  RtpsRelay::GroupEntryDataReader_ptr reader_;
  DDS::QueryCondition_var group_query_condition_;
  DDS::QueryCondition_var guid_query_condition_;

  void insert(const std::string& a_guid, const std::string & a_group, const ACE_Time_Value& a_now);
  void remove(const std::string& a_guid, const std::string & a_group);
};

#endif /* RTPSRELAY_GROUP_TABLE_H_ */
