#include "DataReaderListenerImpl.h"
#include "FooTypeTypeSupportC.h"
#include <dds/DCPS/Service_Participant.h>

DataReaderListenerImpl::DataReaderListenerImpl(size_t publisher_count, size_t samples_per_publisher, const char* progress_fmt)
  : mutex_()
#ifdef ACE_HAS_CPP11
  , condition_()
#else
  , condition_(mutex_)
#endif
  , publisher_count_(publisher_count)
  , samples_per_publisher_(samples_per_publisher)
  , maximum_possible_samples_(publisher_count_ * samples_per_publisher_)
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
  , expected_total_samples_(maximum_possible_samples_)
  , expected_samples_per_publisher_(samples_per_publisher_)
#else
  , expected_total_samples_(publisher_count_)
  , expected_samples_per_publisher_(1)
#endif
  , received_total_samples_(0)
  , received_samples_map_()
  , progress_(progress_fmt, maximum_possible_samples_)
{}

DataReaderListenerImpl::~DataReaderListenerImpl()
{}

void DataReaderListenerImpl::wait_received() const
{
  Lock lock(mutex_);
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) sub wait_received %d:%d\n"), received_total_samples_, expected_total_samples_));
  while (!received_all()) {
#ifdef ACE_HAS_CPP11
    condition_.wait(lock);
#else
    OpenDDS::DCPS::ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
    condition_.wait(thread_status_manager);
#endif
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) sub condition_.wait returned\n")));
  }
}

int DataReaderListenerImpl::check_received() const
{
  Lock lock(mutex_);
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) sub check_received\n")));
  int ret = 0;
  for (size_t p = 0; p < publisher_count_; ++p) {
    ReceivedSamplesMap::const_iterator i = received_samples_map_.find(p);
    if (i != received_samples_map_.end()) {
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
      for (size_t s = 0; s < expected_samples_per_publisher_; ++s) {
        if (i->second.count(s) == 0) {
          ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: missing pub%d sample%d\n"), p, s));
          ++ret;
        }
      }
#else
      if (i->second.size() == 0) {
        // Theoretically this should never happen, since the only reason to have a map entry is receiving a sample
        ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: missing any sample from pub%d\n"), p));
        ++ret;
      }
#endif
    } else {
      ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: missing pub%d samples\n"), p));
      ++ret;
    }
  }
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
  if (received_total_samples_ != expected_total_samples_) {
#else
  if (received_total_samples_ < expected_total_samples_) {
#endif
    ACE_DEBUG((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: sub received %d of expected %d samples.\n"),
      received_total_samples_, expected_total_samples_));
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
      Lock lock(mutex_);
      if (update_and_check(static_cast<size_t>(foo.x), static_cast<size_t>(foo.y))) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)  sub condition_.notify_all\n")));
        condition_.notify_all();
      }
    }
  }
}

bool DataReaderListenerImpl::received_all() const
{
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
  return received_total_samples_ >= expected_total_samples_;
#else
  ReceivedSamplesMap::const_iterator i = received_samples_map_.begin();
  for (size_t p = 0; p < publisher_count_; ++p) {
    if (i == received_samples_map_.end() || i->first != p || i->second.size() == 0) {
      return false;
    }
    ++i;
  }
  return true;
#endif
}

bool DataReaderListenerImpl::update_and_check(size_t x, size_t y)
{
  if (received_samples_map_[x].insert(y).second) {
    ++received_total_samples_;
    ++progress_;
    return received_all();
  }
  return false;
}
