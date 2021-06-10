#include "DataReaderListenerImpl.h"
#include "FooTypeTypeSupportC.h"

DataReaderListenerImpl::DataReaderListenerImpl(const std::size_t expected_samples, const char* progress_fmt)
  : mutex_()
  , condition_(mutex_)
  , expected_samples_(expected_samples)
  , received_samples_(0)
  , task_samples_map_()
  , progress_(progress_fmt, expected_samples_)
{}

DataReaderListenerImpl::~DataReaderListenerImpl()
{}

bool DataReaderListenerImpl::wait_received(const OpenDDS::DCPS::TimeDuration& duration, size_t target)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  const OpenDDS::DCPS::MonotonicTimePoint deadline(OpenDDS::DCPS::MonotonicTimePoint::now() + duration);
  while (received_samples_ < target && OpenDDS::DCPS::MonotonicTimePoint::now() < deadline) {
    condition_.wait(&deadline.value());
  }
  return received_samples_ >= target;
}

int DataReaderListenerImpl::check_received(const size_t n_publishers) const
{
  int ret = 0;
  const size_t samples_per_publisher = expected_samples_ / n_publishers;
  for (size_t p = 0; p < n_publishers; ++p) {
    TaskSamplesMap::const_iterator i = task_samples_map_.find(p);
    if (i != task_samples_map_.end()) {
      for (size_t s = 0; s < samples_per_publisher; ++s) {
        if (i->second.count(s) == 0) {
          ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: missing pub%d sample%d\n"), p, s));
          ++ret;
        }
      }
    } else {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: missing pub%d samples\n"), p));
      ++ret;
    }
  }
  if (received_samples_ != expected_samples_) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: sub received %d of expected %d samples.\n"),
      received_samples_, expected_samples_));
    ++ret;
  }
  return ret;
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  FooDataReader_var reader_i = FooDataReader::_narrow(reader);
  if (!reader_i) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: on_data_available() _narrow failed!\n")));
    return;
  }

  // Intentionally inefficient to simulate backpressure with multiple writers:
  // take only one sample at a time.
  Foo foo;
  DDS::SampleInfo si;
  while (reader_i->take_next_sample(foo, si) == DDS::RETCODE_OK) {
    if (si.valid_data) {
    //ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      ACE_Guard<ACE_SYNCH_MUTEX> g(mutex_);
      ++received_samples_;
      ++progress_;
      task_samples_map_[(size_t) foo.x].insert((size_t) foo.y);
      condition_.broadcast();
    }
  }
}
