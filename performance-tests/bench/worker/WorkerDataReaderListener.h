#pragma once

#include "DataHandler.h"
#include "DataReaderListener.h"
#include "BenchTypeSupportImpl.h"
#include "PropertyStatBlock.h"

#include <unordered_map>

namespace Bench {

class WorkerDataReaderListener : public Builder::DataReaderListener {
public:

  WorkerDataReaderListener();
  WorkerDataReaderListener(size_t expected);

  void add_handler(DataHandler& handler);
  void remove_handler(const DataHandler& handler);

  // From DDS::DataReaderListener

  void on_requested_deadline_missed(DDS::DataReader_ptr reader, const DDS::RequestedDeadlineMissedStatus& status) override;
  void on_requested_incompatible_qos(DDS::DataReader_ptr reader, const DDS::RequestedIncompatibleQosStatus& status) override;
  void on_sample_rejected(DDS::DataReader_ptr reader, const DDS::SampleRejectedStatus& status) override;
  void on_liveliness_changed(DDS::DataReader_ptr reader, const DDS::LivelinessChangedStatus& status) override;
  void on_data_available(DDS::DataReader_ptr reader) override;
  void on_subscription_matched(DDS::DataReader_ptr reader, const DDS::SubscriptionMatchedStatus& status) override;
  void on_sample_lost(DDS::DataReader_ptr reader, const DDS::SampleLostStatus& status) override;

  // From Builder::DataReaderListener

  void set_datareader(Builder::DataReader& datareader) override;

protected:
  std::mutex mutex_;
  size_t expected_count_{0};
  size_t matched_count_{0};
  Builder::DataReader* datareader_{0};
  DataDataReader_var data_dr_;
  std::vector<DataHandler*> handlers_;

  Builder::PropertyIndex last_discovery_time_;

  std::shared_ptr<PropertyStatBlock> latency_stat_block_;
  std::unordered_map<DDS::InstanceHandle_t, double> previous_latency_map_;

  std::shared_ptr<PropertyStatBlock> jitter_stat_block_;
};

}

