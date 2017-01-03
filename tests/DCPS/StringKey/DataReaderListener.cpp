// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"

using namespace Messenger;
using namespace std;

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0),
  num_received_dispose_(0),
  num_received_unregister_(0),
  last_hdl_(::DDS::HANDLE_NIL),
  same_instance_(true)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  ++num_reads_;
  try {
    ::Messenger::MessageDataReader_var message_dr =
        ::Messenger::MessageDataReader::_narrow(reader);
    if (CORBA::is_nil (message_dr.in ())) {
      cerr << "read: _narrow failed." << endl;
      exit(1);
    }

    Messenger::Message message;
    DDS::SampleInfo si ;
    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si) ;

    if (last_hdl_ != ::DDS::HANDLE_NIL && last_hdl_ != si.instance_handle)
    {
      same_instance_ = false;
    }

    ::DDS::InstanceHandle_t  previous_handle = this->last_hdl_;
    last_hdl_ = si.instance_handle;

    if (status == DDS::RETCODE_OK) {

      cout << "SampleInfo.sample_rank = " << si.sample_rank << endl;
      cout << "SampleInfo.instance_state = " << si.instance_state << endl;
      cout << "SampleInfo.instance_handle = " << si.instance_handle << endl;
      if( previous_handle != this->last_hdl_) {
        cout << "HANDLE CHANGED: previous handle = " << previous_handle << endl;
      }

      if (si.valid_data == 1)
      {
        cout << "Message: subject    = " << message.subject.in() << endl
          << "         subject_id = " << message.subject_id   << endl
          << "         from       = " << message.from.in()    << endl
          << "         count      = " << message.count        << endl
          << "         text       = " << message.text.in()    << endl;
      }
      else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
      {
        cout << "instance is disposed" << endl;
        ++ num_received_dispose_;
      }
      else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
      {
        cout << "instance is unregistered" << endl;
        ++ num_received_unregister_;
      }
      else
      {
      ACE_ERROR ((LM_ERROR, "(%P|%t) DataReaderListenerImpl::on_data_available:"
                             " received unknown instance state %d\n", si.instance_state));
      }
    } else if (status == DDS::RETCODE_NO_DATA) {
      cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << endl;
    } else {
      cerr << "ERROR: read Message: Error: " <<  status << endl;
    }
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in read:" << endl << e << endl;
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
    const DDS::SubscriptionMatchedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_matched" << endl;
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
