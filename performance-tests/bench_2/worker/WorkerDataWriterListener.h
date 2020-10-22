#pragma once

#include "DataWriterListener.h"
#include "PropertyStatBlock.h"

#include <condition_variable>

namespace Bench {

class WorkerDataWriterListener : public Builder::DataWriterListener {
public:

  WorkerDataWriterListener();
  WorkerDataWriterListener(const Builder::PropertySeq& properties);
  virtual ~WorkerDataWriterListener();

  // From DDS::DataWriterListener

  void on_offered_deadline_missed(DDS::DataWriter_ptr writer, const DDS::OfferedDeadlineMissedStatus& status) override;
  void on_offered_incompatible_qos(DDS::DataWriter_ptr writer, const DDS::OfferedIncompatibleQosStatus& status) override;
  void on_liveliness_lost(DDS::DataWriter_ptr writer, const DDS::LivelinessLostStatus& status) override;
  void on_publication_matched(DDS::DataWriter_ptr writer, const DDS::PublicationMatchedStatus& status) override;

  // From Builder::DataWriterListener

  void set_datawriter(Builder::DataWriter& datawriter) override;
  void unset_datawriter(Builder::DataWriter& datawriter) override;

  bool wait_for_expected_match(const std::chrono::system_clock::time_point& deadline) const;

protected:
  mutable std::mutex mutex_;
  size_t expected_match_count_{0};
  size_t match_count_{0};
  Builder::DataWriter* datawriter_{0};
  Builder::ConstPropertyIndex enable_time_;
  Builder::PropertyIndex last_discovery_time_;
  std::shared_ptr<PropertyStatBlock> discovery_delta_stat_block_;
  mutable std::condition_variable expected_match_cv;
};

}

