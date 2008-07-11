// -*- C++ -*-
//
// $Id$
#include "StackDataReaderListener.h"
#include "MessageTypeSupportC.h"
#include "MessageTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

using namespace Messenger;

// Implementation skeleton constructor
StackDataReaderListenerImpl::StackDataReaderListenerImpl()
  : num_reads_(0)
{
}

// Implementation skeleton destructor
StackDataReaderListenerImpl::~StackDataReaderListenerImpl ()
{
}

void StackDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
  throw (CORBA::SystemException)
{
  num_reads_ ++;

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
    // Alternate code to read directlty via the servant
    //MessageDataReaderImpl* dr_servant =
    //  reference_to_servant<MessageDataReaderImpl>(message_dr.in());
    //DDS::ReturnCode_t status = dr_servant->take_next_sample(message, si) ;

    if (status == DDS::RETCODE_OK) {
      cout << "Message: subject    = " << message.subject.in() << endl
           << "         subject_id = " << message.subject_id   << endl
           << "         from       = " << message.from.in()    << endl
           << "         count      = " << message.count        << endl
           << "         text       = " << message.text.in()    << endl;
      cout << "SampleInfo.sample_rank = " << si.sample_rank << endl;
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

void StackDataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "StackDataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void StackDataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
  cerr << "StackDataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void StackDataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "StackDataReaderListenerImpl::on_liveliness_changed" << endl;
}

void StackDataReaderListenerImpl::on_subscription_match (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus &)
  throw (CORBA::SystemException)
{
  cerr << "StackDataReaderListenerImpl::on_subscription_match" << endl;
}

void StackDataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{
  cerr << "StackDataReaderListenerImpl::on_sample_rejected" << endl;
}

void StackDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{
  cerr << "StackDataReaderListenerImpl::on_sample_lost" << endl;
}
