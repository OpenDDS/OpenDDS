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
int wait_match<DDS::DataWriter_var>(const DDS::DataWriter_var& writer, unsigned int num_readers, CmpOp cmp)
{
  DDS::StatusCondition_var condition = writer->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

  DDS::WaitSet_var ws(new DDS::WaitSet);
  ws->attach_condition(condition);

  DDS::ReturnCode_t stat;

  DDS::ConditionSeq conditions;
  DDS::PublicationMatchedStatus ms = { 0, 0, 0, 0, 0 };

  const DDS::Duration_t wake_interval = { 3, 0 };

  bool stop = false;
  while (!stop) {
    stat = writer->get_publication_matched_status(ms);
    if (stat != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((
                  LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() -")
                  ACE_TEXT(" get_publication_matched_status failed!\n")),
                 -1);
    } else {
      if (cmp == LT && ms.current_count < static_cast<CORBA::Long>(num_readers)) {
        stop = true;
      } else if (cmp == LTE && ms.current_count <= static_cast<CORBA::Long>(num_readers)) {
        stop = true;
      } else if (cmp == EQ && ms.current_count == static_cast<CORBA::Long>(num_readers)) {
        stop = true;
      } else if (cmp == GTE && ms.current_count >= static_cast<CORBA::Long>(num_readers)) {
        stop = true;
      } else if (cmp == GT && ms.current_count > static_cast<CORBA::Long>(num_readers)) {
        stop = true;
      } else {
        // wait for a change
        stat = ws->wait(conditions, wake_interval);
        if ((stat != DDS::RETCODE_OK) && (stat != DDS::RETCODE_TIMEOUT)) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() -")
                            ACE_TEXT(" wait failed!\n")),
                           -1);
        }
      }
    }
  }

  ws->detach_condition(condition);
  return 0;
}

template <>
int wait_match<DDS::DataReader_var>(const DDS::DataReader_var& reader, unsigned int num_writers, CmpOp cmp)
{
  DDS::StatusCondition_var condition = reader->get_statuscondition();
  condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

  DDS::WaitSet_var ws(new DDS::WaitSet);
  ws->attach_condition(condition);

  DDS::ReturnCode_t stat;

  DDS::ConditionSeq conditions;
  DDS::SubscriptionMatchedStatus ms = { 0, 0, 0, 0, 0 };

  const DDS::Duration_t wake_interval = { 3, 0 };

  bool stop = false;
  while (!stop) {
    stat = reader->get_subscription_matched_status(ms);
    if (stat != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((
                  LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() -")
                  ACE_TEXT(" get_subscription_matched_status failed!\n")),
                 -1);
    } else {
      if (cmp == LT && ms.current_count < static_cast<CORBA::Long>(num_writers)) {
        stop = true;
      } else if (cmp == LTE && ms.current_count <= static_cast<CORBA::Long>(num_writers)) {
        stop = true;
      } else if (cmp == EQ && ms.current_count == static_cast<CORBA::Long>(num_writers)) {
        stop = true;
      } else if (cmp == GTE && ms.current_count >= static_cast<CORBA::Long>(num_writers)) {
        stop = true;
      } else if (cmp == GT && ms.current_count > static_cast<CORBA::Long>(num_writers)) {
        stop = true;
      } else {
        // wait for a change
        stat = ws->wait(conditions, wake_interval);
        if ((stat != DDS::RETCODE_OK) && (stat != DDS::RETCODE_TIMEOUT)) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() -")
                            ACE_TEXT(" wait failed!\n")),
                           -1);
        }
      }
    }
  }

  ws->detach_condition(condition);
  return 0;
}

}

#endif

