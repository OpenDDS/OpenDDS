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
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <iostream>

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0)
  , valid_(true)
  , rediscovered_(false)
{}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  ++num_reads_;

  try {
    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(message_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    Messenger::Message message;
    DDS::SampleInfo si;

    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si) ;

    if (status == DDS::RETCODE_OK) {
      std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
      std::cout << "SampleInfo.instance_state = " << OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state) << std::endl;

      if (si.valid_data) {

        if (rediscovered_) {
          rediscovery_data_ = true;
        } else {
          initial_discovery_data_ = true;
        }

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
    ACE_OS::exit(-1);
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
  const DDS::SubscriptionMatchedStatus& status)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
  // For the reverse lease test, the DataReader will rediscover the DataWriter.
  // The DataWriter may have some unacknowledged samples that it will send again.
  // Reset the count to avoid interpretting these as duplicates.
  if (status.current_count_change > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched() - resetting counts\n")));
    counts_.clear();
  }
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

void DataReaderListenerImpl::mark_rediscovered()
{
  rediscovered_ = true;
}

bool DataReaderListenerImpl::is_valid(bool check_lease_recovery, bool expect_unmatch) const
{
  if (check_lease_recovery) {
    return valid_ && initial_discovery_data_ &&
      ( (expect_unmatch && rediscovery_data_) ||
        (!expect_unmatch && *(counts_.begin()) == 0 && *(counts_.rbegin()) == 29));
  }

  CORBA::Long expected = 0;
  Counts::const_iterator count = counts_.begin();
  bool valid_count = true;
  while (count != counts_.end() && expected < 10) {
    if (expected != *count) {
      if (expected < *count) {
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
  else if ((int)counts_.size() < 10) {
    std::cout << "ERROR: received " << counts_.size() << " messages, but expected " << 10 << std::endl;
    valid_count = false;
  }

  return valid_ && valid_count;
}
