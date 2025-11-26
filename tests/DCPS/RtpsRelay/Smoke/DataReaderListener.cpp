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

DataReaderListenerImpl::DataReaderListenerImpl(DistributedConditionSet_rch dcs)
  : dcs_(dcs)
  , valid_(true)
{}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  Messenger::MessageDataReader_var message_dr = Messenger::MessageDataReader::_narrow(reader);
  Messenger::Message message;
  DDS::SampleInfo si;
  DDS::ReturnCode_t status;
  while ((status = message_dr->take_next_sample(message, si)) == DDS::RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "SampleInfo.sample_rank = %d\n", si.sample_rank));
    ACE_DEBUG((LM_DEBUG, "SampleInfo.instance_state = %C\n", OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state)));

    if (si.valid_data) {

      if (dcs_ && !counts_.insert(message.count).second) {
        ACE_DEBUG((LM_ERROR, "ERROR: Repeat\n"));
        valid_ = false;
      }

      ACE_DEBUG((LM_DEBUG, "Message: subject    = %C\n", message.subject.in()));
      ACE_DEBUG((LM_DEBUG, "         subject_id = %d\n", message.subject_id));
      ACE_DEBUG((LM_DEBUG, "         from       = %C\n", message.from.in()));
      ACE_DEBUG((LM_DEBUG, "         count      = %d\n", message.count));
      ACE_DEBUG((LM_DEBUG, "         text       = %C\n", message.text.in()));
      if (dcs_) {
        dcs_->post("Subscriber", "count_" + OpenDDS::DCPS::to_dds_string(message.count));
      }

      if (std::string("Comic Book Guy") != message.from.in() &&
          std::string("OpenDDS-Java") != message.from.in()) {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid message.from\n"));
        valid_ = false;
      }
      if (std::string("Review") != message.subject.in()) {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid message.subject\n"));
        valid_ = false;
      }
      if (dcs_ && std::string("Worst. Movie. Ever.") != message.text.in()) {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid message.text\n"));
        valid_ = false;
      }
      if (dcs_ && message.subject_id != 99) {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid message.subject_id\n"));
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
  if (dcs_) {
    dcs_->post("Subscriber", OpenDDS::DCPS::String("on_subscription_matched")
               + "_" + OpenDDS::DCPS::to_dds_string(status.total_count)
               + "_" + OpenDDS::DCPS::to_dds_string(status.total_count_change)
               + "_" + OpenDDS::DCPS::to_dds_string(status.current_count)
               + "_" + OpenDDS::DCPS::to_dds_string(status.current_count_change));
  }

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

bool DataReaderListenerImpl::is_valid() const
{
  return valid_;
}
