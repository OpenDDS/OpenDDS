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
  : reader_(reader), received_(0)
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
    switch (++received_) {
      case 1: read(msg_dr); break;
      case 2: read_next_sample(msg_dr); break;
      case 3: take(msg_dr); break;
      default: take_next_sample(msg_dr); break;
    }
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    throw;
  }
}

void DataReaderListenerImpl::read(Messenger::MessageDataReader_var mdr)
{
  Messenger::MessageSeq msgs;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t r = mdr->read(msgs, infos, DDS::LENGTH_UNLIMITED,
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (r != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: read() ERROR: %d\n"), r));
  }
}

void DataReaderListenerImpl::take(Messenger::MessageDataReader_var mdr)
{
  Messenger::MessageSeq msgs;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t r = mdr->take(msgs, infos, DDS::LENGTH_UNLIMITED,
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (r != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: take() ERROR: %d\n"), r));
  }
}

void DataReaderListenerImpl::read_next_sample(Messenger::MessageDataReader_var mdr)
{
  Messenger::Message m;
  DDS::SampleInfo i;
  DDS::ReturnCode_t r = mdr->read_next_sample(m, i) ;
  if (r == DDS::RETCODE_OK) {
    if (i.valid_data) {
      std::cout << reader_ << ": read_next_sample " << m.subject << m.subject_id << ':' << m.count << ':' << m.text << std::endl;
    } else if (i.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));
    } else if (i.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: read_next_sample() ERROR: %d\n"), i.instance_state));
    }
  } else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: read_next_sample() ERROR: r: %d\n"), r));
  }
}

void DataReaderListenerImpl::take_next_sample(Messenger::MessageDataReader_var mdr)
{
  Messenger::Message m;
  DDS::SampleInfo i;
  DDS::ReturnCode_t r = mdr->take_next_sample(m, i) ;
  if (r == DDS::RETCODE_OK) {
    if (i.valid_data) {
      std::cout << reader_ << ": take_next_sample " << m.subject << m.subject_id << ':' << m.count << ':' << m.text << std::endl;
    } else if (i.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));
    } else if (i.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: take_next_sample() ERROR: %d\n"), i.instance_state));
    }
  } else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: take_next_sample() ERROR: r: %d\n"), r));
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
