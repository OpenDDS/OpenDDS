#include "WorkerDataReaderListener.h"
#include <cmath>
namespace Bench {

WorkerDataReaderListener::WorkerDataReaderListener() {
}

WorkerDataReaderListener::WorkerDataReaderListener(size_t expected) : expected_count_(expected) {
}

WorkerDataReaderListener::~WorkerDataReaderListener() {
}

void WorkerDataReaderListener::add_handler(DataHandler& handler) {
  handlers_.push_back(&handler);
}

void WorkerDataReaderListener::remove_handler(const DataHandler& handler) {
  bool found = true;
  while (found) {
    found = false;
    for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
      if (&handler == (*it)) {
        handlers_.erase(it);
        found = true;
        break;
      }
    }
  }
}

void WorkerDataReaderListener::on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/, const DDS::RequestedDeadlineMissedStatus& /*status*/) {
}

void WorkerDataReaderListener::on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/, const DDS::RequestedIncompatibleQosStatus& /*status*/) {
}

void WorkerDataReaderListener::on_sample_rejected(DDS::DataReader_ptr /*reader*/, const DDS::SampleRejectedStatus& status) {
  rejected_sample_count_->value.ull_prop(rejected_sample_count_->value.ull_prop() + status.total_count_change);
}

void WorkerDataReaderListener::on_liveliness_changed(DDS::DataReader_ptr /*reader*/, const DDS::LivelinessChangedStatus& /*status*/) {
}

void WorkerDataReaderListener::on_data_available(DDS::DataReader_ptr reader) {
  //std::cout << "WorkerDataReaderListener::on_data_available" << std::endl;
  if (reader != data_dr_.in()) {
    data_dr_ = DataDataReader::_narrow(reader);
  }
  if (data_dr_) {
    Data data;
    DDS::SampleInfo si;
    DDS::ReturnCode_t status = data_dr_->take_next_sample(data, si);
    if (status == DDS::RETCODE_OK && si.valid_data) {

      // Calculate the stateless stuff
      const Builder::TimeStamp& now = Builder::get_time();
      double latency = Builder::to_seconds_double(now - data.sent_time);
      double jitter = -1.0;
      double round_trip_latency = -1.0;
      double round_trip_jitter = -1.0;
      if (data.total_hops != 0 && data.hop_count == data.total_hops) {
        round_trip_latency = Builder::to_seconds_double(now - data.created_time) / static_cast<double>(data.total_hops);
      }

      std::unique_lock<std::mutex> lock(mutex_);

      // Update Latency & Calculate / Update Jitter
      auto pl_it = previous_latency_map_.find(si.publication_handle);
      if (pl_it == previous_latency_map_.end()) {
        pl_it = previous_latency_map_.insert(std::unordered_map<DDS::InstanceHandle_t, double>::value_type(si.publication_handle, 0.0)).first;
      } else {
        jitter = std::fabs(pl_it->second - latency);
      }
      pl_it->second = latency;
      if (datareader_) {
        latency_stat_block_->update(latency);

        if (jitter >= 0.0) {
          jitter_stat_block_->update(jitter);
        }
      }

      // Update Round-Trip Latency & Calculate / Update Round-Trip Jitter
      if (data.total_hops != 0 && data.hop_count == data.total_hops) {
        auto prtl_it = previous_round_trip_latency_map_.find(si.publication_handle);
        if (prtl_it == previous_round_trip_latency_map_.end()) {
          prtl_it = previous_round_trip_latency_map_.insert(std::unordered_map<DDS::InstanceHandle_t, double>::value_type(si.publication_handle, 0.0)).first;
        } else {
          round_trip_jitter = std::fabs(prtl_it->second - round_trip_latency);
        }
        prtl_it->second = round_trip_latency;
        if (datareader_) {
          round_trip_latency_stat_block_->update(round_trip_latency);

          if (round_trip_jitter >= 0.0) {
            round_trip_jitter_stat_block_->update(round_trip_jitter);
          }
        }
      }

      for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
        (*it)->on_data(data);
      }
    }
  }
}

void WorkerDataReaderListener::on_subscription_matched(DDS::DataReader_ptr /*reader*/, const DDS::SubscriptionMatchedStatus& status) {
  //std::cout << "WorkerDataReaderListener::on_subscription_matched" << std::endl;
  std::unique_lock<std::mutex> lock(mutex_);
  if (expected_count_ != 0) {
    if (static_cast<size_t>(status.current_count) == expected_count_) {
      //std::cout << "WorkerDataReaderListener reached expected count!" << std::endl;
      if (datareader_) {
        last_discovery_time_->value.time_prop(Builder::get_time());
      }
    }
  } else {
    if (static_cast<size_t>(status.current_count) > matched_count_) {
      if (datareader_) {
        last_discovery_time_->value.time_prop(Builder::get_time());
      }
    }
  }
  matched_count_ = status.current_count;
}

void WorkerDataReaderListener::on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& status) {
  lost_sample_count_->value.ull_prop(lost_sample_count_->value.ull_prop() + status.total_count_change);
}

void WorkerDataReaderListener::set_datareader(Builder::DataReader& datareader) {
  datareader_ = &datareader;

  last_discovery_time_ = get_or_create_property(datareader_->get_report().properties, "last_discovery_time", Builder::PVK_TIME);
  lost_sample_count_ = get_or_create_property(datareader_->get_report().properties, "lost_sample_count", Builder::PVK_ULL);
  rejected_sample_count_ = get_or_create_property(datareader_->get_report().properties, "rejected_sample_count", Builder::PVK_ULL);

  latency_stat_block_ = std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "latency", 1000);
  jitter_stat_block_ = std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "jitter", 1000);
  round_trip_latency_stat_block_ = std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "round_trip_latency", 1000);
  round_trip_jitter_stat_block_ = std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "round_trip_jitter", 1000);
}

void WorkerDataReaderListener::unset_datareader(Builder::DataReader& datareader) {
  if (datareader_ == &datareader) {

    latency_stat_block_->write_median();
    jitter_stat_block_->write_median();

    round_trip_latency_stat_block_->write_median();
    round_trip_jitter_stat_block_->write_median();

    datareader_ = NULL;
  }
}

}

