// #include "Args.h"
#include "Domain.h"
#include "DataReaderListenerImpl.h"
#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportC.h>
#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Service_Participant.h>
#include <iostream>
#include <cstdlib>

DataReaderListenerImpl::DataReaderListenerImpl(const std::string& reader)
  : reader_(reader), received_(0), lastSeq_(0), validSeq_(true)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr dr)
{
  try {
    Messenger::MessageDataReader_var msg_dr = Messenger::MessageDataReader::_narrow(dr);
    if (CORBA::is_nil(msg_dr.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: TestMsgDataReader::_narrow failed!\n")));
      ACE_OS::exit(EXIT_FAILURE);
    }

    Messenger::Message msg;
    DDS::SampleInfo si;
    DDS::ReturnCode_t status = msg_dr->take_next_sample(msg, si) ;
    if (status == DDS::RETCODE_OK) {
      if (si.valid_data) {
        ++received_;
        std::cout << reader_ << " received " << msg.subject_id << ':' << msg.count << ':' << msg.text << std::endl;
        if (lastSeq_ < msg.count) {
          lastSeq_ = msg.count;
        } else {
          std::cout << "ERROR: sequence order " << lastSeq_ << ':' << msg.count << std::endl;
          validSeq_ = false;
        }
      } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));
      } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));
      } else {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: on_data_available() ERROR: %d\n"), si.instance_state));
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: on_data_available() ERROR: status: %d\n"), status));
    }
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    throw;
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
}

void DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr, const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr, const DDS::SampleLostStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}

bool DataReaderListenerImpl::valid() const
{
  std::cout << reader_ << " received: " << received_ << "/" << Domain::N_MSG
    << ((received_ > 0) ? "\n" : " <--- ERROR\n");
  return validSeq_ && (received_ > 0);
}
