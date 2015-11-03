/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Service_Participant.h>

#include "Writer.h"
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <iostream>

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_samples_(0)
  , valid_(true)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
throw(CORBA::SystemException)
{
  try {
    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(message_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
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
          if (process_writers_[message.process_id.in()][message.writer_id].insert(message.sample_id).second) {
            ++num_samples_;
          }
          else {
            std::cout << "ERROR: duplicate ";
            valid_ = false;
          }

          // output for console to consume
          std::cout << "Message: process_id = " << message.process_id.in()
                    << " writer_id = " << message.writer_id
                    << " sample_id = " << message.sample_id << '\n';
          // also track it in the log file
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("%N:%l: Message: process_id = %C ")
                     ACE_TEXT("writer_id = %d ")
                     ACE_TEXT("sample_id = %d\n"),
                     message.process_id.in(),
                     message.writer_id,
                     message.sample_id));

          if (message.sample_id < processToWriterSamples_[message.process_id.in()][message.writer_id]) {
            std::cout << "ERROR: Out of order message from process_id " << message.process_id.in() << " writer_id "
                      << message.writer_id << " sample_id " << message.sample_id << " already received sample with id: "
                      << processToWriterSamples_[std::string(message.process_id)][message.writer_id]
                      << " (" << message.from.in() << ")\n";
            valid_ = false;
          } else {
              processToWriterSamples_[std::string(message.process_id)][message.writer_id] = message.sample_id;
          }
          if ((message.writer_id == 1 && std::string(message.from) != "Comic Book Guy 1") ||
              (message.writer_id == 2 && std::string(message.from) != "Comic Book Guy 2")) {
            std::cout << "ERROR: Bad from for process_id " << message.process_id.in() << " writer_id "
                      << message.writer_id << " sample_id " << message.sample_id
                      << " (" << message.from.in() << ")\n";
            valid_ = false;
          }
          else if (message.writer_id != 1 && message.writer_id != 2) {
            std::cout << "ERROR: Bad writer_id for process_id " << message.process_id.in()
                      << " writer_id " << message.writer_id << " sample_id "
                      << message.sample_id << " (" << message.from.in() << ")\n";
            valid_ = false;
          }

          const unsigned int data_size = Writer::calc_sample_length(
            message.writer_id, message.sample_id
          );
          if (message.data.length() != data_size) {
            std::cout << "ERROR: Expected message.data to have a size of " << data_size
                      << " but it is " << message.data.length() << "\n";
            valid_ = false;
          }
          for (CORBA::ULong j = 0; j < message.data.length(); ++j) {
            if ((message.writer_id == 1 &&
                 (message.data[j] != j % 256)) ||
                (message.writer_id == 2 &&
                 (message.data[j] != 255 - (j % 256)))) {
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
    ACE_OS::exit(-1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus& status)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed() alive=%d, not alive=%d\n"),
             status.alive_count, status.not_alive_count));
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
}

void DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}

bool DataReaderListenerImpl::data_consistent() const
{
  bool valid_and_done = valid_;
  if (process_writers_.size() != NUM_PROCESSES) {
    std::cout << "ERROR: expect to receive data from " << NUM_PROCESSES
              << " processes but instead received from "
              << process_writers_.size() << std::endl;
    valid_and_done = false;
  }
  for (ProcessWriters::const_iterator process = process_writers_.begin();
       process !=  process_writers_.end();
       ++process) {
    if (process->second.size() != NUM_WRITERS_PER_PROCESS) {
      std::cout << "ERROR: expect to receive from " << NUM_WRITERS_PER_PROCESS
                << " writers from process " << process->first
                << " but instead received from "
                << process->second.size() << std::endl;
      valid_and_done = false;
    }
    for (WriterCounts::const_iterator writer = process->second.begin();
         writer != process->second.end();
         ++writer) {
      if (writer->second.size() > NUM_SAMPLES_PER_WRITER) {
        std::cout << "ERROR: expect to receive no more than " << NUM_SAMPLES_PER_WRITER
                  << " samples from process=" << process->first
                  << " writer=" << writer->first
                  << " but instead received "
                  << writer->second.size() << std::endl;
        valid_and_done = false;
      }
    }
  }
  return valid_and_done;
}
