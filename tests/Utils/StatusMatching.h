#pragma once

#ifndef TESTUTILS_STATUS_MATCHING_H
#define TESTUTILS_STATUS_MATCHING_H

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/WaitSet.h"

namespace Utils {

enum CmpOp { LT, LTE, EQ, GTE, GT };
const DDS::Duration_t wake_interval = { 3, 0 };

template <typename Entity>
int wait_match(const Entity& entity, unsigned int count, CmpOp cmp = EQ);

template <>
int wait_match<DDS::DataWriter_var>(const DDS::DataWriter_var& writer, unsigned int num_readers, CmpOp cmp)
{
  DDS::StatusCondition_var condition = writer->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  DDS::WaitSet_var ws(new DDS::WaitSet);
  ws->attach_condition(condition);

  DDS::PublicationMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::ConditionSeq conditions;
  bool match = false;
  while (!match) {
    if (writer->get_publication_matched_status(ms) == DDS::RETCODE_OK) {
      if (cmp == LT && ms.current_count < static_cast<CORBA::Long>(num_readers)) {
        match = true;
      } else if (cmp == LTE && ms.current_count <= static_cast<CORBA::Long>(num_readers)) {
        match = true;
      } else if (cmp == EQ && ms.current_count == static_cast<CORBA::Long>(num_readers)) {
        match = true;
      } else if (cmp == GTE && ms.current_count >= static_cast<CORBA::Long>(num_readers)) {
        match = true;
      } else if (cmp == GT && ms.current_count > static_cast<CORBA::Long>(num_readers)) {
        match = true;
      } else {
        DDS::ReturnCode_t r = ws->wait(conditions, wake_interval);
        if ((r != DDS::RETCODE_OK) && (r != DDS::RETCODE_TIMEOUT)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - wait returned %d\n"), r));
          break;
        }
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - get_publication_matched_status failed!\n")));
      break;
    }
  }

  ws->detach_condition(condition);
  return match ? 0 : -1;
}

template <>
int wait_match<DDS::DataReader_var>(const DDS::DataReader_var& reader, unsigned int num_writers, CmpOp cmp)
{
  DDS::StatusCondition_var condition = reader->get_statuscondition();
  condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
  DDS::WaitSet_var ws(new DDS::WaitSet);
  ws->attach_condition(condition);

  DDS::SubscriptionMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::ConditionSeq conditions;
  bool match = false;
  while (!match) {
    if (reader->get_subscription_matched_status(ms) == DDS::RETCODE_OK) {
      if (cmp == LT && ms.current_count < static_cast<CORBA::Long>(num_writers)) {
        match = true;
      } else if (cmp == LTE && ms.current_count <= static_cast<CORBA::Long>(num_writers)) {
        match = true;
      } else if (cmp == EQ && ms.current_count == static_cast<CORBA::Long>(num_writers)) {
        match = true;
      } else if (cmp == GTE && ms.current_count >= static_cast<CORBA::Long>(num_writers)) {
        match = true;
      } else if (cmp == GT && ms.current_count > static_cast<CORBA::Long>(num_writers)) {
        match = true;
      } else {
        DDS::ReturnCode_t r = ws->wait(conditions, wake_interval);
        if ((r != DDS::RETCODE_OK) && (r != DDS::RETCODE_TIMEOUT)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - wait returned %d\n"), r));
          break;
        }
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - get_subscription_matched_status failed!\n")));
      break;
    }
  }

  ws->detach_condition(condition);
  return match ? 0 : -1;
}

}

#endif
