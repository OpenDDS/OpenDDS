#include "WorkerDataWriterListener.h"

#include "Utils.h"

namespace Bench {

WorkerDataWriterListener::WorkerDataWriterListener()
{
}

WorkerDataWriterListener::WorkerDataWriterListener(const Builder::PropertySeq& properties)
{
  size_t expected_match_count = 0;
  auto expected_match_count_prop = get_property(properties, "expected_match_count", Builder::PVK_ULL);
  if (expected_match_count_prop) {
    expected_match_count = static_cast<size_t>(expected_match_count_prop->value.ull_prop());
  }
  expected_match_count_ = expected_match_count;
}

WorkerDataWriterListener::~WorkerDataWriterListener()
{
}

void
WorkerDataWriterListener::on_offered_deadline_missed(
  DDS::DataWriter_ptr /*writer*/,
  const DDS::OfferedDeadlineMissedStatus& /*status*/)
{
}

void
WorkerDataWriterListener::on_offered_incompatible_qos(
  DDS::DataWriter_ptr /*writer*/,
  const DDS::OfferedIncompatibleQosStatus& /*status*/)
{
}

void
WorkerDataWriterListener::on_liveliness_lost(
  DDS::DataWriter_ptr /*writer*/,
  const DDS::LivelinessLostStatus& /*status*/)
{
}

void
WorkerDataWriterListener::on_publication_matched(
  DDS::DataWriter_ptr /*writer*/,
  const DDS::PublicationMatchedStatus& status)
{
  std::unique_lock<std::mutex> lock(mutex_);
  if (expected_match_count_ != 0) {
    if (static_cast<size_t>(status.current_count) == expected_match_count_) {
      expected_match_cv.notify_all();
      if (datawriter_) {
        last_discovery_time_->value.time_prop(Builder::get_hr_time());
      }
    } else if (static_cast<size_t>(status.current_count) > match_count_) {
      if (datawriter_) {
        discovery_delta_stat_block_->update(Builder::to_seconds_double(Builder::get_hr_time() - enable_time_->value.time_prop()));
      }
    }
  } else {
    if (static_cast<size_t>(status.current_count) > match_count_) {
      if (datawriter_) {
        auto now = Builder::get_hr_time();
        last_discovery_time_->value.time_prop(now);
        discovery_delta_stat_block_->update(Builder::to_seconds_double(now - enable_time_->value.time_prop()));
      }
    }
  }
  match_count_ = status.current_count;
}

void
WorkerDataWriterListener::set_datawriter(Builder::DataWriter& datawriter)
{
  datawriter_ = &datawriter;

  const Builder::PropertySeq& global_properties = get_global_properties();
  Builder::ConstPropertyIndex buffer_size_prop =
    get_property(global_properties, "default_stat_median_buffer_size", Builder::PVK_ULL);
  size_t buffer_size = buffer_size_prop ? static_cast<size_t>(buffer_size_prop->value.ull_prop()) : DEFAULT_STAT_BLOCK_BUFFER_SIZE;

  enable_time_ =
    get_property(datawriter_->get_report().properties, "enable_time", Builder::PVK_TIME);
  last_discovery_time_ =
    get_or_create_property(datawriter_->get_report().properties, "last_discovery_time", Builder::PVK_TIME);

  discovery_delta_stat_block_ =
    std::make_shared<PropertyStatBlock>(datawriter_->get_report().properties, "discovery_delta", buffer_size);
}

void
WorkerDataWriterListener::unset_datawriter(Builder::DataWriter& datawriter)
{
  if (datawriter_ == &datawriter) {
    discovery_delta_stat_block_->finalize();
    datawriter_ = nullptr;
  }
}

bool WorkerDataWriterListener::wait_for_expected_match(const std::chrono::system_clock::time_point& deadline) const
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
