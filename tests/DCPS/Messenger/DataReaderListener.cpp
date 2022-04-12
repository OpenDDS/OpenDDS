/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Args.h"
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>

#include <dds/DdsDcpsSubscriptionC.h>

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <iostream>
#include <cstdlib>

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0)
  , valid_(true)
  , reliable_(is_reliable())
  , gc_()
{
  std::cout << "Transport is " << (reliable_ ? "" : "UN-") << "RELIABLE" <<  std::endl;
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

bool DataReaderListenerImpl::is_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return gc->instances_[0]->transport_type_ != "udp" &&
         !(gc->instances_[0]->transport_type_ == "multicast" && !gc->instances_[0]->is_reliable());
}

void DataReaderListenerImpl::set_expected_reads(size_t expected)
{
  expected_reads_ = expected;
}

void DataReaderListenerImpl::set_guard_condition(DDS::GuardCondition_var gc)
{
  gc_ = gc;
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  ++num_reads_;

  try {
    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);
    if (!message_dr) {
      if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataReaderListenerImpl::on_data_available: _narrow failed!\n"));
      }
      ACE_OS::exit(EXIT_FAILURE);
    }

    Messenger::Message message;
    DDS::SampleInfo si;

    const DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);

    if (status == DDS::RETCODE_OK) {
      std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
      std::cout << "SampleInfo.instance_state = " <<
        OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state) << std::endl;

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
        if (message.subject_id != 99) {
          std::cout << "ERROR: Invalid message.subject_id" << std::endl;
          valid_ = false;
        }
      } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Debug) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: DataReaderListenerImpl::on_data_available: "
                     "instance is disposed\n")));
        }
      } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
        if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Debug) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: DataReaderListenerImpl::on_data_available: "
                    "instance is unregistered\n")));
        }
      } else {
        if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataReaderListenerImpl::on_data_available: "
                     "unknown instance state: %d\n", si.instance_state));
        }
        valid_ = false;
      }

    } else {
      if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DataReaderListenerImpl::on_data_available: "
                   "unexpected status: %d\n", status));
      }
      valid_ = false;
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    ACE_OS::exit(EXIT_FAILURE);
  }
  if (counts_.size() == expected_reads_) {
    gc_->set_trigger_value(true);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
{
  if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: DataReaderListenerImpl::on_requested_deadline_missed\n")));
  }
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
{
  if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: DataReaderListenerImpl::on_requested_incompatible_qos()\n")));
  }
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus &)
{
  if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: DataReaderListenerImpl::on_liveliness_changed()\n")));
  }
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
{
  if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: DataReaderListenerImpl::on_subscription_matched()\n")));
  }
}

void DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
  if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: DataReaderListenerImpl::on_sample_rejected()\n")));
  }
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Debug) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DEBUG: DataReaderListenerImpl::on_sample_lost()\n")));
  }
}

bool DataReaderListenerImpl::is_valid() const
{
  CORBA::ULong expected = 0;
  Counts::const_iterator count = counts_.begin();
  bool valid_count = true;
  while (count != counts_.end() && expected < expected_reads_) {
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
      } else {
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

  if (reliable_ && count != counts_.end()) {
    std::cout << "ERROR: received messages with count higher than expected values" << std::endl;
    valid_count = false;
  } else if (counts_.size() < expected_reads_) {
    std::cout << "ERROR: received " << counts_.size() << " messages, but expected " <<
      expected_reads_ << std::endl;
    valid_count = false;
  }

  return valid_ && valid_count;
}
