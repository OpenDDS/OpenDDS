#pragma once

#ifndef TESTUTILS_STATUS_MATCHING_H
#define TESTUTILS_STATUS_MATCHING_H

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/WaitSet.h"

namespace Utils {

enum CmpOp {
  LT,
  LTE,
  EQ,
  GTE,
  GT
};

template <typename Entity>
int wait_match(const Entity& entity, unsigned int count, CmpOp cmp = EQ);

template <>
inline int wait_match<DDS::DataWriter_var>(const DDS::DataWriter_var& writer, unsigned int num_readers, CmpOp cmp)
{
  int ret = -1;
  DDS::StatusCondition_var condition = writer->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  DDS::WaitSet_var ws(new DDS::WaitSet);
  DDS::ReturnCode_t a = ws->attach_condition(condition);
  if (a != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - attach_condition returned %d\n"), a));
    return ret;
  }

  const CORBA::Long n_readers = static_cast<CORBA::Long>(num_readers);
  const DDS::Duration_t wake_interval = { 3, 0 };
  DDS::PublicationMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::ConditionSeq conditions;
  while (ret != 0) {
    if (writer->get_publication_matched_status(ms) == DDS::RETCODE_OK) {
      if ((cmp == LT  && ms.current_count <  n_readers) ||
          (cmp == LTE && ms.current_count <= n_readers) ||
          (cmp == EQ  && ms.current_count == n_readers) ||
          (cmp == GTE && ms.current_count >= n_readers) ||
          (cmp == GT  && ms.current_count >  n_readers)) {
        ret = 0;
      } else { // wait for a change
        DDS::ReturnCode_t w = ws->wait(conditions, wake_interval);
        if ((w != DDS::RETCODE_OK) && (w != DDS::RETCODE_TIMEOUT)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - wait returned %d\n"), w));
          break;
        }
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - get_publication_matched_status failed!\n")));
      break;
    }
  }

  ws->detach_condition(condition);
  return ret;
}

template <>
inline int wait_match<DDS::DataReader_var>(const DDS::DataReader_var& reader, unsigned int num_writers, CmpOp cmp)
{
  int ret = -1;
  DDS::StatusCondition_var condition = reader->get_statuscondition();
  condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
  DDS::WaitSet_var ws(new DDS::WaitSet);
  DDS::ReturnCode_t a = ws->attach_condition(condition);
  if (a != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - attach_condition returned %d\n"), a));
    return ret;
  }

  const CORBA::Long n_writers = static_cast<CORBA::Long>(num_writers);
  const DDS::Duration_t wake_interval = { 3, 0 };
  DDS::SubscriptionMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::ConditionSeq conditions;
  while (ret != 0) {
    if (reader->get_subscription_matched_status(ms) == DDS::RETCODE_OK) {
      if ((cmp == LT  && ms.current_count <  n_writers) ||
          (cmp == LTE && ms.current_count <= n_writers) ||
          (cmp == EQ  && ms.current_count == n_writers) ||
          (cmp == GTE && ms.current_count >= n_writers) ||
          (cmp == GT  && ms.current_count >  n_writers)) {
        ret = 0;
      } else { // wait for a change
        DDS::ReturnCode_t w = ws->wait(conditions, wake_interval);
        if ((w != DDS::RETCODE_OK) && (w != DDS::RETCODE_TIMEOUT)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - wait returned %d\n"), w));
          break;
        }
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() - get_subscription_matched_status failed!\n")));
      break;
    }
  }

  ws->detach_condition(condition);
  return ret;
}

}

#endif
