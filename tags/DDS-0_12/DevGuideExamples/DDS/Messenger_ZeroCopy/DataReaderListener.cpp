// -*- C++ -*-
//
// $Id$
#include "DataReaderListener.h"
#include "MessageTypeSupportC.h"
#include "MessageTypeSupportImpl.h"
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
      MessageDataReaderImpl* dr_impl =
        ::TAO::DCPS::reference_to_servant< MessageDataReaderImpl,
        MessageDataReader_ptr> (MessageDataReader::_narrow (reader));
      if (0 == dr_impl) {
        cerr << "Failed to obtain DataReader Implementation\n" << endl;
        exit(1);
      }

      CORBA::Long MAX_ELEMS_TO_RETURN = 3;
      // using types supporting zero-copy read 
      // Note: the 0 enables zero-copy reads/takes.
      //       A value > 0 will enable single-copy reads/takes
      //       The default contructor enables zero-copy reads/takes.
      MessageZCSeq the_data (0, MAX_ELEMS_TO_RETURN);
      ::TAO::DCPS::SampleInfoZCSeq the_info(0,MAX_ELEMS_TO_RETURN);

      // Use the zero-copy API to get data buffers directly.
      DDS::ReturnCode_t status = dr_impl->read (the_data
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
      dr_impl->return_loan (the_data, the_info);
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
  const ::TAO::DCPS::SubscriptionDisconnectedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::TAO::DCPS::SubscriptionReconnectedStatus &)
  throw (CORBA::SystemException)
{
  cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_lost (
  DDS::DataReader_ptr,
  const ::TAO::DCPS::SubscriptionLostStatus &)
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
