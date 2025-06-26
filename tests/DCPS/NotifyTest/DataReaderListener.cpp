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

DataReaderListenerImpl::DataReaderListenerImpl(DistributedConditionSet_rch dcs,
                                               int expected)
  : dcs_(dcs)
  , expected_(expected)
  , num_reads_(0)
  , num_received_dispose_(0)
  , num_received_unregister_(0)
  , shutdown_ (false)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  try {
    ::Messenger::MessageDataReader_var message_dr =
        ::Messenger::MessageDataReader::_narrow(reader);
    if (CORBA::is_nil (message_dr.in ())) {
      cerr << "read: _narrow failed." << endl;
      exit(1);
    }

    Messenger::Message message;
    DDS::SampleInfo si ;
    for (DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);
         status == DDS::RETCODE_OK;
         status = message_dr->take_next_sample(message, si)) {
      cout << "SampleInfo.sample_rank = " << si.sample_rank << endl;
      cout << "SampleInfo.instance_state = " << OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state) << endl;
      cout << "SampleInfo.valid_data = " << si.valid_data << endl;

      if (si.valid_data == 1)
      {
        cout << "Message: subject    = " << message.subject.in() << endl
          << "         subject_id = " << message.subject_id   << endl
          << "         from       = " << message.from.in()    << endl
          << "         count      = " << message.count        << endl
          << "         text       = " << message.text.in()    << endl;

        if (! shutdown_) {
          ++num_reads_;
          cout << "progress " << num_reads_ << '/' << expected_ << endl;
          if (num_reads_ == expected_) {
            dcs_->post("sub", "data");
          }
        }
      }
      else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
      {
        cout << "instance is disposed" << endl;
        if (! shutdown_) {
          ++ num_received_dispose_;
          dcs_->post("sub", "dispose");
        }
      }
      else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
      {
        cout << "instance is unregistered" << endl;
        if (! shutdown_) {
          ++ num_received_unregister_;
          dcs_->post("sub", "unregister");
        }
      }
      else
      {
      ACE_ERROR ((LM_ERROR, "(%P|%t) DataReaderListenerImpl::on_data_available:"
                             " received unknown instance state %d\n", si.instance_state));
      }
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
