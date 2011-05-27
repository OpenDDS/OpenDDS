// -*- C++ -*-
//
// $Id$
#include "DataReaderListener.h"
#include "MessageTypeSupportC.h"
#include "MessageTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

extern int num_reads_before_crash;

// Implementation skeleton constructor
DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0)
{
}

// Implementation skeleton destructor
DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
  throw (CORBA::SystemException)
{
  num_reads_ ++;

  try {
    MessageDataReader_var message_dr = MessageDataReader::_narrow(reader);
    if (CORBA::is_nil (message_dr.in ())) {
      ACE_DEBUG ((LM_DEBUG , 
        "(%P|%t)read: _narrow failed.\n"));
      exit(1);
    }

    Messenger::Message message;
    DDS::SampleInfo si ;
    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si) ;
    // Alternate code to read directlty via the servant
    //MessageDataReaderImpl* dr_servant =
    //  reference_to_servant< MessageDataReaderImpl,
    //                        MessageDataReader_ptr>(message_dr.in());
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

  if (num_reads_before_crash && num_reads_before_crash == this->num_reads_)
  {
    ACE_DEBUG((LM_DEBUG, "(%P|%t)Subscriber crash after %dth reads\n", num_reads_before_crash));
    ACE_OS::abort ();
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
  throw (CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG , 
    "(%P|%t)DataReaderListenerImpl::on_requested_deadline_missed\n"));
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG , 
    "(%P|%t)DataReaderListenerImpl::on_requested_incompatible_qos\n"));
}

void DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG , 
    "(%P|%t)DataReaderListenerImpl::on_liveliness_changed\n"));
}

void DataReaderListenerImpl::on_subscription_match (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus &)
  throw (CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG , 
    "(%P|%t)DataReaderListenerImpl::on_subscription_match\n"));
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG , 
    "(%P|%t)DataReaderListenerImpl::on_sample_rejected\n"));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG , 
    "(%P|%t)DataReaderListenerImpl::on_sample_lost\n"));
}

void DataReaderListenerImpl::on_subscription_lost (
  DDS::DataReader_ptr reader,
  const ::TAO::DCPS::SubscriptionLostStatus & status)
  throw (CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG , 
    "(%P|%t)DataReaderListenerImpl::on_subscription_lost "
    "total_count=%d total_count_change=%d \n",
    status.total_count, status.total_count_change));

  for (CORBA::Long i = 0; i < status.total_count; ++i)
  {
    ACE_DEBUG ((LM_DEBUG , 
      "(%P|%t)Lost subscription to writer %d \n", status.publication_handles[i]));
  }
}