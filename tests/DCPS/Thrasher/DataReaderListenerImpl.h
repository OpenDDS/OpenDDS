#ifndef DCPS_THRASHER_DATAREADERLISTENERIMPL_H
#define DCPS_THRASHER_DATAREADERLISTENERIMPL_H

#include "ProgressIndicator.h"

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/PoolAllocator.h>

#ifdef ACE_HAS_CPP11
#include <condition_variable>
#include <mutex>
#else
#include <dds/DCPS/ConditionVariable.h>
#endif
#include <cstdlib>

class DataReaderListenerImpl : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  DataReaderListenerImpl(size_t publisher_count, size_t samples_per_publisher, const char* progress_fmt);
  virtual ~DataReaderListenerImpl();

  void wait_received() const;
  int check_received() const;
  virtual void on_data_available(DDS::DataReader_ptr reader);

  virtual void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&) {}
  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&) {}
  virtual void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&) {}
  virtual void on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&) {}
  virtual void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&) {}
  virtual void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&) {}

private:
  typedef OPENDDS_MAP(size_t, OPENDDS_SET(size_t)) ReceivedSamplesMap;
  bool received_all() const;
  bool update_and_check(size_t x, size_t y);
#ifdef ACE_HAS_CPP11
  typedef std::mutex Mutex;
  typedef std::condition_variable Condition;
  typedef std::unique_lock<Mutex> Lock;
#else
  typedef ACE_Thread_Mutex Mutex;
  typedef OpenDDS::DCPS::ConditionVariable<Mutex> Condition;
  typedef ACE_Guard<Mutex> Lock;
#endif
  mutable Mutex mutex_;
  mutable Condition condition_;
  const size_t publisher_count_;
  const size_t samples_per_publisher_;
  const size_t maximum_possible_samples_;
  const size_t expected_total_samples_;
  const size_t expected_samples_per_publisher_;
  size_t received_total_samples_;
  ReceivedSamplesMap received_samples_map_;
  ProgressIndicator progress_;
};

#endif /* DCPS_THRASHER_DATAREADERLISTENERIMPL_H */
