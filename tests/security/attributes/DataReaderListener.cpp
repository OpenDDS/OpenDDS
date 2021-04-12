/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderListener.h"

#include "SecurityAttributesMessageTypeSupportImpl.h"

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include <dds/DdsDcpsSubscriptionC.h>
#ifndef DDS_HAS_MINIMUM_BIT
#  include <dds/DdsDcpsCoreTypeSupportC.h>
#endif

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <iostream>
#include <string>

DataReaderListenerImpl::DataReaderListenerImpl(const Args& args, DDS::DataReader* part_reader)
  : args_(args)
  , num_reads_(0)
  , valid_(true)
  , reliable_(is_reliable())
  , part_reader_(part_reader)
  , saw_part_user_data_(false)
{
  std::cout << "Transport is " << (reliable_ ? "" : "UN-") << "RELIABLE" <<  std::endl;
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

bool DataReaderListenerImpl::is_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return gc->instances_[0]->transport_type_ != "udp";
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  try {
#ifdef DDS_HAS_MINIMUM_BIT
    ACE_UNUSED_ARG(reader);
#else
    if (reader == part_reader_) {
      DDS::ParticipantBuiltinTopicDataDataReader_var part_reader_impl =
        DDS::ParticipantBuiltinTopicDataDataReader::_narrow(reader);
      if (!part_reader_impl) {
        std::cerr << "ERROR: Failed to get particpant builtin topic reader\n";
        return;
      }

      DDS::ParticipantBuiltinTopicDataSeq part_data;
      DDS::SampleInfoSeq part_info;
      DDS::ReturnCode_t rc = part_reader_impl->take(part_data, part_info, DDS::LENGTH_UNLIMITED,
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
      if (rc == DDS::RETCODE_OK) {
        for (CORBA::ULong si = 0; si < part_info.length(); si++) {
          if (part_info[si].valid_data) {
            std::string user_data;
            for (CORBA::ULong i = 0; i < part_data[si].user_data.value.length(); ++i) {
              user_data.push_back(part_data[si].user_data.value[i]);
            }
            std::cout << "USER DATA: " << user_data << std::endl;
            const std::string expected =
              args_.expect_blank_part_user_data_ ? "" : part_user_data_string;
            if (user_data == expected) {
              if (num_reads_ > 0 && !saw_part_user_data_) {
                std::cerr <<
                  "Error: first expected participant user data come before any reads\n";
                valid_ = false;
              }
              saw_part_user_data_ = true;
            } else if (args_.expect_part_user_data_) {
              std::cerr << "Error: expected participant user data to be \""
                << expected << "\" but got \"" << user_data << "\"\n";
              valid_ = false;
            }
          }
        }
      } else if (rc != DDS::RETCODE_NO_DATA) {
        std::cerr << "Error: Listener got " << OpenDDS::DCPS::retcode_to_string(rc)
          << " error trying to read part bit\n";
        ACE_OS::exit(1);
      }

      return;
    }
#endif

    ++num_reads_;

    SecurityAttributes::MessageDataReader_var message_dr =
      SecurityAttributes::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(message_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(1);
    }

    SecurityAttributes::Message message;
    DDS::SampleInfo si;

    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si) ;

    if (status == DDS::RETCODE_OK) {
      std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
      std::cout << "SampleInfo.instance_state = " << si.instance_state << std::endl;

      if (si.valid_data) {
        if (!counts_.insert(message.count).second) {
          std::cout << "ERROR: Repeat ";
          valid_ = false;
        }

        std::cout << "Message: subject    = " << message.subject.in() << std::endl
                  << "         subject_id = " << message.subject_id   << std::endl
                  << "         from       = " << message.from.in()    << std::endl
                  << "         count      = " << message.count        << std::endl
                  << "         text       = " << message.text.in()    << std::endl;

        if (std::string("Comic Book Guy") != message.from.in() &&
            std::string("OpenDDS-Java") != message.from.in()) {
          std::cout << "ERROR: Invalid message.from" << std::endl;
          valid_ = false;
        }
        if (std::string("Review") != message.subject.in()) {
          std::cout << "ERROR: Invalid message.subject" << std::endl;
          valid_ = false;
        }
        if (std::string("Worst. Movie. Ever.") != message.text.in()) {
          std::cout << "ERROR: Invalid message.text" << std::endl;
          valid_ = false;
        }
        static CORBA::Long previous_subject_id = 100;
        if (message.subject_id != previous_subject_id - 1) {
          std::cout << "ERROR: Invalid message.subject_id" << std::endl;
          valid_ = false;
        }
        previous_subject_id = message.subject_id;
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

    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: unexpected status: %d\n"),
                 status));
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
  const DDS::LivelinessChangedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
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

bool DataReaderListenerImpl::is_valid() const
{
  CORBA::Long expected = 0;
  Counts::const_iterator count = counts_.begin();
  bool valid_count = true;
  while (count != counts_.end() && expected < args_.num_messages_) {
    if (expected != *count) {
      if (expected < *count) {
        if (reliable_) {
          // if missing multiple
          const bool multi = (expected + 1 < *count);
          std::cout << "ERROR: missing message" << (multi ? "s" : "")
                    << " with count=" << expected;
          if (multi) {
            std::cout << " to count=" << (*count - 1);
          }
          std::cout << std::endl;
          expected = *count;
          // don't increment count;
          valid_count = false;
          continue;
        }
      }
      else {
        bool multi = false;
        while (++count != counts_.end() && *count < expected) {
          multi = true;
        }
        std::cout << "ERROR: received message" << (multi ? "s" : "")
                  << " with a negative count" << std::endl;
        valid_count = false;
        continue;
      }
    }

    ++expected;
    ++count;
  }

  if (count != counts_.end()) {
    std::cout << "ERROR: received messages with count higher than expected values" << std::endl;
    valid_count = false;
  }
  // if didn't receive all the messages (for reliable transport) or didn't receive even get 1/4, then report error
  else if ((int)counts_.size() < args_.num_messages_ &&
           (reliable_ || (int)(counts_.size() * 4) < args_.num_messages_)) {
    std::cout << "ERROR: received " << counts_.size() << " messages, but expected " << args_.num_messages_ << std::endl;
    valid_count = false;
  }

  if (args_.expect_part_user_data_ && !saw_part_user_data_) {
    std::cerr << "ERROR: Expected to get participant user data, but didn't" << std::endl;
    return false;
  }

  return valid_ && valid_count;
}
