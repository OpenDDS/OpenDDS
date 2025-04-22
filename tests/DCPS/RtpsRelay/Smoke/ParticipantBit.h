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
      ACE_ERROR((LM_ERROR, "ERROR: %C failed to get_discovered_participants\n", user_data));
      return EXIT_FAILURE;
    }
    ACE_DEBUG((LM_DEBUG, "%C: waiting for discovery\n", user_data));;
    ACE_OS::sleep(1);
  }

  if (static_cast<int>(handles.length()) != expected_instances) {
    ACE_ERROR((LM_ERROR, "ERROR: %C discovered %d participants but expected %d\n", user_data, handles.length(), expected_instances));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

inline
int test_participant_discovery(DistributedConditionSet_rch distributed_condition_set,
                               DDS::DomainParticipant* participant, int expected_instances,
                               const char* user_data, const char* other_user_data,
                               const OpenDDS::DCPS::TimeDuration& resend)
{
  static const auto barrier_entities_created = "entities created";
  distributed_condition_set->post(user_data, barrier_entities_created);
  distributed_condition_set->wait_for(user_data, other_user_data, barrier_entities_created);

  int status = check_participant_bit(participant, expected_instances, user_data);
  if (status != EXIT_SUCCESS) {
    return status;
  }

  ACE_ERROR((LM_ERROR, "%C: Discovered %d participant(s)\n",  user_data, expected_instances));

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

