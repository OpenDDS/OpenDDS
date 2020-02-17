#include "WorkerDataWriterListener.h"

namespace Bench {

WorkerDataWriterListener::WorkerDataWriterListener()
{
}

WorkerDataWriterListener::WorkerDataWriterListener(size_t expected) : expected_count_(expected)
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
  if (expected_count_ != 0) {
    if (static_cast<size_t>(status.current_count) == expected_count_) {
      //std::cout << "WorkerDataWriterListener reached expected count!" << std::endl;
      if (datawriter_) {
        last_discovery_time_->value.time_prop(Builder::get_time());
      }
    }
  } else {
    if (static_cast<size_t>(status.current_count) > matched_count_) {
      if (datawriter_) {
        last_discovery_time_->value.time_prop(Builder::get_time());
      }
    }
  }
  matched_count_ = status.current_count;
}

void
WorkerDataWriterListener::set_datawriter(Builder::DataWriter& datawriter)
{
  datawriter_ = &datawriter;
  last_discovery_time_ =
    get_or_create_property(datawriter_->get_report().properties, "last_discovery_time", Builder::PVK_TIME);
}

void
WorkerDataWriterListener::unset_datawriter(Builder::DataWriter& datawriter)
{
  if (datawriter_ == &datawriter) {
    datawriter_ = nullptr;
  }
}

}
