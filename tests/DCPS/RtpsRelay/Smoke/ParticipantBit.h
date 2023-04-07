#include <tests/Utils/DistributedConditionSet.h>


#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsInfrastructureC.h>

#include <iostream>
#include <cstdlib>

inline
int check_participant_bit(DDS::DomainParticipant* participant, int expected_instances,
                          const char* user_data)
{
  DDS::InstanceHandleSeq handles;
  static const auto limit = 10;
  for (int retries = 0;
       static_cast<int>(handles.length()) < expected_instances && retries < limit;
       ++retries) {
    if (participant->get_discovered_participants(handles) != DDS::RETCODE_OK) {
      std::cerr << "ERROR: " << user_data << " failed to get_discovered_participants\n";
      return EXIT_FAILURE;
    }
    std::cerr << user_data << ": waiting for discovery\n";
    ACE_OS::sleep(1);
  }

  if (static_cast<int>(handles.length()) != expected_instances) {
    std::cerr << "ERROR: " << user_data << " discovered " << handles.length() <<
      " participants but expected " << expected_instances << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

inline
int test_participant_discovery(DDS::DomainParticipant* participant, int expected_instances,
                               const char* user_data, const char* other_user_data,
                               const OpenDDS::DCPS::TimeDuration& resend)
{
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  static const auto barrier_entities_created = "entities created";
  distributed_condition_set->post(user_data, barrier_entities_created);
  distributed_condition_set->wait_for(user_data, other_user_data, barrier_entities_created);

  int status = check_participant_bit(participant, expected_instances, user_data);
  if (status != EXIT_SUCCESS) {
    return status;
  }

  std::cerr << user_data << ": Discovered " << expected_instances << " participant(s)\n";

  static const auto barrier_check1_done = "check1 done";
  distributed_condition_set->post(user_data, barrier_check1_done);
  distributed_condition_set->wait_for(user_data, other_user_data, barrier_check1_done);

  // wait to see if any other DomainParticipant is discovered
  ACE_OS::sleep(3 * resend.value());

  status = check_participant_bit(participant, expected_instances, user_data);

  static const auto barrier_check2_done = "check2 done";
  distributed_condition_set->post(user_data, barrier_check2_done);
  distributed_condition_set->wait_for(user_data, other_user_data, barrier_check2_done);

  return status;
}

