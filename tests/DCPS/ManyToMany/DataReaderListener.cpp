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

#include "DataReaderListener.h"
#include "tests/DCPS/LargeSample/MessengerTypeSupportC.h"
#include "tests/DCPS/LargeSample/MessengerTypeSupportImpl.h"

#include <iostream>
#include <sstream>

namespace
{
  unsigned int calc_num_samples(const Options& options)
  {
    if (options.no_validation)
      return options.num_pub_processes * options.num_pub_participants *
        options.num_writers * options.num_samples;
    else
      return 0;
  }
}

DataReaderListenerImpl::DataReaderListenerImpl(const Options& options,
                                               const std::string& process,
                                               unsigned int participant,
                                               unsigned int writer)
  : options_(options)
  , expected_num_samples_(calc_num_samples(options))
  , num_samples_(0)
{
  std::stringstream ss;
  ss << process << "->" << participant << "->" << writer;
  id_ = ss.str();
  std::cout << "Starting DataReaderListenerImpl for " << id_ << std::endl;
  std::cout << "Readers/Writers identified as (process_id)->(participant_id)->(writer_id)" << std::endl;
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
                 ACE_TEXT("%T %N:%l: on_data_available()")
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

          // output for console to consume
          std::stringstream ss;
          ss << "Message: from writer " << message.process_id.in()
             << "->" << message.participant_id
             << "->" << message.writer_id
             << " sample_id = " << message.sample_id
             << " for reader=" << id_
             << std::endl;
          std::cerr << ss.str();
          // also track it in the log file
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("%T %N:%l: Message: process_id = %C ")
                     ACE_TEXT("participant_id = %d ")
                     ACE_TEXT("writer_id = %d ")
                     ACE_TEXT("sample_id = %d ")
                     ACE_TEXT("for reader = %C\n"),
                     message.process_id.in(),
                     message.participant_id,
                     message.writer_id,
                     message.sample_id,
                     id_.c_str()));

          for (CORBA::ULong i = 0; i < message.data.length(); ++i) {
            if (message.data[i] != i % 256) {
              std::cout << "ERROR: Bad data at index " << i << " writer_id "
                        << message.writer_id << " sample_id " << message.sample_id
                        << std::endl;
              break;
            }
          }
          if (!options_.no_validation) {
            std::string process_id(message.process_id.in());
            processes_[process_id][message.participant_id][message.writer_id].insert(message.sample_id);
          }

          ++num_samples_;

        } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: instance is disposed\n")));

        } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: instance is unregistered\n")));

        } else {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%T %N:%l: on_data_available()")
                     ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                     si.instance_state));
        }
      }
    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%T %N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: unexpected status: %d\n"),
                 error));
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
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: on_requested_deadline_missed()\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus& status)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: on_liveliness_changed() alive=%d, not alive=%d\n"),
             status.alive_count, status.not_alive_count));
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: on_subscription_matched()\n")));
}

void DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: on_sample_rejected()\n")));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T %N:%l: INFO: on_sample_lost()\n")));
}

bool DataReaderListenerImpl::done() const
{
  return done(false);
}

void DataReaderListenerImpl::report_errors() const
{
  done(true);
}

bool DataReaderListenerImpl::done(bool report) const
{
  bool valid_and_done = true;
  if (expected_num_samples_ > 0) {
    const bool complete = num_samples_ >= expected_num_samples_;
    if (report && (!complete || num_samples_ > expected_num_samples_)) {
      std::cout << "ERROR: only received " << num_samples_
                << " out of " << expected_num_samples_ << " samples for reader "
                << id_ << "." << std::endl;
    }
    return complete;
  }

  if (processes_.size() != options_.num_pub_processes) {
    if (report)
      std::cout << "ERROR: received samples from "
                << processes_.size() << " processes but expected to receive "
                << options_.num_pub_processes << " for reader "
                << id_ << "." << std::endl;
    valid_and_done = false;
  }

  for (ProcessParticipants::const_iterator process = processes_.begin();
       process != processes_.end();
       ++process) {
    if (process->second.size() != options_.num_pub_participants) {
      if (report)
        std::cout << "ERROR: received samples from " << process->second.size()
                  << " participants but expected to receive " << options_.num_pub_participants
                  << " for " << process->first << " for reader "
                  << id_ << std::endl;

      valid_and_done = false;
    }
    for (ParticipantWriters::const_iterator participant = process->second.begin();
         participant != process->second.end();
         ++participant) {
      if (participant->second.size() < options_.num_writers) {
        if (report)
          std::cout << "ERROR: received samples from " << participant->second.size()
                    << " writers but expected to receive " << options_.num_writers
                    << " for " << process->first
                    << "->" << participant->first << " for reader "
                    << id_ << std::endl;

        valid_and_done = false;
      }
      for (WriterCounts::const_iterator writer = participant->second.begin();
           writer != participant->second.end();
           ++writer) {
        if (writer->second.size() != options_.num_samples) {
          if (report) {
            if (options_.reliable || writer->second.size() > options_.num_samples)
              std::cout << "ERROR: ";

            std::cout << "received " << writer->second.size()
                      << " samples but expected to receive " << options_.num_samples
                      << " for " << process->first
                      << "->" << participant->first
                      << "->" << writer->first << " for reader "
                      << id_ << std::endl;
          }
          valid_and_done = false;
        }
      }
    }
  }

  return valid_and_done;
}
