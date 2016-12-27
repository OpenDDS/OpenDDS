// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"

using namespace Messenger;
using namespace std;

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0),
    num_liveliness_change_callbacks_ (0)
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

    if (status == DDS::RETCODE_OK) {

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
      }
      else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
      {
        cout << "instance is unregistered" << endl;
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
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status)
{
  ++ num_liveliness_change_callbacks_;

  const OpenDDS::DCPS::RepoId id = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(reader->get_subscriber()->get_participant())->get_repoid(status.last_publication_handle);
  OpenDDS::DCPS::GuidConverter converter(id);

  ACE_DEBUG((LM_INFO, "DataReaderListenerImpl::on_liveliness_changed #%d\n"
                      "  alive_count = %d\n"
                      "  not_alive_count = %d\n"
                      "  alive_count_change = %d\n"
                      "  not_alive_count_change = %d\n"
                      "  last_publication_handle = %d (%C)\n",
                      num_liveliness_change_callbacks_,
                      status.alive_count,
                      status.not_alive_count,
                      status.alive_count_change,
                      status.not_alive_count_change,
                      status.last_publication_handle, OPENDDS_STRING(converter).c_str()));
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
