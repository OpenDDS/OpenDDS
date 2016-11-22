#include "Sync.h"
#include <dds/DCPS/debug.h>
#include <dds/DCPS/WaitSet.h>
#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

using OpenDDS::DCPS::DCPS_debug_level;

OpenDDS::Model::WriterSync::WriterSync(DDS::DataWriter_var writer,
                                       unsigned int num_readers) :
writer_(writer)
{
  if (wait_match(writer_, num_readers)) {
    throw std::runtime_error("wait_match failure");
  }
}

OpenDDS::Model::WriterSync::~WriterSync() OPENDDS_NOEXCEPT_FALSE
{
  if (wait_ack(writer_)) {
    throw std::runtime_error("wait_ack failure");
  }
}

int
OpenDDS::Model::WriterSync::wait_match(const DDS::DataWriter_var& writer,
                                       unsigned int num_readers)
{
  DDS::StatusCondition_var condition = writer->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);
  DDS::ConditionSeq conditions;
  DDS::PublicationMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::Duration_t timeout = { 3, 0 };
  DDS::ReturnCode_t stat;
  do {
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("WriterSync: waiting for pub matched\n")));
    }
    stat = writer->get_publication_matched_status(ms);
    if (stat != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((
                  LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() -")
                  ACE_TEXT(" get_publication_matched_status failed!\n")),
                 -1);
    } else if (ms.current_count >= (CORBA::Long)num_readers) {
      break;  // matched
    }
    // wait for a change
    stat = ws->wait(conditions, timeout);
    if ((stat != DDS::RETCODE_OK) && (stat != DDS::RETCODE_TIMEOUT)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() -")
                        ACE_TEXT(" wait failed!\n")),
                       -1);
    }
  } while (true);
  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_NOTICE, ACE_TEXT("WriterSync: pub matched\n")));
  }
  ws->detach_condition(condition);
  return 0;
}

int
OpenDDS::Model::WriterSync::wait_ack(const DDS::DataWriter_var& writer)
{
  DDS::ReturnCode_t stat;
  DDS::Duration_t timeout = { 30, 0 };
  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_NOTICE, ACE_TEXT("WriterSync: waiting for acks\n")));
  }
  stat = writer->wait_for_acknowledgments(timeout);
  if ((stat != DDS::RETCODE_OK) && (stat != DDS::RETCODE_TIMEOUT)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_ack() -")
                      ACE_TEXT(" wait_for_acknowledgments failed!\n")),
                     -1);
  }
  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_NOTICE, ACE_TEXT("WriterSync: acks received\n")));
  }
  return 0;
}

int
OpenDDS::Model::WriterSync::wait_unmatch(const DDS::DataWriter_var& writer,
                                         unsigned int num_readers)
{
  DDS::ReturnCode_t stat;
  DDS::StatusCondition_var condition = writer->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);
  DDS::ConditionSeq conditions;
  DDS::PublicationMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::Duration_t timeout = { 1, 0 };
  do {
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("WriterSync: pub checking unmatched\n")));
    }
    stat = writer->get_publication_matched_status(ms);
    if (stat != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((
                  LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_unmatch() -")
                  ACE_TEXT(" get_publication_matched_status failed!\n")),
                 -1);
    } else if (ms.current_count == 0 && (unsigned int)ms.total_count >= num_readers) {
      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_NOTICE, ACE_TEXT("WriterSync: pub match count %d total count %d\n"),
                                       ms.current_count, ms.total_count));
      }
      break;  // unmatched
    }
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("WriterSync: pub match count %d total count %d\n"),
                                     ms.current_count, ms.total_count));
    }
    // wait for a change
    stat = ws->wait(conditions, timeout);
    if ((stat != DDS::RETCODE_OK) && (stat != DDS::RETCODE_TIMEOUT)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_unmatch() -")
                        ACE_TEXT(" wait failed!\n")),
                       -1);
    }
  } while (true);
  ws->detach_condition(condition);
  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_NOTICE, ACE_TEXT("WriterSync: pub unmatched\n")));
  }
  return 0;
}

OpenDDS::Model::ReaderSync::ReaderSync(DDS::DataReader_var reader,
                                       unsigned int num_writers) :
reader_(reader),
num_writers_(num_writers)
{
}

OpenDDS::Model::ReaderSync::~ReaderSync() OPENDDS_NOEXCEPT_FALSE
{
  if (wait_unmatch(reader_, num_writers_)) {
    throw std::runtime_error("wait_unmatch failure");
  }
}

int
OpenDDS::Model::ReaderSync::wait_unmatch(const DDS::DataReader_var& reader,
                                         unsigned int num_writers)
{
  DDS::ReturnCode_t stat;
  DDS::StatusCondition_var condition = reader->get_statuscondition();
  condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);
  DDS::ConditionSeq conditions;
  DDS::SubscriptionMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::Duration_t timeout = { 1, 0 };
  do {
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("ReaderSync: sub checking unmatched\n")));
    }
    stat = reader->get_subscription_matched_status(ms);
    if (stat != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((
                  LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_unmatch() -")
                  ACE_TEXT(" get_subscription_matched_status failed!\n")),
                 -1);
    } else if (ms.current_count == 0 && (unsigned int)ms.total_count >= num_writers) {
      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_NOTICE, ACE_TEXT("ReaderSync: sub match count %d total count %d\n"),
                                       ms.current_count, ms.total_count));
      }
      break;  // unmatched
    }
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_NOTICE, ACE_TEXT("ReaderSync: sub match count %d total count %d\n"),
                                     ms.current_count, ms.total_count));
    }
    // wait for a change
    stat = ws->wait(conditions, timeout);
    if ((stat != DDS::RETCODE_OK) && (stat != DDS::RETCODE_TIMEOUT)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_unmatch() -")
                        ACE_TEXT(" wait failed!\n")),
                       -1);
    }
  } while (true);
  ws->detach_condition(condition);
  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_NOTICE, ACE_TEXT("ReaderSync: sub unmatched\n")));
  }
  return 0;
}

OpenDDS::Model::ReaderCondSync::ReaderCondSync(
     DDS::DataReader_var reader,
     ACE_Condition<ACE_SYNCH_MUTEX>& condition) :
reader_(reader),
complete_(false),
condition_(condition)
{
}

OpenDDS::Model::ReaderCondSync::~ReaderCondSync()
{
  ACE_GUARD(ACE_SYNCH_MUTEX, conditionGuard, condition_.mutex());
  while (!complete_) {
    condition_.wait();
  }
}

void OpenDDS::Model::ReaderCondSync::signal()
{
  ACE_GUARD(ACE_SYNCH_MUTEX, conditionGuard, condition_.mutex());
  complete_ = true;
  condition_.broadcast();
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
