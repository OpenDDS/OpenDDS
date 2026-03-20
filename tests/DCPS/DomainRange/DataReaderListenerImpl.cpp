/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/DataReaderImpl_T.h"

#include "DataReaderListenerImpl.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <iostream>

DataReaderListenerImpl::~DataReaderListenerImpl()
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C is done\n", id_.c_str()));
  if (expected_samples_ && received_samples_ != expected_samples_) {
    ACE_ERROR((LM_ERROR, "ERROR: DataReader %C expected %d but received %d\n",
               id_.c_str(), expected_samples_, received_samples_.value()));
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
  Messenger::MessageDataReader_var reader_i =
    Messenger::MessageDataReader::_narrow(reader);

  if (!reader_i) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(EXIT_FAILURE);
  }

  OpenDDS::DCPS::DataReaderImpl_T<Messenger::Message>* ptr = dynamic_cast<OpenDDS::DCPS::DataReaderImpl_T<Messenger::Message>*>(reader_i.ptr());

  if (!ptr) throw ptr;

  Messenger::Message message;
  DDS::SampleInfo info;

  while (reader_i->take_next_sample(message, info) == DDS::RETCODE_OK) {
    if (info.valid_data) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Domain: %d. %C - %C - received message %d from %C.\n", domain_, id_.c_str(), OpenDDS::DCPS::LogGuid(ptr->subscription_id()).c_str(), message.count, OPENDDS_STRING(message.from).c_str()));

      if (++received_samples_ == expected_samples_) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C has received expected number of samples\n", id_.c_str()));
        done_callback_();
      }
    }
  }
}

void
DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr reader,
  const DDS::SubscriptionMatchedStatus& status)
{
  OpenDDS::DCPS::DataReaderImpl_T<Messenger::Message>* ptr = dynamic_cast<OpenDDS::DCPS::DataReaderImpl_T<Messenger::Message>*>(reader);
  if (!ptr) throw ptr;
  if (status.current_count != 0) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Domain: %d. %C - %C - associated with writer.\n", domain_, id_.c_str(), OpenDDS::DCPS::LogGuid(ptr->subscription_id()).c_str()));
  } else {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Domain: %d. %C - %C - disassociated with writer.\n", domain_, id_.c_str(), OpenDDS::DCPS::LogGuid(ptr->subscription_id()).c_str()));
  }
}

void
DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleLostStatus& /*status*/)
{
}
