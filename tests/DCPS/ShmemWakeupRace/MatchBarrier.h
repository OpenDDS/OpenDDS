/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef SHMEM_WAKEUP_RACE_MATCH_BARRIER_H
#define SHMEM_WAKEUP_RACE_MATCH_BARRIER_H

#include <dds/DCPS/WaitSet.h>

#include <ace/OS_NS_unistd.h>
#include <ace/Time_Value.h>

namespace ShmemWakeupRace {

inline bool wait_for_match(DDS::DataWriter_ptr writer)
{
  DDS::PublicationMatchedStatus match_status;
  if (writer->get_publication_matched_status(match_status) != DDS::RETCODE_OK) {
    return false;
  }
  if (match_status.current_count >= 1) {
    return true;
  }

  DDS::StatusCondition_var condition = writer->get_statuscondition();
  if (condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::WaitSet_var wait_set = new DDS::WaitSet;
  if (wait_set->attach_condition(condition.in()) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::ConditionSeq active;
  const DDS::Duration_t timeout = { 5, 0 };
  const DDS::ReturnCode_t wait_result = wait_set->wait(active, timeout);
  const DDS::ReturnCode_t detach_result = wait_set->detach_condition(condition.in());
  return wait_result == DDS::RETCODE_OK && detach_result == DDS::RETCODE_OK &&
    writer->get_publication_matched_status(match_status) == DDS::RETCODE_OK &&
    match_status.current_count >= 1;
}

inline bool wait_for_match(DDS::DataReader_ptr reader)
{
  DDS::SubscriptionMatchedStatus match_status;
  if (reader->get_subscription_matched_status(match_status) != DDS::RETCODE_OK) {
    return false;
  }
  if (match_status.current_count >= 1) {
    return true;
  }

  DDS::StatusCondition_var condition = reader->get_statuscondition();
  if (condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::WaitSet_var wait_set = new DDS::WaitSet;
  if (wait_set->attach_condition(condition.in()) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::ConditionSeq active;
  const DDS::Duration_t timeout = { 5, 0 };
  const DDS::ReturnCode_t wait_result = wait_set->wait(active, timeout);
  const DDS::ReturnCode_t detach_result = wait_set->detach_condition(condition.in());
  return wait_result == DDS::RETCODE_OK && detach_result == DDS::RETCODE_OK &&
    reader->get_subscription_matched_status(match_status) == DDS::RETCODE_OK &&
    match_status.current_count >= 1;
}

inline void settle_after_matches()
{
  ACE_OS::sleep(ACE_Time_Value(1, 200000));
}

}

#endif
