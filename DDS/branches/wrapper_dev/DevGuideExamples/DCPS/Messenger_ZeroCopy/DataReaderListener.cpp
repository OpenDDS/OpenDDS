// -*- C++ -*-
//
// $Id$
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>


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

  try
    {
      Messenger::MessageDataReader_var dr = 
        Messenger::MessageDataReader::_narrow (reader);
      if (CORBA::is_nil (dr.in ())) {
        cerr << "Failed to obtain DataReader Implementation\n" << endl;
        exit(1);
      }

      CORBA::Long MAX_ELEMS_TO_RETURN = 3;
      // Note: the 0 enables zero-copy reads/takes.
      //       A value > 0 will enable single-copy reads/takes
      //       The default contructor enables zero-copy reads/takes.
      Messenger::MessageSeq the_data (0, MAX_ELEMS_TO_RETURN);
      DDS::SampleInfoSeq    the_info (0, MAX_ELEMS_TO_RETURN);

      // get references to the samples  (zero-copy read of the samples)
      DDS::ReturnCode_t status = dr->read (the_data
                                           , the_info
                                           , MAX_ELEMS_TO_RETURN
                                           , ::DDS::ANY_SAMPLE_STATE
                                           , ::DDS::ANY_VIEW_STATE
                                           , ::DDS::ANY_INSTANCE_STATE);

      if (status == DDS::RETCODE_OK) {
        cout << "Message: subject    = " << the_data[0].subject.in() << endl
             << "         subject_id = " << the_data[0].subject_id   << endl
             << "         from       = " << the_data[0].from.in()    << endl
             << "         count      = " << the_data[0].count        << endl
             << "         text       = " << the_data[0].text.in()    << endl;
        cout << "SampleInfo.sample_rank = " << the_info[0].sample_rank << endl;
      } else if (status == DDS::RETCODE_NO_DATA) {
        cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << endl;
      } else {
        cerr << "ERROR: read Message: Error: " <<  status << endl;
      }

      // the application is required to return the loaned samples
      dr->return_loan (the_data, the_info);
    }
  catch (CORBA::Exception& e) {
    cerr << "Exception caught in read:" << endl << e << endl;
    exit(1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_liveliness_changed" << endl;
}

void DataReaderListenerImpl::on_subscription_match (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_match" << endl;
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_sample_rejected" << endl;
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_sample_lost" << endl;
}

void DataReaderListenerImpl::on_subscription_disconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_lost (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionLostStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_lost" << endl;
}

void DataReaderListenerImpl::on_connection_deleted (
  DDS::DataReader_ptr)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_connection_deleted" << endl;
}
