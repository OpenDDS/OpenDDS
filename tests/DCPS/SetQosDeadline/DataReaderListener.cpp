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

DataReaderListenerImpl::DataReaderListenerImpl ()
  : num_reads_(0),
    num_deadline_missed_ (0)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void
DataReaderListenerImpl::on_data_available (DDS::DataReader_ptr reader)
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

    if (status == DDS::RETCODE_OK) {

      cerr << "SampleInfo.sample_rank = " << si.sample_rank << endl;
      cerr << "SampleInfo.instance_state = " << si.instance_state << endl;

      if (si.valid_data == 1)
      {
        cerr << "Message: subject    = " << message.subject.in() << endl
          << "         subject_id = " << message.subject_id   << endl
          << "         from       = " << message.from.in()    << endl
          << "         count      = " << message.count        << endl
          << "         text       = " << message.text.in()    << endl;
      }
      else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
      {
        cerr << "instance is disposed" << endl;
      }
      else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
      {
        cerr << "instance is unregistered" << endl;
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


void
DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr /*reader*/,
    DDS::RequestedDeadlineMissedStatus const & status)
{
  this->num_deadline_missed_ = status.total_count;
  cerr << "DataReaderListenerImpl::on_requested_deadline_missed" << endl
       << "  total_count        = " << status.total_count << endl
       << "  total_count_change = " << status.total_count_change << endl;
}


void
DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  cerr << "DataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void
DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  cerr << "DataReaderListenerImpl::on_liveliness_changed" << endl;
}

void
DataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr dr,
    const DDS::SubscriptionMatchedStatus& sms)
{
  cerr << "DataReaderListenerImpl[" << dr << "]::on_subscription_matched "
    "tc=" << sms.total_count << " tcc=" << sms.total_count_change << " cc=" <<
    sms.current_count << " ccc=" << sms.current_count_change << endl;
}

void
DataReaderListenerImpl::on_sample_rejected (
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  cerr << "DataReaderListenerImpl::on_sample_rejected" << endl;
}

void
DataReaderListenerImpl::on_sample_lost (
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
{
  cerr << "DataReaderListenerImpl::on_sample_lost" << endl;
}

void
DataReaderListenerImpl::on_subscription_disconnected (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void
DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void
DataReaderListenerImpl::on_subscription_lost (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::SubscriptionLostStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_lost" << endl;
}

void
DataReaderListenerImpl::on_budget_exceeded (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::BudgetExceededStatus&)
{
  cerr << "DataReaderListenerImpl::on_budget_exceeded" << endl;
}
void
DataReaderListenerImpl::on_connection_deleted (DDS::DataReader_ptr)
{
  cerr << "DataReaderListenerImpl::on_connection_deleted" << endl;
}
