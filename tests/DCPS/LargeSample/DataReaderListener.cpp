/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderListener.h"

#include "common.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Service_Participant.h>

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <iostream>

DataReaderListenerImpl::DataReaderListenerImpl(
  size_t writer_process_count, size_t writers_per_process, size_t samples_per_writer,
  unsigned data_field_length_offset)
  : num_samples_(0)
  , valid_(true)
  , writer_process_count_(writer_process_count)
  , writers_per_process_(writers_per_process)
  , samples_per_writer_(samples_per_writer)
  , data_field_length_offset_(data_field_length_offset)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  try {
    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(message_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(1);
    }

    Messenger::MessageSeq messages;
    DDS::SampleInfoSeq info;

    DDS::ReturnCode_t error = message_dr->take(messages,
                                               info,
                                               DDS::LENGTH_UNLIMITED,
                                               DDS::ANY_SAMPLE_STATE,
                                               DDS::ANY_VIEW_STATE,
                                               DDS::ANY_INSTANCE_STATE);

    if (error == DDS::RETCODE_OK) {

      for (unsigned int i = 0; i < messages.length(); ++i) {
        const DDS::SampleInfo& si = info[i];
        if (si.valid_data) {
          const Messenger::Message& message = messages[i];
          // only sample_id unique entries
          if (process_writers_[message.process_id][message.writer_id].insert(message.sample_id).second) {
            ++num_samples_;
          } else {
            std::cout << "ERROR: duplicate ";
            valid_ = false;
          }

          // output for console to consume
          std::cout << "Received message: process_id = " << message.process_id
                    << " writer_id = " << message.writer_id
                    << " sample_id = " << message.sample_id
                    << " data.length = " << message.data.length()
                    << std::endl;
          // also track it in the log file
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("%N:%l: Received message: process_id = %u ")
                     ACE_TEXT("writer_id = %d ")
                     ACE_TEXT("sample_id = %d data.length = %u\n"),
                     message.process_id,
                     message.writer_id,
                     message.sample_id,
                     message.data.length()));

          if (message.sample_id < processToWriterSamples_[message.process_id][message.writer_id]) {
            std::cout << "ERROR: Out of order message from process_id " << message.process_id << " writer_id "
                      << message.writer_id << " sample_id " << message.sample_id << " already received sample with id: "
                      << processToWriterSamples_[message.process_id][message.writer_id]
                      << " (" << message.from.in() << ")\n";
            valid_ = false;
          } else {
            processToWriterSamples_[message.process_id][message.writer_id] = message.sample_id;
          }

          const unsigned int data_size = expected_data_field_length(
            data_field_length_offset_, message.writer_id, message.sample_id);
          if (message.data.length() != data_size) {
            std::cout << "ERROR: Expected message.data to have a size of " << data_size
                      << " but it is " << message.data.length() << "\n";
            valid_ = false;
          }
          for (CORBA::ULong j = 0; j < message.data.length(); ++j) {
            if (message.data[j] != expected_data_field_element(message.writer_id, message.sample_id, j)) {
              std::cout << "ERROR: Bad data at index " << j << " writer_id "
                        << message.writer_id << " sample_id " << message.sample_id
                        << "\n";
              valid_ = false;
              break;
            }
          }
        } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));

        } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));

        } else {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: on_data_available()")
                     ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                     si.instance_state));
          valid_ = false;
        }
      }
    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: unexpected status: %d\n"),
                 error));
      valid_ = false;
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    ACE_OS::exit(1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus& status)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed() alive=%d, not alive=%d\n"),
             status.alive_count, status.not_alive_count));
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
}

void DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}

bool DataReaderListenerImpl::data_consistent() const
{
  bool valid_and_done = valid_;
  if (process_writers_.size() != writer_process_count_) {
    std::cout << "ERROR: expect to receive data from " << writer_process_count_
              << " processes but instead received from "
              << process_writers_.size() << std::endl;
    valid_and_done = false;
  }
  for (ProcessWriters::const_iterator process = process_writers_.begin();
       process != process_writers_.end();
       ++process) {
    if (process->second.size() != writers_per_process_) {
      std::cout << "ERROR: expect to receive from " << writers_per_process_
                << " writers from process " << process->first
                << " but instead received from "
                << process->second.size() << std::endl;
      valid_and_done = false;
    }
    for (WriterCounts::const_iterator writer = process->second.begin();
         writer != process->second.end();
         ++writer) {
      if (writer->second.size() != samples_per_writer_) {
        std::cout << "ERROR: Expected to receive " << samples_per_writer_
                  << " samples from process=" << process->first
                  << " writer=" << writer->first
                  << " but instead received "
                  << writer->second.size() << std::endl;
        valid_and_done = false;
        if (writer->second.size() < samples_per_writer_) {
          for (size_t sample = 0; sample < samples_per_writer_; ++sample) {
            std::cerr
              << (writer->second.count(static_cast<int>(sample)) ? "           Got" : "ERROR: Missing")
              << " process " << process->first
              << " writer " << writer->first
              << " sample " << sample
              << " expected data length " << expected_data_field_length(
                data_field_length_offset_, static_cast<int>(sample), writer->first)
              << std::endl;
          }
        }
      }
    }
  }
  return valid_and_done;
}
