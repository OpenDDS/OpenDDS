/*
 * $Id$
 */

#include "DataReaderListenerImpl.h"
#include "FooTypeTypeSupportC.h"

DataReaderListenerImpl::DataReaderListenerImpl(std::size_t& received_samples,
                                               const ProgressIndicator& progress)
  : received_samples_(received_samples),
    progress_(progress)
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
  // dull and take only one sample at a time.

  Foo foo;
  DDS::SampleInfo si;

  while (reader_i->take_next_sample(foo, si) == DDS::RETCODE_OK)
  {
    ++received_samples_;
    ++progress_;
  }
}

void
DataReaderListenerImpl::on_requested_deadline_missed(
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus&)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_requested_incompatible_qos(
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus&)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_liveliness_changed(
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus&)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_subscription_match(
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus&)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{}

void
DataReaderListenerImpl::on_sample_lost(
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{}
