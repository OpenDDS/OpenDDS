#include "DataReaderListenerImpl.h"
#include "FooTypeTypeSupportC.h"

DataReaderListenerImpl::DataReaderListenerImpl(const std::size_t expected_samples, const char* progress_fmt)
  : mutex_()
#ifdef ACE_HAS_CPP11
  , condition_()
#else
  , condition_(mutex_)
#endif
  , expected_samples_(expected_samples)
  , received_samples_(0)
  , task_samples_map_()
  , progress_(progress_fmt, expected_samples_)
{}

DataReaderListenerImpl::~DataReaderListenerImpl()
{}

void DataReaderListenerImpl::wait_received() const
{
  Lock lock(mutex_);
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) sub wait_received %d:%d\n"), received_samples_, expected_samples_));
  while (received_samples_ < expected_samples_) {
#ifdef ACE_HAS_CPP11
    condition_.wait(lock);
#else
    condition_.wait();
#endif
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) sub condition_.wait returned\n")));
  }
}

int DataReaderListenerImpl::check_received(const size_t n_publishers) const
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) sub check_received\n")));
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
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) sub check_received returns %d\n"), ret));
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
      if (received_all((size_t) foo.x, (size_t) foo.y)) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)  sub condition_.notify_all\n")));
        condition_.notify_all();
      }
    }
  }
}

bool DataReaderListenerImpl::received_all(const size_t x, const size_t y)
{
  Lock lock(mutex_);
  ++received_samples_;
  ++progress_;
  task_samples_map_[x].insert(y);
  return received_samples_ >= expected_samples_;
}
