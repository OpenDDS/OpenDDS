// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

extern int num_reads_before_crash;
extern int read_delay_ms;
extern int actual_lost_sub_notification;

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  if (read_delay_ms > 0)
    ACE_OS::sleep (ACE_Time_Value (read_delay_ms/1000,
                                   read_delay_ms%1000*1000));

  ++num_reads_;

  try {
    Messenger::MessageDataReader_var message_dr = Messenger::MessageDataReader::_narrow(reader);
    if (CORBA::is_nil (message_dr.in ())) {
      ACE_DEBUG ((LM_DEBUG ,
        "(%P|%t) read: _narrow failed.\n"));
      exit(1);
    }

    Messenger::Message message;
    DDS::SampleInfo si;
    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);

    if (status == DDS::RETCODE_OK && si.valid_data) {
      cout << "Message: subject    = " << message.subject.in() << endl
           << "         subject_id = " << message.subject_id   << endl
           << "         from       = " << message.from.in()    << endl
           << "         count      = " << message.count        << endl
           << "         text       = " << message.text.in()    << endl;
      cout << "SampleInfo.sample_rank = " << si.sample_rank << endl;
    } else if (status == DDS::RETCODE_OK) {
      cout << "SampleInfo.instance_state = " << si.instance_state << endl;
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
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Subscriber crash after %dth reads\n", num_reads_before_crash));
    ACE_OS::abort ();
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n"));
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n"));
}

void DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n"));
}

void DataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataReaderListenerImpl::on_subscription_matched\n"));
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataReaderListenerImpl::on_sample_rejected\n"));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataReaderListenerImpl::on_sample_lost\n"));
}


void DataReaderListenerImpl::on_subscription_disconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus & status)
{
  CORBA::ULong len = status.publication_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_subscription_disconnected writer " << status.publication_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t) on_subscription_disconnected  writer %d \n", status.publication_handles[i]));
  }
}


void DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus & status)
{
  CORBA::ULong len = status.publication_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_subscription_reconnected writer " << status.publication_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t) on_subscription_reconnected  writer %d \n", status.publication_handles[i]));
  }
}


void DataReaderListenerImpl::on_subscription_lost (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionLostStatus & status)
{
  ++actual_lost_sub_notification;

  CORBA::ULong len = status.publication_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_subscription_lost writer " << status.publication_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t) on_subscription_lost  writer %d \n", status.publication_handles[i]));
  }
}


void DataReaderListenerImpl::on_budget_exceeded (
  ::DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::BudgetExceededStatus&)
{
  ACE_DEBUG ((LM_DEBUG, "(%P|%t) received on_budget_exceeded \n"));
}


void DataReaderListenerImpl::on_connection_deleted (
  ::DDS::DataReader_ptr)
{
  ACE_DEBUG ((LM_DEBUG, "(%P|%t) received on_connection_deleted  \n"));
}
