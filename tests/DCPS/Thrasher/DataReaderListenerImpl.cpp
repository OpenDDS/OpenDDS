/*
 */

#include "DataReaderListenerImpl.h"
#include "FooTypeTypeSupportC.h"

DataReaderListenerImpl::DataReaderListenerImpl(std::size_t& received_samples,
                                               const ProgressIndicator& progress)
  : mutex_(),
    condition_(mutex_),
    received_samples_(received_samples),
    progress_(progress)
{}

DataReaderListenerImpl::~DataReaderListenerImpl()
{}

void
DataReaderListenerImpl::on_data_available(
    DDS::DataReader_ptr reader)
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
    if (si.valid_data)
    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      ++received_samples_;
      ++progress_;
      task_sample_set_map[(size_t) foo.x].insert((size_t) foo.y);
      condition_.broadcast();
    }
  }
}

void
DataReaderListenerImpl::on_requested_deadline_missed(
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus&)
{}

void
DataReaderListenerImpl::on_requested_incompatible_qos(
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus&)
{}

void
DataReaderListenerImpl::on_liveliness_changed(
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus&)
{}

void
DataReaderListenerImpl::on_subscription_matched(
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus&)
{}

void
DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{}

void
DataReaderListenerImpl::on_sample_lost(
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
{}

bool
DataReaderListenerImpl::wait_received(const OpenDDS::DCPS::TimeDuration& duration, size_t target)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  const OpenDDS::DCPS::MonotonicTimePoint deadline(OpenDDS::DCPS::MonotonicTimePoint::now() + duration);
  while (received_samples_ < target && OpenDDS::DCPS::MonotonicTimePoint::now() < deadline) {
    condition_.wait(&deadline.value());
  }
  return received_samples_ >= target;
}
