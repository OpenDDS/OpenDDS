/*
 * $Id$
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

DataReaderListenerImpl::DataReaderListenerImpl(const Options& options,
                                               const std::string& process,
                                               unsigned int participant,
                                               unsigned int writer)
  : options_(options)
{
  std::stringstream ss;
  ss << process << "->" << participant << "->" << writer;
  id_ = ss.str();
  std::cerr << "Starting DataReaderListenerImpl for " << id_ << std::endl;
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
          ss << "Message: from writer " << message.subject.in()
             << "->" << message.participant_id
             << "->" << message.subject_id
             << " count = " << message.count
             << " for reader=" << id_
             << std::endl;
          std::cerr << ss.str();
          // also track it in the log file
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("%T %N:%l: Message: subject = %C ")
                     ACE_TEXT("participant_id = %d ")
                     ACE_TEXT("subject_id = %d ")
                     ACE_TEXT("count = %d ")
                     ACE_TEXT("for reader = %C\n"),
                     message.subject.in(),
                     message.participant_id,
                     message.subject_id,
                     message.count,
                     id_.c_str()));

          for (CORBA::ULong i = 0; i < message.data.length(); ++i) {
            if (message.data[i] != i % 256) {
              std::cout << "ERROR: Bad data at index " << i << " subjid "
                        << message.subject_id << " count " << message.count
                        << std::endl;
              break;
            }
          }
          std::string subject(message.subject.in());
          processes_[subject][message.participant_id][message.subject_id].insert(message.count);
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
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Processes %d %d\n"), processes_.size(), options_.num_pub_processes));
  if (processes_.size() < options_.num_pub_processes) {
    if (report)
      std::cout << "ERROR: only received samples from "
                << processes_.size() << " out of "
                << options_.num_pub_processes << " processes for reader "
                << id_ << "." << std::endl;
    return false;
  }

  for (ProcessParticipants::const_iterator process = processes_.begin();
       process != processes_.end();
       ++process) {
    if (process->second.size() < options_.num_pub_participants) {
      if (report)
        std::cout << "ERROR: only received samples from " << process->second.size()
                  << " out of " << options_.num_pub_participants
                  << " participants for " << process->first << " for reader "
                  << id_ << std::endl;

      return false;
    }
    for (ParticipantWriters::const_iterator participant = process->second.begin();
         participant != process->second.end();
         ++participant) {
      if (participant->second.size() < options_.num_writers) {
        if (report)
          std::cout << "ERROR: only received samples from " << participant->second.size()
                    << " out of " << options_.num_writers
                    << " writers for " << process->first
                    << "->" << participant->first << " for reader "
                    << id_ << std::endl;

        return false;
      }
      for (WriterCounts::const_iterator writer = participant->second.begin();
           writer != participant->second.end();
           ++writer) {
        if (writer->second.size() < options_.num_samples) {
          if (report) {
            if (options_.reliable)
              std::cout << "ERROR: ";

            std::cout << "only received " << writer->second.size()
                      << " out of " << options_.num_samples
                      << " samples for " << process->first
                      << "->" << participant->first
                      << "->" << writer->first << " for reader "
                      << id_ << std::endl;
          }
          return false;
        }
      }
    }
  }

  return true;
}
