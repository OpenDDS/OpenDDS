// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include "dds/DCPS/BuiltInTopicUtils.h"
#include <ace/streams.h>
#include <iostream>
#include "tests/Utils/ExceptionStreams.h"
#include <dds/DdsDcpsCoreTypeSupportImpl.h>

using namespace std;


DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0),
    publication_handle_ (::DDS::HANDLE_NIL),
    post_restart_publication_handle_ (::DDS::HANDLE_NIL),
    builtin_read_error_(false)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  ++num_reads_;

  try {
    ::Messenger::MessageDataReader_var message_dr = ::Messenger::MessageDataReader::_narrow(reader);
    if (CORBA::is_nil (message_dr.in ())) {
      cerr << "DataReaderListener: read: _narrow failed." << endl;
      exit(1);
    }

    Messenger::Message message;
    DDS::SampleInfo si ;
    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si) ;

    if (status == DDS::RETCODE_OK) {

      if (si.valid_data)
      {
        if (si.publication_handle == ::DDS::HANDLE_NIL
          || (si.publication_handle != this->publication_handle_
            && si.publication_handle != this->post_restart_publication_handle_))
        {
          cerr << "DataReaderListener: ERROR: publication_handle validate failed." << endl;
          exit(1);
        }

        cout << "DataReaderListener:" << endl
            << "   Message: subject    = " << message.subject.in() << endl
            << "            subject_id = " << message.subject_id   << endl
            << "            from       = " << message.from.in()    << endl
            << "            count      = " << message.count        << endl
            << "            text       = " << message.text.in()    << endl;
        cout << "   SampleInfo.sample_rank = " << si.sample_rank << endl;
      }
      else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is disposed\n")));
      }
      else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is unregistered\n")));
      }
    } else if (status == DDS::RETCODE_NO_DATA) {
      cerr << "DataReaderListener: ERROR: reader received DDS::RETCODE_NO_DATA!" << endl;
    } else {
      cerr << "DataReaderListener: ERROR: read Message: Error: " <<  status << endl;
    }
  } catch (CORBA::Exception& e) {
    cerr << "DataReaderListener: Exception caught in read:" << endl << e << endl;
    exit(1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  cerr << "DataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  cerr << "DataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  cerr << "DataReaderListenerImpl::on_liveliness_changed" << endl;
}

void DataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus & status)
{
  if (this->publication_handle_ == ::DDS::HANDLE_NIL) {
    this->publication_handle_ = status.last_publication_handle;
    cerr << "DataReaderListenerImpl::on_subscription_matched handle="
      << publication_handle_ << endl;
  }
  else {
    this->post_restart_publication_handle_ = status.last_publication_handle;
    cerr << "DataReaderListenerImpl::on_subscription_matched handle="
      << post_restart_publication_handle_ << endl;
  }

  DDS::PublicationBuiltinTopicDataDataReader_var rdr =
    DDS::PublicationBuiltinTopicDataDataReader::_narrow(builtin_);
  DDS::PublicationBuiltinTopicDataSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = rdr->read_instance(data, infos, 1,
                                    status.last_publication_handle,
                                    DDS::NOT_READ_SAMPLE_STATE,
                                    DDS::ANY_VIEW_STATE,
                                    DDS::ALIVE_INSTANCE_STATE);

  switch (ret)
  {
  case ::DDS::RETCODE_OK:
    cerr << "read bit instance returned ok" << endl;
    break;
  case ::DDS::RETCODE_NO_DATA:
    cerr << "read bit instance returned no data" << endl;
    break;
  case ::DDS::RETCODE_BAD_PARAMETER:
    cerr << "ERROR read bit instance returned bad parameter" << endl;
    builtin_read_error_ = true;
    break;
  default:
    cerr << "read bit instance returned " << ret << endl;
    builtin_read_error_ = true;
  };
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  cerr << "DataReaderListenerImpl::on_sample_rejected" << endl;
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  cerr << "DataReaderListenerImpl::on_sample_lost" << endl;
}

void DataReaderListenerImpl::on_subscription_disconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_lost (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionLostStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_lost" << endl;
}

void DataReaderListenerImpl::on_connection_deleted (
  DDS::DataReader_ptr)
{
  cerr << "DataReaderListenerImpl::on_connection_deleted" << endl;
}

void DataReaderListenerImpl::set_builtin_datareader (
  DDS::DataReader_ptr builtin)
{
  builtin_ = DDS::DataReader::_duplicate(builtin);
}
