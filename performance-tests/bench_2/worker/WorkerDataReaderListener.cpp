#include "WorkerDataReaderListener.h"

#include "Utils.h"

#include <cmath>

namespace Bench {

WorkerDataReaderListener::WorkerDataReaderListener()
{
}

WorkerDataReaderListener::WorkerDataReaderListener(const Builder::PropertySeq& properties)
{
  size_t expected_match_count = 0;
  auto expected_match_count_prop = get_property(properties, "expected_match_count", Builder::PVK_ULL);
  if (expected_match_count_prop) {
    expected_match_count = static_cast<size_t>(expected_match_count_prop->value.ull_prop());
  }
  expected_match_count_ = expected_match_count;

  size_t expected_sample_count = 0;
  auto expected_sample_count_prop = get_property(properties, "expected_sample_count", Builder::PVK_ULL);
  if (expected_sample_count_prop) {
    expected_sample_count = static_cast<size_t>(expected_sample_count_prop->value.ull_prop());
  }
  expected_sample_count_ = expected_sample_count;
}

WorkerDataReaderListener::~WorkerDataReaderListener()
{
}

void
WorkerDataReaderListener::add_handler(DataHandler& handler)
{
  handlers_.push_back(&handler);
}

void
WorkerDataReaderListener::remove_handler(const DataHandler& handler)
{
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

void
WorkerDataReaderListener::on_requested_deadline_missed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedDeadlineMissedStatus& /*status*/)
{
}

void
WorkerDataReaderListener::on_requested_incompatible_qos(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedIncompatibleQosStatus& /*status*/)
{
}

void
WorkerDataReaderListener::on_sample_rejected(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleRejectedStatus& status)
{
  rejected_sample_count_->value.ull_prop(rejected_sample_count_->value.ull_prop() + status.total_count_change);
}

void
WorkerDataReaderListener::on_liveliness_changed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::LivelinessChangedStatus& /*status*/) {
}

void
WorkerDataReaderListener::on_data_available(DDS::DataReader_ptr reader)
{
  if (reader != data_dr_.in()) {
    data_dr_ = DataDataReader::_narrow(reader);
  }
  if (data_dr_) {
    Data data;
    DDS::SampleInfo si;
    DDS::ReturnCode_t status = data_dr_->take_next_sample(data, si);
    if (status == DDS::RETCODE_OK && si.valid_data) {

      // Calculate the stateless stuff
      const Builder::TimeStamp& now = Builder::get_sys_time();
      double latency = Builder::to_seconds_double(now - data.sent_time);
      double jitter = -1.0;
      double round_trip_latency = -1.0;
      if (data.total_hops != 0 && data.hop_count == data.total_hops) {
        round_trip_latency = Builder::to_seconds_double(now - data.created_time) / static_cast<double>(data.total_hops);
      }

      std::unique_lock<std::mutex> lock(mutex_);

      bool new_writer = false;
      auto ws_it = writer_state_map_.find(si.publication_handle);
      if (ws_it == writer_state_map_.end()) {
        new_writer = true;
        ws_it = writer_state_map_.insert(WriterStateMap::value_type(si.publication_handle, WriterState())).first;
      }
      WriterState& ws = ws_it->second;

      if (ws.sample_count_ == 0) {
        ws.first_data_count_ = data.msg_count;
        ws.prev_data_count_ = data.msg_count;
        ws.current_data_count_ = data.msg_count;
        if (durable_ && (history_keep_all_ || history_depth_ > data.msg_count)) {
          ws.data_received_.insert(0);
        }
      } else {
        ws.prev_data_count_ = ws.current_data_count_;
        ws.current_data_count_ = data.msg_count;
        if (ws.current_data_count_ < ws.prev_data_count_) {
          // once we have one out-of-order, how do we count subsequent ones?
          // one option is to consider everything out of order until it's 'fixed'
          // another (this way) is to only capture relative out-of-order issues
          // which will have a greater penalty for repeated violations
          ++(ws.out_of_order_data_count_);
          ws.out_of_order_data_received_.insert(ws.current_data_count_);
        }
      }

      if (!ws.data_received_.insert(data.msg_count)) {
        ++(ws.duplicate_data_count_);
        ws.duplicate_data_received_.insert(ws.current_data_count_);
      }
      if (!ws.previously_disjoint_ && ws.data_received_.disjoint()) {
        //std::cout << "This shouldn't happen... " << std::endl;
      }
      ws.previously_disjoint_ = ws.data_received_.disjoint();
      ++(ws.sample_count_);
      ++sample_count_;

      // Update Latency & Calculate / Update Jitter
      if (!new_writer) {
        jitter = std::fabs(ws.previous_latency_ - latency);
      }
      ws.previous_latency_ = latency;
      if (datareader_) {
        latency_stat_block_->update(latency);

        if (jitter >= 0.0) {
          jitter_stat_block_->update(jitter);
        }
      }

      // Update Round-Trip Latency & Calculate / Update Round-Trip Jitter
      if (data.total_hops != 0 && data.hop_count == data.total_hops) {
        double round_trip_jitter = -1.0;
        if (!new_writer) {
          round_trip_jitter = std::fabs(ws.previous_round_trip_latency_ - round_trip_latency);
        }
        ws.previous_round_trip_latency_ = round_trip_latency;
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

void
WorkerDataReaderListener::on_subscription_matched(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SubscriptionMatchedStatus& status)
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (expected_match_count_ != 0) {
    if (static_cast<size_t>(status.current_count) == expected_match_count_) {
      expected_match_cv.notify_all();
      if (datareader_) {
        last_discovery_time_->value.time_prop(Builder::get_hr_time());
      }
    } else if (static_cast<size_t>(status.current_count) > match_count_) {
      if (datareader_) {
        discovery_delta_stat_block_->update(Builder::to_seconds_double(Builder::get_hr_time() - enable_time_->value.time_prop()));
      }
    }
  } else {
    if (static_cast<size_t>(status.current_count) > match_count_) {
      if (datareader_) {
        auto now = Builder::get_hr_time();
        last_discovery_time_->value.time_prop(now);
        discovery_delta_stat_block_->update(Builder::to_seconds_double(now - enable_time_->value.time_prop()));
      }
    }
  }
  match_count_ = status.current_count;
}

void
WorkerDataReaderListener::on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& status)
{
  lost_sample_count_->value.ull_prop(lost_sample_count_->value.ull_prop() + status.total_count_change);
}

void
WorkerDataReaderListener::set_datareader(Builder::DataReader& datareader)
{
  datareader_ = &datareader;

  auto durability_kind = datareader_->get_qos().durability.kind;
  if (durability_kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS ||
      durability_kind == DDS::TRANSIENT_DURABILITY_QOS ||
      durability_kind == DDS::PERSISTENT_DURABILITY_QOS) {
    durable_ = true;
  }
  history_keep_all_ = datareader_->get_qos().history.kind == DDS::KEEP_ALL_HISTORY_QOS;
  history_depth_ = datareader_->get_qos().history.depth;
  reliable_ = datareader_->get_qos().reliability.kind == DDS::RELIABLE_RELIABILITY_QOS;

  enable_time_ =
    get_property(datareader_->get_report().properties, "enable_time", Builder::PVK_TIME);
  last_discovery_time_ =
    get_or_create_property(datareader_->get_report().properties, "last_discovery_time", Builder::PVK_TIME);

  lost_sample_count_ =
    get_or_create_property(datareader_->get_report().properties, "lost_sample_count", Builder::PVK_ULL);
  rejected_sample_count_ =
    get_or_create_property(datareader_->get_report().properties, "rejected_sample_count", Builder::PVK_ULL);

  out_of_order_data_count_ =
    get_or_create_property(datareader_->get_report().properties, "out_of_order_data_count", Builder::PVK_ULL);
  out_of_order_data_details_ =
    get_or_create_property(datareader_->get_report().properties, "out_of_order_data_details", Builder::PVK_STRING);
  duplicate_data_count_ =
    get_or_create_property(datareader_->get_report().properties, "duplicate_data_count", Builder::PVK_ULL);
  duplicate_data_details_ =
    get_or_create_property(datareader_->get_report().properties, "duplicate_data_details", Builder::PVK_STRING);
  missing_data_count_ =
    get_or_create_property(datareader_->get_report().properties, "missing_data_count", Builder::PVK_ULL);
  missing_data_details_ =
    get_or_create_property(datareader_->get_report().properties, "missing_data_details", Builder::PVK_STRING);

  const Builder::PropertySeq& global_properties = get_global_properties();
  Builder::ConstPropertyIndex buffer_size_prop =
    get_property(global_properties, "default_stat_median_buffer_size", Builder::PVK_ULL);
  size_t buffer_size = buffer_size_prop ? static_cast<size_t>(buffer_size_prop->value.ull_prop()) : DEFAULT_STAT_BLOCK_BUFFER_SIZE;

  discovery_delta_stat_block_ =
    std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "discovery_delta", buffer_size);
  latency_stat_block_ =
    std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "latency", buffer_size);
  jitter_stat_block_ =
    std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "jitter", buffer_size);
  round_trip_latency_stat_block_ =
    std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "round_trip_latency", buffer_size);
  round_trip_jitter_stat_block_ =
    std::make_shared<PropertyStatBlock>(datareader_->get_report().properties, "round_trip_jitter", buffer_size);
}

void
WorkerDataReaderListener::unset_datareader(Builder::DataReader& datareader)
{
  if (datareader_ == &datareader) {

    size_t out_of_order_data_count = 0;
    size_t duplicate_data_count = 0;
    size_t missing_data_count = 0;
    std::stringstream missing_data_details;
    std::stringstream out_of_order_data_details;
    std::stringstream duplicate_data_details;
    bool new_writer;

    // out of order count / details
    for (auto it = writer_state_map_.begin(); it != writer_state_map_.end(); ++it) {
      new_writer = true;
      out_of_order_data_count += it->second.out_of_order_data_count_; // update count
      if (!it->second.out_of_order_data_received_.empty()) {
        if (out_of_order_data_details.str().empty()) {
          out_of_order_data_details << "Topic Name: " << datareader_->get_topic_name() << ", Reliable: " << (reliable_ ? "true" : "false") << ", Durable: " << (durable_ ? "true" : "false") << std::flush;
        }
        auto psr = it->second.out_of_order_data_received_.present_sequence_ranges();
        for (auto it2 = psr.begin(); it2 != psr.end(); ++it2) {
          if (new_writer) {
            out_of_order_data_details << " [PH: " << it->first << " (" << it->second.data_received_.low().getValue() << "-" << it->second.data_received_.high().getValue() << ")] " << std::flush;
            new_writer = false;
          } else {
            out_of_order_data_details << ", " << std::flush;
          }
          if (it2->first.getValue() == it2->second.getValue()) {
            out_of_order_data_details << it2->first.getValue() << std::flush;
          } else {
            out_of_order_data_details << it2->first.getValue() << "-" << it2->second.getValue() << std::flush;
          }
        }
      }
    }

    // duplicate data count / details
    for (auto it = writer_state_map_.begin(); it != writer_state_map_.end(); ++it) {
      new_writer = true;
      duplicate_data_count += it->second.duplicate_data_count_; // update count
      if (!it->second.duplicate_data_received_.empty()) {
        if (duplicate_data_details.str().empty()) {
          duplicate_data_details << "Topic Name: " << datareader_->get_topic_name() << ", Reliable: " << (reliable_ ? "true" : "false") << ", Durable: " << (durable_ ? "true" : "false") << std::flush;
        }
        auto psr = it->second.duplicate_data_received_.present_sequence_ranges();
        for (auto it2 = psr.begin(); it2 != psr.end(); ++it2) {
          if (new_writer) {
            duplicate_data_details << " [PH: " << it->first << " (" << it->second.data_received_.low().getValue() << "-" << it->second.data_received_.high().getValue() << ")] " << std::flush;
            new_writer = false;
          } else {
            duplicate_data_details << ", " << std::flush;
          }
          if (it2->first.getValue() == it2->second.getValue()) {
            duplicate_data_details << it2->first.getValue() << std::flush;
          } else {
            duplicate_data_details << it2->first.getValue() << "-" << it2->second.getValue() << std::flush;
          }
        }
      }
    }

    // missing data count / details
    for (auto it = writer_state_map_.begin(); it != writer_state_map_.end(); ++it) {
      new_writer = true;
      if (it->second.data_received_.disjoint()) {
        if (missing_data_details.str().empty()) {
          missing_data_details << "ERROR :: Topic Name: " << datareader_->get_topic_name() << ", Reliable: " << (reliable_ ? "true" : "false") << ", Durable: " << (durable_ ? "true" : "false") << std::flush;
        }
        auto msr = it->second.data_received_.missing_sequence_ranges();
        for (auto it2 = msr.begin(); it2 != msr.end(); ++it2) {
          missing_data_count += static_cast<ptrdiff_t>(it2->second.getValue() - (it2->first.getValue() - 1)); // update count
          if (new_writer) {
            missing_data_details << " [PH: " << it->first << " (" << it->second.data_received_.low().getValue() << "-" << it->second.data_received_.high().getValue() << ")] " << std::flush;
            new_writer = false;
          } else {
            missing_data_details << ", " << std::flush;
          }
          if (it2->first.getValue() == it2->second.getValue()) {
            missing_data_details << it2->first.getValue() << std::flush;
          } else {
            missing_data_details << it2->first.getValue() << "-" << it2->second.getValue() << std::flush;
          }
        }
      }
    }
    // if we didn't meet the expected sample count, add difference to missing sample count
    if (expected_sample_count_ && sample_count_ < expected_sample_count_) {
      missing_data_count += expected_sample_count_ - sample_count_;
      missing_data_details << " ERROR Expected Sample Deficit: " << expected_sample_count_ - sample_count_ << std::flush;
    }

    out_of_order_data_count_->value.ull_prop(out_of_order_data_count);
    out_of_order_data_details_->value.string_prop(out_of_order_data_details.str().c_str());
    duplicate_data_count_->value.ull_prop(duplicate_data_count);
    duplicate_data_details_->value.string_prop(duplicate_data_details.str().c_str());
    missing_data_count_->value.ull_prop(missing_data_count);
    missing_data_details_->value.string_prop(missing_data_details.str().c_str());

    discovery_delta_stat_block_->finalize();

    latency_stat_block_->finalize();
    jitter_stat_block_->finalize();

    round_trip_latency_stat_block_->finalize();
    round_trip_jitter_stat_block_->finalize();

    datareader_ = nullptr;
  }
}

bool WorkerDataReaderListener::wait_for_expected_match(const std::chrono::system_clock::time_point& deadline) const
{
  std::unique_lock<std::mutex> expected_lock(mutex_);

  while (expected_match_count_ != match_count_) {
    if (expected_match_cv.wait_until(expected_lock, deadline) == std::cv_status::timeout) {
      return match_count_ == expected_match_count_;
    }
  }
  return true;
}

}

