// -*- C++ -*-
//
#include "DataReaderListener.h"

#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/ExceptionStreams.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <ace/streams.h>

#include <iostream>

using namespace std;

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0),
    publication_handle_(::DDS::HANDLE_NIL),
    post_restart_publication_handle_(::DDS::HANDLE_NIL)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  ++num_reads_;

  try {
    ::Messenger::MessageDataReader_var message_dr = ::Messenger::MessageDataReader::_narrow(reader);
    if (CORBA::is_nil(message_dr.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) DataReaderListener: read: _narrow failed.\n")));
      exit(1);
    }

    Messenger::Message message;
    DDS::SampleInfo si;
    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);

    if (status == DDS::RETCODE_OK) {
      if (si.valid_data) {
        if (si.publication_handle == ::DDS::HANDLE_NIL ||
            (si.publication_handle != this->publication_handle_ &&
             si.publication_handle != this->post_restart_publication_handle_)) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) DataReaderListener: ERROR: publication_handle validate failed.\n")));
          exit(1);
        }

        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListener: Message count = %i\n"), message.count));

      } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is disposed\n")));
      } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is unregistered\n")));
      }
    } else if (status == DDS::RETCODE_NO_DATA) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) DataReaderListener: ERROR: reader received DDS::RETCODE_NO_DATA!\n")));
    } else {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) DataReaderListener: ERROR: read Message: Error: %C\n"),
        OpenDDS::DCPS::retcode_to_string(status)));
    }
  } catch (CORBA::Exception& e) {
    e._tao_print_exception("DataReaderListener: Exception caught in read:", stderr);
    exit(1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus&)
{
  ACE_ERROR((LM_ERROR, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus&)
{
  ACE_ERROR((LM_ERROR, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n")));
}

void DataReaderListenerImpl::on_subscription_matched(DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus& status)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_subscription_matched handle=%i\n"),
    status.last_publication_handle));

  if (this->publication_handle_ == ::DDS::HANDLE_NIL) {
    this->publication_handle_ = status.last_publication_handle;
  } else {
    this->post_restart_publication_handle_ = status.last_publication_handle;
  }
}

bool DataReaderListenerImpl::read_bit_instance()
{
  DDS::PublicationBuiltinTopicDataDataReader_var rdr =
    DDS::PublicationBuiltinTopicDataDataReader::_narrow(builtin_);
  for (int i = 0; i < 100; ++i) {
    // BIT Data is not necessarily ready when this callback happens
    DDS::InstanceHandle_t handle;
    if (post_restart_publication_handle_ != ::DDS::HANDLE_NIL) {
      handle = post_restart_publication_handle_;
    } else if (publication_handle_ != ::DDS::HANDLE_NIL) {
      handle = publication_handle_;
    } else {
      ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) Can't read bit instance, pre and post restart handles are invalid.\n")));
      return false;
    }
    DDS::PublicationBuiltinTopicDataSeq data;
    DDS::SampleInfoSeq infos;
    DDS::ReturnCode_t ret = rdr->read_instance(data, infos, 1, handle,
                                               DDS::NOT_READ_SAMPLE_STATE,
                                               DDS::ANY_VIEW_STATE,
                                               DDS::ALIVE_INSTANCE_STATE);

    switch (ret) {
    case DDS::RETCODE_OK:
      ACE_DEBUG((LM_DEBUG, ACE_TEXT(
        "(%P|%t) read bit instance returned ok\n")));
      return true;
    case DDS::RETCODE_NO_DATA:
      ACE_ERROR((LM_WARNING, ACE_TEXT(
        "(%P|%t) read bit instance returned no data\n")));
      break;
    case DDS::RETCODE_BAD_PARAMETER:
      ACE_ERROR((LM_ERROR, ACE_TEXT(
        "(%P|%t) read bit instance returned bad parameter\n")));
      break;
    default:
      ACE_ERROR((LM_ERROR, ACE_TEXT(
        "(%P|%t) ERROR read bit instance returned %C\n"),
        OpenDDS::DCPS::retcode_to_string(ret)));
      return false;
    }
    ACE_OS::sleep(ACE_Time_Value(0, 100000));
  }

  ACE_ERROR((LM_ERROR, ACE_TEXT(
    "(%P|%t) ERROR read bit instance: giving up after retries\n")));
  return false;
}

void DataReaderListenerImpl::on_sample_rejected(DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
  ACE_ERROR((LM_ERROR, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_sample_rejected\n")));
}

void DataReaderListenerImpl::on_sample_lost(DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  ACE_ERROR((LM_ERROR, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_sample_lost\n")));
}

void DataReaderListenerImpl::on_subscription_disconnected(DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus&)
{
  ACE_ERROR((LM_WARNING, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_subscription_disconnected\n")));
}

void DataReaderListenerImpl::on_subscription_reconnected(DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus&)
{
  ACE_ERROR((LM_WARNING, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_subscription_reconnected\n")));
}

void DataReaderListenerImpl::on_subscription_lost(DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionLostStatus&)
{
  ACE_ERROR((LM_WARNING, ACE_TEXT(
    "(%P|%t) DataReaderListenerImpl::on_subscription_lost\n")));
}

void DataReaderListenerImpl::set_builtin_datareader(DDS::DataReader_ptr builtin)
{
  builtin_ = DDS::DataReader::_duplicate(builtin);
}
