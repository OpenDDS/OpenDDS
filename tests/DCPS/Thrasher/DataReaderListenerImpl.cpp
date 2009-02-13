/*
 * $Id$
 */

#include "DataReaderListenerImpl.h"
#include "FooTypeTypeSupportC.h"

DataReaderListenerImpl::DataReaderListenerImpl(size_t& received_samples)
  : received_samples_(received_samples)
{}

DataReaderListenerImpl::~DataReaderListenerImpl()
{}

void
DataReaderListenerImpl::on_data_available(
    DDS::DataReader_ptr reader)
  throw (CORBA::SystemException)
{
  FooDataReader_var reader_i = FooDataReader::_narrow(reader);
  if (CORBA::is_nil(reader_i))
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l: on_data_available()")
               ACE_TEXT(" _narrow failed!\n")));
    return;
  }
  
  // The following is intentionally inefficient to simulate
  // backpressure with multiple writers; we should be especially
  // dull take only one sample at a time.

  Foo foo;
  DDS::SampleInfo si;
  
  if (reader_i->take_next_sample(foo, si) != DDS::RETCODE_OK)
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l: on_data_available()")
               ACE_TEXT(" take_next_sample failed!\n")));
    return;
  }

  ++this->received_samples_;
}

void
DataReaderListenerImpl::on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_subscription_match(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchStatus& status)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status)
  throw (CORBA::SystemException)
{}
