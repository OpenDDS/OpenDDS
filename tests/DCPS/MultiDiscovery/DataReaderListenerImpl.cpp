/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include "dds/DCPS/WaitSet.h"

#include "DataReaderListenerImpl.h"
#include "TestMsgTypeSupportC.h"
#include "TestMsgTypeSupportImpl.h"

#include <iostream>

DataReaderListenerImpl::~DataReaderListenerImpl()
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C is done\n", id_.c_str()));
  if (expected_samples_ && received_samples_ != expected_samples_) {
    ACE_ERROR((LM_ERROR, "ERROR: DataReader %C expected %d but received %d\n",
               id_.c_str(), expected_samples_, received_samples_));
  } else if (expected_samples_) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C Expected number of samples received\n", id_.c_str()));
  }
}

void
DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedDeadlineMissedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedIncompatibleQosStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleRejectedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::LivelinessChangedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  TestMsgDataReader_var reader_i =
    TestMsgDataReader::_narrow(reader);

  if (!reader_i) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  TestMsg message;
  DDS::SampleInfo info;

  DDS::ReturnCode_t error = reader_i->take_next_sample(message, info);

  if (error == DDS::RETCODE_OK) {
    if (info.valid_data) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C has received message: %d from: %C\n", id_.c_str(), message.value, std::string(message.from).c_str()));

      if (!origin_) {
        TestMsgDataWriter_var message_writer =
          TestMsgDataWriter::_narrow(writer_);

        if (!message_writer) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                     ACE_TEXT(" _narrow failed!\n")));
          ACE_OS::exit(-1);
        }

        // Block until Subscriber is available
        DDS::StatusCondition_var condition = writer_->get_statuscondition();
        condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

        DDS::WaitSet_var ws = new DDS::WaitSet;
        ws->attach_condition(condition);

        while (true) {
          DDS::PublicationMatchedStatus matches;
          if (writer_->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                              ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                              ACE_TEXT(" get_publication_matched_status failed!\n")));
            ACE_OS::exit(-1);
          }

          ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C has %d of %d readers\n", writer_id_.c_str(), matches.current_count, total_readers_));
          if (matches.current_count >= total_readers_) {
            break;
          }

          DDS::ConditionSeq conditions;
          DDS::Duration_t timeout = { 60, 0 };
          if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                       ACE_TEXT(" wait failed!\n")));
            ACE_OS::exit(-1);
          }
        }

        ws->detach_condition(condition);
        std::string from_list = std::string(message.from) + "->" + writer_id_;
        message.from = from_list.c_str();
        DDS::ReturnCode_t error;
        do {
          error = message_writer->write(message, DDS::HANDLE_NIL);

          if ((error != DDS::RETCODE_OK) && (error != DDS::RETCODE_TIMEOUT)) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                       ACE_TEXT(" write returned %d!\n"), error));
          }
        } while (error == DDS::RETCODE_TIMEOUT);
      }
      if (++received_samples_ == expected_samples_) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C has received expected number of samples\n", id_.c_str()));
        if (!origin_) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) DataWriter %C is waiting for acknowledgments\n", writer_id_.c_str()));
          DDS::Duration_t timeout = { 30, 0 };
          writer_->wait_for_acknowledgments(timeout);
        }
        done_callback_();
      }
    }

  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" take_next_sample failed!\n")));
  }
}

void
DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SubscriptionMatchedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleLostStatus& /*status*/)
{
}
