#include "WorkerDataReaderListener.h"

namespace Stool {

WorkerDataReaderListener::WorkerDataReaderListener() {
}

WorkerDataReaderListener::WorkerDataReaderListener(size_t expected) : expected_count_(expected) {
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

void WorkerDataReaderListener::on_sample_rejected(DDS::DataReader_ptr /*reader*/, const DDS::SampleRejectedStatus& /*status*/) {
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
      const Builder::TimeStamp& now = Builder::get_time();
      double latency = Builder::to_seconds_double(now - data.created.time);
      //std::cout << "WorkerDataReaderListener::on_data_available() - Valid Data :: Latency = " << std::fixed << std::setprecision(6) << latency << " seconds" << std::endl;

      std::unique_lock<std::mutex> lock(mutex_);
      if (datareader_) {
        auto prev_latency_mean = latency_mean_->value.double_prop();
        auto prev_latency_var_x_sample_count = latency_var_x_sample_count_->value.double_prop();

        sample_count_->value.ull_prop(sample_count_->value.ull_prop() + 1);

        if (latency < latency_min_->value.double_prop()) {
          latency_min_->value.double_prop(latency);
        }
        if (latency_max_->value.double_prop() < latency) {
          latency_max_->value.double_prop(latency);
        }
        if (sample_count_->value.ull_prop() == 0) {
          latency_mean_->value.double_prop(latency);
          latency_var_x_sample_count_->value.double_prop(latency);
        } else {
          // Incremental mean calculation (doesn't require storing all the data)
          latency_mean_->value.double_prop(prev_latency_mean + ((latency - prev_latency_mean) / static_cast<double>(sample_count_->value.ull_prop())));
          // Incremental (variance * sample_count) calculation (doesn't require storing all the data, can be used to easily find variance / standard deviation)
          latency_var_x_sample_count_->value.double_prop(prev_latency_var_x_sample_count + ((latency - prev_latency_mean) * (latency - latency_mean_->value.double_prop())));
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

void WorkerDataReaderListener::on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& /*status*/) {
}

void WorkerDataReaderListener::set_datareader(Builder::DataReader& datareader) {
  datareader_ = &datareader;

  last_discovery_time_ = get_or_create_property(datareader_->get_report().properties, "last_discovery_time", Builder::PVK_TIME);

  sample_count_ = get_or_create_property(datareader_->get_report().properties, "sample_count", Builder::PVK_ULL);
  sample_count_->value.ull_prop(0);
  latency_min_ = get_or_create_property(datareader_->get_report().properties, "latency_min", Builder::PVK_DOUBLE);
  latency_min_->value.double_prop(std::numeric_limits<double>::max());
  latency_max_ = get_or_create_property(datareader_->get_report().properties, "latency_max", Builder::PVK_DOUBLE);
  latency_max_->value.double_prop(0.0);
  latency_mean_ = get_or_create_property(datareader_->get_report().properties, "latency_mean", Builder::PVK_DOUBLE);
  latency_mean_->value.double_prop(0.0);
  latency_var_x_sample_count_ = get_or_create_property(datareader_->get_report().properties, "latency_var_x_sample_count", Builder::PVK_DOUBLE);
  latency_var_x_sample_count_->value.double_prop(0.0);
}

}

