#pragma once

#include "DataHandler.h"
#include "DataReaderListener.h"
#include "PropertyStatBlock.h"

#include "dds/DCPS/DisjointSequence.h"

#include <unordered_map>
#include <condition_variable>

namespace Bench {

class WorkerDataReaderListener : public Builder::DataReaderListener {
public:

  WorkerDataReaderListener();
  WorkerDataReaderListener(const Builder::PropertySeq& properties);
  virtual ~WorkerDataReaderListener();

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
  void unset_datareader(Builder::DataReader& datareader) override;

  bool wait_for_expected_match(const std::chrono::system_clock::time_point& deadline) const;

protected:
  mutable std::mutex mutex_;
  bool durable_{false};
  bool reliable_{false};
  bool history_keep_all_{false};
  size_t history_depth_{false};
  size_t expected_match_count_{0};
  size_t match_count_{0};
  size_t expected_sample_count_{0};
  size_t sample_count_{0};
  Builder::DataReader* datareader_{0};
  DataDataReader_var data_dr_;
  std::vector<DataHandler*> handlers_;
  mutable std::condition_variable expected_match_cv;

  Builder::ConstPropertyIndex enable_time_;
  Builder::PropertyIndex last_discovery_time_;
  Builder::PropertyIndex lost_sample_count_;
  Builder::PropertyIndex rejected_sample_count_;
  Builder::PropertyIndex out_of_order_data_count_;
  Builder::PropertyIndex out_of_order_data_details_;
  Builder::PropertyIndex duplicate_data_count_;
  Builder::PropertyIndex duplicate_data_details_;
  Builder::PropertyIndex missing_data_count_;
  Builder::PropertyIndex missing_data_details_;

  struct WriterState {
    size_t sample_count_{0};
    size_t first_data_count_{0};
    size_t prev_data_count_{0};
    size_t current_data_count_{0};
    OpenDDS::DCPS::DisjointSequence data_received_;
    bool previously_disjoint_{false};
    size_t out_of_order_data_count_{0};
    OpenDDS::DCPS::DisjointSequence out_of_order_data_received_;
    size_t duplicate_data_count_{0};
    OpenDDS::DCPS::DisjointSequence duplicate_data_received_;
    double previous_latency_{0.0};
    double previous_round_trip_latency_{0.0};
  };

  typedef std::unordered_map<DDS::InstanceHandle_t, WriterState> WriterStateMap;

  WriterStateMap writer_state_map_;

  std::shared_ptr<PropertyStatBlock> discovery_delta_stat_block_;

  // Normal Latency / Jitter
  std::shared_ptr<PropertyStatBlock> latency_stat_block_;
  std::shared_ptr<PropertyStatBlock> jitter_stat_block_;

  // Round-Trip Latency / Jitter
  std::shared_ptr<PropertyStatBlock> round_trip_latency_stat_block_;
  std::shared_ptr<PropertyStatBlock> round_trip_jitter_stat_block_;
};

}

