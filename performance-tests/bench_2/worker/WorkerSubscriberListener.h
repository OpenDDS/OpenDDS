#pragma once

#include "SubscriberListener.h"

namespace Bench {

class WorkerSubscriberListener : public Builder::SubscriberListener {
public:

  // From DDS::DataReaderListener

  void on_requested_deadline_missed(DDS::DataReader_ptr reader, const DDS::RequestedDeadlineMissedStatus& status) override;
  void on_requested_incompatible_qos(DDS::DataReader_ptr reader, const DDS::RequestedIncompatibleQosStatus& status) override;
  void on_sample_rejected(DDS::DataReader_ptr reader, const DDS::SampleRejectedStatus& status) override;
  void on_liveliness_changed(DDS::DataReader_ptr reader, const DDS::LivelinessChangedStatus& status) override;
  void on_data_available(DDS::DataReader_ptr reader) override;
  void on_subscription_matched(DDS::DataReader_ptr reader, const DDS::SubscriptionMatchedStatus& status) override;
  void on_sample_lost(DDS::DataReader_ptr reader, const DDS::SampleLostStatus& status) override;

  // From DDS::SubscriberListener

  void on_data_on_readers(DDS::Subscriber_ptr subs) override;

  // From Builder::SubscriberListener

  void set_subscriber(Builder::Subscriber& subscriber) override;

protected:
  std::mutex mutex_;
  Builder::Subscriber* subscriber_{0};
};

}

