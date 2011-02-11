#include "Sync.h"
#include <dds/DCPS/WaitSet.h>
#include <iostream>
#include <stdexcept>

OpenDDS::Model::WriterSync::WriterSync(DDS::DataWriter_var& writer) :
writer_(writer)
{
  if (wait_match(writer_)) {
    throw std::runtime_error("wait_match failure");
  }
}

OpenDDS::Model::WriterSync::~WriterSync()
{
  if (wait_ack(writer_)) {
    throw std::runtime_error("wait_ack failure");
  }
}

int
OpenDDS::Model::WriterSync::wait_match(DDS::DataWriter_var& writer)
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
    // std::cout << "waiting for pub matched" << std::endl;
    stat = writer->get_publication_matched_status(ms);
    if (stat != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((
                  LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_match() -")
                  ACE_TEXT(" get_publication_matched_status failed!\n")),
                 -1);
    } else if (ms.current_count > 0) {
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
  // std::cout << "pub matched" << std::endl;
  ws->detach_condition(condition);
  return 0;
}

int
OpenDDS::Model::WriterSync::wait_ack(DDS::DataWriter_var& writer)
{
  DDS::ReturnCode_t stat;
  DDS::Duration_t timeout = { 30, 0 };
  // std::cout << "waiting for acks" << std::endl;
  stat = writer->wait_for_acknowledgments(timeout);
  if ((stat != DDS::RETCODE_OK) && (stat != DDS::RETCODE_TIMEOUT)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_ack() -")
                      ACE_TEXT(" wait_for_acknowledgments failed!\n")),
                     -1);
  }
  // std::cout << "Acks received" << std::endl;
  return 0;
}

OpenDDS::Model::ReaderSync::ReaderSync(DDS::DataReader_var& reader) :
reader_(reader)
{
}

OpenDDS::Model::ReaderSync::~ReaderSync()
{
  if (wait_unmatch(reader_)) {
    throw std::runtime_error("wait_unmatch failure");
  }
}

int
OpenDDS::Model::ReaderSync::wait_unmatch(DDS::DataReader_var& reader)
{
  DDS::ReturnCode_t stat;
  DDS::StatusCondition_var condition = reader->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);
  DDS::ConditionSeq conditions;
  DDS::SubscriptionMatchedStatus ms = { 0, 0, 0, 0, 0 };
  DDS::Duration_t timeout = { 1, 0 };
  do {
    // std::cout << "sub checking unmatched" << std::endl;
    stat = reader->get_subscription_matched_status(ms);
    if (stat != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((
                  LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: %N:%l: wait_unmatch() -")
                  ACE_TEXT(" get_subscription_matched_status failed!\n")),
                 -1);
    } else if (ms.current_count == 0 && ms.total_count > 0) {
      break;  // unmatched
    }
    // std::cout << "sub match count " << ms.current_count
    //           <<    " total count " << ms.total_count << std::endl;
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
  // std::cout << "sub unmatched" << std::endl;
  return 0;
}

